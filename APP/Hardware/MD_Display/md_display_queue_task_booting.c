/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-启动中                                                   *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#define     dispTASK_BOOTING_CYCLE_TIME         100 //任务时间
#define     BOOT_AQUA_X                         20U
#define     BOOT_AQUA_Y                         11U
#define     BOOT_AQUA_W                         88U
#define     BOOT_AQUA_H                         42U
#define     BOOT_FISH_X_BASE                    62U
#define     BOOT_FISH_Y                         29U
#define     BOOT_ANIM_TOTAL_FRAMES              24U
#define     BOOT_SWIM_FRAME_MAX                 16U

//****************************************************参数初始化**************************************************//
static u8 s_ucBootAnimFrame = 0U;
static u8 s_ucBootAnimCnt = 0U;

//****************************************************局部函数定义************************************************//
static bool b_disp_render_booting_page(void);
static void v_boot_draw_pixel(s16 x, s16 y);
static void v_boot_draw_wave(u8 y_base, u8 phase, u8 width);
static void v_boot_draw_seaweed(u8 frame);
static void v_boot_draw_aquarium_frame(u8 frame);
static s16 s_boot_get_fish_offset(u8 frame);
static void v_boot_draw_fish(s16 x, s16 y, s8 direction, u8 tail_phase);
static void v_boot_draw_bubbles(u8 frame);
static void v_boot_draw_progress(u8 frame);


/***********************************************************************************************************************
-----函数功能    启动中显示任务
-----说明(备注)  打开显示并刷新启动进度页, 有新任务入队时退出当前任务
-----传入参数    tp_task:任务对象指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_disp_queue_task_booting(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
        //初始化
        case 0:
        {
            if(tDisp.eDevState != DS_BOOTING)
                bDisp_SetDevState(DS_BOOTING);
			vDisp_PageSyncByState(DS_BOOTING);

            if(g_bDispPageDirty && tDispPageCtx.usDirtyMask == DDM_FULL)
                vDisp_Init();

            bDisp_Switch(ST_ON, true);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }

        //加载动画
        case 1:
        {
            if(b_disp_render_booting_page() == true)
            {
                bDisp_SetDevState(DS_WORK);
                bDisp_Switch(ST_ON, true);
                tDispPageCtx.usDirtyMask = DDM_FULL;
                g_bDispPageDirty = true;
                bDisp_RenderUi();
                cQueue_GotoStep(tp_task, STEP_END);
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
    vTaskDelay(dispTASK_BOOTING_CYCLE_TIME);
    #endif  //boardUSE_OS
}

/***********************************************************************************************************************
-----函数功能    渲染启动中页面
-----说明(备注)  启动页只属于启动队列, 由统一渲染入口分发到这里执行
-----传入参数    none
-----输出参数    none
-----返回值      true:动画完成  false:继续显示
************************************************************************************************************************/
static bool b_disp_render_booting_page(void)
{
    u8 frame = s_ucBootAnimFrame;
    s16 fish_offset = s_boot_get_fish_offset(frame);
    s16 next_offset = s_boot_get_fish_offset((u8)(frame + 1U));
    s8 fish_dir = (next_offset >= fish_offset) ? 1 : -1;

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetDrawColor(&u8g2, 1);

    u8g2_DrawBox(&u8g2, 0, 0, OLED_WIDTH_PIXELS, 10U);
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawStr(&u8g2, 44, 7, "BOOTING");
    u8g2_SetDrawColor(&u8g2, 1);

    v_boot_draw_aquarium_frame(frame);
    v_boot_draw_wave(18U, (u8)(frame * 3U), 84U);
    v_boot_draw_wave(22U, (u8)(frame * 3U + 10U), 80U);
    v_boot_draw_fish((s16)BOOT_FISH_X_BASE + fish_offset, BOOT_FISH_Y, fish_dir, frame);
    v_boot_draw_bubbles(s_ucBootAnimCnt);
    v_boot_draw_progress(frame);

    s_ucBootAnimFrame += 2;
    if(s_ucBootAnimFrame >= BOOT_SWIM_FRAME_MAX)
        s_ucBootAnimFrame = 0U;

    if(s_ucBootAnimCnt < BOOT_ANIM_TOTAL_FRAMES)
        s_ucBootAnimCnt++;

    vDisp_Refresh();
    tDispPageCtx.usDirtyMask = DDM_NONE;
    g_bDispPageDirty = false;

    if(s_ucBootAnimCnt >= BOOT_ANIM_TOTAL_FRAMES)
    {
        s_ucBootAnimFrame = 0U;
        s_ucBootAnimCnt = 0U;
        return true;
    }

    return false;
}

