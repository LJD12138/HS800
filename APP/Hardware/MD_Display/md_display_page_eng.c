/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_eng.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 工程模式页面 (TFT+LVGL) - 主菜单/记忆参数设置/系统设置/确认对话框
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

//****************************************************Macros********************************************************************//
/* 工程模式菜单为静态页, 按键更新由事件即时唤醒; 50ms 仅用于后台数据刷新 */
#define			dispPAGE_ENG_CYCLE_TIME_MS				50U

//****************************************************Function Declaration******************************************************//
static void v_page_eng_enter(void);
static void v_page_eng_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_eng_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_eng_exit(void);

static void v_page_eng_ps_enter(void);
static void v_page_eng_ss_enter(void);
static void v_page_eng_cfm_enter(void);
static void v_page_eng_sub_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_eng_sub_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_eng_ps_exit(void);
static void v_page_eng_ss_exit(void);
static void v_page_eng_cfm_exit(void);

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
 * 函数功能    : 退出工程模式页面
 * 说明(备注)  : 释放工程模式 UI 动态分配资源
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_exit(void)
{
    /* 有返回链(压栈进入子页)时仅释放菜单并保留基础容器;
       场景切换/overlay 抢占/弹栈到根时由 PageLeave 整树释放, 避免 ENG 资源跨场景驻留 */
    vEngMode_PageLeave(ENG_PAGE_MAIN_MENU);
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式页面帧更新
 * 说明(备注)  : 50ms 节拍驱动 eng_mode_ui_tick();
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

/*================记忆参数设置 / 系统设置 / 确认对话框 (框架页面栈二级/三级页)================*/
const DispPage_T G_tPageEngParamSet =
{
    .ePageId     = PAGE_ID_ENG_PARAM_SET,
    .vOnEnter    = v_page_eng_ps_enter,
    .vOnExit     = v_page_eng_ps_exit,
    .vOnUpdate   = v_page_eng_sub_update,
    .bOnEvent    = b_page_eng_sub_event,
    .usRefreshMs = dispPAGE_ENG_CYCLE_TIME_MS,
};

const DispPage_T G_tPageEngSysSet =
{
    .ePageId     = PAGE_ID_ENG_SYS_SET,
    .vOnEnter    = v_page_eng_ss_enter,
    .vOnExit     = v_page_eng_ss_exit,
    .vOnUpdate   = v_page_eng_sub_update,
    .bOnEvent    = b_page_eng_sub_event,
    .usRefreshMs = dispPAGE_ENG_CYCLE_TIME_MS,
};

const DispPage_T G_tPageEngConfirm =
{
    .ePageId     = PAGE_ID_ENG_CONFIRM,
    .vOnEnter    = v_page_eng_cfm_enter,
    .vOnExit     = v_page_eng_cfm_exit,
    .vOnUpdate   = v_page_eng_sub_update,
    .bOnEvent    = b_page_eng_sub_event,
    .usRefreshMs = dispPAGE_ENG_CYCLE_TIME_MS,
};

/***********************************************************************************************************************
 * 函数功能    : 进入记忆参数设置页
 * 说明(备注)  : 点亮背光 -> 创建/显示参数设置 UI -> 同步首帧
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_ps_enter(void)
{
    bDisp_Switch(ST_ON, false);
    vEngMode_PageEnter(ENG_PAGE_PARAM_SET);
    lv_refr_now(NULL);
}

/***********************************************************************************************************************
 * 函数功能    : 进入系统设置页
 * 说明(备注)  : 点亮背光 -> 创建/显示系统设置 UI -> 同步首帧
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_ss_enter(void)
{
    bDisp_Switch(ST_ON, false);
    vEngMode_PageEnter(ENG_PAGE_SYS_SET);
    lv_refr_now(NULL);
}

/***********************************************************************************************************************
 * 函数功能    : 进入确认对话框页
 * 说明(备注)  : 点亮背光 -> 在基础容器上创建确认遮罩 -> 同步首帧
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_cfm_enter(void)
{
    bDisp_Switch(ST_ON, false);
    vEngMode_PageEnter(ENG_PAGE_CONFIRM);
    lv_refr_now(NULL);
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式子页帧更新
 * 说明(备注)  : 50ms 节拍驱动 eng_mode_ui_tick (含导航请求与数据刷新)
 * 传入参数    : p_data: 快照数据指针; b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_sub_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

    vEngMode_UiTick();
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式子页事件处理
 * 说明(备注)  : 按键导航由按键任务经 ePendingNav 转入显示任务执行, 页面不直接消费事件
 * 传入参数    : e_event: 事件枚举; ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传框架默认处理
 ************************************************************************************************************************/
static bool b_page_eng_sub_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;

    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 退出记忆参数设置页
 * 说明(备注)  : 有返回链时仅删本页; 离场景时整树释放
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_ps_exit(void)
{
    vEngMode_PageLeave(ENG_PAGE_PARAM_SET);
}

/***********************************************************************************************************************
 * 函数功能    : 退出系统设置页
 * 说明(备注)  : 有返回链时仅删本页; 离场景时整树释放
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_ss_exit(void)
{
    vEngMode_PageLeave(ENG_PAGE_SYS_SET);
}

/***********************************************************************************************************************
 * 函数功能    : 退出确认对话框页
 * 说明(备注)  : 有返回链时仅删本页; 离场景时整树释放
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_cfm_exit(void)
{
    vEngMode_PageLeave(ENG_PAGE_CONFIRM);
}

#endif  /* boardDISPLAY_EN && boardENG_MODE_EN */

