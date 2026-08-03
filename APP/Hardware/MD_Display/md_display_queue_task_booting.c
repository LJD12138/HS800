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
#include "lvgl.h"

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
            vDisp_UpdateDevParam();
            vDisp_UiInit();
            vDisp_UiRefresh();
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        //加载进度条
        case 1:
        {
            bDisp_Switch(ST_ON, false);
            
            if(ucLoadingStep < 100)
                ucLoadingStep += 3;
            if(ucLoadingStep > 100)
                ucLoadingStep = 100;

            lv_bar_set_value(objects.uc_booting_bar, ucLoadingStep,LV_ANIM_ON);
            if(tSysInfo.eDevState == DS_WORK && ucLoadingStep >= 100)
                cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        //关背光,切换到Work屏,渲染完成后立即亮屏
        case 2:
        {
            bDisp_Switch(ST_OFF, false);
            /* 先立即加载屏幕(无动画),再调用loadScreen仅更新currentScreen,
               因act_scr已等于main_work,loadScreen内部的lv_scr_load_anim会直接返回 */
            lv_scr_load(objects.main_work);
            loadScreen(SCREEN_ID_MAIN_WORK);
            vDisp_Main1UiStart();
            vDisp_UiRefresh();
            bDisp_Switch(ST_ON, false);
            cQueue_GotoStep(tp_task, STEP_END);

            //系统已经已经切换到工作状态, 则强制调度显示任务为工作
            if(tSysInfo.eDevState == DS_WORK)
                cQueue_AddQueueTask(tpDispTask, DISPTI_WORK, 0, false);
            return;
        }

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
