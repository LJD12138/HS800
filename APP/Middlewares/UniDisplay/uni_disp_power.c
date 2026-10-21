/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_power.c
 * Date    : 2026-10-15
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 电源域实现 - 背光开关/息屏倒计时/强制刷新标志 (由 uni_disp_core.c 拆分, 逻辑未变)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "uni_disp_power.h"

#if (boardDISPLAY_EN)
#include "uni_disp_core.h"      /* 项目操作集 / 临界区适配 */
#include "uni_disp_page.h"


//****************************************************Parameter Initialization**************************************************//
/*----------------背光与息屏 (Power Manager)----------------*/
static bool     S_bLight            = false;            /* 背光状态 */
static bool     S_bLastLight        = false;            /* 上一个背光状态 (息屏边沿补帧判定) */
static bool     S_bForceRefresh     = true;             /* 切页/点亮强制刷新标志 */
static uint16_t S_usAutoOffTime     = 0;                /* 息屏时间 (0=常亮) */
static uint16_t S_usAutoOffCnt      = 0;                /* 息屏倒计时 */
static uint8_t  S_ucCurrentDevState = dispDEV_STATE_NONE;   /* 最近一次快照的设备状态 */

//****************************************************Function Declaration******************************************************//
static uint16_t us_disp_ops_auto_off_time(void);
static bool b_disp_ops_auto_off_armed(uint8_t uc_dev_state);
static void v_disp_ops_set_backlight(bool on);
static void v_disp_ops_wake(void);
static void v_disp_ops_beep(void);
static void v_disp_ops_flush(void);

/***********************************************************************************************************************
 * 函数功能    : 电源域初始化
 * 说明(备注)  : 框架核心初始化时调用; 背光关闭, 息屏参数按持久化值复位
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PowerInit(void)
{
    S_bLight            = false;
    S_bLastLight        = false;
    S_bForceRefresh     = true;
    S_usAutoOffTime     = us_disp_ops_auto_off_time();
    S_usAutoOffCnt      = 0;
    S_ucCurrentDevState = dispDEV_STATE_NONE;
}

/***********************************************************************************************************************
 * 函数功能    : 背光开关 (任意任务上下文)
 * 说明(备注)  : 点亮时进入运行模式并标记强制刷新, 关闭时进入待机模式并清显存;
 *               b_fore_en=true 表示常亮(息屏时间清零), false 按板级默认息屏时间重置倒计时
 * 传入参数    : e_type: OFF/ON/TOGGLE; b_fore_en: true=常亮
 * 输出参数    : 无
 * 返回值      : true: 已处理
 ************************************************************************************************************************/
bool bDisp_Switch(SwitchType_E e_type, bool b_fore_en)
{
    if (e_type == ST_NULL)
        e_type = S_bLight ? ST_OFF : ST_ON;

    if (e_type == ST_ON)
    {
        if (!S_bLight)
        {
            vDisp_CoreEnterCritical();          /* 标志写与引擎读/清同临界区, 防强制刷新丢失更新 */
            S_bLight        = true;
            S_bForceRefresh = true;             /* 标记下次渲染全量重绘 */
            vDisp_CoreExitCritical();

            v_disp_ops_set_backlight(true);     /* 硬件调用置于临界区外 */
        }

        /* 更新息屏参数: 优先使用持久化参数, 为0则使用板级默认值 (临界区防与 1S 节拍递减竞态) */
        vDisp_CoreEnterCritical();
        S_usAutoOffTime = b_fore_en ? 0 : us_disp_ops_auto_off_time();
        S_usAutoOffCnt  = S_usAutoOffTime;
        vDisp_CoreExitCritical();
    }
    else if (e_type == ST_OFF)
    {
        if (S_bLight)
        {
            vDisp_CoreEnterCritical();
            S_bLight = false;
            vDisp_CoreExitCritical();

            v_disp_ops_set_backlight(false);    /* 硬件调用置于临界区外 */
        }

        /* 复位息屏参数 (临界区防与 1S 节拍递减竞态) */
        vDisp_CoreEnterCritical();
        S_usAutoOffTime = us_disp_ops_auto_off_time();
        S_usAutoOffCnt  = 0;
        vDisp_CoreExitCritical();
    }

    /* 唤醒可能处于低频休眠心跳的显示任务 */
    v_disp_ops_wake();

    if (b_fore_en && S_bLight)
        v_disp_ops_beep();

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
 * 函数功能    : 查询显示是否处于活动状态
 * 说明(备注)  : 项目提供 bDisplayActive 时以其为准 (断码屏可恒 true); 未提供时跟随背光状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 显示活动 (允许渲染); false: 显示关闭
 ************************************************************************************************************************/
bool bDisp_IsDisplayActive(void)
{
    const DispOps_T *p_ops = pDisp_GetOps();

    if ((p_ops != NULL) && (p_ops->bDisplayActive != NULL))
        return p_ops->bDisplayActive();

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
    vDisp_CoreEnterCritical();
    S_usAutoOffCnt = S_usAutoOffTime;
    vDisp_CoreExitCritical();
}

/***********************************************************************************************************************
 * 函数功能    : 显示 1S 定时节拍 (息屏电源管理)
 * 说明(备注)  : 由 timer_task 的 1S 重复定时器直接调用;
 *               脱离 EnginePoll 轮询次数, 计时绝对准确; 倒计时递减临界区保护防并发写竞态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_TickTimer(void)
{
    bool     b_timeout = false;
    bool     b_light;
    uint16_t us_off_time;
    uint8_t  uc_dev_state;

    /* 读侧快照与 bDisp_Switch 写侧同临界区: 防读到 "新息屏时间 + 旧倒计时" 撕裂组合 */
    vDisp_CoreEnterCritical();
    b_light      = S_bLight;
    us_off_time  = S_usAutoOffTime;
    uc_dev_state = S_ucCurrentDevState;
    vDisp_CoreExitCritical();

    if (b_light && (us_off_time > 0U) && b_disp_ops_auto_off_armed(uc_dev_state))
    {
        vDisp_CoreEnterCritical();               /* 16 位 RMW 临界区保护, 防与其它任务写倒计时竞态 */
        if (S_usAutoOffCnt > 0)
        {
            S_usAutoOffCnt--;
            if (S_usAutoOffCnt == 0)
                b_timeout = true;
        }
        vDisp_CoreExitCritical();

        if (b_timeout)
            bDisp_Switch(ST_OFF, false);         /* 倒计时结束息屏 (临界区外: 内含 OS/硬件调用) */
    }
}

