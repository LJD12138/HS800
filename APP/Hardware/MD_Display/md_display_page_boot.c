/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_boot.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 启动页面 (TFT+LVGL) - 开机进度条动画与工作态平滑过渡
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
#include "MD_Display/md_display_api.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/user_ui/main_1_ui.h"
#include "Middlewares/LVGL/lvgl.h"

//****************************************************Parameter Initialization**************************************************//
static uint8_t s_uc_loading_step = 0;

//****************************************************Function Declaration******************************************************//
static void v_page_boot_enter(void);
static void v_page_boot_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_boot_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_boot_exit(void);

const DispPage_T G_tPageBoot =
{
    .ePageId     = PAGE_ID_BOOT,
    .vOnEnter    = v_page_boot_enter,
    .vOnUpdate   = v_page_boot_update,
    .bOnEvent    = b_page_boot_event,
    .vOnExit     = v_page_boot_exit,
    .usRefreshMs = 33,
};

/***********************************************************************************************************************
 * 函数功能    : 进入开机页面
 * 说明(备注)  : 初始化 UI 环境、加载开机屏幕、清零进度条并开启背光
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_boot_enter(void)
{
    /* 确保 LVGL UI 已完成全局初始化 */
    vDisp_UiInit();

    /* 加载开机画面 */
    vDisp_LoadScreen(SCREEN_ID_MAIN_BOOTING);

    /* 重置进度条 */
    s_uc_loading_step = 0;
    if (objects.uc_booting_bar != NULL)
        lv_bar_set_value(objects.uc_booting_bar, 0, LV_ANIM_OFF);

    /* 刷新设备参数 */
    vDisp_UpdateDevParam();

    /* 开启背光 */
    bDisp_SwitchBacklight(DISP_BKL_ON, false);
}

/***********************************************************************************************************************
 * 函数功能    : 开机页面帧更新
 * 说明(备注)  : 33ms 节拍步进进度条；当进度达到 100% 且系统已进入 DS_WORK 时，显式请求切换至工作页
 * 传入参数    : p_data: 只读原子数据快照指针, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_boot_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)b_force;

    /* 步进进度条 */
    if (s_uc_loading_step < 100)
    {
        s_uc_loading_step += 3;
        if (s_uc_loading_step > 100)
            s_uc_loading_step = 100;

        if (objects.uc_booting_bar != NULL)
            lv_bar_set_value(objects.uc_booting_bar, s_uc_loading_step, LV_ANIM_ON);
    }

    /* 当进度条走满且系统已经处于工作态时，请求切入工作页面 */
    if ((p_data->ucDevState == DS_WORK) && (s_uc_loading_step >= 100))
        vDisp_RequestPage(PAGE_ID_WORK);
}

/***********************************************************************************************************************
 * 函数功能    : 开机页面事件处理
 * 说明(备注)  : none
 * 传入参数    : e_event: 页面事件类型; ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传框架默认处理（本页不消费任何事件）
 ************************************************************************************************************************/
static bool b_page_boot_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;
    return false;   /* 透传框架默认处理 */
}

/***********************************************************************************************************************
 * 函数功能    : 退出开机页面
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_boot_exit(void)
{
    /* 无特殊退出清理 */
}

#endif  /* boardDISPLAY_EN */
