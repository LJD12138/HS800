/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-启动中 - TFT+LVGL版本                                    *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_task.h"
#include "Print/print_task.h"

#define dispTASK_BOOTING_CYCLE_TIME         100

void v_disp_queue_task_booting(Task_T *tp_task)
{
    switch(tp_task->ucStep)
    {
        case 0:
            if(tDisp.eDevState != DS_BOOTING)
                bDisp_SetDevState(DS_BOOTING);
            vDisp_Init();
            bDisp_Switch(ST_ON, true);
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            cQueue_GotoStep(tp_task, STEP_END);
            break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

#if(boardUSE_OS)
    vTaskDelay(dispTASK_BOOTING_CYCLE_TIME);
#endif
}

#endif  /*boardDISPLAY_EN*/