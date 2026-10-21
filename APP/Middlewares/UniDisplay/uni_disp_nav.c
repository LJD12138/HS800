/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_nav.c
 * Date    : 2026-10-15
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 导航域实现 - 页面注册表/页面导航/页面栈 (由 uni_disp_core.c 拆分, 逻辑未变;
 *           对外公共 API vDisp_PageRegister/vDisp_RequestPage(vDisp_RequestPageEx)/bDisp_PagePush/bDisp_PagePop/
 *           bDisp_PageCanPop/ulDisp_GetNavParam/usDisp_GetFramePeriod 声明见 uni_disp_core.h)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "uni_disp_nav.h"

#if (boardDISPLAY_EN)
#include "uni_disp_core.h"      /* 项目操作集 / 默认帧周期 / 临界区适配 */


//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
/*----------------页面静态注册表----------------*/
static const DispPage_T *S_aptPages[dispPAGE_REG_MAX] = { NULL };

/*----------------路由状态----------------*/
static const DispPage_T *S_ptCurrentPage   = NULL;    /* 当前页面控制块 (NULL=空页/无映射) */
static DispPageId_E      S_eCurrentPageId  = dispPAGE_NONE;
static uint16_t          S_usFramePeriodMs = 0;       /* 当前页面帧刷新周期 ms (0=未加载, getter 回退默认) */

/* 目标页槽 (含导航参数, 写路径临界区保护; 读路径在切页时临界区取快照) */
static struct
{
	DispPageId_E	e_id;							/* 目标页面 ID (dispPAGE_NONE=无请求) */
	uint32_t		ul_param;						/* 带给目标页的导航参数 */
}S_tTarget = { dispPAGE_NONE, 0U };

/*----------------页面栈 (页内多级导航: 场景=流程权威, 栈=页内钻取返回链)----------------*/
#if (boardDISP_NAV_STACK_EN)
typedef struct
{
	DispPageId_E	e_page;							/* 压栈时保存的页面 ID (弹出时请求恢复) */
}DispNavEntry_T;

static DispNavEntry_T	S_atNavStack[dispPAGE_STACK_DEPTH];	/* 页内钻取返回链 (深度 dispPAGE_STACK_DEPTH) */
static uint8_t			S_ucNavTop = 0;						/* 栈顶索引 (0=空栈, 满=dispPAGE_STACK_DEPTH) */
#endif  /* boardDISP_NAV_STACK_EN */
static uint32_t			S_ulNavParam = 0;					/* 导航参数槽: 最近一次导航带给目标页的参数 (vOnEnter 内读取) */

//****************************************************Function Declaration******************************************************//
static uint16_t v_disp_load_page_period(const DispPage_T *p_page);

/***********************************************************************************************************************
 * 函数功能    : 导航域初始化
 * 说明(备注)  : 框架核心初始化时调用; 初始为空页(dispPAGE_NONE), 由场景调度驱动首个页面
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_NavInit(void)
{
	S_ptCurrentPage    = NULL;
	S_eCurrentPageId   = dispPAGE_NONE;
	S_tTarget.e_id     = dispPAGE_NONE;
	S_tTarget.ul_param = 0U;
	S_usFramePeriodMs  = usDisp_CoreDefaultFramePeriod();

	vDisp_NavStackClear();
	S_ulNavParam = 0U;
}

/***********************************************************************************************************************
 * 函数功能    : 页面静态注册
 * 说明(备注)  : 页面对象须为 static/全局 const 生命周期; 越界/空指针/重复注册忽略
 * 传入参数    : p_page: 页面控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PageRegister(const DispPage_T *p_page)
{
    if ((p_page != NULL) && (p_page->ePageId < dispPAGE_REG_MAX) &&
        (S_aptPages[p_page->ePageId] == NULL))
        S_aptPages[p_page->ePageId] = p_page;
}

/***********************************************************************************************************************
 * 函数功能    : 页面表批量注册
 * 说明(备注)  : 显示任务创建前调用; 逐项转调 vDisp_PageRegister, 越界/空项忽略
 * 传入参数    : p_pages: [IN] 页面控制块指针数组; uc_count: [IN] 数组元素个数
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PageRegisterAll(const DispPage_T *const *p_pages, uint8_t uc_count)
{
    uint8_t uc_idx;

    if (p_pages == NULL)
        return;

    for (uc_idx = 0U; uc_idx < uc_count; uc_idx++)
        vDisp_PageRegister(p_pages[uc_idx]);
}

/***********************************************************************************************************************
 * 函数功能    : 请求切换页面 (任意任务上下文)
 * 说明(备注)  : 显式导航; 等价 vDisp_RequestPageEx(e_new_page, 0)
 *               (场景调度推进与外部调试后门共用的唯一定页原语)
 * 传入参数    : e_new_page: 目标页面 ID
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_RequestPage(DispPageId_E e_new_page)
{
    vDisp_RequestPageEx(e_new_page, 0U);
}

/***********************************************************************************************************************
 * 函数功能    : 带参请求切换页面 (任意任务上下文)
 * 说明(备注)  : 显式导航; 目标页槽为结构体, 写路径临界区保护 (跨任务写竞态安全),
 *               显示任务下一轮询生效; ul_param 暂存目标槽, 切页时与目标页 ID 成对转入导航参数槽,
 *               目标页 vOnEnter 内经 ulDisp_GetNavParam 读取
 * 传入参数    : e_new_page: 目标页面 ID; ul_param: 带给目标页的导航参数
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_RequestPageEx(DispPageId_E e_new_page, uint32_t ul_param)
{
    const DispOps_T *p_ops;

    if (e_new_page < dispPAGE_REG_MAX)
    {
        vDisp_CoreEnterCritical();
        S_tTarget.e_id     = e_new_page;
        S_tTarget.ul_param = ul_param;
        vDisp_CoreExitCritical();

        /* 唤醒显示任务立即生效切页, 避免息屏低频心跳下最长 2s 的切页延迟 (项目未提供时为空操作) */
        p_ops = pDisp_GetOps();
        if ((p_ops != NULL) && (p_ops->vWakeTask != NULL))
            p_ops->vWakeTask();
    }
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
    return (S_usFramePeriodMs != 0U) ? S_usFramePeriodMs : usDisp_CoreDefaultFramePeriod();
}

