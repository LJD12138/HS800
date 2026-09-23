/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_core.c
 * Date    : 2026-09-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 调度核心 - 页面注册表/状态路由/N 深弹窗栈/背光息屏管理/轮询引擎
 * -------------------------------------------------------
 * 调度时序约定 (优先级从高到低):
 * 1. 设备状态路由 (状态变化沿, EnginePoll 步骤 2): 显式导航语义, 覆盖弹窗栈;
 * 2. 事件队列分发 (EnginePoll 步骤 3): 页面消费/框架默认处理两级;
 * 3. 页内导航 (页面回调内的 Request/Push/Pop): 状态未变化时不受干扰。
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "uni_disp_core.h"

#if (boardDISPLAY_EN)
#include "uni_disp_port.h"
#include "MD_Display/md_display_data.h"
#include "MD_Display/md_display_api.h"

//****************************************************Parameter Initialization**************************************************//
/*----------------页面静态注册表----------------*/
static const DispPage_T *S_aptPages[PAGE_ID_MAX] = { NULL };

/*----------------路由与弹窗栈----------------*/
#define			dispPOPUP_STACK_DEPTH					(4U)	/* 弹窗返回栈深度 */

static const DispPage_T *S_ptCurrentPage  = NULL;   /* 当前页面控制块 (NULL=空页/无映射) */
static DispPageId_E      S_eCurrentPageId = PAGE_ID_MAX;
static DispPageId_E      S_eTargetPageId  = PAGE_ID_MAX;
static DispPageId_E      S_aePopupStack[dispPOPUP_STACK_DEPTH];   /* 弹窗返回栈 */
static uint8_t           S_ucPopupDepth   = 0;
static bool              S_bForceRefresh  = true;   /* 切页/首帧强制刷新标志 */
static uint8_t           S_ucMappedDevState = 0xFFU; /* 已路由的设备状态 (初始无效) */
static uint16_t          S_usFramePeriodMs  = 0;     /* 当前页面帧刷新周期 ms (0=未加载, getter 回退默认) */

/*----------------背光与息屏 (Power Manager)----------------*/
static bool     S_bLight            = false;            /* 背光状态 */
static uint16_t S_usAutoOffTime     = 0;                /* 息屏时间 (0=常亮) */
static uint16_t S_usAutoOffCnt      = 0;                /* 息屏倒计时 */
static uint8_t  S_ucCurrentDevState = 0xFFU;            /* 最近一次快照的设备状态 */

//****************************************************Function Declaration******************************************************//
static void v_disp_set_target(DispPageId_E e_page);
static void v_disp_route_by_dev_state(const DispDataSnapshot_T *p_snapshot);
static void v_disp_event_default(DispEvent_E e_event, uint32_t ul_param);
static uint16_t v_disp_load_page_period(const DispPage_T *p_page);



