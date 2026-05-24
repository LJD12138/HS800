/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-初始化 - TFT+LVGL版本                                    *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if(boardDISPLAY_EN)

#include <string.h>
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#define dispTASK_INIT_CYCLE_TIME            100

void v_disp_queue_task_init(Task_T *tp_task)
{
    switch(tp_task->ucStep)
    {
        case 0:
            if(tDisp.eDevState != DS_INIT)
                bDisp_SetDevState(DS_INIT);
            bDisp_Switch(ST_OFF, false);
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            /* TFT+LVGL版本 - 初始化显示 */
            bDisp_Switch(ST_ON, true);
            tSysInfo.uInit.tFinish.bIF_DispTask = 1;
            cQueue_GotoStep(tp_task, STEP_END);
            break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

#if(boardUSE_OS)
    vTaskDelay(dispTASK_INIT_CYCLE_TIME);
#endif
}

#endif  /*boardDISPLAY_EN*/