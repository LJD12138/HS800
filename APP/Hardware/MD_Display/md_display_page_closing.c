/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_closing.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 关机页面 - 关机画面展示与延时息屏
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

//****************************************************Parameter Initialization**************************************************//
static uint16_t s_us_close_wait_cnt = 0;
#define			dispPAGE_CLOSING_HOLD_CNT				(1500U / 33U)	/* 关机画面保持 1500ms (约 45 帧) */

//****************************************************Function Declaration******************************************************//
static void v_page_closing_enter(void);
static void v_page_closing_exit(void);
static void v_page_closing_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_closing_event(DispEvent_E e_event, uint32_t ul_param);

const DispPage_T G_tPageClosing =
{
    .ePageId     = PAGE_ID_CLOSING,
    .vOnEnter    = v_page_closing_enter,
	.vOnExit     = v_page_closing_exit,
    .vOnUpdate   = v_page_closing_update,
    .bOnEvent    = b_page_closing_event,
    .usRefreshMs = 33,
};

/***********************************************************************************************************************
 * 函数功能    : 进入关机页面
 * 说明(备注)  : 加载关机屏幕并点亮背光
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_closing_enter(void)
{
    s_us_close_wait_cnt = 0;
    vDisp_LoadScreen(SCREEN_ID_MAIN_CLOSING);
    bDisp_Switch(ST_ON, false);
}

/***********************************************************************************************************************
 * 函数功能    : 退出关机页面
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_closing_exit(void)
{
    s_us_close_wait_cnt = 0;
}

/***********************************************************************************************************************
 * 函数功能    : 关机页面帧更新
 * 说明(备注)  : 维持 1500ms 完整展示关机动画，防止主界面残影，倒计时结束后关闭背光
 * 传入参数    : p_data: 数据快照
 * 输出参数    : b_force: 是否强制刷新
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_closing_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

    if (s_us_close_wait_cnt < dispPAGE_CLOSING_HOLD_CNT)
    {
        s_us_close_wait_cnt++;
        if (s_us_close_wait_cnt >= dispPAGE_CLOSING_HOLD_CNT)
            /* 延时达到, 关闭背光 */
            bDisp_Switch(ST_OFF, false);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 关机页面事件处理
 * 说明(备注)  : none
 * 传入参数    : e_event: 页面事件类型; ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传框架默认处理（本页不消费任何事件）
 ************************************************************************************************************************/
static bool b_page_closing_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;
    return false;
}


#endif  /* boardDISPLAY_EN */

