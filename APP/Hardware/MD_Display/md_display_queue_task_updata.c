/*******************************************************************************************************************************
 * Project : APP
 * Module  : G:\\1-Baiku_Projects\\25-HS800\\1.software\\HS800\\APP\\Hardware\\MD_Display
 * File    : md_display_queue_task_updata.c
 * Date    : 2026-06-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示-升级模式 - TFT+LVGL版本
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

//****************************************************Parameter Initialization************************************************//
static uint8_t  S_ucUpdateUiState = 0;       /* 0: Waiting, 1: Updating, 2: Success, 3: Failed */
static uint32_t S_ulStateTick = 0;           /* 记录状态机内部计时的 Tick */
static uint16_t S_usLastFrmCnt = 0;          /* 上一次接收到的升级帧数，用于超时检测 */
static uint32_t S_ulLastCountdownTick = 0;    /* 倒计时变化定时器 */
static uint8_t  S_ucCountdownSec = 10;       /* 倒计时秒数 */

void v_disp_queue_task_updata(Task_T *tp_task)
{
    if(lwrb_get_full(&tp_task->tQueueBuff) > 0U)
        cQueue_GotoStep(tp_task, STEP_END);

    switch(tp_task->ucStep)
    {
        case 0:
            if(tDisp.eDevState != DS_UPDATE_MODE)
                bDisp_SetDevState(DS_UPDATE_MODE);
            bDisp_Switch(ST_ON, false);

            /* 载入升级专属屏幕 */
            loadScreen(SCREEN_ID_MAIN_UPDATE);

            /* 升级状态机参数初始化 */
            S_ucUpdateUiState = 0;
            S_ulStateTick = xTaskGetTickCount();
            S_usLastFrmCnt = 0;
            S_ulLastCountdownTick = 0;
            S_ucCountdownSec = 10;

            /* 初始化 UI 绑定变量 */
            set_var_uca_update_progress("");
            set_var_uca_update_countdown("");
            set_var_uca_update_state(0);
            set_var_uca_update_obj("");
            set_var_uca_update_channel("");
            set_var_uca_update_proto("");
            set_var_uca_update_frame("");
            set_var_uca_update_timeout("");
            set_var_uca_update_msg("Waiting for update.");

            /* 初始状态文字设为默认暗灰色 */
            if (objects.obj_status_label != NULL)
            {
                lv_obj_set_style_text_color(objects.obj_status_label, lv_color_hex(0xAAAAAA), LV_PART_MAIN | LV_STATE_DEFAULT);
            }

            /* 初始显示 Spinner，隐藏 Arc/Pct */
            if (objects.uc_update_spinner != NULL)
                lv_obj_remove_flag(objects.uc_update_spinner, LV_OBJ_FLAG_HIDDEN);
            if (objects.obj_progress_arc != NULL)
                lv_obj_add_flag(objects.obj_progress_arc, LV_OBJ_FLAG_HIDDEN);
            if (objects.obj_pct_label != NULL)
                lv_obj_add_flag(objects.obj_pct_label, LV_OBJ_FLAG_HIDDEN);

            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            /* TFT+LVGL版本 - 升级模式状态机 */
            if(tDisp.eDevState != DS_UPDATE_MODE)
            {
                cQueue_GotoStep(tp_task, STEP_END);
                break;
            }

            uint32_t t_now_tick = xTaskGetTickCount();
            uint16_t us_total_frms = tUpdate.usTotalFrmValue;
            uint16_t us_rec_frms = tUpdate.usRecFrameCnt;
            uint16_t percent = 0;

            if (us_total_frms > 0)
            {
                percent = (us_rec_frms * 100) / us_total_frms;
                if (percent > 100)
                    percent = 100;
            }

            /* 实时更新左侧信息面板的数据源并转换格式 */
            const char *p_obj_str;
            switch(tUpdate.eObj)
            {
                case UO_DEFAULT: p_obj_str = "Host"; break;
                case UO_CONSOLE: p_obj_str = "Console"; break;
                case UO_BMS:     p_obj_str = "BMS"; break;
                case UO_MPPT:    p_obj_str = "MPPT"; break;
                case UO_DCAC:    p_obj_str = "DCAC"; break;
                default:         p_obj_str = "Invalid"; break;
            }
            set_var_uca_update_obj(p_obj_str);

            const char *p_ch_str;
            switch(tUpdate.eChType)
            {
                case CT_NULL:    p_ch_str = "None"; break;
                case CT_CONSOLE: p_ch_str = "Console"; break;
                case CT_PRINT:   p_ch_str = "Print"; break;
                default:         p_ch_str = "Invalid"; break;
            }
            set_var_uca_update_channel(p_ch_str);

            const char *p_proto_str;
            switch(tUpdate.eProtoType)
            {
                case PT_NULL:    p_proto_str = "None"; break;
                case PT_XMODEM:  p_proto_str = "Xmodem"; break;
                case PT_BAIKU:   p_proto_str = "Baiku"; break;
                default:         p_proto_str = "Invalid"; break;
            }
            set_var_uca_update_proto(p_proto_str);

            char c_frame_str[32];
            snprintf(c_frame_str, sizeof(c_frame_str), "%04u/%04u", us_rec_frms, us_total_frms);
            set_var_uca_update_frame(c_frame_str);

            char c_timeout_str[16];
            snprintf(c_timeout_str, sizeof(c_timeout_str), "%03u", UPDATE_TICK_TO_SEC(tUpdate.usLostOverTimeCnt));
            set_var_uca_update_timeout(c_timeout_str);

            switch (S_ucUpdateUiState)
            {
                /* 状态 0: 等待开始升级 */
                case 0:
                    set_var_uca_update_state(0);
                    
                    /* 省略号动画，每 200ms (6 * 33ms) 步进 */
                    {
                        static uint8_t s_uc_anim_cnt = 0;
                        s_uc_anim_cnt++;
                        uint8_t anim_step = (s_uc_anim_cnt / 6) % 3;
                        if (anim_step == 0) {
                            set_var_uca_update_msg("Waiting for update.");
                        } else if (anim_step == 1) {
                            set_var_uca_update_msg("Waiting for update..");
                        } else {
                            set_var_uca_update_msg("Waiting for update...");
                        }
                    }

                    if (us_total_frms > 0 && us_rec_frms > 0)
                    {
                        S_ucUpdateUiState = 1;
                        S_ulStateTick = t_now_tick;
                        S_usLastFrmCnt = us_rec_frms;

                        /* 隐藏 Spinner，显示 Arc/Pct */
                        if (objects.uc_update_spinner != NULL)
                            lv_obj_add_flag(objects.uc_update_spinner, LV_OBJ_FLAG_HIDDEN);
                        if (objects.obj_progress_arc != NULL)
                            lv_obj_remove_flag(objects.obj_progress_arc, LV_OBJ_FLAG_HIDDEN);
                        if (objects.obj_pct_label != NULL)
                            lv_obj_remove_flag(objects.obj_pct_label, LV_OBJ_FLAG_HIDDEN);
                    }
                    /* 等待握手/开始指令超过 180s 判定失败 */
                    else if (t_now_tick - S_ulStateTick >= pdMS_TO_TICKS(180000))
                    {
                        S_ucUpdateUiState = 3;
                        S_ulStateTick = t_now_tick;
                        S_ulLastCountdownTick = t_now_tick;
                        S_ucCountdownSec = 10;

                        /* 隐藏 Spinner，显示 Arc/Pct */
                        if (objects.uc_update_spinner != NULL)
                            lv_obj_add_flag(objects.uc_update_spinner, LV_OBJ_FLAG_HIDDEN);
                        if (objects.obj_progress_arc != NULL)
                            lv_obj_remove_flag(objects.obj_progress_arc, LV_OBJ_FLAG_HIDDEN);
                        if (objects.obj_pct_label != NULL)
                            lv_obj_remove_flag(objects.obj_pct_label, LV_OBJ_FLAG_HIDDEN);
                    }
                    break;

                /* 状态 1: 升级传输进行中 */
                case 1:
                {
                    set_var_uca_update_state(1);
                    set_var_uca_update_msg("Upgrading...");

                    /* 确保文字为白色 */
                    if (objects.obj_status_label != NULL)
                    {
                        lv_obj_set_style_text_color(objects.obj_status_label, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    }

                    /* 格式化进度文本 (纯数字，无%) */
                    char c_percent_str[10];
                    snprintf(c_percent_str, sizeof(c_percent_str), "%u", percent);
                    set_var_uca_update_progress(c_percent_str);

                    /* 刷新 Arc */
                    if (objects.obj_progress_arc != NULL)
                    {
                        lv_arc_set_value(objects.obj_progress_arc, percent);
                    }

                    /* 接收完所有帧，跳转升级成功 */
                    if (percent >= 100)
                    {
                        S_ucUpdateUiState = 2;
                        S_ulStateTick = t_now_tick;
                        S_ulLastCountdownTick = t_now_tick;
                        S_ucCountdownSec = 10;
                        break;
                    }

                    /* 超时检测：如果连续 15 秒没收到任何新升级帧，跳转升级失败 */
                    if (us_rec_frms != S_usLastFrmCnt)
                    {
                        S_usLastFrmCnt = us_rec_frms;
                        S_ulStateTick = t_now_tick;
                    }
                    else if (t_now_tick - S_ulStateTick >= pdMS_TO_TICKS(15000))
                    {
                        S_ucUpdateUiState = 3;
                        S_ulStateTick = t_now_tick;
                        S_ulLastCountdownTick = t_now_tick;
                        S_ucCountdownSec = 10;
                    }
                    break;
                }

                /* 状态 2: 升级成功倒计时 */
                case 2:
                {
                    set_var_uca_update_state(2);
                    set_var_uca_update_progress("100");
                    set_var_uca_update_msg("Update Complete!");

                    /* 刷新 Arc 到 100 */
                    if (objects.obj_progress_arc != NULL)
                    {
                        lv_arc_set_value(objects.obj_progress_arc, 100);
                    }

                    /* 将状态文字设为绿色 */
                    if (objects.obj_status_label != NULL)
                    {
                        lv_obj_set_style_text_color(objects.obj_status_label, lv_color_hex(0x4CAF50), LV_PART_MAIN | LV_STATE_DEFAULT);
                    }

                    if (t_now_tick - S_ulLastCountdownTick >= pdMS_TO_TICKS(1000))
                    {
                        S_ulLastCountdownTick = t_now_tick;
                        if (S_ucCountdownSec > 0)
                            S_ucCountdownSec--;
                    }

                    char c_countdown_ok[24];
                    snprintf(c_countdown_ok, sizeof(c_countdown_ok), "Reboot in %us", S_ucCountdownSec);
                    set_var_uca_update_countdown(c_countdown_ok);

                    /* 倒计时归零，添加重启任务并退出 */
                    if (S_ucCountdownSec == 0)
                    {
                        cQueue_AddQueueTask(tpSysTask, STI_RESET, NULL, true);
                        cQueue_GotoStep(tp_task, STEP_END);
                    }
                    break;
                }

                /* 状态 3: 升级失败倒计时 */
                case 3:
                {
                    set_var_uca_update_state(3);
                    set_var_uca_update_msg("Update Failed!");

                    /* 将状态文字设为红色 */
                    if (objects.obj_status_label != NULL)
                    {
                        lv_obj_set_style_text_color(objects.obj_status_label, lv_color_hex(0xF44336), LV_PART_MAIN | LV_STATE_DEFAULT);
                    }

                    if (t_now_tick - S_ulLastCountdownTick >= pdMS_TO_TICKS(1000))
                    {
                        S_ulLastCountdownTick = t_now_tick;
                        if (S_ucCountdownSec > 0)
                            S_ucCountdownSec--;
                    }

                    char c_countdown_err[24];
                    snprintf(c_countdown_err, sizeof(c_countdown_err), "Reboot in %us", S_ucCountdownSec);
                    set_var_uca_update_countdown(c_countdown_err);

                    /* 失败倒计时归零后也执行安全重启 */
                    if (S_ucCountdownSec == 0)
                    {
                        cQueue_AddQueueTask(tpSysTask, STI_RESET, NULL, true);
                        cQueue_GotoStep(tp_task, STEP_END);
                    }
                    break;
                }

                default:
                    break;
            }

            vDisp_UiRefresh();
            break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

    #if(boardUSE_OS)
    vTaskDelay(dispTASK_UPDATA_CYCLE_TIME);
    #endif
}
#endif  /* boardDISPLAY_EN */
