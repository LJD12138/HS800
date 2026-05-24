/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-错误 - TFT+LVGL版本                                      *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if(boardDISPLAY_EN)

#include <string.h>
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#define dispTASK_ERR_CYCLE_TIME             100

void v_disp_queue_task_err(Task_T *tp_task)
{
    //新的任务
    if(lwrb_get_full(&tp_task->tQueueBuff))
        cQueue_GotoStep(tp_task, STEP_END);

    if((tSysInfo.eDevState != DS_ERR) && (tp_task->ucStep != 0U))
    {
        cQueue_GotoStep(tp_task, STEP_END);
    }

    if((tDisp.bLight == false) && (tp_task->ucStep != 0U))
    {
        tp_task->ucStep = 0U;
        #if(boardUSE_OS)
        vTaskDelay(dispTASK_ERR_CYCLE_TIME);
        #endif
        return;
    }

    switch(tp_task->ucStep)
    {
        case 0:
            if(tDisp.eDevState != DS_ERR)
                bDisp_SetDevState(DS_ERR);
            bDisp_Switch(ST_ON, true);
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            /* TFT+LVGL版本 - 错误状态处理 */
            if(uPrint.tFlag.bDispTask)
                sMyPrint("DispTask: 错误模式运行中\r\n");
            cQueue_GotoStep(tp_task, STEP_END);
            break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

#if(boardUSE_OS)
    vTaskDelay(dispTASK_ERR_CYCLE_TIME);
#endif
}

#endif  /*boardDISPLAY_EN*/