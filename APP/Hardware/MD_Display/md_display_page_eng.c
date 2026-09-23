/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_eng.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 工程模式页面 (TFT+LVGL) - 复用 eng_mode_ui.c 实现三段式生命周期与超时保护
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN && boardENG_MODE_EN)
#include "uni_disp_core.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/user_ui/eng_mode_ui.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Middlewares/LVGL/lvgl.h"

#if (boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_hm_task.h"
#endif  /* boardHEAT_MANAGE_EN */

//****************************************************Parameter Initialization**************************************************//
#define			dispPAGE_ENG_CYCLE_TIME_MS				33U
#define			dispENG_MODE_TIMEOUT_MS					60000U
#define			dispPAGE_ENG_TIMEOUT_CNT				(dispENG_MODE_TIMEOUT_MS / dispPAGE_ENG_CYCLE_TIME_MS)

static uint32_t s_ul_eng_timeout_cnt = 0;

//****************************************************Function Declaration******************************************************//
static void v_page_eng_enter(void);
static void v_page_eng_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_eng_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_eng_exit(void);

const DispPage_T G_tPageEng =
{
    .ePageId     = PAGE_ID_ENG_MODE,
    .vOnEnter    = v_page_eng_enter,
    .vOnUpdate   = v_page_eng_update,
    .bOnEvent    = b_page_eng_event,
    .vOnExit     = v_page_eng_exit,
    .usRefreshMs = dispPAGE_ENG_CYCLE_TIME_MS,
};

/***********************************************************************************************************************
 * 函数功能    : 重置工程模式无操作超时计数
 * 说明(备注)  : 供按键事件或外部系统任务刷新超时调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_EngModeResetTimeout(void)
{
    s_ul_eng_timeout_cnt = 0;
}

/***********************************************************************************************************************
 * 函数功能    : 进入工程模式页面
 * 说明(备注)  : 创建工程模式 UI 组件，立即刷新显存并点亮背光
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_enter(void)
{
    /* 确保 UI 已初始化 */
    vDisp_UiInit();

    /* 点亮背光 */
    bDisp_SwitchBacklight(DISP_BKL_ON, false);

    /* 创建工程模式专属 UI */
    vEngMode_UiCreate();

    /* 立即渲染首帧到显存 */
    lv_refr_now(NULL);

    /* 重置超时计数 */
    vDisp_EngModeResetTimeout();
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式页面帧更新
 * 说明(备注)  : 33ms 节拍驱动 eng_mode_ui_tick() 并检测 60 秒无操作自动关机
 * 传入参数    : p_data: 快照数据指针, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

    /* 驱动工程模式 UI 刷新 */
    vEngMode_UiTick();

    /* 无操作超时检测: 超过 60 秒自动关机 */
    s_ul_eng_timeout_cnt++;
    if (s_ul_eng_timeout_cnt >= dispPAGE_ENG_TIMEOUT_CNT)
    {
        if (uPrint.tFlag.bDispTask)
            sMyPrint("DispEng: eng mode timeout (60s), shutdown system\r\n");

        vEngMode_UiDelete();

        #if (boardHEAT_MANAGE_EN)
        vFan_ForceOpenFan(false);
        #endif  /* boardHEAT_MANAGE_EN */

        cSys_Switch(SO_KEY, ST_OFF, false);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式页面事件处理
 * 说明(备注)  : 捕获到按键事件时立即重置超时计时
 * 传入参数    : e_event: 事件枚举, ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传给框架默认处理
 ************************************************************************************************************************/
static bool b_page_eng_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)ul_param;

    if (e_event == DISP_EVT_KEY_SHORT || e_event == DISP_EVT_KEY_LONG)
        vDisp_EngModeResetTimeout();

    return false;   /* 透传重置息屏等默认行为 */
}

/***********************************************************************************************************************
 * 函数功能    : 退出工程模式页面
 * 说明(备注)  : 释放工程模式 UI 动态分配资源
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_exit(void)
{
    vEngMode_UiDelete();

    #if (boardHEAT_MANAGE_EN)
    vFan_ForceOpenFan(false);
    #endif  /* boardHEAT_MANAGE_EN */
}

#endif  /* boardDISPLAY_EN && boardENG_MODE_EN */
