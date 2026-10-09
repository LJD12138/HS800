/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_page_update.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 固件升级页面 (TFT+LVGL) - 升级进度/状态机/信息面板与倒计时重启
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN && boardUPDATE)
#include "uni_disp_core.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
#include "app_info.h"
#include "MD_Display/eez_ui/ui.h"
#include "MD_Display/eez_ui/vars.h"
#include "MD_Display/eez_ui/screens.h"
#include "Middlewares/LVGL/lvgl.h"
#include "board_config.h"
#include <stdio.h>
#include <string.h>

#define			dispTASK_UPDATA_CYCLE_TIME				33U
#define			UPDATE_TICK_TO_SEC(tick)				((uint16_t)((tick) * boardREPET_TIMER_CYCLE_TMIE / 1000))

//****************************************************Macros********************************************************************//
#define			dispUPDATE_ANIM_PERIOD_MS				200U	/*!< 等待动画步进周期：200ms */

#define			dispCOLOR_STATUS_NORMAL					0xAAAAAAU	/*!< 等待中状态文字颜色(灰) */
#define			dispCOLOR_STATUS_RUNNING				0xFFFFFFU	/*!< 升级中状态文字颜色(白) */
#define			dispCOLOR_STATUS_SUCCESS				0x4CAF50U	/*!< 升级成功状态文字颜色(绿) */
#define			dispCOLOR_STATUS_FAILURE				0xF44336U	/*!< 升级失败状态文字颜色(红) */

//****************************************************Parameter Initialization**************************************************//
typedef enum
{
	DUPD_STEP_INIT = 0,		/*!< 0: 初始化 */
	DUPD_STEP_PREPARE,		/*!< 1: 准备升级 */
	DUPD_STEP_UPGRADING,	/*!< 2: 升级中 */
	DUPD_STEP_SUCCESS,		/*!< 3: 升级成功 */
	DUPD_STEP_FAILURE,		/*!< 4: 升级失败 */
}DispUpdateStep_E;

//****************************************************Parameter Initialization**************************************************//
static DispUpdateStep_E S_eUpdateStep        = DUPD_STEP_PREPARE;
static uint32_t         S_ulStateTick        = 0;
static uint16_t         S_usLastFrmCnt       = 0;
static uint32_t         S_ulLastCountdownTick= 0;

/* 缓存上次显示值，避免每周期无意义的 UI 刷新，降低资源占用 */
static uint16_t         S_usLastDispFrmCnt   = 0xFFFFU;
static uint16_t         S_usLastDispTotalFrm = 0xFFFFU;
static uint16_t         S_usLastDispPercent  = 0xFFFFU;
static uint16_t         S_usLastDispTimeout  = 0xFFFFU;
static ModuleObject_E   S_eLastDispObj       = MO_INVAILD;   
static ChannelType_E    S_eLastDispCh        = CT_INVAILD;
static ProtoType_E      S_eLastDispProto     = PT_INVAILD;
static UpdateErrCode_E  S_eLastDispErrCode   = (UpdateErrCode_E)0xFFU;
static uint8_t          S_ucAnimCnt          = 0;
static uint8_t          S_ucLastAnimStep     = 0xFFU;

//****************************************************Function Declaration******************************************************//
static const char *pc_update_err_code_str(UpdateErrCode_E e_code);
static const char *pc_update_obj_str(ModuleObject_E e_obj);
static const char *pc_update_ch_str(ChannelType_E e_ch);
static const char *pc_update_proto_str(ProtoType_E e_proto);

static void     v_update_ui_init(void);
static void     v_update_ui_reset(uint32_t t_now_tick);
static void     v_update_ui_set_spinner_visible(bool b_visible);
static void     v_update_ui_set_status_color(uint32_t ul_color);
static void     v_update_ui_set_state(DispUpdateStep_E e_step);
static void     v_update_ui_refresh_info(void);
static uint16_t us_update_calc_percent(void);
static void     v_update_enter_step(DispUpdateStep_E e_step, uint32_t t_now_tick);

static int8_t   c_update_prepare_step(uint32_t t_now_tick);
static int8_t   c_update_upgrading_step(uint32_t t_now_tick);
static void     v_update_success_step(uint32_t t_now_tick);
static void     v_update_failure_step(uint32_t t_now_tick);

