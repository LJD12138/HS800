/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-启动中 - TFT+LVGL版本                                    *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/eez_ui/ui.h"
#include "MD_Display/user_ui/main_1_ui.h"
#include "Print/print_task.h"

#define dispTASK_BOOTING_CYCLE_TIME         33


/***********************************************************************************************************************
-----函数功能    启动中显示任务
-----说明(备注)  打开显示并刷新启动进度页, 有新任务入队时退出当前任务
-----传入参数    tp_task:任务对象指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_disp_queue_task_booting(Task_T *tp_task)
{
    switch(tp_task->ucStep)
    {
        case 0:
        {
            if(tDisp.eDevState != DS_BOOTING)
            bDisp_SetDevState(DS_BOOTING);
            bDisp_Switch(ST_OFF, false);
            ui_init();
            vDisp_Main1UiStart();
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

    vDisp_UiRefresh();

    #if(boardUSE_OS)
    vTaskDelay(dispTASK_BOOTING_CYCLE_TIME);
    #endif
}

#endif  /*boardDISPLAY_EN*/