/***********************************************************************************************************************
 * 函数功能    : 框架核心初始化
 * 说明(备注)  : 显示任务创建前调用; 初始为空页(PAGE_ID_MAX), 由状态路由驱动首个页面
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_CoreInit(void)
{
    S_ucPopupDepth     = 0;
    S_ptCurrentPage    = NULL;
    S_eCurrentPageId   = PAGE_ID_MAX;
    S_eTargetPageId    = PAGE_ID_MAX;
    S_ucMappedDevState = 0xFFU;
    S_bForceRefresh    = true;
    S_usFramePeriodMs  = dispREFRESH_TIME_MS;

    S_bLight            = false;
    S_usAutoOffTime     = dispAUTO_OFF_TIME_S;
    S_usAutoOffCnt      = 0;
    S_ucCurrentDevState = 0xFFU;
}

/***********************************************************************************************************************
 * 函数功能    : 页面静态注册
 * 说明(备注)  : 页面对象须为 static/全局 const 生命周期; 越界或空指针忽略
 * 传入参数    : p_page: 页面控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PageRegister(const DispPage_T *p_page)
{
    if (p_page != NULL && p_page->ePageId < PAGE_ID_MAX)
        S_aptPages[p_page->ePageId] = p_page;
}

/***********************************************************************************************************************
 * 函数功能    : 请求切换页面 (任意任务上下文)
 * 说明(备注)  : 显式导航语义: 清空弹窗栈, 与"弹窗返回"互斥;
 *               目标页单字节写原子, 显示任务下一轮询生效
 * 传入参数    : e_new_page: 目标页面 ID
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_RequestPage(DispPageId_E e_new_page)
{
    if (e_new_page < PAGE_ID_MAX)
    {
        S_ucPopupDepth = 0;                     /* 显式导航清栈 */
        S_eTargetPageId = e_new_page;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 弹窗式压栈 (仅显示任务上下文: 页面回调/事件默认处理内调用)
 * 说明(备注)  : 幂等保护: 目标页是当前页或已在栈中时忽略, 防止重复事件卡死在弹窗页
 * 传入参数    : e_popup_page: 弹窗页面 ID
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PushPage(DispPageId_E e_popup_page)
{
    uint8_t i;

    /* 已在弹窗页: 幂等忽略 */
    if (e_popup_page >= PAGE_ID_MAX || e_popup_page == S_eCurrentPageId)
        return;

    for (i = 0; i < S_ucPopupDepth; i++)
    {
        /* 已在栈中: 幂等忽略 */
        if (S_aePopupStack[i] == e_popup_page)
            return;
    }

    if (S_ucPopupDepth < dispPOPUP_STACK_DEPTH)
    {
        S_aePopupStack[S_ucPopupDepth++] = S_eCurrentPageId;
        v_disp_set_target(e_popup_page);        /* 内部设定目标页, 不清栈 */
    }
}

/***********************************************************************************************************************
 * 函数功能    : 弹窗退栈返回 (仅显示任务上下文)
 * 说明(备注)  : 只退一级, 嵌套弹窗逐级返回
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PopPage(void)
{
    if (S_ucPopupDepth > 0U)
        v_disp_set_target(S_aePopupStack[--S_ucPopupDepth]);
}

/***********************************************************************************************************************
 * 函数功能    : 背光开关 (任意任务上下文)
 * 说明(备注)  : 平移旧 bDisp_Switch(): 点亮时进入运行模式并标记强制刷新, 关闭时进入待机模式并清显存;
 *               b_fore_en=true 表示常亮(息屏时间清零), false 按板级默认息屏时间重置倒计时
 * 传入参数    : e_type: OFF/ON/TOGGLE; b_fore_en: true=常亮
 * 输出参数    : 无
 * 返回值      : true: 已处理
 ************************************************************************************************************************/
bool bDisp_SwitchBacklight(DispBacklight_E e_type, bool b_fore_en)
{
    if (e_type == DISP_BKL_TOGGLE)
        e_type = S_bLight ? DISP_BKL_OFF : DISP_BKL_ON;

    if (e_type == DISP_BKL_ON)
    {
        if (!S_bLight)
        {
            S_bLight = true;
            vDisp_SetBacklight(true);
        }

        /* 更新息屏参数 */
        S_usAutoOffTime = b_fore_en ? 0 : dispAUTO_OFF_TIME_S;
        S_usAutoOffCnt  = S_usAutoOffTime;

        /* 唤醒可能处于低频休眠心跳的显示任务, 下一轮 EnginePoll 即时渲染 */
        vDisp_PortWakeTask();
    }
    else if (e_type == DISP_BKL_OFF)
    {
        if (S_bLight)
        {
            S_bLight = false;
            vDisp_SetBacklight(false);

            /* 复位息屏参数 (与旧 v_disp_param_init 一致) */
            S_usAutoOffTime = dispAUTO_OFF_TIME_S;
            S_usAutoOffCnt  = 0;
        }
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 查询背光状态 (任意任务上下文)
 * 说明(备注)  : none
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 背光开启; false: 息屏
 ************************************************************************************************************************/
bool bDisp_IsBacklightOn(void)
{
    return S_bLight;
}

/***********************************************************************************************************************
 * 函数功能    : 重置息屏倒计时 (任意任务上下文)
 * 说明(备注)  : 不点亮屏幕, 仅重置自动息屏计数 (供按键事件默认处理调用)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_ResetAutoOffCountdown(void)
{
    S_usAutoOffCnt = S_usAutoOffTime;
}

/***********************************************************************************************************************
 * 函数功能    : 显示 1S 定时节拍 (息屏电源管理)
 * 说明(备注)  : 由 timer_task 的 1S 重复定时器直接调用;
 *               脱离 EnginePoll 轮询次数, 计时绝对准确
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_TickTimer(void)
{
    if (S_bLight && (S_usAutoOffTime > 0) && bDisp_DataAutoOffArmed(S_ucCurrentDevState))
    {
        if (S_usAutoOffCnt > 0)
        {
            S_usAutoOffCnt--;
            if (S_usAutoOffCnt == 0)
                bDisp_SwitchBacklight(DISP_BKL_OFF, false);     /* 倒计时结束息屏 */
        }
    }
}

