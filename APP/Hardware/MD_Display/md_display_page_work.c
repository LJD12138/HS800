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

//****************************************************Macros********************************************************************//
/* 主工作页帧周期: 20ms=50FPS (能量环/呼吸动画); 若实测渲染超时/CPU 偏高, 可改为 33ms(30FPS) 或 50ms 对比 */
#define			dispPAGE_WORK_CYCLE_MS					20U

//****************************************************Function Declaration******************************************************//
static void v_page_work_enter(void);
static void v_page_work_exit(void);
static void v_page_work_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_work_event(DispEvent_E e_event, uint32_t ul_param);


const DispPage_T G_tPageWorkMain1 =
{
    .ePageId     = PAGE_ID_WORK_MAIN1,
    .vOnEnter    = v_page_work_enter,
    .vOnExit     = v_page_work_exit,
    .vOnUpdate   = v_page_work_update,
    .bOnEvent    = b_page_work_event,
    .usRefreshMs = dispPAGE_WORK_CYCLE_MS,
};

/***********************************************************************************************************************
 * 函数功能    : 进入工作页面
 * 说明(备注)  : 加载主工作界面后, 先同步渲染并推出静态框架(标签/图标/数据), 缩短切屏等待感知;
 *               能量环/呼吸动画等重组件随后创建, 并同步推出其首绘, 页面进入即完整呈现
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_work_enter(void)
{
    DispDataSnapshot_T t_snapshot;

    /* 确保当前活动屏幕为主工作屏幕 */
    if (lv_screen_active() != objects.main_work)
        vDisp_LoadScreen(SCREEN_ID_MAIN_WORK);

    /* 抓取一次快照用于首帧静态渲染 */
    vDisp_DataCaptureSnapshot(&t_snapshot);

    /* 初始刷新一次 EEZ 静态数据 (仅设置控件属性, 标记脏区域; 不触碰能量环避免其提前创建) */
    vDisp_UpdateDevParam(&t_snapshot);

    lv_refr_now(NULL);

    /* 再启动自绘组件与动画 (创建能量环等重组件) */
    vDisp_Main1UiStart();

    /* 能量环对象已创建, 补一次主界面数据刷新写入 SOC/充电状态 */
    bDisp_Main1DataUpdate(&t_snapshot);

    /* 进入工作页点亮背光 (bDisp_Switch 内部幂等: 已亮屏仅重置息屏倒计时) */
    bDisp_Switch(ST_ON, false);
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

/***********************************************************************************************************************
 * 函数功能    : 工作页面帧更新
 * 说明(备注)  : 驱动 EEZ 数据绑定与参数周期更新
 * 传入参数    : p_data: 快照数据指针, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_work_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)b_force;

    /* 周期更新设备数据与图标动画 (数据源: 帧快照) */
    vDisp_UpdateDevParam(p_data);
    bDisp_Main1DataUpdate(p_data);
}

/***********************************************************************************************************************
 * 函数功能    : 工作页事件回调 (WORK 组入口页)
 * 说明(备注)  : WORK 组当前唯一页面, 无组内跳转; 按键透传框架默认处理 (重置息屏倒计时)
 * 传入参数    : e_event: 事件类型
 * 输出参数    : ul_param: 事件参数
 * 返回值      : true: 已消费; false: 透传框架默认处理
 ************************************************************************************************************************/
static bool b_page_work_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;
    return false;   /* 透传框架默认处理: 按键重置息屏倒计时 */
}

#endif  /* boardDISPLAY_EN */