/***********************************************************************************************************************
 * 函数功能    : 更新当前设备状态快照值
 * 说明(备注)  : 仅框架引擎每帧调用
 * 传入参数    : uc_dev_state: 当前快照设备状态
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PowerUpdateDevState(uint8_t uc_dev_state)
{
    S_ucCurrentDevState = uc_dev_state;
}

/***********************************************************************************************************************
 * 函数功能    : 置位强制刷新标志
 * 说明(备注)  : 切页/点亮背光后调用, 下一帧 vOnUpdate 收到 b_force=true 并全量重绘
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PowerMarkForceRefresh(void)
{
    S_bForceRefresh = true;
}

/***********************************************************************************************************************
 * 函数功能    : 查询强制刷新标志
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 本帧强制刷新
 ************************************************************************************************************************/
bool bDisp_PowerIsForceRefresh(void)
{
    return S_bForceRefresh;
}

/***********************************************************************************************************************
 * 函数功能    : 清除强制刷新标志
 * 说明(备注)  : 渲染输出后由引擎调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PowerClearForceRefresh(void)
{
    vDisp_CoreEnterCritical();              /* 与 bDisp_Switch 置位同临界区, 防并发置位被一并清掉 */
    S_bForceRefresh = false;
    S_bLastLight    = S_bLight;             /* 渲染完成即同步显示状态, 供关屏边沿补帧判定 */
    vDisp_CoreExitCritical();
}

/***********************************************************************************************************************
 * 函数功能    : 息屏渲染出口 (背光关闭边沿补一帧)
 * 说明(备注)  : 仅框架引擎息屏分支调用; 背光刚关闭的边沿补一次 UI 刷新 (清显存),
 *               并同步上一帧背光状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_PowerIdleEdgeRefresh(void)
{
    if (S_bLight != S_bLastLight)
        v_disp_ops_flush();

    S_bLastLight = S_bLight;
}

/***********************************************************************************************************************
 * 函数功能    : 读取项目息屏时间
 * 说明(备注)  : 项目未提供 usGetAutoOffTime 时返回 0 (常亮)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 自动息屏时间 (秒)
 ************************************************************************************************************************/
static uint16_t us_disp_ops_auto_off_time(void)
{
    const DispOps_T *p_ops = pDisp_GetOps();

    if ((p_ops != NULL) && (p_ops->usGetAutoOffTime != NULL))
        return p_ops->usGetAutoOffTime();

    return 0U;
}

/***********************************************************************************************************************
 * 函数功能    : 判断当前状态是否参与息屏倒计时
 * 说明(备注)  : 项目未提供 bAutoOffArmed 时返回 false (不自动息屏)
 * 传入参数    : uc_dev_state: 设备状态
 * 输出参数    : 无
 * 返回值      : true: 参与倒计时
 ************************************************************************************************************************/
static bool b_disp_ops_auto_off_armed(uint8_t uc_dev_state)
{
    const DispOps_T *p_ops = pDisp_GetOps();

    if ((p_ops != NULL) && (p_ops->bAutoOffArmed != NULL))
        return p_ops->bAutoOffArmed(uc_dev_state);

    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 设置背光硬件
 * 说明(备注)  : 项目未提供 vBacklight 时为空操作 (断码屏常亮)
 * 传入参数    : on: true-点亮, false-熄灭
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_disp_ops_set_backlight(bool on)
{
    const DispOps_T *p_ops = pDisp_GetOps();

    if ((p_ops != NULL) && (p_ops->vBacklight != NULL))
        p_ops->vBacklight(on);
}

/***********************************************************************************************************************
 * 函数功能    : 唤醒显示任务
 * 说明(备注)  : 项目未提供 vWakeTask 时为空操作 (裸机轮询)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_disp_ops_wake(void)
{
    const DispOps_T *p_ops = pDisp_GetOps();

    if ((p_ops != NULL) && (p_ops->vWakeTask != NULL))
        p_ops->vWakeTask();
}

/***********************************************************************************************************************
 * 函数功能    : 常亮亮屏提示
 * 说明(备注)  : 项目未提供 vBeepOnLightOn 时为空操作
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_disp_ops_beep(void)
{
    const DispOps_T *p_ops = pDisp_GetOps();

    if ((p_ops != NULL) && (p_ops->vBeepOnLightOn != NULL))
        p_ops->vBeepOnLightOn();
}

/***********************************************************************************************************************
 * 函数功能    : 执行一轮 UI 刷新
 * 说明(备注)  : 项目未提供 vFlush 时为空操作
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_disp_ops_flush(void)
{
    const DispOps_T *p_ops = pDisp_GetOps();

    if ((p_ops != NULL) && (p_ops->vFlush != NULL))
        p_ops->vFlush();
}

#endif  /* boardDISPLAY_EN */
