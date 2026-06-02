/*****************************************************************************************************************
 *                                                                                                                *
 *                                         显示队列任务-工作中 - TFT+LVGL版本                                    *
 *                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"
#include "Print/print_api.h"

#if (boardDISPLAY_EN)
#include "MD_Display/eez_ui/ui.h"
#include "MD_Display/eez_ui/vars.h"
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/user_ui/main_1_ui.h"
#include "Print/print_task.h"
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
#define dispTASK_WORK_CYCLE_TIME boardDISP_REFRESH_TIME
#define dispTASK_WORK_SLEEP_OFF_MS 100U

//****************************************************局部变量定义************************************************//
/* s_ucDispWorkUpdateIndex removed: all params updated in one pass */

//****************************************************局部函数定义************************************************//
static void v_update_dev_param(void);
#if (boardBMS_EN)
static void v_disp_work_format_remaining_time(char *pc_str, size_t str_size, u16 us_total_minutes);
#endif

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
        vDisp_Main1Exit();
        cQueue_GotoStep(tp_task, STEP_END);
        return;
    }

    v_update_dev_param();
    bDisp_Main1DataUpdate();

    switch (tp_task->ucStep)
    {
        // 初始化
        case 0: 
        {
            if (tDisp.eDevState != DS_WORK)
                bDisp_SetDevState(DS_WORK);
            
             vDisp_UiRefresh();
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        // 打开背光,避免显示加载过程
        case 1: 
        {
            bDisp_Switch(ST_ON, true);
            vDisp_SetAcWorkMode(IMG_ANIM_MODE_CHG_DISCHG);
            vDisp_UiRefresh();
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        // 更新数据
        case 2: 
        {
           

            // 背光打开时才更新 UI
            if (tDisp.bLight == true)
                vDisp_UiRefresh();
        }
        break;

        default:
            vDisp_Main1Exit();
            cQueue_GotoStep(tp_task, STEP_END);
            return;
    }

    // sMyPrint("显示刷新 \r\n");

    vTaskDelay(dispTASK_WORK_CYCLE_TIME);
}

/***********************************************************************************************************************
-----函数功能    更新设备参数
-----说明(备注)  这里只需要更新数据,eez_ui里面会根据数据变化自动更新显示;但是DevStateIcon需要单独更新,在user_ui中实现
-----传入参数    none
-----输出参数    none
-----返回值      true:文本数据有变化 false:文本数据无变化
-----日期        2026-05-28
************************************************************************************************************************/
__STATIC_INLINE void v_update_dev_param(void)
{
    char cStr[10];

    /* 一次性更新所有参数, 避免分散到多个刷新周期 */
    #if (boardBMS_EN)
    snprintf(cStr, sizeof(cStr), "%d", tBmsRx.usSOC);
    set_var_uca_bat_soc_value(cStr);

    {
        u16 usTotalMinutes;
        if (tBms.eWorkState == BWS_CHG)
            usTotalMinutes = tBmsRx.usChgFullTime;
        else
            usTotalMinutes = tBmsRx.usDisChgEmptyTime;
        v_disp_work_format_remaining_time(cStr, sizeof(cStr), usTotalMinutes);
        set_var_uca_remaining_usage_time(cStr);
    }
    #endif // boardBMS_EN

    #if (boardUSB_EN)
    vDisp_SetDevStateIcon(DEV_TYPE_USB, (tUsb.eDevState == DS_WORK));
    #endif // boardUSB_EN

    #if (boardDC_EN)
    vDisp_SetDevStateIcon(DEV_TYPE_DC, (tDc.eDevState == DS_WORK));
    #endif // boardDC_EN

    #if (boardMPPT_EN)
    vDisp_SetDevStateIcon(DEV_TYPE_PV, (tMppt.eDevState >= DS_BOOTING));
    #endif // boardMPPT_EN

    #if (boardDCAC_EN)
    vDisp_SetDevStateIcon(DEV_TYPE_AC_OUT, (tDcac.eDisChgState >= IOS_STARTING));
    vDisp_SetDevStateIcon(DEV_TYPE_AC_IN, (tDcac.eChgState >= IOS_STARTING));
    #endif // boardDCAC_EN

    snprintf(cStr, sizeof(cStr), "%d", tSysInfo.usOutPwr);
    set_var_uca_out_pwr_value(cStr);

    snprintf(cStr, sizeof(cStr), "%d", tSysInfo.usInPwr);
    set_var_uca_in_pwr_value(cStr);
}

/***********************************************************************************************************************
-----函数功能    格式化剩余使用时间
-----说明(备注)  将分钟数转换为固定宽度的小时/分钟显示文本
-----传入参数    pc_str:输出缓存  str_size:缓存大小  us_total_minutes:总分钟数
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if (boardBMS_EN)
__STATIC_INLINE void v_disp_work_format_remaining_time(char *pc_str, size_t str_size, u16 us_total_minutes)
{
    u16 us_hours = us_total_minutes / 60U;
    u16 us_minutes = us_total_minutes % 60U;

    if (us_hours > 99U)
    {
        us_hours = 99U;
        us_minutes = 99U;
    }

    if (us_hours >= 10U && us_minutes >= 10U)
        snprintf(pc_str, str_size, "%2uh %2um", (unsigned int)us_hours, (unsigned int)us_minutes);
    else if (us_hours >= 10U)
        snprintf(pc_str, str_size, "%2uh  %1um", (unsigned int)us_hours, (unsigned int)us_minutes);
    else if (us_minutes >= 10U)
        snprintf(pc_str, str_size, " %1uh %2um", (unsigned int)us_hours, (unsigned int)us_minutes);
    else
        snprintf(pc_str, str_size, " %1uh  %1um", (unsigned int)us_hours, (unsigned int)us_minutes);
}
#endif // boardBMS_EN

#endif /*boardDISPLAY_EN*/