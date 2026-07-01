/*******************************************************************************************************************************
 * Project : APP
 * Module  : G:\\1-Baiku_Projects\\25-HS800\\1.software\\HS800\\APP\\Hardware\\MD_Display
 * File    : md_display_queue_task_updata.c
 * Date    : 2026-06-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示-升级模式 - TFT+LVGL版本
 * -------------------------------------------------------
 * 升级流程（基于 tp_task->ucStep 的 5 步状态机）：
 *   0. DUPD_STEP_INIT      : 初始化显示环境、加载升级页面、复位状态变量
 *   1. DUPD_STEP_PREPARE   : 准备升级，等待握手/首帧，播放等待动画
 *   2. DUPD_STEP_UPGRADING : 升级进行中，实时刷新进度与信息面板
 *   3. DUPD_STEP_SUCCESS   : 升级成功，倒计时后安全重启
 *   4. DUPD_STEP_FAILURE   : 升级失败，记录错误码，倒计时后安全重启（故障恢复）
 *
 * 状态转换：
 *   INIT -> PREPARE -> UPGRADING -> SUCCESS -> END(reboot)
 *                    |            |
 *                    +------------+-> FAILURE -> END(reboot)
 *   PREPARE/UPGRADING 在超时或检测到错误码时跳转至 FAILURE。
 * -------------------------------------------------------
 * todo    :
 * 1. 配合 EEZ Studio 生成的 ui_font_default_14 字体和 vars.h 使用
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_queue_task.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
#include "app_info.h"
#include "MD_Display/eez_ui/ui.h"
#include "MD_Display/eez_ui/vars.h"
#include "lvgl.h"
#include "board_config.h"
#include <stdio.h>
#include <string.h>

#define     dispTASK_UPDATA_CYCLE_TIME          33
#define     UPDATE_TICK_TO_SEC(tick)            ((uint16_t)((tick) * boardREPET_TIMER_CYCLE_TMIE / 1000))

/* ========================================== 宏定义 ========================================== */
#define dispUPDATE_ANIM_PERIOD_MS           200U        /*!< 等待动画步进周期：200ms */

#define dispCOLOR_STATUS_NORMAL            0xAAAAAAU   /*!< 等待中状态文字颜色(灰) */
#define dispCOLOR_STATUS_RUNNING           0xFFFFFFU   /*!< 升级中状态文字颜色(白) */
#define dispCOLOR_STATUS_SUCCESS           0x4CAF50U   /*!< 升级成功状态文字颜色(绿) */
#define dispCOLOR_STATUS_FAILURE           0xF44336U   /*!< 升级失败状态文字颜色(红) */

/* ========================================== 步骤枚举 ========================================== */
typedef enum
{
    DUPD_STEP_INIT = 0,         /*!< 0: 初始化 */
    DUPD_STEP_PREPARE,          /*!< 1: 准备升级 */
    DUPD_STEP_UPGRADING,        /*!< 2: 升级中 */
    DUPD_STEP_SUCCESS,          /*!< 3: 升级成功 */
    DUPD_STEP_FAILURE,          /*!< 4: 升级失败 */
} DispUpdateStep_E;

//****************************************************Parameter Initialization************************************************//
static uint32_t S_ulStateTick = 0;           /* 记录状态机内部计时的 Tick */
static uint16_t S_usLastFrmCnt = 0;          /* 上一次接收到的升级帧数，用于超时检测 */
static uint32_t S_ulLastCountdownTick = 0;   /* 倒计时变化定时器 */

/* 缓存上次显示值，避免每周期无意义的 UI 刷新，降低资源占用 */
static uint16_t         S_usLastDispFrmCnt   = 0xFFFFU;
static uint16_t         S_usLastDispTotalFrm = 0xFFFFU;
static uint16_t         S_usLastDispPercent  = 0xFFFFU;
static uint16_t         S_usLastDispTimeout  = 0xFFFFU;
static UpdateObj_E      S_eLastDispObj       = UO_INVAILD;
static ChannelType_E    S_eLastDispCh        = CT_INVAILD;
static ProtoType_E      S_eLastDispProto     = PT_INVAILD;
static UpdateErrCode_E  S_eLastDispErrCode   = (UpdateErrCode_E)0xFFU;
static uint8_t          S_ucAnimCnt          = 0;
static uint8_t          S_ucLastAnimStep     = 0xFFU;

