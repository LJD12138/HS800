/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-工作中 - TFT+LVGL版本                                    *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_task.h"
#include "Print/print_task.h"

#include "lvgl.h"

#define dispTASK_WORK_CYCLE_TIME            100

/* LVGL UI对象 */
static lv_obj_t *s_pLabel = NULL;

static void vDemo_CreateTestUI(void);


void v_disp_queue_task_work(Task_T *tp_task)
{
    if(lwrb_get_full(&tp_task->tQueueBuff) > 0U)
    {
        cQueue_GotoStep(tp_task, STEP_END);
    }

    if((tDisp.bLight == false) && (tp_task->ucStep != 0U))
    {
        tp_task->ucStep = 0U;
        #if(boardUSE_OS)
        vTaskDelay(dispTASK_WORK_CYCLE_TIME);
        #endif
        return;
    }

    switch(tp_task->ucStep)
    {
        case 0:
            if(tDisp.eDevState != DS_WORK)
                bDisp_SetDevState(DS_WORK);
            bDisp_Switch(ST_ON, true);
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;

        case 1:
            vDemo_CreateTestUI();
            cQueue_GotoStep(tp_task, STEP_END);
            break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

#if(boardUSE_OS)
    vTaskDelay(dispTASK_WORK_CYCLE_TIME);
#endif
}


/**
 * 创建测试UI界面
 */
static void vDemo_CreateTestUI(void)
{
    /* 设置默认背景色为深蓝色 */
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);
    
    /* 创建标题标签 */
    lv_obj_t *title_label = lv_label_create(lv_screen_active());
    lv_label_set_text(title_label, "HS800 TFT Demo");
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    
    /* 创建主标签 */
    s_pLabel = lv_label_create(lv_screen_active());
    lv_label_set_text(s_pLabel, "Hello TFT!\nZJY240KP-IF10\n240x320 RGB565");
    lv_obj_set_style_text_color(s_pLabel, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_pLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_pLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(s_pLabel);
    
    /* 创建状态标签 */
    lv_obj_t *status_label = lv_label_create(lv_screen_active());
    lv_label_set_text(status_label, "LVGL v9.4 + ST7789V2");
    lv_obj_set_style_text_color(status_label, lv_color_make(0, 255, 0), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
}

#endif  /*boardDISPLAY_EN*/