/***********************************************************************************************************************
-----函数功能    启动页绘制单个像素点
-----说明(备注)  带OLED屏幕边界检查, 超出范围则忽略
-----传入参数    x:像素X坐标  y:像素Y坐标
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_boot_draw_pixel(s16 x, s16 y)
{
    if(x < 0 || y < 0 || x >= (s16)OLED_WIDTH_PIXELS || y >= (s16)OLED_HEIGHT_PIXELS)
        return;

    u8g2_DrawPixel(&u8g2, (u8)x, (u8)y);
}

/***********************************************************************************************************************
-----函数功能    启动页绘制波浪线
-----说明(备注)  使用预定义的波形查找表绘制上下波动的线条
-----传入参数    y_base:波浪线Y轴基准位置  phase:波形相位偏移  width:波浪线宽度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_boot_draw_wave(u8 y_base, u8 phase, u8 width)
{
    static const s8 c_wave_lut[16] = {0, 1, 1, 2, 2, 1, 1, 0, 0, -1, -1, -2, -2, -1, -1, 0};
    u8 x_start = (u8)((OLED_WIDTH_PIXELS - width) / 2U);

    for(u8 x = 0U; x < width; x++)
    {
        s16 wave_y = (s16)y_base + c_wave_lut[(u8)(x + phase) & 0x0FU];
        v_boot_draw_pixel((s16)x_start + x, wave_y);
        v_boot_draw_pixel((s16)x_start + x, (s16)(wave_y + 1));
    }
}

/***********************************************************************************************************************
-----函数功能    启动页绘制海草
-----说明(备注)  绘制3株随帧摆动的海草, 使用查找表实现摇曳效果
-----传入参数    frame:当前动画帧序号
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_boot_draw_seaweed(u8 frame)
{
    static const u8 c_weed_x[3] = {35U, 75U, 90U};
    static const s8 c_shift_lut[8] = {0, 1, 1, 2, 1, 0, -1, -1};
    u8 bottom_y = (u8)(BOOT_AQUA_Y + BOOT_AQUA_H - 7U);

    for(u8 weed = 0U; weed < 3U; weed++)
    {
        for(u8 i = 0U; i < 8U; i++)
        {
            s16 wx = (s16)c_weed_x[weed] + c_shift_lut[(u8)(i + frame + (weed * 2U)) & 0x07U];
            s16 wy = (s16)bottom_y - i;
            v_boot_draw_pixel(wx, wy);
            if((i & 0x01U) == 0U)
                v_boot_draw_pixel((s16)(wx + 1), wy);
        }
    }
}

/***********************************************************************************************************************
-----函数功能    启动页绘制水族箱框架
-----说明(备注)  绘制圆角矩形外框、顶部高光及底部沙砾效果
-----传入参数    frame:当前动画帧序号(用于海草动画)
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_boot_draw_aquarium_frame(u8 frame)
{
    u8 x = BOOT_AQUA_X;
    u8 y = BOOT_AQUA_Y;
    u8 w = BOOT_AQUA_W;
    u8 h = BOOT_AQUA_H;

    u8g2_DrawRFrame(&u8g2, x, y, w, h, 4U);
    u8g2_DrawLine(&u8g2, (u8)(x + 3U), (u8)(y + 3U), (u8)(x + w - 4U), (u8)(y + 3U));

    for(u8 sx = (u8)(x + 4U); sx < (u8)(x + w - 4U); sx += 2U)
    {
        v_boot_draw_pixel(sx, (s16)(y + h - 6U));
        if((sx & 0x03U) == 0U)
            v_boot_draw_pixel((s16)(sx + 1U), (s16)(y + h - 5U));
    }

    v_boot_draw_seaweed(frame);
}

/***********************************************************************************************************************
-----函数功能    获取启动页鱼的游动水平偏移
-----说明(备注)  使用预定义的正弦查找表, 实现鱼在水中左右游动的位置计算
-----传入参数    frame:当前动画帧序号
-----输出参数    none
-----返回值      鱼相对于基准位置的X轴偏移量(像素)
************************************************************************************************************************/
static s16 s_boot_get_fish_offset(u8 frame)
{
    static const s8 c_swim_lut[BOOT_SWIM_FRAME_MAX] = {
        0, 7, 13, 17, 18, 17, 13, 7,
        0, -7, -13, -17, -18, -17, -13, -7
    };

    return c_swim_lut[frame % BOOT_SWIM_FRAME_MAX];
}