/***********************************************************************************************************************
 * 函数功能    : 查询当前页面控制块 (框架内部)
 * 说明(备注)  : 仅框架内部调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 当前页面控制块指针; 无当前页返回 NULL
 ************************************************************************************************************************/
const DispPage_T *pDisp_NavGetCurrentPage(void)
{
    return S_ptCurrentPage;
}

/***********************************************************************************************************************
 * 函数功能    : 按页面 ID 查注册表取页面控制块 (框架内部)
 * 说明(备注)  : 仅框架内部调用; 场景调度步骤推进时查步骤页的 bOnDone 自完成判定使用
 * 传入参数    : e_page: 页面 ID
 * 输出参数    : 无
 * 返回值      : 页面控制块指针; 越界/未注册返回 NULL
 ************************************************************************************************************************/
const DispPage_T *pDisp_NavGetPage(DispPageId_E e_page)
{
    if (e_page >= dispPAGE_REG_MAX)
        return NULL;

    return S_aptPages[e_page];
}

/***********************************************************************************************************************
 * 函数功能    : 查询当前页面 ID (导航域内部)
 * 说明(备注)  : 仅框架内部调用; 供组内导航归属校验使用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 当前页面 ID; 无当前页返回 dispPAGE_NONE
 ************************************************************************************************************************/
DispPageId_E eDisp_NavGetCurrentPageId(void)
{
    return S_eCurrentPageId;
}

/***********************************************************************************************************************
 * 函数功能    : 压栈跳转 (仅显示任务上下文, 页面回调内调用)
 * 说明(备注)  : 页内多级钻取: 当前页入栈形成返回链, 带参请求目标页;
 *               栈满返回 false (调用方降格处理, 如提示/直接请求); 栈操作非原子, 禁止跨任务调用;
 *               目标页==当前页拒绝自压栈 (ApplySwitch 对同页请求不触发 OnEnter, 返回链会静默失效)
 * 传入参数    : e_page: 目标页面 ID; ul_param: 带给目标页的导航参数
 * 输出参数    : 无
 * 返回值      : true: 已受理; false: 未注册/栈满/自压栈
 ************************************************************************************************************************/
