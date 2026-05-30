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
#include "Print/print_task.h"

#define     dispTASK_ENG_CYCLE_TIME             10

void v_disp_queue_task_eng(Task_T *tp_task)
{
    switch(tp_task->ucStep)
    {
        case 0:
            if(tDisp.eDevState != DS_ENG_MODE)
                bDisp_SetDevState(DS_ENG_MODE);
            bDisp_Switch(ST_ON, true);
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            /* TFT+LVGL版本 - 工程模式状态处理 */
            cQueue_GotoStep(tp_task, STEP_END);
            break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

#if(boardUSE_OS)
    vTaskDelay(dispTASK_ENG_CYCLE_TIME);
#endif
}
#endif  /*boardDISPLAY_EN && boardENG_MODE_EN*/