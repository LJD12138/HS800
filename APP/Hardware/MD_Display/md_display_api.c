/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_api.c
 * Date    : 2026-09-22
 * Author  : LJD(291483914@qq.com)
 * Desc    : Display API - TFT + LVGL
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_api.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/lv_port_disp.h"
#include "MD_Display/md_display_iface.h"
#include "MD_Display/user_ui/main_1_ui.h"
#include "Middlewares/LVGL/lvgl.h"
#include "Middlewares/LVGL/lv_conf.h"

#include "ui.h"
#include "screens.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
/* UI屏幕对象已创建标志 (防重复 ui_init 导致 EEZ 屏幕对象重复创建) */
static bool G_bUiInitialized = false;

//****************************************************Function Declaration******************************************************//
static void v_disp_delay_ms(u16 ms);
static void v_disp_tft_set_power(bool on);
static void v_disp_set_window(u16 x1, u16 y1, u16 x2, u16 y2);

#if(boardUSE_OS)
static uint32_t ul_lv_tick_get(void);
#endif  /* boardUSE_OS */


/***********************************************************************************************************************
 * 函数功能    : 显示模块初始化函数
 * 说明(备注)  : 这是显示模块的主初始化函数，应优先调用
 *               完成LVGL初始化、电源开启和首次刷新
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_Init(void)
{
    lv_obj_t *active_screen;

    /* 初始化LVGL核心库（如果尚未初始化） */
    if(!lv_is_initialized())
        lv_init();

    /* 注册LVGL系统时间基：直接复用FreeRTOS节拍计数(1ms分辨率)，替代lv_tick_inc喂值方式 */
    #if(boardUSE_OS)
    lv_tick_set_cb(ul_lv_tick_get);
    #endif  /* boardUSE_OS */

    /* 初始化显示驱动（如果尚未初始化） */
    if(lv_display_get_default() == NULL)
        lv_port_disp_init();


    if (G_bUiInitialized == false)
    {
        G_bUiInitialized = true;
        ui_init();
    }

    active_screen = lv_screen_active();
    if(active_screen != NULL)
        lv_obj_invalidate(active_screen);

    /* 开启显示屏电源 */
    v_disp_tft_set_power(true);

    /* 关闭背光 */
    vDisp_SetBacklight(false);
}

/***********************************************************************************************************************
 * 函数功能    : 安全加载指定屏幕
 * 说明(备注)  : 封装 EEZ Studio loadScreen, 仅在显示任务上下文调用
 * 传入参数    : screen_id: 屏幕枚举 (如 SCREEN_ID_MAIN_WORK, SCREEN_ID_MAIN_BOOT 等)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_LoadScreen(int screen_id)
{
    if (lv_is_initialized())
        loadScreen(screen_id);
}

/***********************************************************************************************************************
 * 函数功能    : 执行一轮 UI 刷新
 * 说明(备注)  : 渲染输出统一入口 (由 UniDisplay 移植层 vDisp_PortUiRefresh 转调): EEZ 变量绑定
 *               -> 自绘动画 -> LVGL 定时器与重绘
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_UiRefresh(void)
{
    if (!lv_is_initialized())
        return;

    /* 1. EEZ Studio 变量绑定与视图更新 */
    ui_tick();

    /* 2. 用户自定义自绘图片呼吸/动画更新 */
    vImgAnim_ManualTick();

    /* 3. LVGL 核心定时器与重绘处理 */
    lv_timer_handler();
}

/***********************************************************************************************************************
 * 函数功能    : 显示就绪状态检查函数
 * 说明(备注)  : 用于在调用其他显示函数前检查模块状态
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:显示模块已初始化并就绪   false:显示模块未初始化
 ************************************************************************************************************************/
bool bDisp_IsReady(void)
{
    return lv_is_initialized();
}

/***********************************************************************************************************************
 * 函数功能    : 填充矩形区域
 * 说明(备注)  : 用指定颜色填充矩形区域
 * 传入参数    : x:起始X坐标  y:起始Y坐标  w:宽度  h:高度  color:RGB565颜色
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
#if(!LV_USE_ST7789)
void vDisp_FastDrawColor(u16 x, u16 y, u16 w, u16 h, u16 *color)
{
    uint32_t draw_size = (w - x + 1) * (h - y  + 1);

    //设置需要画的窗口大小
    v_disp_set_window(x, y, w, h);
    
    //批量发送颜色数据，底层接口会根据配置选择DMA或软件SPI发送
    vDisp_WriteBuffer((u8 *)color, draw_size * 2);

}
#endif  //LV_USE_ST7789

/***********************************************************************************************************************
 * 函数功能    : 毫秒级延时函数
 * 说明(备注)  : 支持FreeRTOS操作系统和裸机两种模式
 *               - OS模式：使用vTaskDelay进行任务延时
 *               - 裸机模式：使用CPU空循环实现延时
 * 传入参数    : ms: 延时毫秒数
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_disp_delay_ms(u16 ms)
{
    #if(boardUSE_OS)
    /* FreeRTOS模式：将毫秒转换为系统节拍数进行延时 */
    vTaskDelay(pdMS_TO_TICKS(ms));
    #else
    /* 裸机模式：通过CPU空循环实现延时 */
    while(ms--)
    {
        /* 根据系统时钟频率计算循环次数，实现约1ms延时 */
        for(volatile u32 i = 0U; i < (SystemCoreClock / 8000U); i++)
            __NOP();  /* 空指令，防止编译器优化 */
    }
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : TFT显示屏电源控制函数
 * 说明(备注)  : 通过发送ST7789显示开/关命令控制显示状态
 *               开启显示需要较长延时(120ms)，关闭显示延时较短(20ms)
 * 传入参数    : on: true-开启显示，false-关闭显示
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_disp_tft_set_power(bool on)
{
    /* 根据电源状态选择对应的显示命令 */
    vDisp_WriteCommand(on ? ST7789_DISPON : ST7789_DISPOFF);
    /* 开启显示需要更长的稳定时间 */
    v_disp_delay_ms(on ? 50U : 20U);
}

/***********************************************************************************************************************
 * 函数功能    : 设置显示窗口
 * 说明(备注)  : 设置TFT的列地址和行地址
 * 传入参数    : x1:起始X坐标  y1:起始Y坐标  x2:结束X坐标  y2:结束Y坐标
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_disp_set_window(u16 x1, u16 y1, u16 x2, u16 y2)
{
    vDisp_WriteCommand(ST7789_CASET);
    vDisp_WriteData16(x1);
    vDisp_WriteData16(x2);
    
    vDisp_WriteCommand(ST7789_RASET);
    vDisp_WriteData16(y1);
    vDisp_WriteData16(y2);
    
    vDisp_WriteCommand(ST7789_RAMWR);
}

#if(boardUSE_OS)
/***********************************************************************************************************************
 * 函数功能    : LVGL系统时间基获取回调
 * 说明(备注)  : 经lv_tick_set_cb注册后LVGL直接读取FreeRTOS节拍计数，无需lv_tick_inc喂数
 *               configTICK_RATE_HZ=1000时1个节拍=1ms，计数值即毫秒时间
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 当前系统时间(毫秒)
 ************************************************************************************************************************/
static uint32_t ul_lv_tick_get(void)
{
    return (uint32_t)xTaskGetTickCount();
}
#endif  /* boardUSE_OS */

#endif  /*boardDISPLAY_EN*/