bool bDisp_PagePush(DispPageId_E e_page, uint32_t ul_param)
{
    #if (boardDISP_NAV_STACK_EN)
    if ((pDisp_NavGetPage(e_page) == NULL) || (S_ucNavTop >= dispPAGE_STACK_DEPTH) ||
        (e_page == S_eCurrentPageId))
        return false;

    S_atNavStack[S_ucNavTop].e_page   = S_eCurrentPageId;
    S_ucNavTop++;
    vDisp_RequestPageEx(e_page, ul_param);
    return true;
    #else   /* boardDISP_NAV_STACK_EN */
    (void)e_page;
    (void)ul_param;
    return false;
    #endif  /* boardDISP_NAV_STACK_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 出栈返回 (仅显示任务上下文, 页面回调内调用)
 * 说明(备注)  : 以 ul_result 结束当前子页并请求栈顶父页恢复; ul_result 写入导航参数槽,
 *               父页 vOnEnter 内经 ulDisp_GetNavParam 读取 (结果回传); 空栈返回 false (调用方兜底)
 * 传入参数    : ul_result: 回传给父页的结果参数
 * 输出参数    : 无
 * 返回值      : true: 已受理; false: 空栈
 ************************************************************************************************************************/
bool bDisp_PagePop(uint32_t ul_result)
{
    #if (boardDISP_NAV_STACK_EN)
    if (S_ucNavTop == 0U)
        return false;

    S_ucNavTop--;
    vDisp_RequestPageEx(S_atNavStack[S_ucNavTop].e_page, ul_result);
    return true;
    #else   /* boardDISP_NAV_STACK_EN */
    (void)ul_result;
    return false;
    #endif  /* boardDISP_NAV_STACK_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 查询可否出栈 (仅显示任务上下文)
 * 说明(备注)  : 栈非空表示存在返回链: 供页面 vOnExit 决定粒度释放, 也可用于返回箭头显隐
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 栈非空可返回; false: 空栈
 ************************************************************************************************************************/
bool bDisp_PageCanPop(void)
{
    #if (boardDISP_NAV_STACK_EN)
    return (S_ucNavTop > 0U);
    #else   /* boardDISP_NAV_STACK_EN */
    return false;
    #endif  /* boardDISP_NAV_STACK_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 读取导航参数 (仅显示任务上下文, 页面 vOnEnter 内调用)
 * 说明(备注)  : 返回最近一次导航 (RequestPageEx/PagePush/PagePop) 带入的参数;
 *               跨任务写者经临界区写参数槽, 显示任务单读者在 OnEnter 时读取安全
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 导航参数 (无参导航为 0)
 ************************************************************************************************************************/
uint32_t ulDisp_GetNavParam(void)
{
    return S_ulNavParam;
}

/***********************************************************************************************************************
 * 函数功能    : 读取当前页面上下文 (仅显示任务上下文, 页面回调内调用)
 * 说明(备注)  : 返回当前页 DispPage_T.pUser (页面模板复用时挂的数据源/状态块); 框架不解释该指针
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 页面上下文指针; 无当前页/未设置返回 NULL
 ************************************************************************************************************************/
void *pDisp_GetPageUser(void)
{
    return (S_ptCurrentPage != NULL) ? S_ptCurrentPage->pUser : NULL;
}

/***********************************************************************************************************************
 * 函数功能    : 清空页面栈 (框架内部)
 * 说明(备注)  : 场景边界仲裁/overlay 压入时由场景域调用 (抢占清栈, 防跨场景/跨层级残留)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_NavStackClear(void)
{
    #if (boardDISP_NAV_STACK_EN)
    S_ucNavTop = 0U;
    #endif  /* boardDISP_NAV_STACK_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 应用页面切换 (框架内部)
 * 说明(备注)  : 仅引擎每帧调用; 目标页 != 当前页时执行 OnExit -> OnEnter -> 加载页面帧周期
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 本帧发生切页 (引擎据此置强制刷新)
 ************************************************************************************************************************/
bool bDisp_NavApplySwitch(void)
{
    bool b_switched = false;
    DispPageId_E e_target;
    uint32_t ul_param;

    /* 目标页 ID 与参数成对取快照, 防止切页与 OnEnter 之间被后续请求拆散 */
    vDisp_CoreEnterCritical();
    e_target = S_tTarget.e_id;
    ul_param = S_tTarget.ul_param;
    vDisp_CoreExitCritical();

    if ((e_target != S_eCurrentPageId) && (e_target < dispPAGE_REG_MAX) &&
        (S_aptPages[e_target] != NULL))
    {
        const DispOps_T *p_ops    = pDisp_GetOps();
        DispPageId_E     e_from  = S_eCurrentPageId;

        if (S_ptCurrentPage != NULL && S_ptCurrentPage->vOnExit != NULL)
            S_ptCurrentPage->vOnExit();

        S_eCurrentPageId = e_target;
        S_ptCurrentPage  = S_aptPages[S_eCurrentPageId];
        S_ulNavParam     = ul_param;            /* 参数与目标页成对生效 */

        /* 切页钩子 (OnExit 后/OnEnter 前): TFT/LVGL 项目挂切屏动画, 断码屏 NULL 零开销 */
        if ((p_ops != NULL) && (p_ops->vOnPageSwitch != NULL))
            p_ops->vOnPageSwitch(e_from, e_target);

        if (S_ptCurrentPage != NULL && S_ptCurrentPage->vOnEnter != NULL)
            S_ptCurrentPage->vOnEnter();

        /* 加载页面声明的帧周期 (0/空页回退默认, 下限钳位) */
        S_usFramePeriodMs = v_disp_load_page_period(S_ptCurrentPage);

        b_switched = true;
    }

    return b_switched;
}

/***********************************************************************************************************************
 * 函数功能    : 加载页面声明的帧周期 (导航域内部)
 * 说明(备注)  : 仅显示任务上下文调用; 0/空页回退默认, 低于下限钳位 (防误配打满 CPU)
 * 传入参数    : p_page: [IN] 页面控制块指针 (可为 NULL)
 * 输出参数    : 无
 * 返回值      : 生效的帧刷新周期 ms
 ************************************************************************************************************************/
static uint16_t v_disp_load_page_period(const DispPage_T *p_page)
{
    if ((p_page == NULL) || (p_page->usRefreshMs == 0U))
        return usDisp_CoreDefaultFramePeriod();

    if (p_page->usRefreshMs < dispFRAME_PERIOD_MIN_MS)
        return dispFRAME_PERIOD_MIN_MS;

    return p_page->usRefreshMs;
}

#endif  /* boardDISPLAY_EN */
