/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_fault.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 故障告警页面 (TFT+LVGL) - 故障码与告警图标轮显
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN)
#include "uni_disp_core.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/user_ui/main_1_ui.h"
#include "Middlewares/LVGL/lvgl.h"

//****************************************************Function Declaration******************************************************//
static void v_page_fault_enter(void);
static void v_page_fault_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_fault_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_fault_exit(void);

const DispPage_T G_tPageFault =
{
    .ePageId     = PAGE_ID_FAULT,
    .vOnEnter    = v_page_fault_enter,
    .vOnUpdate   = v_page_fault_update,
    .bOnEvent    = b_page_fault_event,
    .vOnExit     = v_page_fault_exit,
    .usRefreshMs = 33,
};

/***********************************************************************************************************************
 * 函数功能    : 进入故障页面
 * 说明(备注)  : 确保背光常亮，防止故障在黑屏状态下无法被用户观察到
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_fault_enter(void)
{
    /* 确保主工作界面可见 */
    if (lv_screen_active() != objects.main_work)
        vDisp_LoadScreen(SCREEN_ID_MAIN_WORK);

    /* 点亮背光 */
    bDisp_SwitchBacklight(DISP_BKL_ON, false);

    /* 立即更新一次故障码与报警状态 */
    vDisp_UpdateDevParam();
    bDisp_Main1DataUpdate();
}

/***********************************************************************************************************************
 * 函数功能    : 故障页面帧更新
 * 说明(备注)  : none
 * 传入参数    : p_data: 快照数据指针, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_fault_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

    /* 周期刷新错误码与图标闪烁状态 */
    vDisp_UpdateDevParam();
    bDisp_Main1DataUpdate();
}

/***********************************************************************************************************************
 * 函数功能    : 故障页面事件处理
 * 说明(备注)  : none
 * 传入参数    : e_event: 页面事件类型; ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : true: 已处理 DISP_EVT_FAULT_CLEAR 并退栈返回工作页; false: 非本页事件，透传框架默认处理
 ************************************************************************************************************************/
static bool b_page_fault_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)ul_param;

    if (e_event == DISP_EVT_FAULT_CLEAR)
    {
        /* 故障清除, 退栈返回主工作页 */
        vDisp_PopPage();
        return true;
    }

    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 退出故障页面
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_fault_exit(void)
{
    /* 无特殊清理 */
}

#endif  /* boardDISPLAY_EN */
