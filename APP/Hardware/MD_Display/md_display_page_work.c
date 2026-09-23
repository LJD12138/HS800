/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_work.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 主工作页面 (TFT+LVGL) - 调度 EEZ 工作界面与 user_ui 实时渲染
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

//****************************************************Function Declaration******************************************************//
static void v_page_work_enter(void);
static void v_page_work_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_work_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_work_exit(void);

const DispPage_T G_tPageWork =
{
    .ePageId     = PAGE_ID_WORK,
    .vOnEnter    = v_page_work_enter,
    .vOnUpdate   = v_page_work_update,
    .bOnEvent    = b_page_work_event,
    .vOnExit     = v_page_work_exit,
    .usRefreshMs = 33,
};

/***********************************************************************************************************************
 * 函数功能    : 进入工作页面
 * 说明(备注)  : 加载主工作界面、启动自绘组件与动画，开启背光
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_work_enter(void)
{
    /* 确保当前活动屏幕为主工作屏幕 */
    if (lv_screen_active() != objects.main_work)
        vDisp_LoadScreen(SCREEN_ID_MAIN_WORK);

    /* 启动用户自定义自绘组件与动画 */
    vDisp_Main1UiStart();

    /* 初始刷新一次数据 */
    vDisp_UpdateDevParam();
    bDisp_Main1DataUpdate();

    /* 开启背光并启动自动息屏管理 */
    bDisp_SwitchBacklight(DISP_BKL_ON, false);
}

/***********************************************************************************************************************
 * 函数功能    : 工作页面帧更新
 * 说明(备注)  : 33ms 节拍驱动 EEZ 数据绑定与自绘部件更新
 * 传入参数    : p_data: 快照数据指针, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_work_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

    /* 周期更新设备数据与图标动画 */
    vDisp_UpdateDevParam();
    bDisp_Main1DataUpdate();
}

/***********************************************************************************************************************
 * 函数功能    : 工作页事件回调
 * 说明(备注)  : 断码屏工作页不消费按键, 透传框架默认处理(重置息屏倒计时)
 * 传入参数    : e_event: 事件类型
 * 输出参数    : ul_param: 事件参数
 * 返回值      : false (事件透传给框架, 不触发自动跳转)
 ************************************************************************************************************************/
static bool b_page_work_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;
    return false;   /* 透传框架默认处理: 按键重置息屏倒计时, 故障自动弹窗等 */
}

/***********************************************************************************************************************
 * 函数功能    : 退出工作页面
 * 说明(备注)  : 停止主工作界面自绘动画，清理资源
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_work_exit(void)
{
    vDisp_Main1Exit();
}

#endif  /* boardDISPLAY_EN */