/***********************************************************************************************************************
-----函数功能    启动页绘制鱼
-----说明(备注)  绘制鱼的身体、眼睛及摆动的尾巴, 支持左右游动方向
-----传入参数    x:鱼的X坐标  y:鱼的Y坐标  direction:游动方向(>0向右,<0向左)  tail_phase:尾巴摆动相位
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_boot_draw_fish(s16 x, s16 y, s8 direction, u8 tail_phase)
{
    static const u8 c_body[6][6] = {
        {0U, 0U, 1U, 1U, 0U, 0U},
        {0U, 1U, 1U, 1U, 1U, 0U},
        {1U, 1U, 1U, 1U, 1U, 1U},
        {1U, 1U, 1U, 1U, 1U, 1U},
        {0U, 1U, 1U, 1U, 1U, 0U},
        {0U, 0U, 1U, 1U, 0U, 0U},
    };

    for(u8 row = 0U; row < 6U; row++)
    {
        for(u8 col = 0U; col < 6U; col++)
        {
            if(c_body[row][col] != 0U)
            {
                s16 px = (direction < 0) ? (s16)(x + (5U - col)) : (s16)(x + col);
                v_boot_draw_pixel(px, (s16)(y + row));
            }
        }
    }

    u8g2_SetDrawColor(&u8g2, 0);
    v_boot_draw_pixel((direction > 0) ? (s16)(x + 4) : (s16)(x + 1), (s16)(y + 2));
    u8g2_SetDrawColor(&u8g2, 1);

    for(u8 i = 0U; i < 3U; i++)
    {
        bool draw_tail = true;

        if((tail_phase & 0x03U) == 1U && i == 0U)
            draw_tail = false;
        else if((tail_phase & 0x03U) == 3U && i == 2U)
            draw_tail = false;

        if(draw_tail == true)
        {
            s16 tx = (direction > 0) ? (s16)(x + 6) : (s16)(x - 2);
            v_boot_draw_pixel(tx, (s16)(y + 1U + i));
        }
    }

    v_boot_draw_pixel((s16)(x + 2), (s16)(y - 1));
    v_boot_draw_pixel((s16)(x + 3), (s16)(y - 1));
}

/***********************************************************************************************************************
-----函数功能    启动页绘制气泡
-----说明(备注)  绘制5个上升的气泡, 不同气泡具有不同速度和初始偏移
-----传入参数    frame:当前动画帧序号
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_boot_draw_bubbles(u8 frame)
{
    static const struct
    {
        u8 x;
        u8 base_y;
        u8 speed;
        u8 offset;
    } c_bubbles[5] = {
        {45U, 48U, 1U, 0U},
        {55U, 50U, 2U, 5U},
        {70U, 49U, 1U, 10U},
        {85U, 50U, 2U, 15U},
        {95U, 48U, 1U, 20U},
    };

    for(u8 i = 0U; i < 5U; i++)
    {
        u8 rise = (u8)((frame * c_bubbles[i].speed + c_bubbles[i].offset) % 29U);
        s16 by = (s16)c_bubbles[i].base_y - rise;

        if(by < (s16)(BOOT_AQUA_Y + 5U) || by > (s16)(BOOT_AQUA_Y + BOOT_AQUA_H - 5U))
            continue;

        if((rise & 0x01U) == 0U)
            u8g2_DrawCircle(&u8g2, c_bubbles[i].x, (u8)by, 1U, U8G2_DRAW_ALL);
        else
            u8g2_DrawBox(&u8g2, c_bubbles[i].x, (u8)by, 2U, 2U);
    }
}

/***********************************************************************************************************************
-----函数功能    启动页绘制进度提示
-----说明(备注)  在底部显示"Booting"文字及旋转的等待点
-----传入参数    frame:当前动画帧序号
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_boot_draw_progress(u8 frame)
{
    static const s8 c_dot_pos[8][2] = {
        {0, -2}, {1, -1}, {2, 0}, {1, 1},
        {0, 2}, {-1, 1}, {-2, 0}, {-1, -1}
    };
    u8 idx = frame & 0x07U;

    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawStr(&u8g2, 38, 61, "Booting");
    v_boot_draw_pixel((s16)(80 + c_dot_pos[idx][0]), (s16)(60 + c_dot_pos[idx][1]));
}

