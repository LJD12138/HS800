/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_init.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 初始化页面 (TFT+LVGL) - 系统上电初始化态息屏与环境准备
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
#include "Sys/sys_task.h"

//****************************************************Function Declaration******************************************************//
static void v_page_init_enter(void);
static void v_page_init_exit(void);
static void v_page_init_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_init_event(DispEvent_E e_event, uint32_t ul_param);

const DispPage_T G_tPageInit =
{
    .ePageId     = PAGE_ID_INIT,
    .vOnEnter    = v_page_init_enter,
    .vOnUpdate   = v_page_init_update,
    .bOnEvent    = b_page_init_event,
    .vOnExit     = v_page_init_exit,
    .usRefreshMs = 100,
};

/***********************************************************************************************************************
 * 函数功能    : 进入初始化页
 * 说明(备注)  : 系统上电自检初始化阶段，完成屏幕驱动与LVGL初始化，保持息屏并置位任务完成标志
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_init_enter(void)
{
    /* 1. 初始化 LVGL 核心及屏幕驱动 (处于显示任务上下文，RTOS调度器已启动) */
    vDisp_Init();

    /* 2. 保持息屏 */
    bDisp_SwitchBacklight(DISP_BKL_OFF, false);
}

/***********************************************************************************************************************
 * 函数功能    : 退出初始化页
 * 说明(备注)  : 离开初始化阶段进入启动动画
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_init_exit(void)
{
    /* 标记系统显示任务已就绪 */
    tSysInfo.uInit.tFinish.bIF_DispTask = 1;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化页帧刷新
 * 说明(备注)  : 保持息屏状态
 * 传入参数    : p_data: 数据快照, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_init_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

    bDisp_SwitchBacklight(DISP_BKL_OFF, false);
}

/***********************************************************************************************************************
 * 函数功能    : 初始化页事件回调 (透传框架默认处理)
 * 说明(备注)  : 无需新增事件处理
 * 传入参数    : e_event: 事件类型, ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false (事件透传给框架)
 ************************************************************************************************************************/
static bool b_page_init_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;
    return false;
}

#endif  /* boardDISPLAY_EN */
