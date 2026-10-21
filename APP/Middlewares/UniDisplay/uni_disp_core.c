/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_core.c
 * Date    : 2026-10-15
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 调度核心 - 轮询引擎 (聚合各域: uni_disp_scene 场景状态机 / uni_disp_nav 导航 /
 *           uni_disp_power 背光息屏; 本文件只保留引擎时序与事件默认处理)
 * -------------------------------------------------------
 * 调度时序约定 (优先级从高到低):
 * 1. 场景流程调度 (Scene Manager, EnginePoll 步骤 2): 唯一导航权威 - FAULT overlay 电平评估 ->
 *    场景边界仲裁 -> 步骤推进 (完成条件/超时兜底), 显式导航语义;
 * 2. 事件队列分发 (EnginePoll 步骤 3): 页面消费/框架默认处理两级;
 * 3. 页内导航 (页面回调内的 RequestPage): 场景与状态未变化时不受干扰。
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "uni_disp_core.h"

#if (boardDISPLAY_EN)
#include "uni_disp_nav.h"       /* 导航域内部接口 */
#include "uni_disp_power.h"     /* 电源域内部接口 */
#include "uni_disp_scene.h"     /* 场景域内部接口 */


//****************************************************Parameter Initialization**************************************************//
/* 项目操作集 (框架唯一项目耦合点; 未注册时各调用点使用默认行为) */
static const DispOps_T *S_ptOps = NULL;

//****************************************************Function Declaration******************************************************//
static void v_disp_event_default(DispEvent_E e_event, uint32_t ul_param);

/***********************************************************************************************************************
 * 函数功能    : 框架核心初始化
 * 说明(备注)  : 显示任务创建前调用; 先注册项目操作集, 再聚合转调各域初始化 (电源/导航/场景)
 * 传入参数    : p_ops: [IN] 项目操作集指针 (可为 NULL, 各域使用默认行为)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_CoreInit(const DispOps_T *p_ops)
{
    S_ptOps = p_ops;
    vDisp_PowerInit();
    vDisp_NavInit();
    vDisp_SceneInit();
}

/***********************************************************************************************************************
 * 函数功能    : 获取项目操作集
 * 说明(备注)  : 框架内部与项目件均可读取; 未注册返回 NULL
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 项目操作集指针
 ************************************************************************************************************************/
const DispOps_T *pDisp_GetOps(void)
{
    return S_ptOps;
}

/***********************************************************************************************************************
 * 函数功能    : 读取快照中的设备状态
 * 说明(备注)  : 经项目 ops 适配, 通用件不感知快照结构; 无适配返回 dispDEV_STATE_NONE
 * 传入参数    : p_snapshot: [IN] 快照指针 (可为 NULL)
 * 输出参数    : 无
 * 返回值      : 设备状态
 ************************************************************************************************************************/
uint8_t ucDisp_DataDevState(const DispDataSnapshot_T *p_snapshot)
{
    if ((p_snapshot != NULL) && (S_ptOps != NULL) && (S_ptOps->ucGetDevState != NULL))
        return S_ptOps->ucGetDevState(p_snapshot);

    return dispDEV_STATE_NONE;
}

/***********************************************************************************************************************
 * 函数功能    : 进入框架临界区
 * 说明(备注)  : 经项目 ops 映射 (如 mainENTER_CRITICAL); 项目未提供时为空实现 (裸机单上下文)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_CoreEnterCritical(void)
{
    if ((S_ptOps != NULL) && (S_ptOps->vEnterCritical != NULL))
        S_ptOps->vEnterCritical();
}

/***********************************************************************************************************************
 * 函数功能    : 退出框架临界区
 * 说明(备注)  : 必须与 vDisp_CoreEnterCritical 成对调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_CoreExitCritical(void)
{
    if ((S_ptOps != NULL) && (S_ptOps->vExitCritical != NULL))
        S_ptOps->vExitCritical();
}

/***********************************************************************************************************************
 * 函数功能    : 获取默认帧刷新周期
 * 说明(备注)  : 页面 usRefreshMs=0/空页时回退; 项目未配置时使用通用常量
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 默认帧刷新周期 ms
 ************************************************************************************************************************/
