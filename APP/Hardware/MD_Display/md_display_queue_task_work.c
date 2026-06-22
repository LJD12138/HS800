/*****************************************************************************************************************
 *                                                                                                                *
 *                                         显示队列任务-工作中 - TFT+LVGL版本                                    *
 *                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if (boardDISPLAY_EN)
#include "MD_Display/eez_ui/ui.h"
#include "MD_Display/eez_ui/vars.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/user_ui/main_1_ui.h"
#include "Print/print_task.h"
#include "Print/print_api.h"
#include "Sys/sys_task.h"

#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#include "MD_Dcac/md_dcac_task.h"

// #include "Adc/adc_task.h"
#include "Dc/dc_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Light/md_light_task.h"
#include "MD_Mppt/md_mppt_task.h"
#include "Usb/usb_task.h"

#include "lvgl.h"
#include <string.h>

//****************************************************局部宏定义初始化*********************************************//
#define dispTASK_WORK_DATA_UPDATE_MS boardDISP_REFRESH_TIME
#define dispTASK_WORK_LVGL_PERIOD_MS 33U
#define dispTASK_WORK_SLEEP_OFF_MS 100U

//****************************************************局部变量定义************************************************//
static TickType_t s_tDispWorkLastDataUpdateTick = 0U;
static bool s_bDispWorkDataUpdateTickValid = false;
static u8 uc_refresh_ui_index = 0U;

//****************************************************局部函数定义************************************************//

/***********************************************************************************************************************
-----函数功能    工作显示任务
-----说明(备注)  1.eez_ui是EEZ Studio软件输出的项目,所以显示更新机制是基于数据变化自动更新显示的,因此这里只需要定时更新数据即可;
                  但是eez_ui中其他数据(如Image)都需要user_ui区进行控制.
                2.user_ui是用户自定义的显示界面,需要在这里控制显示刷新,以避免无效刷新导致的性能问题;当背光关闭时,不更新显示以节省资源;
                3.这里的任务调度函数由队列管理函数装载,当存在新任务时,会退出当前任务,因此不需要在这里单独处理任务切换的情况;
-----传入参数    tp_task:任务对象指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_disp_queue_task_work(Task_T *tp_task)
{
    // 存在新任务,退出当前任务
    if (lwrb_get_full(&tp_task->tQueueBuff) > 0U)
    {
        s_bDispWorkDataUpdateTickValid = false;
        vDisp_Main1Exit();
        cQueue_GotoStep(tp_task, STEP_END);
        return;
    }

    switch (tp_task->ucStep)
    {
        // 初始化:更新数据并亮屏
        case 0: 
        {
            if (tDisp.eDevState != DS_WORK)
                bDisp_SetDevState(DS_WORK);
            
            if (lv_screen_active() != objects.main_work)
                loadScreen(SCREEN_ID_MAIN_WORK);

            uc_refresh_ui_index = 0U;
            vDisp_UiRefresh();
            bDisp_Switch(ST_ON, true);
            s_tDispWorkLastDataUpdateTick = xTaskGetTickCount();
            s_bDispWorkDataUpdateTickValid = true;
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        // //加载能量环数据
        // case 1: 
        // {
        //     vDisp_UpdateDevParam();
        //     uc_refresh_ui_index++;
        // }
        // break;

        // 持续刷新显示
        case 1: 
        {
            TickType_t t_now_tick = xTaskGetTickCount();

            if (tDisp.bLight == true)
            {
                if ((s_bDispWorkDataUpdateTickValid == false) ||
                    ((t_now_tick - s_tDispWorkLastDataUpdateTick) >= pdMS_TO_TICKS(dispTASK_WORK_DATA_UPDATE_MS)))
                {
                    s_tDispWorkLastDataUpdateTick = t_now_tick;
                    s_bDispWorkDataUpdateTickValid = true;

                    switch(uc_refresh_ui_index)
                    {
                       case 0U:
                       {
                            vDisp_UpdateDevParam();
                            uc_refresh_ui_index++;
                       }
                       break;

                       case 1U:
                       {
                            bDisp_Main1DataUpdate();
                            uc_refresh_ui_index = 0U;
                       }
                       break;
                    
                       default:
                            uc_refresh_ui_index = 0U;
                            break;
                    }
                }

                vDisp_UiRefresh();
                vTaskDelay(pdMS_TO_TICKS(dispTASK_WORK_LVGL_PERIOD_MS));
                return;
            }

            s_bDispWorkDataUpdateTickValid = false;
            vTaskDelay(pdMS_TO_TICKS(dispTASK_WORK_SLEEP_OFF_MS));
            return;
        }
        break;

        default:
            s_bDispWorkDataUpdateTickValid = false;
            vDisp_Main1Exit();
            cQueue_GotoStep(tp_task, STEP_END);
            return;
    }

    // sMyPrint("显示刷新 \r\n");

    vTaskDelay(pdMS_TO_TICKS(dispTASK_WORK_LVGL_PERIOD_MS));
}

#endif /*boardDISPLAY_EN*/