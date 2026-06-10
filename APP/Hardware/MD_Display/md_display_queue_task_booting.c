/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-启动中 - TFT+LVGL版本                                    *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"
#include "Sys/sys_task.h"
#include <stdbool.h>

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
    static uint8_t ucLoadingStep = 0;
    switch(tp_task->ucStep)
    {
        //初始化
        case 0:
        {
            if(tDisp.eDevState != DS_BOOTING)
                bDisp_SetDevState(DS_BOOTING);
            ucLoadingStep = 0;
            ui_init();
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        //加载进度条
        case 1:
        {
            bDisp_Switch(ST_ON, true);
            
            if(ucLoadingStep < 100)
                ucLoadingStep += 3;
            if(ucLoadingStep > 100)
                ucLoadingStep = 100;

            lv_bar_set_value(objects.uc_booting_bar, ucLoadingStep,LV_ANIM_ON);
            if(tSysInfo.eDevState == DS_WORK && ucLoadingStep >= 100)
                cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        //进度条到100后停留50ms
        case 2:
        {
            // tp_task->usStepWaitCnt++;
            // if(tp_task->usStepWaitCnt >= (250 / dispTASK_BOOTING_CYCLE_TIME))
            {
                bDisp_Switch(ST_OFF, true);
                cQueue_GotoStep(tp_task, STEP_NEXT);
            } 
        }
        // break;

        //关背光,切换到Work屏,初始化Work UI
        case 3:
        {
            loadScreen(SCREEN_ID_MAIN_WORK);
            vDisp_Main1UiStart();
            vDisp_UiRefresh();
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        //等待LVGL渲染完成后结束
        case 4:
        {
            vDisp_UiRefresh();
            vDisp_UiRefresh();
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
