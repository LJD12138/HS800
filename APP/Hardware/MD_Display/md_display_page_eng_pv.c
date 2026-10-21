/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_eng_pv.c
 * Date    : 2026-10-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 工程模式参数查看二/三级页面
 *           钻取链: PAGE_ID_ENG_MODE(主菜单) --Push--> PAGE_ID_ENG_PV --Push--> PAGE_ID_ENG_PV_DETAIL --Pop/Pop--> 回主菜单
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
#include "Middlewares/LVGL/lvgl.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/user_ui/eng_mode_ui.h"

//****************************************************Macros********************************************************************//
/* 参数查看页为静态页, 按键/导航由事件即时唤醒; 50ms 仅用于后台数据刷新 */
#define			dispPAGE_ENG_PV_CYCLE_MS				50U

//****************************************************Function Declaration******************************************************//
static void v_page_eng_pv_enter(void);
static void v_page_eng_pv_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_eng_pv_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_eng_pv_exit(void);

static void v_page_eng_pv_detail_enter(void);
static void v_page_eng_pv_detail_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_eng_pv_detail_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_eng_pv_detail_exit(void);

const DispPage_T G_tPageEngPv =
{
    .ePageId     = PAGE_ID_ENG_PV,
    .vOnEnter    = v_page_eng_pv_enter,
    .vOnUpdate   = v_page_eng_pv_update,
    .bOnEvent    = b_page_eng_pv_event,
    .vOnExit     = v_page_eng_pv_exit,
    .usRefreshMs = dispPAGE_ENG_PV_CYCLE_MS,
};

const DispPage_T G_tPageEngPvDetail =
{
    .ePageId     = PAGE_ID_ENG_PV_DETAIL,
    .vOnEnter    = v_page_eng_pv_detail_enter,
    .vOnUpdate   = v_page_eng_pv_detail_update,
    .bOnEvent    = b_page_eng_pv_detail_event,
    .vOnExit     = v_page_eng_pv_detail_exit,
    .usRefreshMs = dispPAGE_ENG_PV_CYCLE_MS,
};

/***********************************************************************************************************************
 * 函数功能    : 进入参数查看页 (页面栈二级页)
 * 说明(备注)  : 复用 eng_mode_ui 页内子级粒度生命周期: 创建/显示 PV UI, 标题随框架页更新
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_pv_enter(void)
{
    bDisp_Switch(ST_ON, false);                 /* 点亮背光 */
    vEngMode_PageEnter(ENG_PAGE_PARAM_VIEW);
    lv_refr_now(NULL);                          /* 立即渲染首帧到显存 */
}

/***********************************************************************************************************************
 * 函数功能    : 参数查看页帧更新
 * 说明(备注)  : 50ms 节拍驱动 eng_mode_ui_tick (含 PV 实时数据刷新与待处理导航请求)
 * 传入参数    : p_data: 快照数据指针, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_pv_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

    vEngMode_UiTick();
}

/***********************************************************************************************************************
 * 函数功能    : 参数查看页事件处理
 * 说明(备注)  : ENG 键路由由按键任务经 vEngMode_KeyXxx + ePendingNav 队列转入显示任务执行,
 *               本页不直接消费事件, 全部透传框架默认处理
 * 传入参数    : e_event: 事件枚举, ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传给框架默认处理
 ************************************************************************************************************************/
static bool b_page_eng_pv_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;

    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 退出参数查看页
 * 说明(备注)  : 粒度删除 PV UI (基础容器/标题栏由主菜单页与 ENG 场景生命周期管理)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_pv_exit(void)
{
    vEngMode_PageLeave(ENG_PAGE_PARAM_VIEW);
}

/***********************************************************************************************************************
 * 函数功能    : 进入参数查看详情页 (页面栈三级页)
 * 说明(备注)  : 读取 Push 带入的导航参数 (低 8 位=PV Tab 索引) 创建详情 UI; 同步内部状态机当前页(含基础容器按需重建)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_pv_detail_enter(void)
{
    bDisp_Switch(ST_ON, false);
    vEngMode_PageEnter(ENG_PAGE_PV_DETAIL);
    vEngPvDetail_UiCreate(ulDisp_GetNavParam());
    lv_refr_now(NULL);
}

/***********************************************************************************************************************
 * 函数功能    : 详情页帧更新
 * 说明(备注)  : 详情页为静态内容, 仅驱动 ENG 导航队列 (ePendingNav) 以保持键响应
 * 传入参数    : p_data: 快照数据指针, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_pv_detail_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

    vEngMode_UiTick();
}

/***********************************************************************************************************************
 * 函数功能    : 详情页事件处理
 * 说明(备注)  : 键路由同 PV 页, 全部透传
 * 传入参数    : e_event: 事件枚举, ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传给框架默认处理
 ************************************************************************************************************************/
static bool b_page_eng_pv_detail_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;

    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 退出详情页
 * 说明(备注)  : 删除详情 UI, 内部状态机当前页保持 ENG_PAGE_PV_DETAIL (返回后由 PV 页 enter 重置)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_pv_detail_exit(void)
{
    vEngPvDetail_UiDelete();
    vEngMode_PageLeave(ENG_PAGE_PV_DETAIL);
}

#endif  /* boardDISPLAY_EN && boardENG_MODE_EN */