/***********************************************************************************************************************
 * 函数功能    : UniDisplay 调度轮询引擎 (仅显示任务调用)
 * 说明(备注)  : 快照抓取 -> 息屏计时 -> 状态路由 -> 事件分发 -> 页面切换 -> 息屏门控渲染
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_EnginePoll(void)
{
    DispDataSnapshot_T t_snapshot;
    DispEventMsg_T     t_msg;

    /* 1. 抓取快照 (状态路由与页面渲染共用一份数据, 帧内一致) */
    vDisp_DataCaptureSnapshot(&t_snapshot);
    S_ucCurrentDevState = t_snapshot.ucDevState;

    /* 2. 设备状态路由 (状态变化沿, 高优先级: 覆盖弹窗栈) */
    v_disp_route_by_dev_state(&t_snapshot);

    /* 3. 事件分发: 在本任务上下文统一消费事件队列 */
    while (bDisp_EventDequeue(&t_msg))
    {
        if (S_ptCurrentPage != NULL && S_ptCurrentPage->bOnEvent != NULL)
        {
            if (S_ptCurrentPage->bOnEvent(t_msg.eEvent, t_msg.ulParam))
                continue;                       /* 页面已消费 */
        }
        v_disp_event_default(t_msg.eEvent, t_msg.ulParam);
    }

    /* 4. 页面切换生命周期 (OnExit -> OnEnter -> 强刷置位) */
    if ((S_eTargetPageId != S_eCurrentPageId) && (S_eTargetPageId < PAGE_ID_MAX))
    {
        if (S_ptCurrentPage != NULL && S_ptCurrentPage->vOnExit != NULL)
            S_ptCurrentPage->vOnExit();

        S_eCurrentPageId = S_eTargetPageId;
        S_ptCurrentPage  = S_aptPages[S_eCurrentPageId];

        if (S_ptCurrentPage != NULL && S_ptCurrentPage->vOnEnter != NULL)
            S_ptCurrentPage->vOnEnter();

        /* 加载页面声明的帧周期 (0/空页回退默认, 下限钳位) */
        S_usFramePeriodMs = v_disp_load_page_period(S_ptCurrentPage);

        S_bForceRefresh = true;
    }

    /* 5. 渲染输出: 背光关闭时跳过 (息屏省电; 首帧/切页强刷由 b_force 传递给页面) */
    if (S_bLight || S_bForceRefresh)
    {
        if (S_ptCurrentPage != NULL && S_ptCurrentPage->vOnUpdate != NULL)
            S_ptCurrentPage->vOnUpdate(&t_snapshot, S_bForceRefresh);

        vDisp_UiRefresh();
        S_bForceRefresh = false;
    }
}