//****************************************************Function Declaration************************************************//
static const char *pc_update_err_code_str(UpdateErrCode_E e_code);
static const char *pc_update_obj_str(UpdateObj_E e_obj);
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

static void     v_update_prepare_step(Task_T *tp_task, uint32_t t_now_tick);
static void     v_update_upgrading_step(Task_T *tp_task, uint32_t t_now_tick);
static void     v_update_success_step(Task_T *tp_task, uint32_t t_now_tick);
static void     v_update_failure_step(Task_T *tp_task, uint32_t t_now_tick);

/***********************************************************************************************************************
-----函数功能   显示-升级模式 - TFT+LVGL版本
-----返回值     无
-----参数       无
-----说明       无
-----传入参数   tp_task
-----作者       LJD
-----日期       2026-07-01
************************************************************************************************************************/
void v_disp_queue_task_updata(Task_T *tp_task)
{
    /* 边界检查：任务指针为空 */
    if(tp_task == NULL)
        return;

    /* 若队列中有新任务，提前结束当前升级显示任务，让出执行权 */
    if(lwrb_get_full(&tp_task->tQueueBuff) > 0U)
    {
        cQueue_GotoStep(tp_task, STEP_END);
        return;
    }

    /* 系统状态异常时结束任务，避免在非升级模式下占用资源 */
    if(tSysInfo.eDevState != DS_UPDATE_MODE)
    {
        cQueue_GotoStep(tp_task, STEP_END);
        return;
    }
	
	if(tUpdate.eHostResult == UTR_CANCEL
		|| tUpdate.eHostResult == UTR_FAIL
		|| tUpdate.eSlaveResult == UTR_CANCEL
		|| tUpdate.eSlaveResult == UTR_FAIL)
	{
		v_update_ui_set_status_color(dispCOLOR_STATUS_FAILURE);
		cQueue_GotoStep(tp_task, DUPD_STEP_FAILURE);
	}

    uint32_t t_now_tick = xTaskGetTickCount();

    switch(tp_task->ucStep)
    {
        /*---------------- 步骤0：初始化升级显示环境 ----------------*/
        case DUPD_STEP_INIT:
        {
            if(tDisp.eDevState != DS_UPDATE_MODE)
                bDisp_SetDevState(DS_UPDATE_MODE);
            bDisp_Switch(ST_ON, false);

            /* 载入升级专属屏幕 */
            loadScreen(SCREEN_ID_MAIN_UPDATE);

            /* 复位状态机参数与显示缓存 */
            v_update_ui_reset(t_now_tick);

            /* 初始化 UI 绑定变量 */
            v_update_ui_init();

            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        /*---------------- 步骤1：准备升级（等待握手/首帧） ----------------*/
        case DUPD_STEP_PREPARE:
        {
            v_update_prepare_step(tp_task, t_now_tick);
        }
        break;

        /*---------------- 步骤2：升级进行中 ----------------*/
        case DUPD_STEP_UPGRADING:
        {
            v_update_upgrading_step(tp_task, t_now_tick);
        }
        break;

        /*---------------- 步骤3：升级成功倒计时 ----------------*/
        case DUPD_STEP_SUCCESS:
        {
            v_update_success_step(tp_task, t_now_tick);
        }
        break;

        /*---------------- 步骤4：升级失败倒计时与故障恢复 ----------------*/
        case DUPD_STEP_FAILURE:
        {
            v_update_failure_step(tp_task, t_now_tick);
        }
        break;

        default:
        {
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;
    }

    vDisp_UiRefresh();

    #if(boardUSE_OS)
    vTaskDelay(dispTASK_UPDATA_CYCLE_TIME);
    #endif
}

/***********************************************************************************************************************
-----函数功能   准备升级步骤处理
-----说明(备注) 等待首帧/握手，播放等待动画；超时或检测到错误码时进入失败处理
-----传入参数   tp_task: 任务对象指针
                t_now_tick: 当前 Tick
-----返回值     无
************************************************************************************************************************/
static void v_update_prepare_step(Task_T *tp_task, uint32_t t_now_tick)
{
    v_update_ui_set_state(DUPD_STEP_PREPARE);
    v_update_ui_refresh_info();

    /* 异常捕获：外部已设置错误码，直接进入失败处理 */
    if(tUpdate.eErrCode != UEF_NONE)
    {
        v_update_enter_step(DUPD_STEP_FAILURE, t_now_tick);
        v_update_ui_set_spinner_visible(false);
        cQueue_GotoStep(tp_task, DUPD_STEP_FAILURE);
        return;
    }

    /* 检测到首帧接收，进入升级中状态：隐藏 Spinner，显示进度 Arc */
    if(tUpdate.usRecFrameCnt > 0U)
    {
        v_update_ui_set_spinner_visible(false);
        v_update_enter_step(DUPD_STEP_UPGRADING, t_now_tick);
        cQueue_GotoStep(tp_task, DUPD_STEP_UPGRADING);
        return;
    }

    /* 等待动画，每约 200ms 步进一次，仅在变化时刷新，减少 UI 开销 */
    uint8_t anim_div = (uint8_t)(dispUPDATE_ANIM_PERIOD_MS / dispTASK_UPDATA_CYCLE_TIME);
    if(anim_div == 0)
        anim_div = 1;

    S_ucAnimCnt++;
    uint8_t anim_step = (S_ucAnimCnt / anim_div) % 3;
    if(S_ucLastAnimStep != anim_step)
    {
        S_ucLastAnimStep = anim_step;
        if(anim_step == 0)
            set_var_uca_update_msg("Waiting for update.");
        else if(anim_step == 1)
            set_var_uca_update_msg("Waiting for update..");
        else
            set_var_uca_update_msg("Waiting for update...");
    }
}

/***********************************************************************************************************************
-----函数功能   升级中步骤处理
-----说明(备注) 实时刷新进度、信息面板与错误码；完成后进入成功，丢帧超时进入失败
-----传入参数   tp_task: 任务对象指针
                t_now_tick: 当前 Tick
-----返回值     无
************************************************************************************************************************/
static void v_update_upgrading_step(Task_T *tp_task, uint32_t t_now_tick)
{
    uint16_t percent = us_update_calc_percent();

    v_update_ui_set_state(DUPD_STEP_UPGRADING);
    v_update_ui_refresh_info();

    /* 异常捕获：升级过程中检测到错误码，立即进入失败处理 */
    if(tUpdate.eErrCode != UEF_NONE)
    {
        v_update_enter_step(DUPD_STEP_FAILURE, t_now_tick);
        v_update_ui_set_status_color(dispCOLOR_STATUS_FAILURE);
        cQueue_GotoStep(tp_task, DUPD_STEP_FAILURE);
        return;
    }

    set_var_uca_update_msg("Upgrading...");

    /* 确保状态文字为白色 */
    v_update_ui_set_status_color(dispCOLOR_STATUS_RUNNING);

    /* 仅在进度变化时刷新进度文本与 Arc，降低渲染开销 */
    if(S_usLastDispPercent != percent)
    {
        S_usLastDispPercent = percent;

        char c_percent_str[10];
        snprintf(c_percent_str, sizeof(c_percent_str), "%u", percent);
        set_var_uca_update_progress(c_percent_str);

        if(objects.obj_progress_arc != NULL)
            lv_arc_set_value(objects.obj_progress_arc, percent);
    }

    /* 接收完所有帧，跳转升级成功 */
    if(percent >= 100U)
    {
        v_update_enter_step(DUPD_STEP_SUCCESS, t_now_tick);
        v_update_ui_set_status_color(dispCOLOR_STATUS_SUCCESS);
        cQueue_GotoStep(tp_task, STEP_NEXT);
        return;
    }
}

/***********************************************************************************************************************
-----函数功能   升级成功步骤处理
-----说明(备注) 显示成功状态与倒计时，倒计时结束后安全重启
-----传入参数   tp_task: 任务对象指针
                t_now_tick: 当前 Tick
-----返回值     无
************************************************************************************************************************/
static void v_update_success_step(Task_T *tp_task, uint32_t t_now_tick)
{
    v_update_ui_set_state(DUPD_STEP_SUCCESS);

    set_var_uca_update_progress("100");
    set_var_uca_update_msg("Update Complete!");

    /* 刷新 Arc 到 100 */
    if(S_usLastDispPercent != 100U)
    {
        S_usLastDispPercent = 100U;
        if(objects.obj_progress_arc != NULL)
            lv_arc_set_value(objects.obj_progress_arc, 100);
    }

    /* 将状态文字设为绿色 */
    v_update_ui_set_status_color(dispCOLOR_STATUS_SUCCESS);

    /* 每秒更新一次倒计时 */
    if((t_now_tick - S_ulLastCountdownTick) >= pdMS_TO_TICKS(1000))
        S_ulLastCountdownTick = t_now_tick;

    char c_countdown_ok[24];
    snprintf(c_countdown_ok, sizeof(c_countdown_ok), "Reboot in %us", tUpdate.usLostOverTimeCnt / 10);
    set_var_uca_update_countdown(c_countdown_ok);
}

/***********************************************************************************************************************
-----函数功能   升级失败步骤处理
-----说明(备注) 显示失败状态、错误码与倒计时；倒计时结束后安全重启，完成故障恢复
-----传入参数   tp_task: 任务对象指针
                t_now_tick: 当前 Tick
-----返回值     无
************************************************************************************************************************/
static void v_update_failure_step(Task_T *tp_task, uint32_t t_now_tick)
{
    v_update_ui_set_state(DUPD_STEP_FAILURE);

    set_var_uca_update_msg("Update Failed!");

    /* 刷新错误码信息，确保用户界面展示最新故障原因 */
    v_update_ui_refresh_info();

    /* 将状态文字设为红色 */
    v_update_ui_set_status_color(dispCOLOR_STATUS_FAILURE);

    /* 每秒更新一次倒计时 */
    if((t_now_tick - S_ulLastCountdownTick) >= pdMS_TO_TICKS(1000))
        S_ulLastCountdownTick = t_now_tick;

    char c_countdown_err[24];
    snprintf(c_countdown_err, sizeof(c_countdown_err), "Reboot in %us", tUpdate.usLostOverTimeCnt / 10);
    set_var_uca_update_countdown(c_countdown_err);
}

/***********************************************************************************************************************
-----函数功能   重置升级显示状态变量
-----说明(备注) 在初始化或步骤切换时调用，保证状态机计时与缓存一致性
-----传入参数   t_now_tick: 当前 Tick
-----返回值     无
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
    S_eLastDispObj          = UO_INVAILD;
    S_eLastDispCh           = CT_INVAILD;
    S_eLastDispProto        = PT_INVAILD;
    S_eLastDispErrCode      = (UpdateErrCode_E)0xFFU;
    S_ucAnimCnt             = 0;
    S_ucLastAnimStep        = 0xFFU;
}

/***********************************************************************************************************************
-----函数功能   初始化升级页面 UI 变量
-----说明(备注) 仅在步骤 0 调用一次，设置初始文字、颜色与控件可见性
-----返回值     无
************************************************************************************************************************/
static void v_update_ui_init(void)
{
    set_var_uca_update_progress("");
    set_var_uca_update_countdown("");
    set_var_uca_update_state(0);
    set_var_uca_update_obj("");
    set_var_uca_update_channel("");
    set_var_uca_update_proto("");
    set_var_uca_update_frame("");
    set_var_uca_update_timeout("");
    set_var_uca_update_err_info("");
    set_var_uca_update_msg("Waiting for update.");

    v_update_ui_set_status_color(dispCOLOR_STATUS_NORMAL);
    v_update_ui_set_spinner_visible(true);
}

/***********************************************************************************************************************
-----函数功能   设置等待 Spinner / 进度 Arc / 百分比标签的可见性
-----说明(备注) b_visible = true 显示 Spinner，隐藏 Arc/Pct；false 反之
-----传入参数   b_visible: 是否显示 Spinner
-----返回值     无
************************************************************************************************************************/
static void v_update_ui_set_spinner_visible(bool b_visible)
{
    if(objects.uc_update_spinner != NULL)
    {
        if(b_visible)
            lv_obj_remove_flag(objects.uc_update_spinner, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(objects.uc_update_spinner, LV_OBJ_FLAG_HIDDEN);
    }

    if(objects.obj_progress_arc != NULL)
    {
        if(b_visible)
            lv_obj_add_flag(objects.obj_progress_arc, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_remove_flag(objects.obj_progress_arc, LV_OBJ_FLAG_HIDDEN);
    }

    if(objects.obj_pct_label != NULL)
    {
        if(b_visible)
            lv_obj_add_flag(objects.obj_pct_label, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_remove_flag(objects.obj_pct_label, LV_OBJ_FLAG_HIDDEN);
    }
}

/***********************************************************************************************************************
-----函数功能   设置状态标签文字颜色（带缓存，减少重复 LVGL 样式设置）
-----传入参数   ul_color: RGB 颜色值
-----返回值     无
************************************************************************************************************************/
static void v_update_ui_set_status_color(uint32_t ul_color)
{
    static uint32_t s_ul_last_color = 0xFFFFFFFFU;

    if(s_ul_last_color != ul_color)
    {
        s_ul_last_color = ul_color;
        if(objects.obj_status_label != NULL)
            lv_obj_set_style_text_color(objects.obj_status_label, lv_color_hex(ul_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

/***********************************************************************************************************************
-----函数功能   根据当前步骤设置 UI 状态变量
-----说明(备注) 与 EEZ 生成的 state 变量保持一致：0 等待，1 升级中，2 成功，3 失败
-----传入参数   e_step: 当前升级步骤
-----返回值     无
************************************************************************************************************************/
static void v_update_ui_set_state(DispUpdateStep_E e_step)
{
    int32_t state;

    switch(e_step)
    {
        case DUPD_STEP_UPGRADING:
            state = 1;
            break;
        case DUPD_STEP_SUCCESS:
            state = 2;
            break;
        case DUPD_STEP_FAILURE:
            state = 3;
            break;
        case DUPD_STEP_INIT:
        case DUPD_STEP_PREPARE:
        default:
            state = 0;
            break;
    }

    set_var_uca_update_state(state);
}

/***********************************************************************************************************************
-----函数功能   刷新左侧信息面板数据
-----说明(备注) 采用缓存对比，仅当数据源变化时才更新 UI 字符串，降低格式化与渲染开销
-----返回值     无
************************************************************************************************************************/
static void v_update_ui_refresh_info(void)
{
    if(S_eLastDispObj != tUpdate.eObj)
    {
        S_eLastDispObj = tUpdate.eObj;
        set_var_uca_update_obj(pc_update_obj_str(tUpdate.eObj));
    }

    if(S_eLastDispCh != tUpdate.eChType)
    {
        S_eLastDispCh = tUpdate.eChType;
        set_var_uca_update_channel(pc_update_ch_str(tUpdate.eChType));
    }

    if(S_eLastDispProto != tUpdate.eProtoType)
    {
        S_eLastDispProto = tUpdate.eProtoType;
        set_var_uca_update_proto(pc_update_proto_str(tUpdate.eProtoType));
    }

    if((S_usLastDispFrmCnt != tUpdate.usRecFrameCnt) ||
       (S_usLastDispTotalFrm != tUpdate.usTotalFrmValue))
    {
        S_usLastDispFrmCnt   = tUpdate.usRecFrameCnt;
        S_usLastDispTotalFrm = tUpdate.usTotalFrmValue;

        char c_frame_str[32];
        snprintf(c_frame_str, sizeof(c_frame_str), "%04u/%04u", tUpdate.usRecFrameCnt, tUpdate.usTotalFrmValue);
        set_var_uca_update_frame(c_frame_str);
    }

    if(S_usLastDispTimeout != tUpdate.usLostOverTimeCnt)
    {
        S_usLastDispTimeout = tUpdate.usLostOverTimeCnt;

        char c_timeout_str[16];
        snprintf(c_timeout_str, sizeof(c_timeout_str), "%03u", UPDATE_TICK_TO_SEC(tUpdate.usLostOverTimeCnt));
        set_var_uca_update_timeout(c_timeout_str);
    }

    if(S_eLastDispErrCode != tUpdate.eErrCode)
    {
        S_eLastDispErrCode = tUpdate.eErrCode;
        set_var_uca_update_err_info(pc_update_err_code_str(tUpdate.eErrCode));
    }
}

/***********************************************************************************************************************
-----函数功能   计算当前升级进度百分比
-----返回值     0 ~ 100 的百分比
************************************************************************************************************************/
static uint16_t us_update_calc_percent(void)
{
    uint16_t us_total_frms = tUpdate.usTotalFrmValue;
    uint16_t us_rec_frms   = tUpdate.usRecFrameCnt;
    uint16_t percent       = 0;

    if(us_total_frms > 0U)
    {
        percent = (us_rec_frms * 100U) / us_total_frms;
        if(percent > 100U)
            percent = 100U;
    }

    return percent;
}

/***********************************************************************************************************************
-----函数功能   进入新步骤前的公共处理
-----说明(备注) 复位计时基准与倒计时，便于各步骤独立管理超时
-----传入参数   e_step: 目标步骤
                t_now_tick: 当前 Tick
-----返回值     无
************************************************************************************************************************/
static void v_update_enter_step(DispUpdateStep_E e_step, uint32_t t_now_tick)
{
    (void)e_step;

    S_ulStateTick         = t_now_tick;
    S_ulLastCountdownTick = t_now_tick;
    S_usLastFrmCnt        = tUpdate.usRecFrameCnt;
}

/***********************************************************************************************************************
-----函数功能   将升级错误码转换为显示字符串
-----传入参数   e_code
-----返回值     const char*
-----作者       LJD
-----日期       2026-07-01
************************************************************************************************************************/
static const char *pc_update_err_code_str(UpdateErrCode_E e_code)
{
    switch(e_code)
    {
        case UEF_NONE:                return "";
        case UEF_DR_BAUD_REPLY:       return "0x01 F3 baud reply fail";
        case UEF_DR_ERR_FRAME:        return "0x02 err reply frame";
        case UEF_D_RESEND_FAIL:       return "0x10 resend fail";
        case UEF_D_BUFF_NULL:         return "0x12 reply buff null";
        case UEF_D_INVALID_OBJ:       return "0x22 invalid obj";
        case UEF_D_SET_INVALID_BAUD:  return "0x23 invalid baud";
        case UEF_D_CANCEL_REQ:        return "0x2C DCAC cancel";
        case UEF_D_SEND_F0_FAIL:      return "0x13 send F0 fail";
        case UEF_D_F1_CHECK_FAIL:     return "0x14 F1 check fail";
        case UEF_D_SEND_F6_FAIL:      return "0x15 send F6 fail";
        case UEF_D_F7_CHECK_FAIL:     return "0x29 F7 check fail";
        case UEF_D_SEND_F2_FAIL:      return "0x17 send F2 fail";
        case UEF_D_F3_CHECK_FAIL:     return "0x18 F3 check fail";
        case UEF_D_F3_BAUD_REPLY:     return "0x19 F3 baud reply fail";
        case UEF_D_F3_SET_BAUD:       return "0x1A set baud fail";
        case UEF_D_HEAD_LEN_ERR:      return "0x1B head len err";
        case UEF_D_A2_REPLY_ERR:      return "0x26 A2 reply err";
        case UEF_D_A2_TIMEOUT:        return "0x28 A2 timeout";
        case UEF_D_C5_TIMEOUT:        return "0x1E C5 timeout";
        case UEF_D_DATA_LEN_ERR:      return "0x1D data len err";
        case UEF_D_SEND_A3_FAIL:      return "0x2A send A3 fail";
        case UEF_D_A4_CHECK_FAIL:     return "0x20 A4 check fail";
        case UEF_D_A4_REPLY_ERR:      return "0x27 A4 reply err";
        case UEF_D_A4_SEQ_MISMATCH:   return "0x24 A4 seq mismatch";
        case UEF_D_SEND_A5_FAIL:      return "0x1F send A5 fail";
        case UEF_D_A6_REPLY_ERR:      return "0x21 A6 reply err";
        case UEF_D_A6_CHECK_FAIL:     return "0x25 A6 check fail";
        case UEF_D_A6_NOT_COMPLETE:   return "0x2B A6 incomplete";
        case UEF_P_TASK_NULL:         return "0x30 task null";
        case UEF_P_BUFF_NULL:         return "0x35 buff null";
        case UEF_P_INVALID_OBJ:       return "0x33 invalid obj";
        case UEF_P_PENDING_FAIL:      return "0x31 pending fail";
        case UEF_P_SLAVE_RESULT_ERR:  return "0x3C slave result err";
        case UEF_P_C2_DATA_ERR:       return "0x36 C2 data err";
        case UEF_P_C2_REPLY_FAIL:     return "0x37 C2 reply fail";
        case UEF_P_C4_TIMEOUT:        return "0x3B C4/C2/C5 timeout";
        case UEF_P_C5_DATA_ERR:       return "0x38 C5 data err";
        case UEF_P_HEAD_PARSE_FAIL:   return "0x39 head parse fail";
        case UEF_P_FWD_DCAC_FAIL:     return "0x3A fwd DCAC fail";
        case UEF_P_FWD_BMS_FAIL:      return "0x3D fwd BMS fail";
        case UEF_P_FINISH_MISMATCH:   return "0x3E finish mismatch";
        case UEF_P_CANCEL_REQ:        return "0x34 cancel req";
        case UEF_S_TASK_OVER_TIME:    return "0x40 task overtime";
        case UEF_S_LOST_OVERTIME:     return "0x41 lost overtime";
        case UEF_S_REC_OVERTIME:      return "0x42 rec overtime";
        default:                      return "unknown err";
    }
}

/***********************************************************************************************************************
-----函数功能   将升级对象转换为显示字符串
-----传入参数   e_obj
-----返回值     const char*
-----作者       LJD
-----日期       2026-07-01
************************************************************************************************************************/
static const char *pc_update_obj_str(UpdateObj_E e_obj)
{
    switch(e_obj)
    {
        case UO_DEFAULT: return "Host";
        case UO_CONSOLE: return "Console";
        case UO_BMS:     return "BMS";
        case UO_MPPT:    return "MPPT";
        case UO_DCAC:    return "DCAC";
        case UO_MGMT_AC: return "MGMT_AC";
        case UO_MGMT_DC: return "MGMT_DC";
        default:         return "Invalid";
    }
}

/***********************************************************************************************************************
-----函数功能   将升级通道转换为显示字符串
-----传入参数   e_ch
-----返回值     const char*
-----作者       LJD
-----日期       2026-07-01
************************************************************************************************************************/
static const char *pc_update_ch_str(ChannelType_E e_ch)
{
    switch(e_ch)
    {
        case CT_NULL:    return "None";
        case CT_CONSOLE: return "Console";
        case CT_PRINT:   return "Print";
        default:         return "Invalid";
    }
}

/***********************************************************************************************************************
-----函数功能   将升级协议转换为显示字符串
-----传入参数   e_proto
-----返回值     const char*
-----作者       LJD
-----日期       2026-07-01
************************************************************************************************************************/
static const char *pc_update_proto_str(ProtoType_E e_proto)
{
    switch(e_proto)
    {
        case PT_NULL:    return "None";
        case PT_XMODEM:  return "Xmodem";
        case PT_BAIKU:   return "Baiku";
        case PT_MEGMEET: return "Megmeet";
        default:         return "Invalid";
    }
}

#endif  /* boardDISPLAY_EN */
