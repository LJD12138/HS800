/*****************************************************************************************************************
 *                                                                                                                *
  *                                         显示队列任务-工程模式 - TFT+LVGL版本                                  *
 *                                                                                                                *
  ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if(boardENG_MODE_EN && boardDISPLAY_EN)
#include <string.h>
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/user_ui/eng_mode_ui.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_eng.h"
#include "Print/print_task.h"
#include "lvgl.h"

#define     dispTASK_ENG_CYCLE_TIME             10

/* 工程模式超时: 60s / 10ms = 6000 周期 */
#define     dispENG_MODE_TIMEOUT_CNT            6000


void v_disp_queue_task_eng(Task_T *tp_task)
{
    switch(tp_task->ucStep)
    {
        case 0:
            /* 确保 UI 及所有屏幕已经初始化 (防止直入工程模式时未调用 ui_init) */
            vDisp_UiInit();

            sMyPrint("DispEng: case 0, tDisp.eDevState = %d, active_scr = %p, main_eng = %p\r\n", 
                     tDisp.eDevState, lv_screen_active(), objects.main_eng);
            if(tDisp.eDevState != DS_ENG_MODE)
                bDisp_SetDevState(DS_ENG_MODE);
            bDisp_Switch(ST_ON, true);

            /* 创建工程模式UI */
            vEngMode_UiCreate();

            /* 立即渲染首帧, 确保工程模式UI同步到TFT显存 */
            lv_refr_now(NULL);
            sMyPrint("DispEng: case 0 refr_now done, active_scr = %p\r\n", lv_screen_active());

            /* 重置超时计数 */
            vEng_RefreshEngModeTime();
            tp_task->usTaskWaitCnt = 0;

            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            {
                static uint16_t s_log_cnt = 0;
                s_log_cnt++;
                if(s_log_cnt >= 100)
                {
                    s_log_cnt = 0;
                    sMyPrint("DispEng: case 1 running, active_scr = %p\r\n", lv_screen_active());
                }
            }
            /* 工程模式运行态 - 周期性刷新数据 */
            vEngMode_UiTick();

            /* 驱动LVGL渲染管线, 将脏区域的像素数据通过flush回调发送到TFT */
            lv_timer_handler();

            /* 超时检测: 无按键操作超过60秒自动退出 */
            tp_task->usTaskWaitCnt++;
            if(tp_task->usTaskWaitCnt >= dispENG_MODE_TIMEOUT_CNT)
            {
                if(uPrint.tFlag.bDispTask)
                    log_w("bDispTask: eng mode timeout, exiting");
                vEngMode_UiDelete();
                cSys_Switch(SO_KEY, ST_OFF, false);
                cQueue_GotoStep(tp_task, STEP_END);
                break;
            }

            /* 检测退出请求 */
            if(bEngMode_IsExitReq())
            {
                vEngMode_UiDelete();
                cSys_Switch(SO_KEY, ST_ON, false);
                cQueue_GotoStep(tp_task, STEP_END);
            }
            break;

        default:
            vEngMode_UiDelete();
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

#if(boardUSE_OS)
    vTaskDelay(dispTASK_ENG_CYCLE_TIME);
#endif
}
#endif  /*boardDISPLAY_EN && boardENG_MODE_EN*/