/***********************************************************************************************************************
 * 函数功能    : 设定目标页 (框架内部)
 * 说明(备注)  : 仅显示任务上下文调用; 越界 ID 忽略
 * 传入参数    : e_page: [IN] 目标页 ID
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_disp_set_target(DispPageId_E e_page)
{
    if (e_page < PAGE_ID_MAX)
        S_eTargetPageId = e_page;
}

/***********************************************************************************************************************
 * 函数功能    : 查询当前页面帧刷新周期
 * 说明(备注)  : 显示任务切页时单写, 其余任务只读; 未加载时回退框架默认周期
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 当前帧刷新周期 ms (页面声明值经下限钳位后)
 ************************************************************************************************************************/
uint16_t usDisp_GetFramePeriod(void)
{
    return (S_usFramePeriodMs != 0U) ? S_usFramePeriodMs : dispREFRESH_TIME_MS;
}

/***********************************************************************************************************************
 * 函数功能    : 加载页面声明的帧周期 (框架内部)
 * 说明(备注)  : 仅显示任务上下文调用; 0/空页回退默认, 低于下限钳位 (防误配打满 CPU)
 * 传入参数    : p_page: [IN] 页面控制块指针 (可为 NULL)
 * 输出参数    : 无
 * 返回值      : 生效的帧刷新周期 ms
 ************************************************************************************************************************/
static uint16_t v_disp_load_page_period(const DispPage_T *p_page)
{
    if ((p_page == NULL) || (p_page->usRefreshMs == 0U))
        return dispREFRESH_TIME_MS;

    if (p_page->usRefreshMs < dispFRAME_PERIOD_MIN_MS)
        return dispFRAME_PERIOD_MIN_MS;

    return p_page->usRefreshMs;
}

/***********************************************************************************************************************
 * 函数功能    : 设备状态路由 (仅显示任务上下文)
 * 说明(备注)  : 状态变化沿触发(非电平触发, 避免与页内导航互踩);
 *               无映射状态(PAGE_ID_MAX)不产生切换请求, 维持当前页
 * 传入参数    : p_snapshot: [IN] 快照数据
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_disp_route_by_dev_state(const DispDataSnapshot_T *p_snapshot)
{
    DispPageId_E e_mapped;

    if (p_snapshot->ucDevState == S_ucMappedDevState)
        return;                                     /* 未变化 */

    e_mapped = eDisp_MapDevStateToPage(p_snapshot->ucDevState);
    if (e_mapped < PAGE_ID_MAX)
    {
        S_ucMappedDevState = p_snapshot->ucDevState;
        vDisp_RequestPage(e_mapped);                /* 显式导航: 清弹窗栈 */
    }
}

/***********************************************************************************************************************
 * 函数功能    : 事件默认处理 (仅显示任务上下文, 页面未消费时兜底)
 * 说明(备注)  : 仅显示任务上下文调用; 越界 ID 忽略
 * 传入参数    : e_event: [IN] 事件类型
 *               ul_param: [IN] 事件参数
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_disp_event_default(DispEvent_E e_event, uint32_t ul_param)
{
    switch (e_event)
    {
        /* 故障弹出 */
        case DISP_EVT_FAULT_POPUP:
        {
            vDisp_PushPage(PAGE_ID_FAULT);
        }
        break;

        /* 故障清除 */
        case DISP_EVT_FAULT_CLEAR:
        {
            if (S_eCurrentPageId == PAGE_ID_FAULT)
                vDisp_PopPage();
        }
        break;

        /* 页面请求 */
        case DISP_EVT_REQ_PAGE:
        {
            vDisp_RequestPage((DispPageId_E)ul_param);
        }
        break;

        /* 按键事件 */
        case DISP_EVT_KEY_SHORT:
        case DISP_EVT_KEY_LONG:
        {
            vDisp_ResetAutoOffCountdown();          /* 按键重置息屏倒计时 */
        }
        break;

        default:
        {
        }
        break;
    }
}

#endif  /* boardDISPLAY_EN */
