/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-升级模式 - TFT+LVGL版本                                  *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "Print/print_task.h"

#define     dispTASK_UPDATA_CYCLE_TIME          10

void v_disp_queue_task_updata(Task_T *tp_task)
{
    if(lwrb_get_full(&tp_task->tQueueBuff) > 0U)
        cQueue_GotoStep(tp_task, STEP_END);

    switch(tp_task->ucStep)
    {
        case 0:
            if(tDisp.eDevState != DS_UPDATE_MODE)
                bDisp_SetDevState(DS_UPDATE_MODE);
            bDisp_Switch(ST_ON, true);
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            /* TFT+LVGL版本 - 升级模式状态处理 */
            if(tDisp.eDevState != DS_UPDATE_MODE)
                cQueue_GotoStep(tp_task, STEP_END);
            break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

#if(boardUSE_OS)
    vTaskDelay(dispTASK_UPDATA_CYCLE_TIME);
#endif
}