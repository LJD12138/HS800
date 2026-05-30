/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-关闭中 - TFT+LVGL版本                                    *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if(boardDISPLAY_EN)

#include <string.h>
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_task.h"
#include "Print/print_task.h"

#define dispTASK_CLOSE_CYCLE_TIME           10

void v_disp_queue_task_closing(Task_T *tp_task)
{
    switch(tp_task->ucStep)
    {
        case 0:
            if(tDisp.eDevState != DS_CLOSING)
                bDisp_SetDevState(DS_CLOSING);
            bDisp_Switch(ST_ON, true);
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            /* TFT+LVGL版本 - 关闭中状态处理 */
            if(uPrint.tFlag.bDispTask)
                sMyPrint("DispTask: 关闭模式运行中\r\n");
            bDisp_SetDevState(DS_SHUT_DOWN);
            cQueue_GotoStep(tp_task, STEP_END);
            break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

#if(boardUSE_OS)
    vTaskDelay(dispTASK_CLOSE_CYCLE_TIME);
#endif
}

#endif  /*boardDISPLAY_EN*/