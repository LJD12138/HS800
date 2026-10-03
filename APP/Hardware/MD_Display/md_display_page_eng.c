/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_eng.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 工程模式页面 (TFT+LVGL) - 复用 eng_mode_ui.c 实现三段式生命周期
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
#include "Middlewares/LVGL/lvgl.h"

#if (boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_hm_task.h"
#endif  /* boardHEAT_MANAGE_EN */

//****************************************************Parameter Initialization**************************************************//
#define			dispPAGE_ENG_CYCLE_TIME_MS				33U

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
 * 函数功能    : 进入工程模式页面
 * 说明(备注)  : 创建工程模式 UI 组件，立即刷新显存并点亮背光
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_enter(void)
{
    /* 点亮背光 */
    bDisp_Switch(ST_ON, false);

    /* 创建工程模式专属 UI */
    vEngMode_UiCreate();

    /* 立即渲染首帧到显存 */
    lv_refr_now(NULL);
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式页面帧更新
 * 说明(备注)  : 33ms 节拍驱动 eng_mode_ui_tick();
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
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式页面事件处理
 * 说明(备注)  : 
 * 传入参数    : e_event: 事件枚举, ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传给框架默认处理
 ************************************************************************************************************************/
static bool b_page_eng_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;

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
