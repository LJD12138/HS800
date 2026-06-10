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
#include "MD_Display/eez_ui/ui.h"
#include "Print/print_task.h"

#define dispTASK_CLOSE_CYCLE_TIME           10

void v_disp_queue_task_closing(Task_T *tp_task)
{
    switch(tp_task->ucStep)
    {
        case 0:
        {
            if(tDisp.eDevState != DS_CLOSING)
                bDisp_SetDevState(DS_CLOSING);
            bDisp_Switch(ST_ON, true);
            loadScreen(SCREEN_ID_MAIN_CLOSING);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            /* 延迟一段时间让关机画面显示完整，防止Work残影 */
            if (tp_task->usStepWaitCnt < (1500 / dispTASK_CLOSE_CYCLE_TIME))
            {
                tp_task->usStepWaitCnt++;
                break;
            }

            bDisp_SetDevState(DS_SHUT_DOWN);
            cQueue_GotoStep(tp_task, STEP_END);
            
        }
        break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

    vDisp_UiRefresh();
    #if(boardUSE_OS)
    vTaskDelay(dispTASK_CLOSE_CYCLE_TIME);
    #endif
}

#endif  /*boardDISPLAY_EN*/