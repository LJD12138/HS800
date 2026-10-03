/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_shut_down.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 关机状态页面- 松开电源键即息屏
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"

#include "uni_disp_core.h"

//****************************************************Function Declaration******************************************************//
static void v_page_shut_down_enter(void);
static void v_page_shut_down_exit(void);
static void v_page_shut_down_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_shut_down_event(DispEvent_E e_event, uint32_t ul_param);

const DispPage_T G_tPageShutDown =
{
    .ePageId     = PAGE_ID_SHUT_DOWN,
    .vOnEnter    = v_page_shut_down_enter,
    .vOnExit     = v_page_shut_down_exit,
    .vOnUpdate   = v_page_shut_down_update,
    .bOnEvent    = b_page_shut_down_event,
    .usRefreshMs = 50,
};

/***********************************************************************************************************************
 * 函数功能    : 进入关机状态页
 * 说明(备注)  : 息屏
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_shut_down_enter(void)
{
    bDisp_Switch(ST_OFF, false);
}

/***********************************************************************************************************************
 * 函数功能    : 退出关机状态页
 * 说明(备注)  : 无需额外清理
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_shut_down_exit(void)
{
}

/***********************************************************************************************************************
 * 函数功能    : 关机状态页帧刷新
 * 说明(备注)  : DS_SHUT_DOWN 状态: 松开电源键即息屏, 按住电源键保持当前关机画面
 * 传入参数    : p_data: 数据快照, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_shut_down_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;
}

/***********************************************************************************************************************
 * 函数功能    : 关机状态页事件回调 (透传框架默认处理)
 * 说明(备注)  : 无需新增事件处理
 * 传入参数    : e_event: 事件类型, ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false (事件透传给框架)
 ************************************************************************************************************************/
static bool b_page_shut_down_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;
    return false;
}

#endif  /* boardDISPLAY_EN */
