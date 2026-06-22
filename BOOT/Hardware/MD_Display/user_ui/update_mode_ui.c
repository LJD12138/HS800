/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS800\BOOT\Hardware\MD_Display\user_ui
 * File    : update_mode_ui.c
 * Date    : 2026-06-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : 升级模式UI界面显示实现
 * -------------------------------------------------------
 * todo    :
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/user_ui/update_mode_ui.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
#include "Print/print_task.h"
#include "boot_info.h"
#include "Update/update_main.h"
#include <stdio.h>
#include <string.h>

#if (boardDISPLAY_EN)
//****************************************************Macros*******************************************************************//



//****************************************************Parameter Initialization************************************************//



//****************************************************Function Declaration****************************************************//
static void v_draw_progress_text(uint8_t progress);


/*****************************************************************************************************************
 * 函数功能    : 在圆环内部绘制进度数字和百分号
 * 说明(备注)  : 对齐数字大小并防边缘锯齿，数字用3倍字体，百分号用2倍字体
 * 传入参数    : progress: 进度(0-100)
 * 输出参数    : none
 * 返回值      : none
 *****************************************************************************************************************/
static void v_draw_progress_text(uint8_t progress)
{
    char buf[16];
    if (progress < 10)
    {
        vDisp_DrawFillRect(135, 80, 50, 50, 0x10A3);
        sprintf(buf, "%d", progress);
        vDisp_DrawText(140, 81, buf, 0xFFFF, 0x10A3, 3);
        vDisp_DrawText(164, 89, "%", 0xFFFF, 0x10A3, 2);
    }
    else if (progress < 100)
    {
        vDisp_DrawFillRect(120, 80, 80, 50, 0x10A3);
        sprintf(buf, "%d", progress);
        vDisp_DrawText(128, 81, buf, 0xFFFF, 0x10A3, 3);
        vDisp_DrawText(176, 89, "%", 0xFFFF, 0x10A3, 2);
    }
    else
    {
        vDisp_DrawFillRect(110, 80, 100, 50, 0x10A3);
        sprintf(buf, "%d", progress);
        vDisp_DrawText(116, 81, buf, 0xFFFF, 0x10A3, 3);
        vDisp_DrawText(188, 89, "%", 0xFFFF, 0x10A3, 2);
    }
}

/*****************************************************************************************************************
 * 函数功能    : 更新升级模式UI界面显示内容
 * 说明(备注)  : 绘制进度圆环、进度数字、描述提示信息、胶囊进度条和倒计时
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 *****************************************************************************************************************/
void vDisp_UpdateModeUi(void)
{
    static uint8_t s_uc_prev_progress = 0xFF;
    static const char *s_pc_prev_desc = NULL;
    static uint16_t s_us_prev_countdown = 0xFFFF;
    static bool s_b_first_run = true;
    
    if (tDisp.bLight == false)
    {
        s_b_first_run = true;
        return;
    }
    
    uint8_t progress = 0;
    if (tUpdate.usTotalFrmValue > 0)
    {
        progress = (uint32_t)tUpdate.usRecFrameCnt * 100 / tUpdate.usTotalFrmValue;
        if (progress > 100) progress = 100;
    }
    
    // 1. 绘制进度圆环和胶囊进度条（有变化时刷新）
    if (progress != s_uc_prev_progress || s_b_first_run)
    {
        vDisp_DrawProgressCircle(160, 105, 50, 6, progress, 0x06FF, 0x2126, 0x10A3);
        v_draw_progress_text(progress);
        vDisp_DrawPillProgress(20, 195, 280, 14, progress, 0x06FF, 0x4208, 0x10A3);
        s_uc_prev_progress = progress;
    }
    
    // 2. 状态提示信息获取
    const char *p_desc = "Waiting for update...";
    uint16_t desc_color = 0xAD55; // 浅灰
    
    if (tBootMemParam.tParam.eAppState == AS_FINISH)
    {
        p_desc = "Upgrade Success!";
        desc_color = 0xFFFF; // 白色
    }
    else if (tBootMemParam.tParam.eAppState == AS_ERASE)
    {
        if (progress > 0)
        {
            p_desc = "Upgrading...";
        }
        else
        {
            p_desc = "Erasing Flash...";
        }
    }
    
    // 3. 绘制状态提示信息（有变化时刷新）
    if (p_desc != s_pc_prev_desc || s_b_first_run)
    {
        uint16_t desc_len = strlen(p_desc);
        uint16_t desc_x = 160 - (desc_len * 8) / 2;
        vDisp_DrawFillRect(20, 165, 280, 16, 0x10A3);
        vDisp_DrawText(desc_x, 165, p_desc, desc_color, 0x10A3, 1);
        s_pc_prev_desc = p_desc;
    }
    
    // 4. 倒计时获取
    uint16_t countdown_sec = 0;
    if (tUpdate.eProtoType == PT_XMODEM)
    {
        if (tXmodem.eState == XMODEM_STATE_RECEIVING) {
            countdown_sec = tXmodem.usWaitStartOutTimeCnt / 100;
        } else if (tXmodem.eState == XMODEM_STATE_FINISH || tXmodem.eState == XMODEM_STATE_CANCEL) {
            countdown_sec = tXmodem.usWaitExitOutTimeCnt / 100;
        } else {
            countdown_sec = tXmodem.usWaitStartOutTimeCnt / 100;
        }
    }
    else if (tUpdate.eProtoType == PT_BAIKU)
    {
        if (tBaiKuProto.eState == BAIKU_STATE_RECEIVING) {
            countdown_sec = tBaiKuProto.usWaitStartOutTimeCnt / 100;
        } else if (tBaiKuProto.eState == BAIKU_STATE_FINISH || tBaiKuProto.eState == BAIKU_STATE_CANCEL) {
            countdown_sec = tBaiKuProto.usWaitExitOutTimeCnt / 100;
        } else {
            countdown_sec = tBaiKuProto.usWaitStartOutTimeCnt / 100;
        }
    }
    
    // 5. 绘制倒计时（有变化时刷新）
    if (countdown_sec != s_us_prev_countdown || s_b_first_run)
    {
        char exit_buf[32];
        sprintf(exit_buf, "Exit in %ds", countdown_sec);
        uint16_t exit_len = strlen(exit_buf);
        uint16_t exit_x = 160 - (exit_len * 8) / 2;
        vDisp_DrawFillRect(40, 220, 240, 16, 0x10A3);
        vDisp_DrawText(exit_x, 220, exit_buf, 0x8410, 0x10A3, 1);
        s_us_prev_countdown = countdown_sec;
    }
    
    s_b_first_run = false;
}

#endif  /* update_mode_ui.c */
