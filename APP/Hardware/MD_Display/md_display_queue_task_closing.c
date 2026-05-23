/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-关闭中                                                   *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include <stdio.h>


#define     dispTASK_CLOSE_CYCLE_TIME           100 //任务时间

#define     CLOSE_ANIM_TOTAL_FRAMES             24U
#define     CLOSE_WAVE_PHASE_MAX                16U

//****************************************************参数初始化**************************************************//
static u8 s_ucCloseAnimFrame = 0U;

//****************************************************局部函数定义************************************************//
static bool b_disp_render_closing_page(void);
static void v_close_draw_pixel(s16 x, s16 y);
static void v_close_draw_wave(u8 y_base, u8 phase, u8 width, u8 frame);
static void v_close_draw_progress(u8 frame);
static void v_close_draw_saving_indicator(u8 frame);


/***********************************************************************************************************************
-----函数功能    关闭中显示任务
-----说明(备注)  打开显示并刷新关机提示页面
-----传入参数    tp_task:任务对象指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_disp_queue_task_closing(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
        //初始化
        case 0:
        {
            if(tDisp.eDevState != DS_CLOSING)
                bDisp_SetDevState(DS_CLOSING);
                vDisp_PageSyncByState(DS_CLOSING);

            bDisp_Switch(ST_ON, true);

            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        //加载动画
        case 1:
        {
            if(b_disp_render_closing_page() == true)
            {
                bDisp_SetDevState(DS_SHUT_DOWN);
                cQueue_GotoStep(tp_task, STEP_NEXT);
            }
        }
        break;

        //等待新任务
        case 2:
        {
            if(lwrb_get_full(&tp_task->tQueueBuff) > 0)
                cQueue_GotoStep(tp_task, STEP_END);
        }
        break;
        
        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }
    
    #if(boardUSE_OS)
    vTaskDelay(dispTASK_CLOSE_CYCLE_TIME);
    #endif  //boardUSE_OS
}

/***********************************************************************************************************************
-----函数功能    渲染关机中页面
-----说明(备注)  关机页只属于关闭队列, 由统一渲染入口分发到这里执行
                 动画效果: 波浪逐渐平息 + 保存进度条 + Saving旋转指示
                 总时长约2.4秒(24帧 x 100ms)
-----传入参数    none
-----输出参数    none
-----返回值      true:动画完成  false:继续显示
************************************************************************************************************************/
static bool b_disp_render_closing_page(void)
{
    u8 frame = s_ucCloseAnimFrame;

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetDrawColor(&u8g2, 1);

    // 顶部标题栏(黑色背景条)
    u8g2_DrawBox(&u8g2, 0, 0, OLED_WIDTH_PIXELS, 10U);
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawStr(&u8g2, 32, 7, "P30 CLOSING");
    u8g2_SetDrawColor(&u8g2, 1);

    // 波浪动画(振幅随帧递减, 模拟波浪逐渐平息)
    v_close_draw_wave(46U, (u8)(frame * 3U), 84U, frame);

    // Saving 指示(动画最后4帧消失)
    v_close_draw_saving_indicator(frame);

    // 底部进度条
    v_close_draw_progress(frame);

    vDisp_Refresh();
    tDispPageCtx.usDirtyMask = DDM_NONE;
    g_bDispPageDirty = false;

    s_ucCloseAnimFrame++;
    if(s_ucCloseAnimFrame >= CLOSE_ANIM_TOTAL_FRAMES)
    {
        s_ucCloseAnimFrame = 0U;
        return true;
    }
    return false;
}

/***********************************************************************************************************************
-----函数功能    关闭页绘制单个像素点
-----说明(备注)  带OLED屏幕边界检查, 超出范围则忽略
-----传入参数    x:像素X坐标  y:像素Y坐标
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_close_draw_pixel(s16 x, s16 y)
{
    if(x < 0 || y < 0 || x >= (s16)OLED_WIDTH_PIXELS || y >= (s16)OLED_HEIGHT_PIXELS)
        return;

    u8g2_DrawPixel(&u8g2, (u8)x, (u8)y);
}

/***********************************************************************************************************************
-----函数功能    关闭页绘制波浪线
-----说明(备注)  使用预定义的波形查找表绘制上下波动的线条, 振幅随帧数递减模拟波浪平息
                 帧0-3:正常振幅  帧4-7:振幅减半  帧8-11:接近平直  帧12+:波浪消失
-----传入参数    y_base:波浪线Y轴基准位置  phase:波形相位偏移  width:波浪线宽度  frame:当前动画帧序号
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_close_draw_wave(u8 y_base, u8 phase, u8 width, u8 frame)
{
    static const s8 c_wave_lut[16] = {0, 1, 1, 2, 2, 1, 1, 0, 0, -1, -1, -2, -2, -1, -1, 0};
    u8 x_start = (u8)((OLED_WIDTH_PIXELS - width) / 2U);

    s8 amp;
    if(frame < 4U)       amp = 2;
    else if(frame < 8U)  amp = 1;
    else if(frame < 12U) amp = 0;
    else                 return;

    for(u8 x = 0U; x < width; x++)
    {
        s16 wave_y = (s16)y_base + (c_wave_lut[(u8)(x + phase) & 0x0FU] * amp) / 2;
        v_close_draw_pixel((s16)x_start + x, wave_y);
        if(amp >= 2)
            v_close_draw_pixel((s16)x_start + x, wave_y + 1);
    }
}

/***********************************************************************************************************************
-----函数功能    关闭页绘制进度条
-----说明(备注)  在底部显示保存进度条, 随动画帧推进从0%填充至100%
-----传入参数    frame:当前动画帧序号
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_close_draw_progress(u8 frame)
{
    #define PROGRESS_X      14U
    #define PROGRESS_Y      56U
    #define PROGRESS_W      100U
    #define PROGRESS_H      4U

    u8 max_prog = CLOSE_ANIM_TOTAL_FRAMES - 8U;
    u8 prog = (frame > 4U) ? (frame - 4U) : 0U;
    if(prog > max_prog) prog = max_prog;

    // 进度条外框
    u8g2_DrawFrame(&u8g2, PROGRESS_X, PROGRESS_Y, PROGRESS_W, PROGRESS_H);

    // 填充部分
    u8 fill = (prog * (PROGRESS_W - 2U)) / max_prog;
    if(fill > 0U)
        u8g2_DrawBox(&u8g2, PROGRESS_X + 1, PROGRESS_Y + 1, fill, PROGRESS_H - 2U);
}

/***********************************************************************************************************************
-----函数功能    关闭页绘制保存状态指示
-----说明(备注)  显示"Saving"文字及围绕其旋转的等待点
-----传入参数    frame:当前动画帧序号
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_close_draw_saving_indicator(u8 frame)
{
    static const s8 c_dot_pos[8][2] = {
        {0, -2}, {1, -1}, {2, 0}, {1, 1},
        {0, 2}, {-1, 1}, {-2, 0}, {-1, -1}
    };

    if(frame >= CLOSE_ANIM_TOTAL_FRAMES - 4U)
        return;

    u8 idx = frame & 0x07U;
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawStr(&u8g2, 40, 54, "Saving");
    v_close_draw_pixel((s16)(72 + c_dot_pos[idx][0]), (s16)(53 + c_dot_pos[idx][1]));
}