uint16_t usDisp_CoreDefaultFramePeriod(void)
{
    if ((S_ptOps != NULL) && (S_ptOps->usDefaultFramePeriodMs != 0U))
        return S_ptOps->usDefaultFramePeriodMs;

    return dispFRAME_PERIOD_DEFAULT_MS;
}

/***********************************************************************************************************************
 * 函数功能    : UniDisplay 调度轮询引擎 (仅显示任务调用)
 * 说明(备注)  : 快照抓取 -> 场景调度 -> 事件分发 -> 页面切换 -> 息屏门控渲染
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 建议下次轮询间隔 ms (当前页帧周期; 事件/导航请求经 vWakeTask 即时唤醒, 不受此限)
 ************************************************************************************************************************/
uint16_t usDisp_EnginePoll(void)
{
    const DispDataSnapshot_T *p_snapshot = NULL;
    DispEventMsg_T     t_msg;
    const DispPage_T  *p_page;

    /* 1. 抓取快照 (项目持有快照, 框架只读指针; 场景调度与页面渲染共用, 帧内一致) */
    if ((S_ptOps != NULL) && (S_ptOps->pGetSnapshot != NULL))
        p_snapshot = S_ptOps->pGetSnapshot();

    if (p_snapshot != NULL)
    {
        uint8_t uc_dev_state = ucDisp_DataDevState(p_snapshot);     /* 设备状态提取一次, 电源/场景两域共用 */

        vDisp_PowerUpdateDevState(uc_dev_state);

        /* 2. 场景调度 (唯一导航权威) */
        vDisp_SceneSchedule(p_snapshot, uc_dev_state);
    }

    /* 3. 事件分发: 在本任务上下文统一消费事件队列
     * (队列深度 dispEVT_QUEUE_DEPTH, 单帧最多处理 深度-1 条, 事件风暴受限于队列容量) */
    while (bDisp_EventDequeue(&t_msg))
    {
        p_page = pDisp_NavGetCurrentPage();
        if (p_page != NULL && p_page->bOnEvent != NULL)
        {
            if (p_page->bOnEvent(t_msg.eEvent, t_msg.ulParam))
                continue;                       /* 页面已消费 */
        }
        v_disp_event_default(t_msg.eEvent, t_msg.ulParam);
    }

    /* 4. 页面切换生命周期 (OnExit -> OnEnter -> 加载帧周期; 发生切页则置强制刷新) */
    if (bDisp_NavApplySwitch())
    {
        vDisp_PowerMarkForceRefresh();

        /* 切页后审计 (项目可选: 如 LVGL 内存水位/碎片率, 在显示任务上下文执行) */
        if ((S_ptOps != NULL) && (S_ptOps->vPostSwitchAudit != NULL))
            S_ptOps->vPostSwitchAudit();
    }

    /* 5. 渲染输出 (显示活动或强制刷新才渲染) */
    if (bDisp_IsDisplayActive() || bDisp_PowerIsForceRefresh())
    {
        p_page = pDisp_NavGetCurrentPage();
        if (p_page != NULL && p_page->vOnUpdate != NULL)
            p_page->vOnUpdate(p_snapshot, bDisp_PowerIsForceRefresh());

        if ((S_ptOps != NULL) && (S_ptOps->vFlush != NULL))
            S_ptOps->vFlush();

        vDisp_PowerClearForceRefresh();
    }
    else
    {
        /* 显示关闭边沿补一帧 (清显存) */
        vDisp_PowerIdleEdgeRefresh();
    }

    /* 建议下次轮询间隔: 当前页帧周期 (切页后为新页声明值) */
    return usDisp_GetFramePeriod();
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
        /* 页面请求 (调试后门: 显式页切换; 生产导航已由场景调度接管) */
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

        /* 返回请求: 页面栈非空弹出返回, 空栈无行为 (页面可自行消费该事件实现兜底) */
        case DISP_EVT_BACK:
        {
            (void)bDisp_PagePop(0U);
        }
        break;

        default:
        {
        }
        break;
    }
}

#endif  /* boardDISPLAY_EN */