static void v_page_update_enter(void);
static void v_page_update_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_update_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_update_exit(void);

const DispPage_T G_tPageUpdate =
{
    .ePageId     = PAGE_ID_UPDATE,
    .vOnEnter    = v_page_update_enter,
    .vOnUpdate   = v_page_update_update,
    .bOnEvent    = b_page_update_event,
    .vOnExit     = v_page_update_exit,
    .usRefreshMs = dispTASK_UPDATA_CYCLE_TIME,
};

/***********************************************************************************************************************
 * 函数功能    : 进入升级页
 * 说明(备注)  : 
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_page_update_enter(void)
{
    uint32_t t_now_tick = xTaskGetTickCount();

    /* 载入升级专属屏幕并点亮背光 */
    vDisp_LoadScreen(SCREEN_ID_MAIN_UPDATE);
    bDisp_Switch(ST_ON, false);

    /* 复位状态机参数与显示缓存 */
    v_update_ui_reset(t_now_tick);

    /* 初始化 UI 绑定变量 */
    v_update_ui_init();

    S_eUpdateStep = DUPD_STEP_PREPARE;
}

/***********************************************************************************************************************
 * 函数功能    : 升级页面帧更新
 * 说明(备注)  : none
 * 传入参数    : p_data: 快照数据指针（本页未使用）; b_force: 强制刷新标志（本页未使用）
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_update_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

    uint32_t t_now_tick = xTaskGetTickCount();
    int8_t c_ret;

    switch (S_eUpdateStep)
    {
        case DUPD_STEP_PREPARE:
        {
            c_ret = c_update_prepare_step(t_now_tick);
            if (c_ret < 0)
                S_eUpdateStep = DUPD_STEP_FAILURE;
            else if (c_ret > 0)
                S_eUpdateStep = DUPD_STEP_UPGRADING;
        }
        break;

        case DUPD_STEP_UPGRADING:
        {
            c_ret = c_update_upgrading_step(t_now_tick);
            if (c_ret < 0)
                S_eUpdateStep = DUPD_STEP_FAILURE;
            else if (c_ret > 0)
                S_eUpdateStep = DUPD_STEP_SUCCESS;
        }
        break;

        case DUPD_STEP_SUCCESS:
        {
            v_update_success_step(t_now_tick);
        }
        break;

        case DUPD_STEP_FAILURE:
        {
            v_update_failure_step(t_now_tick);
        }
        break;

        default:
        {
        }
        break;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 升级页面事件处理
 * 说明(备注)  : none
 * 传入参数    : e_event: 页面事件类型; ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传框架默认处理（本页不消费任何事件）
 ************************************************************************************************************************/
static bool b_page_update_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;
    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 退出升级页面
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_update_exit(void)
{
    /* 复位升级界面状态 */
    v_update_ui_set_spinner_visible(false);
}

/***********************************************************************************************************************
 * 函数功能    : 准备升级步骤处理
 * 说明(备注)  : none
 * 传入参数    : t_now_tick: 当前系统 tick
 * 输出参数    : 无
 * 返回值      : int8_t: 1 进入升级中; -1 进入失败处理; 0 继续等待
 ************************************************************************************************************************/
