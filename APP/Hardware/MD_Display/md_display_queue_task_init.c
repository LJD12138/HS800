/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-初始化                                                   *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "app_info.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include <stdio.h>


#define     dispTASK_INIT_CYCLE_TIME            100 //任务时间


//****************************************************局部函数定义************************************************//
bool b_disp_render_init_page(void);



/***********************************************************************************************************************
-----函数功能    初始化显示任务
-----说明(备注)  初始化显示驱动, 刷新初始化页面并置位显示任务完成标志
-----传入参数    tp_task:任务对象指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_disp_queue_task_init(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
        case 0:
        {
            if(tDisp.eDevState != DS_INIT)
                bDisp_SetDevState(DS_INIT);

            bDisp_Switch(ST_OFF, false);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }

        case 1:
        {
            vDisp_Init();
            bDisp_Switch(ST_OFF, true);
            vDisp_PageSyncByState(DS_INIT);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        //显示加载进度条
        case 2:
        {
            if(b_disp_render_init_page() == true)
            {
                tSysInfo.uInit.tFinish.bIF_DispTask = 1;
                cQueue_GotoStep(tp_task, STEP_END);
            }
        }
        break;
        
        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }
    
    #if(boardUSE_OS)
    vTaskDelay(dispTASK_INIT_CYCLE_TIME);
    #endif  //boardUSE_OS
}

/***********************************************************************************************************************
-----函数功能    渲染初始化页面
-----说明(备注)  初始化页只属于初始化队列, 由统一渲染入口分发到这里执行
-----传入参数    none
-----输出参数    none
-----返回值      true:初始化动画完成  false:继续显示
************************************************************************************************************************/
bool b_disp_render_init_page(void)
{
    #define INIT_ANIM_STEP     5U
    #define INIT_ANIM_MAX      100U
    static u8 s_ucAnimProgress = 0U;

    char line[32];
    u8 bar_w;

    if(s_ucAnimProgress < INIT_ANIM_MAX)
    {
        s_ucAnimProgress += INIT_ANIM_STEP;
        if(s_ucAnimProgress > INIT_ANIM_MAX)
            s_ucAnimProgress = INIT_ANIM_MAX;
    }

    bar_w = (u8)((106U * s_ucAnimProgress) / INIT_ANIM_MAX);

    vDisp_ClearRegion(0, 0, OLED_WIDTH_PIXELS, OLED_HEIGHT_PIXELS);
    u8g2_DrawRFrame(&u8g2, 1, 1, 126, 62, 4);
    u8g2_DrawBox(&u8g2, 8, 6, 112, 10);
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawStr(&u8g2, 46, 14, "INIT");
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    u8g2_DrawStr(&u8g2, 31, 29, "SmartFishJar");
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    sprintf(line, "SW %s", boardSOFTWARE_VERSION);
    u8g2_DrawStr(&u8g2, 20, 39, line);
    sprintf(line, "HW %s", boardHARDWARE_VERSION);
    u8g2_DrawStr(&u8g2, 20, 48, line);
    sprintf(line, "%s %s", tAppMemParam.tVerInfo.saBuildDate, tAppMemParam.tVerInfo.saBuildTime);
    u8g2_DrawStr(&u8g2, 10, 56, line);
    u8g2_DrawFrame(&u8g2, 10, 58, 108, 4);
    if(bar_w > 0U)
        u8g2_DrawBox(&u8g2, 11, 59, (u8)(bar_w > 106U ? 106U : bar_w), 2);

    vDisp_Refresh();
    tDispPageCtx.usDirtyMask = DDM_NONE;
    g_bDispPageDirty = false;

    if(s_ucAnimProgress >= INIT_ANIM_MAX)
    {
        s_ucAnimProgress = 0U;
        return true;
    }

    return false;
}
#endif //boardDISPLAY_EN
