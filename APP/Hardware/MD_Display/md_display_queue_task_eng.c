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
#include "MD_Display/user_ui/eng_mode_ui.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_eng.h"
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

            /* 创建工程模式UI */
            vEngMode_UiCreate();
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            /* 工程模式运行态 - 周期性刷新数据 */
            vEngMode_UiTick();

            /* 检测退出请求 */
            if(bEngMode_IsExitReq())
            {
                vEngMode_UiDelete();
                cSys_Switch(SO_KEY, ST_OFF, false);
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