static int8_t c_update_prepare_step(uint32_t t_now_tick)
{
    v_update_ui_set_state(DUPD_STEP_PREPARE);
    v_update_ui_refresh_info();

    /* 异常捕获：外部已设置错误码，直接进入失败处理 */
    if (tUpdate.eErrCode != UEF_NONE)
    {
        v_update_enter_step(DUPD_STEP_FAILURE, t_now_tick);
        v_update_ui_set_spinner_visible(false);
        return -1;
    }

    /* 检测到首帧接收，进入升级中状态：隐藏 Spinner，显示进度 Arc */
    if (tUpdate.usRecFrameCnt > 0U)
    {
        v_update_ui_set_spinner_visible(false);
        v_update_enter_step(DUPD_STEP_UPGRADING, t_now_tick);
        return 1;
    }

    /* 等待动画，每约 200ms 步进一次 */
    uint8_t anim_div = (uint8_t)(dispUPDATE_ANIM_PERIOD_MS / dispTASK_UPDATA_CYCLE_TIME);
    if (anim_div == 0)
        anim_div = 1;

    S_ucAnimCnt++;
    uint8_t anim_step = (S_ucAnimCnt / anim_div) % 3;
    if (S_ucLastAnimStep != anim_step)
    {
        S_ucLastAnimStep = anim_step;
        if (anim_step == 0)
            set_var_uca_update_msg("Waiting for update.");
        else if (anim_step == 1)
            set_var_uca_update_msg("Waiting for update..");
        else
            set_var_uca_update_msg("Waiting for update...");
    }

    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 升级中步骤处理
 * 说明(备注)  : none
 * 传入参数    : t_now_tick: 当前系统 tick
 * 输出参数    : 无
 * 返回值      : int8_t: 1 升级成功; -1 进入失败处理; 0 继续升级
 ************************************************************************************************************************/
static int8_t c_update_upgrading_step(uint32_t t_now_tick)
{
    uint16_t percent = us_update_calc_percent();

    v_update_ui_set_state(DUPD_STEP_UPGRADING);
    v_update_ui_refresh_info();

    /* 异常捕获：升级过程中检测到错误码，立即进入失败处理 */
    if (tUpdate.eErrCode != UEF_NONE)
    {
        v_update_enter_step(DUPD_STEP_FAILURE, t_now_tick);
        v_update_ui_set_status_color(dispCOLOR_STATUS_FAILURE);
        return -1;
    }

    set_var_uca_update_msg("Upgrading...");

    /* 确保状态文字为白色 */
    v_update_ui_set_status_color(dispCOLOR_STATUS_RUNNING);

    /* 仅在进度变化时刷新进度文本与 Arc */
    if (S_usLastDispPercent != percent)
    {
        S_usLastDispPercent = percent;

        char c_percent_str[10];
        snprintf(c_percent_str, sizeof(c_percent_str), "%u", percent);
        set_var_uca_update_progress(c_percent_str);

        if (objects.obj_progress_arc != NULL)
            lv_arc_set_value(objects.obj_progress_arc, percent);
    }

    /* 接收完所有帧，跳转升级成功 */
    if (tpSysTask != NULL && tpSysTask->ucStep > US_WAIT_COMPLETE)
    {
        v_update_enter_step(DUPD_STEP_SUCCESS, t_now_tick);
        v_update_ui_set_status_color(dispCOLOR_STATUS_SUCCESS);
        return 1;
    }

    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 升级成功步骤处理
 * 说明(备注)  : none
 * 传入参数    : t_now_tick: 当前系统 tick
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_update_success_step(uint32_t t_now_tick)
{
    v_update_ui_set_state(DUPD_STEP_SUCCESS);

    set_var_uca_update_progress("100");
    set_var_uca_update_msg("Update Complete!");

    /* 刷新 Arc 到 100 */
    if (S_usLastDispPercent != 100U)
    {
        S_usLastDispPercent = 100U;
        if (objects.obj_progress_arc != NULL)
            lv_arc_set_value(objects.obj_progress_arc, 100);
    }

    /* 将状态文字设为绿色 */
    v_update_ui_set_status_color(dispCOLOR_STATUS_SUCCESS);

    /* 每秒更新一次倒计时 */
    if ((t_now_tick - S_ulLastCountdownTick) >= pdMS_TO_TICKS(1000))
        S_ulLastCountdownTick = t_now_tick;

    char c_countdown_ok[24];
    snprintf(c_countdown_ok, sizeof(c_countdown_ok), "Reboot in %us", UPDATE_TICK_TO_SEC(tUpdate.usLostOverTimeCnt));
    set_var_uca_update_countdown(c_countdown_ok);
}

/***********************************************************************************************************************
 * 函数功能    : 升级失败步骤处理
 * 说明(备注)  : none
 * 传入参数    : t_now_tick: 当前系统 tick
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_update_failure_step(uint32_t t_now_tick)
{
    /* 错误已清除, 说明升级已重启, 回退到准备阶段 */
    if (tUpdate.eErrCode == UEF_NONE)
    {
        v_update_ui_reset(t_now_tick);
        v_update_ui_init();
        v_update_ui_set_spinner_visible(true);
        S_eUpdateStep = DUPD_STEP_PREPARE;
        return;
    }

    v_update_ui_set_state(DUPD_STEP_FAILURE);

    #if (boardKEY_EN)
    set_var_uca_update_msg("Failed! Short:retry Long:off");
    #else
    set_var_uca_update_msg("Update Failed!");
    #endif  /* boardKEY_EN */

    /* 刷新错误码信息 */
    v_update_ui_refresh_info();

    /* 将状态文字设为红色 */
    v_update_ui_set_status_color(dispCOLOR_STATUS_FAILURE);

    /* 每秒更新一次倒计时 */
    if ((t_now_tick - S_ulLastCountdownTick) >= pdMS_TO_TICKS(1000))
        S_ulLastCountdownTick = t_now_tick;

    char c_countdown_err[24];
    snprintf(c_countdown_err, sizeof(c_countdown_err), "Reboot in %us", UPDATE_TICK_TO_SEC(tUpdate.usLostOverTimeCnt));
    set_var_uca_update_countdown(c_countdown_err);
}

/***********************************************************************************************************************
 * 函数功能    : 重置升级显示状态变量
 * 说明(备注)  : none
 * 传入参数    : t_now_tick: 当前系统 tick
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_update_ui_reset(uint32_t t_now_tick)
{
    S_ulStateTick           = t_now_tick;
    S_usLastFrmCnt          = 0;
    S_ulLastCountdownTick   = t_now_tick;

    S_usLastDispFrmCnt      = 0xFFFFU;
    S_usLastDispTotalFrm    = 0xFFFFU;
    S_usLastDispPercent     = 0xFFFFU;
    S_usLastDispTimeout     = 0xFFFFU;
    S_eLastDispObj          = MO_INVAILD;   
    S_eLastDispCh           = CT_INVAILD;
    S_eLastDispProto        = PT_INVAILD;
    S_eLastDispErrCode      = (UpdateErrCode_E)0xFFU;
    S_ucAnimCnt             = 0;
    S_ucLastAnimStep        = 0xFFU;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化升级页面 UI 变量
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_update_ui_init(void)
{
    set_var_uca_update_progress("");
    set_var_uca_update_countdown("");
    set_var_uca_update_msg("Waiting for update...");
    set_var_uca_update_obj("Host");
    set_var_uca_update_channel("None");
    set_var_uca_update_proto("None");
    set_var_uca_update_frame("0000/0000");
    set_var_uca_update_timeout("000");
    set_var_uca_update_err_info("");

    v_update_ui_set_spinner_visible(true);

    if (objects.obj_progress_arc != NULL)
        lv_arc_set_value(objects.obj_progress_arc, 0);

    v_update_ui_set_status_color(dispCOLOR_STATUS_NORMAL);
    v_update_ui_set_state(DUPD_STEP_PREPARE);
}

/***********************************************************************************************************************
 * 函数功能    : 控制等待动画与 Arc 显示状态切换
 * 说明(备注)  : none
 * 传入参数    : b_visible: true 显示等待动画（隐藏进度 Arc）; false 显示进度 Arc（隐藏等待动画）
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_update_ui_set_spinner_visible(bool b_visible)
{
    if (objects.uc_update_spinner != NULL)
    {
        if (b_visible)
            lv_obj_remove_flag(objects.uc_update_spinner, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(objects.uc_update_spinner, LV_OBJ_FLAG_HIDDEN);
    }

    if (objects.obj_progress_arc != NULL)
    {
        if (b_visible)
            lv_obj_add_flag(objects.obj_progress_arc, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_remove_flag(objects.obj_progress_arc, LV_OBJ_FLAG_HIDDEN);
    }

    if (objects.obj_pct_label != NULL)
    {
        if (b_visible)
            lv_obj_add_flag(objects.obj_pct_label, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_remove_flag(objects.obj_pct_label, LV_OBJ_FLAG_HIDDEN);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 设置状态标签文字颜色
 * 说明(备注)  : none
 * 传入参数    : ul_color: 目标文字颜色（仅在颜色变化时写寄存器）
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_update_ui_set_status_color(uint32_t ul_color)
{
    static uint32_t s_ul_last_color = 0xFFFFFFFFU;

    if (s_ul_last_color != ul_color)
    {
        s_ul_last_color = ul_color;
        if (objects.obj_status_label != NULL)
            lv_obj_set_style_text_color(objects.obj_status_label, lv_color_hex(ul_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 根据当前步骤设置 UI 状态变量
 * 说明(备注)  : none
 * 传入参数    : e_step: 当前升级步骤（DispUpdateStep_E）
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_update_ui_set_state(DispUpdateStep_E e_step)
{
    int32_t state;

    switch (e_step)
    {
        case DUPD_STEP_UPGRADING:
        {
            state = 1;
        }
        break;

        case DUPD_STEP_SUCCESS:
        {
            state = 2;
        }
        break;

        case DUPD_STEP_FAILURE:
        {
            state = 3;
        }
        break;
        
        case DUPD_STEP_INIT:
        case DUPD_STEP_PREPARE:
        default:
        {
            state = 0;
        }
        break;
    }

    set_var_uca_update_state(state);
}

/***********************************************************************************************************************
 * 函数功能    : 刷新左侧信息面板数据
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_update_ui_refresh_info(void)
{
    if (S_eLastDispObj != tUpdate.eObj)
    {
        S_eLastDispObj = tUpdate.eObj;
        set_var_uca_update_obj(pc_update_obj_str(tUpdate.eObj));
    }

    if (S_eLastDispCh != tUpdate.eChType)
    {
        S_eLastDispCh = tUpdate.eChType;
        set_var_uca_update_channel(pc_update_ch_str(tUpdate.eChType));
    }

    if (S_eLastDispProto != tUpdate.eProtoType)
    {
        S_eLastDispProto = tUpdate.eProtoType;
        set_var_uca_update_proto(pc_update_proto_str(tUpdate.eProtoType));
    }

    if ((S_usLastDispFrmCnt != tUpdate.usRecFrameCnt) ||
        (S_usLastDispTotalFrm != tUpdate.usTotalFrmValue))
    {
        S_usLastDispFrmCnt   = tUpdate.usRecFrameCnt;
        S_usLastDispTotalFrm = tUpdate.usTotalFrmValue;

        char c_frame_str[32];
        snprintf(c_frame_str, sizeof(c_frame_str), "%04u/%04u", tUpdate.usRecFrameCnt, tUpdate.usTotalFrmValue);
        set_var_uca_update_frame(c_frame_str);
    }

    if (S_usLastDispTimeout != tUpdate.usLostOverTimeCnt)
    {
        S_usLastDispTimeout = tUpdate.usLostOverTimeCnt;

        char c_timeout_str[16];
        snprintf(c_timeout_str, sizeof(c_timeout_str), "%03u", UPDATE_TICK_TO_SEC(tUpdate.usLostOverTimeCnt));
        set_var_uca_update_timeout(c_timeout_str);
    }

    if (S_eLastDispErrCode != tUpdate.eErrCode)
    {
        S_eLastDispErrCode = tUpdate.eErrCode;
        set_var_uca_update_err_info(pc_update_err_code_str(tUpdate.eErrCode));
    }
}

/***********************************************************************************************************************
 * 函数功能    : 计算当前升级进度百分比
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : uint16_t: 升级进度百分比（0~100，总帧数为 0 时返回 0）
 ************************************************************************************************************************/
static uint16_t us_update_calc_percent(void)
{
    uint16_t us_total_frms = tUpdate.usTotalFrmValue;
    uint16_t us_rec_frms   = tUpdate.usRecFrameCnt;
    uint16_t percent       = 0;

    if (us_total_frms > 0U)
    {
        percent = (us_rec_frms * 100U) / us_total_frms;
        if (percent > 100U)
            percent = 100U;
    }

    return percent;
}

/***********************************************************************************************************************
 * 函数功能    : 进入新步骤前的公共处理
 * 说明(备注)  : none
 * 传入参数    : e_step: 目标步骤（当前实现未使用）; t_now_tick: 当前系统 tick
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_update_enter_step(DispUpdateStep_E e_step, uint32_t t_now_tick)
{
    (void)e_step;

    S_ulStateTick         = t_now_tick;
    S_ulLastCountdownTick = t_now_tick;
    S_usLastFrmCnt        = tUpdate.usRecFrameCnt;
}

/***********************************************************************************************************************
 * 函数功能    : 将升级错误码转换为显示字符串
 * 说明(备注)  : none
 * 传入参数    : e_code: 升级错误码（UpdateErrCode_E）
 * 输出参数    : 无
 * 返回值      : 错误码对应的显示字符串常量（无匹配时返回 "unknown err"）
 ************************************************************************************************************************/
static const char *pc_update_err_code_str(UpdateErrCode_E e_code)
{
    switch (e_code)
    {
        case UEF_NONE:                    return "";
        /* md_dcac_rec_data_proc.c (01~12) */
        case UEF_DR_F1_CHECK_FAIL:        return "01 F1 check fail";
        case UEF_DR_F3_BAUD_INVALID:      return "02 F3 baud invalid";
        case UEF_DR_F3_CHECK_FAIL:        return "03 F3 check fail";
        case UEF_DR_F3_SET_BAUD_FAIL:     return "04 F3 set baud fail";
        case UEF_DR_F7_CHECK_FAIL:        return "05 F7 check fail";
        case UEF_DR_A2_REPLY_ERR:         return "06 A2 reply err";
        case UEF_DR_A4_SEQ_MISMATCH:      return "07 A4 seq mismatch";
        case UEF_DR_A4_REPLY_ERR:         return "08 A4 reply err";
        case UEF_DR_A6_CHECK_FAIL:        return "09 A6 check fail";
        case UEF_DR_A6_REPLY_ERR:         return "10 A6 reply err";
        case UEF_DR_A6_NOT_COMPLETE:      return "11 A6 incomplete";
        case UEF_DR_ERR_FRAME:            return "12 err frame";
        /* md_dcac_prot_frame.c (13) */
        case UEF_DP_F2_INVALID_BAUD:      return "13 F2 invalid baud";
        /* md_dcac_queue_task_update.c (14~30) */
        case UEF_DQ_PROTO_INIT_FAIL:      return "14 proto init fail";
        case UEF_DQ_INVALID_OBJ:          return "15 invalid obj";
        case UEF_DQ_BUFF_NULL:            return "16 buff null";
        case UEF_DQ_CANCEL_REQ:           return "17 DCAC cancel";
        case UEF_DQ_F0_RETRY_OVER:        return "18 F0 retry over";
        case UEF_DQ_F6_RETRY_OVER:        return "19 F6 retry over";
        case UEF_DQ_F2_RETRY_OVER:        return "20 F2 retry over";
        case UEF_DQ_C4_RETRY_OVER:        return "21 C4 retry over";
        case UEF_DQ_A2_RETRY_OVER:        return "22 A2 retry over";
        case UEF_DQ_A2_RESEND_LEN_ERR:    return "23 A2 resend len err";
        case UEF_DQ_A2_RESEND_PEEK_FAIL:  return "24 A2 resend peek fail";
        case UEF_DQ_C5_RETRY_OVER:        return "25 C5 retry over";
        case UEF_DQ_A3_LEN_RETRY_OVER:    return "26 A3 len retry over";
        case UEF_DQ_A3_RETRY_OVER:        return "27 A3 retry over";
        case UEF_DQ_A4_RESEND_OVER:       return "28 A4 resend over";
        case UEF_DQ_A5_RETRY_OVER:        return "29 A5 retry over";
        case UEF_DQ_A6_WAIT_RETRY_OVER:   return "30 A6 wait retry over";
        /* md_bms_queue_task_update.c (31~34) */
        case UEF_BQ_INIT_BUFF_NULL:       return "31 BMS init buff null";
        case UEF_BQ_PENDING_FAIL:         return "32 BMS pending fail";
        case UEF_BQ_INVALID_OBJ:          return "33 BMS invalid obj";
        case UEF_BQ_BUFF_NULL:            return "34 BMS buff null";
        /* print_queue_task_update.c (35~36) */
        case UEF_PQ_INVALID_OBJ:          return "35 Print invalid obj";
        case UEF_PQ_BUFF_NULL:            return "36 Print buff null";
        /* print_update_dcac.c (37~50) */
        case UEF_PD_PREP_TASK_NULL:       return "37 prep task null";
        case UEF_PD_TRANS_TASK_NULL:      return "38 trans task null";
        case UEF_PD_TRANS_BUFF_NULL:      return "39 trans buff null";
        case UEF_PD_SLAVE_RESULT_ERR:     return "40 slave result err";
        case UEF_PD_C2_LEN_ERR:           return "41 C2 len err";
        case UEF_PD_C2_PROTO_ERR:         return "42 C2 proto err";
        case UEF_PD_C2_REPLY_FAIL:        return "43 C2 reply fail";
        case UEF_PD_C5_HEAD_LEN_ERR:      return "44 C5 head len err";
        case UEF_PD_HEAD_PARSE_FAIL:      return "45 head parse fail";
        case UEF_PD_CACHE_FULL:           return "46 cache full";
        case UEF_PD_CACHE_WRITE_FAIL:     return "47 cache write fail";
        case UEF_PD_HEAD_SEND_FAIL:       return "48 head send fail";
        case UEF_PD_FW_SEND_FAIL:         return "49 fw send fail";
        case UEF_PD_FW_CACHE_FAIL:        return "50 fw cache fail";
        /* Print_update_bms.c (51~61) */
        case UEF_PB_PREP_TASK_NULL:       return "51 prep task null";
        case UEF_PB_C2_LEN_ERR:           return "52 C2 len err";
        case UEF_PB_C2_PROTO_ERR:         return "53 C2 proto err";
        case UEF_PB_C2_PROTO_SELECT_FAIL: return "54 C2 proto select fail";
        case UEF_PB_C2_FWD_FAIL:          return "55 C2 fwd fail";
        case UEF_PB_TRANS_TASK_NULL:      return "56 trans task null";
        case UEF_PB_TRANS_BUFF_NULL:      return "57 trans buff null";
        case UEF_PB_SLAVE_RESULT_ERR:     return "58 slave result err";
        case UEF_PB_FINISH_MISMATCH:      return "59 finish mismatch";
        case UEF_PB_C5_DATA_ERR:          return "60 C5 data err";
        case UEF_PB_C5_FWD_FAIL:          return "61 C5 fwd fail";
        /* sys_queue_task_update.c (62) */
        case UEF_S_REC_OVERTIME:          return "62 rec overtime";
        default:                          return "unknown err";
    }
}

/***********************************************************************************************************************
 * 函数功能    : 将升级对象转换为显示字符串
 * 说明(备注)  : none
 * 传入参数    : e_obj: 升级对象（ModuleObject_E）
 * 输出参数    : 无
 * 返回值      : 对象名称常量字符串（无匹配时返回 "Invalid"）
 ************************************************************************************************************************/
static const char *pc_update_obj_str(ModuleObject_E e_obj)
{
    switch (e_obj)
    {
        case MO_DEFAULT: return "Host";
        case MO_CONSOLE: return "Console";
        case MO_BMS:     return "BMS";
        case MO_MPPT:    return "MPPT";
        case MO_DCAC:    return "DCAC";
        case MO_MGMT_AC: return "MGMT_AC";
        case MO_MGMT_DC: return "MGMT_DC";
        default:         return "Invalid";
    }
}

/***********************************************************************************************************************
 * 函数功能    : 将升级通道转换为显示字符串
 * 说明(备注)  : none
 * 传入参数    : e_ch: 升级通道（ChannelType_E）
 * 输出参数    : 无
 * 返回值      : 通道名称常量字符串（无匹配时返回 "Invalid"）
 ************************************************************************************************************************/
static const char *pc_update_ch_str(ChannelType_E e_ch)
{
    switch (e_ch)
    {
        case CT_NULL:    return "None";
        case CT_CONSOLE: return "Console";
        case CT_PRINT:   return "Print";
        default:         return "Invalid";
    }
}

/***********************************************************************************************************************
 * 函数功能    : 将升级协议转换为显示字符串
 * 说明(备注)  : none
 * 传入参数    : e_proto: 升级协议（ProtoType_E）
 * 输出参数    : 无
 * 返回值      : 协议名称常量字符串（无匹配时返回 "Invalid"）
 ************************************************************************************************************************/
static const char *pc_update_proto_str(ProtoType_E e_proto)
{
    switch (e_proto)
    {
        case PT_NULL:    return "None";
        case PT_XMODEM:  return "Xmodem";
        case PT_BAIKU:   return "Baiku";
        case PT_MEGMEET: return "Megmeet";
        default:         return "Invalid";
    }
}

#endif  /* boardDISPLAY_EN && boardUPDATE */
