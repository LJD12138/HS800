/*****************************************************************************************************************
 *                                                                                                                *
 *                                         Display API - TFT + LVGL                                              *
 *                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_api.h"
#include <stdbool.h>

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_iface.h"

// #include "ui.h"
// #include "screens.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

//****************************************************局部宏定义****************************************//
//ST7789控制器命令定义
#define ST7789_SWRESET                      0x01U   //软件复位命令
#define ST7789_SLPOUT                       0x11U   //退出睡眠模式
#define ST7789_NORON                        0x13U   //正常显示模式开启
#define ST7789_INVON                        0x21U   //显示反转开启
#define ST7789_DISPOFF                      0x28U   //关闭显示
#define ST7789_DISPON                       0x29U   //开启显示
#define ST7789_CASET                        0x2AU   //列地址设置
#define ST7789_RASET                        0x2BU   //行地址设置
#define ST7789_RAMWR                        0x2CU   //内存写入
#define ST7789_COLMOD                       0x3AU   //颜色模式设置
#define ST7789_MADCTL                       0x36U   //内存访问控制
//ST7789内存访问控制位定义
#define ST7789_MADCTL_MY                    0x80U   //Y轴镜像
#define ST7789_MADCTL_MX                    0x40U   //X轴镜像
#define ST7789_MADCTL_MV                    0x20U   //行列交换
#define ST7789_MADCTL_BGR                   0x08U   //BGR颜色顺序

//****************************************************局部变量定义**********************************************//
/* 刷屏缓冲区 - 用于纯色填充 */
// __ALIGNED(4) static uint8_t S_tft_fill_buf[dispTFT_BUF_SIZE];

//****************************************************局部函数声明****************************************************//
static void v_disp_delay_ms(u16 ms);
static void v_disp_tft_set_power(bool on);
static void v_disp_set_window(u16 x1, u16 y1, u16 x2, u16 y2);
static void v_disp_fill_color(uint16_t us_xs, uint16_t us_ys, uint16_t us_xe, uint16_t us_ye, uint16_t us_color);

#if(!LV_USE_ST7789)
static void lcd_send_init_commands(void);
#endif



/***********************************************************************************************************************
 -----函数功能    毫秒级延时函数
 -----说明(备注)  支持FreeRTOS操作系统和裸机两种模式
                 - OS模式：使用vTaskDelay进行任务延时
                 - 裸机模式：使用CPU空循环实现延时
 -----传入参数    ms: 延时毫秒数
 -----输出参数    none
 -----返回值      none
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
    #endif
}

/***********************************************************************************************************************
 -----函数功能    TFT显示屏电源控制函数
 -----说明(备注)  通过发送ST7789显示开/关命令控制显示状态
                 开启显示需要较长延时(120ms)，关闭显示延时较短(20ms)
 -----传入参数    on: true-开启显示，false-关闭显示
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_disp_tft_set_power(bool on)
{
    /* 根据电源状态选择对应的显示命令 */
    vDisp_TftWriteCommand(on ? ST7789_DISPON : ST7789_DISPOFF);
    /* 开启显示需要更长的稳定时间 */
    v_disp_delay_ms(on ? 50U : 20U);
}

/***********************************************************************************************************************
-----函数功能    设置显示窗口
-----说明(备注)  设置TFT的列地址和行地址
-----传入参数    x1:起始X坐标  y1:起始Y坐标  x2:结束X坐标  y2:结束Y坐标
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_set_window(u16 x1, u16 y1, u16 x2, u16 y2)
{
    vDisp_TftWriteCommand(ST7789_CASET);
    vDisp_TftWriteData16(x1);
    vDisp_TftWriteData16(x2);
    
    vDisp_TftWriteCommand(ST7789_RASET);
    vDisp_TftWriteData16(y1);
    vDisp_TftWriteData16(y2);
    
    vDisp_TftWriteCommand(ST7789_RAMWR);
}

/***********************************************************************************************************************
 -----函数功能    快速绘制颜色数据到屏幕
 -----说明(备注)  将RGB565格式的像素数据直接写入TFT显存，适用于全屏刷新等场景
                 需要底层接口支持批量数据写入以发挥性能优势
 -----传入参数    x:起始X坐标  y:起始Y坐标  w:结束X坐标  h:结束Y坐标  color:像素数据指针（RGB565格式）
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_disp_fill_color(uint16_t us_xs, uint16_t us_ys, uint16_t us_xe, uint16_t us_ye, uint16_t us_color)
{
    // uint32_t pixel_count = (uint32_t)(us_xe - us_xs + 1) * (us_ye - us_ys + 1);
    // uint32_t byte_count = pixel_count * 2;
    
    // v_disp_set_window(us_xs, us_ys, us_xe, us_ye);

    // /* 预填充缓冲区 */
    // memset(S_tft_fill_buf, us_color, sizeof(S_tft_fill_buf));
    
    // /* 分批次发送数据 */
    // while (byte_count > 0) {
    //     uint32_t bytes_to_send = (byte_count > dispTFT_BUF_SIZE) ? dispTFT_BUF_SIZE : byte_count;
    //     vDisp_TftWriteBuffer(S_tft_fill_buf, bytes_to_send);
    //     byte_count -= bytes_to_send;
    // }
}

/***********************************************************************************************************************
 -----函数功能    发送初始化命令序列
 -----说明(备注)  按照ST7789 datasheet要求的时序初始化显示屏
                 包括硬件复位、软件复位、颜色模式设置等
                 初始化完成后清屏为黑色
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
#if(!LV_USE_ST7789)
static void lcd_send_init_commands(void)
{
    /* 硬件复位时序：高-低-高，确保控制器可靠复位 */
    dispTFT_RES_H();
    v_disp_delay_ms(10U);
    dispTFT_RES_L();
    v_disp_delay_ms(50U);
    dispTFT_RES_H();
    v_disp_delay_ms(50U);

    /* 退出睡眠模式 */
    vDisp_TftWriteCommand(ST7789_SLPOUT);
    v_disp_delay_ms(120U);

    /* 设置内存访问控制（显示方向） */
    vDisp_TftWriteCommand(ST7789_MADCTL);
    vDisp_TftWriteData8(ST7789_MADCTL_MY | ST7789_MADCTL_MV);

    /* 设置颜色模式为16位RGB565 */
    vDisp_TftWriteCommand(ST7789_COLMOD);
    vDisp_TftWriteData8(0x55U);

    /* Porch控制设置 */
    vDisp_TftWriteCommand(0xB2U);
    vDisp_TftWriteData8(0x0CU);
    vDisp_TftWriteData8(0x0CU);
    vDisp_TftWriteData8(0x00U);
    vDisp_TftWriteData8(0x33U);
    vDisp_TftWriteData8(0x33U);

    /* Gate控制设置 */
    vDisp_TftWriteCommand(0xB7U);
    vDisp_TftWriteData8(0x35U);

    /* VCOM设置 */
    vDisp_TftWriteCommand(0xBBU);
    vDisp_TftWriteData8(0x32U); //Vcom=1.35V

    /* LCM控制设置 */
    vDisp_TftWriteCommand(0xC2U);
    vDisp_TftWriteData8(0x01U);

    /* VDV和VRH命令使能设置 */
    vDisp_TftWriteCommand(0xC3U);
    vDisp_TftWriteData8(0x15U); //GVDD=4.8V

    /* VRH设置 */
    vDisp_TftWriteCommand(0xC4U);
    vDisp_TftWriteData8(0x20U); //VDV, 0x20:0v

    /* 帧率控制设置 */
    vDisp_TftWriteCommand(0xC6U);
    vDisp_TftWriteData8(0x0FU); //0x0F:60Hz

    /* 电源控制设置 */
    vDisp_TftWriteCommand(0xD0U);
    vDisp_TftWriteData8(0xA4U);
    vDisp_TftWriteData8(0xA1U);

    /* 正Gamma校正设置 */
    vDisp_TftWriteCommand(0xE0U);
    vDisp_TftWriteData8(0xD0U);
    vDisp_TftWriteData8(0x08U);
    vDisp_TftWriteData8(0x0EU);
    vDisp_TftWriteData8(0x09U);
    vDisp_TftWriteData8(0x09U);
    vDisp_TftWriteData8(0x05U);
    vDisp_TftWriteData8(0x31U);
    vDisp_TftWriteData8(0x33U);
    vDisp_TftWriteData8(0x48U);
    vDisp_TftWriteData8(0x17U);
    vDisp_TftWriteData8(0x14U);
    vDisp_TftWriteData8(0x15U);
    vDisp_TftWriteData8(0x31U);
    vDisp_TftWriteData8(0x34U);

    /* 负Gamma校正设置 */
    vDisp_TftWriteCommand(0xE1U);
    vDisp_TftWriteData8(0xD0U);
    vDisp_TftWriteData8(0x08U);
    vDisp_TftWriteData8(0x0EU);
    vDisp_TftWriteData8(0x09U);
    vDisp_TftWriteData8(0x09U);
    vDisp_TftWriteData8(0x15U);
    vDisp_TftWriteData8(0x31U);
    vDisp_TftWriteData8(0x33U);
    vDisp_TftWriteData8(0x48U);
    vDisp_TftWriteData8(0x17U);
    vDisp_TftWriteData8(0x14U);
    vDisp_TftWriteData8(0x15U);
    vDisp_TftWriteData8(0x31U);
    vDisp_TftWriteData8(0x34U);

    /* 开启显示反转 */
    vDisp_TftWriteCommand(ST7789_INVON);
    v_disp_delay_ms(10U);

    /* 开启正常显示模式 */
    vDisp_TftWriteCommand(ST7789_NORON);
    v_disp_delay_ms(10U);

    /* 开启显示 */
    vDisp_TftWriteCommand(ST7789_DISPON);
    v_disp_delay_ms(100U);

    /* 开启背光 */
    vDisp_TftSetBacklight(false);
}
#endif  //LV_USE_ST7789














/***********************************************************************************************************************
 -----函数功能    显示模块初始化函数
 -----说明(备注)  这是显示模块的主初始化函数，应优先调用
                 完成LVGL初始化、电源开启和首次刷新
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vDisp_Init(void)
{
    /* 非内置 ST7789 路径下由本地驱动完成面板初始化并清屏。 */
    #if(!LV_USE_ST7789)
    lcd_send_init_commands();
    #else
    /* 内置 ST7789 路径由 lv_port_disp_init() 完成面板初始化，首帧通过 LVGL 全屏刷新覆盖显存。 */
    #endif  //LV_USE_ST7789

    /* 开启显示屏电源 */
    v_disp_tft_set_power(true);

    /* 清屏 */
    vDisp_DrawFillRect(0, 0, dispTFT_WIDTH, dispTFT_HEIGHT, 0x10A3);

    /* 开启背光 */
    vDisp_TftSetBacklight(false);
}

/***********************************************************************************************************************
 -----函数功能    请求执行一次UI业务刷新
 -----说明(备注)  仅设置待处理标志并按需唤醒显示任务，LVGL真正刷新仍由显示任务统一执行
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vDisp_ReqUiRefresh(void)
{
    //当前任务不是显示任务则唤醒显示任务处理UI刷新请求
    // if((tDispTaskHandler != NULL) && (xTaskGetCurrentTaskHandle() != tDispTaskHandler))
    //     xTaskNotifyGive(tDispTaskHandler);
}

/***********************************************************************************************************************
 -----函数功能    执行挂起的UI业务刷新
 -----说明(备注)  仅当存在待处理UI请求时才调用ui_tick，避免工作页无效轮询
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vDisp_UiRefresh(void)
{
}


/***********************************************************************************************************************
-----函数功能    关闭UI界面
-----输入参数    us_color: 填充颜色
-----输出参数    none
-----返回值      none
-----日期        2026-05-28
************************************************************************************************************************/
void vDisp_CloseUi(u16 us_color)
{
    v_disp_fill_color(0, 0, dispTFT_WIDTH - 1, dispTFT_HEIGHT - 1, us_color);
}

/***********************************************************************************************************************
 -----函数功能    显示就绪状态检查函数
 -----说明(备注)  用于在调用其他显示函数前检查模块状态
 -----传入参数    none
 -----输出参数    none
 -----返回值      true:显示模块已初始化并就绪   false:显示模块未初始化
 ************************************************************************************************************************/
bool bDisp_IsReady(void)
{
    return true;
}



#include <math.h>

/* Standard 8x16 ASCII Font table */
static const uint8_t S_ucaAscii8x16[95][16] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 20 
    {0x00,0x00,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x00,0x00,0x0c,0x0c,0x00,0x00,0x00}, // 21 !
    {0x00,0x1e,0x1e,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 22 "
    {0x00,0x00,0x12,0x12,0x12,0x7f,0x24,0x24,0x7f,0x48,0x48,0x48,0x00,0x00,0x00,0x00}, // 23 #
    {0x00,0x08,0x1c,0x2a,0x0a,0x0a,0x1c,0x28,0x28,0x2a,0x1c,0x08,0x08,0x00,0x00,0x00}, // 24 $
    {0x00,0x00,0x63,0x63,0x32,0x18,0x0c,0x06,0x4c,0x63,0x63,0x00,0x00,0x00,0x00,0x00}, // 25 %
    {0x00,0x18,0x3c,0x24,0x24,0x1c,0x32,0x62,0x46,0x4c,0x39,0x00,0x00,0x00,0x00,0x00}, // 26 &
    {0x00,0x0c,0x0c,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 27 '
    {0x00,0x00,0x0c,0x18,0x30,0x30,0x30,0x30,0x30,0x30,0x18,0x0c,0x00,0x00,0x00,0x00}, // 28 (
    {0x00,0x00,0x30,0x18,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x18,0x30,0x00,0x00,0x00,0x00}, // 29 )
    {0x00,0x00,0x00,0x14,0x08,0x3e,0x08,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 2a *
    {0x00,0x00,0x00,0x0c,0x0c,0x0c,0x7e,0x0c,0x0c,0x0c,0x00,0x00,0x00,0x00,0x00,0x00}, // 2b +
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x0c,0x06,0x00,0x00,0x00}, // 2c ,
    {0x00,0x00,0x00,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 2d -
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x0c,0x00,0x00,0x00,0x00}, // 2e .
    {0x00,0x00,0x01,0x03,0x06,0x0c,0x18,0x30,0x60,0xc0,0x80,0x00,0x00,0x00,0x00,0x00}, // 2f /
    {0x00,0x00,0x3e,0x63,0x63,0x6b,0x7b,0x73,0x63,0x63,0x3e,0x00,0x00,0x00,0x00,0x00}, // 30 0
    {0x00,0x00,0x0c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3f,0x00,0x00,0x00,0x00,0x00}, // 31 1
    {0x00,0x00,0x3e,0x63,0x03,0x06,0x0c,0x18,0x30,0x60,0x7f,0x00,0x00,0x00,0x00,0x00}, // 32 2
    {0x00,0x00,0x3e,0x63,0x03,0x1e,0x03,0x03,0x63,0x63,0x3e,0x00,0x00,0x00,0x00,0x00}, // 33 3
    {0x00,0x00,0x06,0x0e,0x1e,0x36,0x66,0x7f,0x06,0x06,0x0f,0x00,0x00,0x00,0x00,0x00}, // 34 4
    {0x00,0x00,0x7f,0x60,0x60,0x7e,0x03,0x03,0x63,0x63,0x3e,0x00,0x00,0x00,0x00,0x00}, // 35 5
    {0x00,0x00,0x1c,0x30,0x60,0x7e,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,0x00,0x00}, // 36 6
    {0x00,0x00,0x7f,0x63,0x03,0x06,0x0c,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00}, // 37 7
    {0x00,0x00,0x3e,0x63,0x63,0x3e,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,0x00,0x00}, // 38 8
    {0x00,0x00,0x3e,0x63,0x63,0x63,0x3f,0x03,0x06,0x0c,0x38,0x00,0x00,0x00,0x00,0x00}, // 39 9
    {0x00,0x00,0x00,0x00,0x0c,0x0c,0x00,0x00,0x00,0x0c,0x0c,0x00,0x00,0x00,0x00,0x00}, // 3a :
    {0x00,0x00,0x00,0x00,0x0c,0x0c,0x00,0x00,0x00,0x0c,0x0c,0x06,0x00,0x00,0x00,0x00}, // 3b ;
    {0x00,0x00,0x02,0x06,0x0c,0x18,0x30,0x18,0x0c,0x06,0x02,0x00,0x00,0x00,0x00,0x00}, // 3c <
    {0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x3e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 3d =
    {0x00,0x00,0x40,0x60,0x30,0x18,0x0c,0x18,0x30,0x60,0x40,0x00,0x00,0x00,0x00,0x00}, // 3e >
    {0x00,0x00,0x3e,0x63,0x63,0x06,0x0c,0x0c,0x00,0x00,0x0c,0x0c,0x00,0x00,0x00,0x00}, // 3f ?
    {0x00,0x00,0x3e,0x63,0x6f,0x6b,0x6b,0x6e,0x60,0x60,0x3e,0x00,0x00,0x00,0x00,0x00}, // 40 @
    {0x00,0x00,0x0c,0x1e,0x36,0x36,0x63,0x7f,0x63,0x63,0x63,0x00,0x00,0x00,0x00,0x00}, // 41 A
    {0x00,0x00,0x7e,0x63,0x63,0x7e,0x63,0x63,0x63,0x63,0x7e,0x00,0x00,0x00,0x00,0x00}, // 42 B
    {0x00,0x00,0x3e,0x63,0x60,0x60,0x60,0x60,0x60,0x63,0x3e,0x00,0x00,0x00,0x00,0x00}, // 43 C
    {0x00,0x00,0x7c,0x66,0x63,0x63,0x63,0x63,0x63,0x66,0x7c,0x00,0x00,0x00,0x00,0x00}, // 44 D
    {0x00,0x00,0x7f,0x60,0x60,0x78,0x60,0x60,0x60,0x60,0x7f,0x00,0x00,0x00,0x00,0x00}, // 45 E
    {0x00,0x00,0x7f,0x60,0x60,0x78,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00}, // 46 F
    {0x00,0x00,0x3e,0x63,0x60,0x60,0x6f,0x63,0x63,0x63,0x3f,0x00,0x00,0x00,0x00,0x00}, // 47 G
    {0x00,0x00,0x63,0x63,0x63,0x7f,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,0x00,0x00}, // 48 H
    {0x00,0x00,0x3f,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3f,0x00,0x00,0x00,0x00,0x00}, // 49 I
    {0x00,0x00,0x1f,0x06,0x06,0x06,0x06,0x06,0x06,0x66,0x3c,0x00,0x00,0x00,0x00,0x00}, // 4a J
    {0x00,0x00,0x63,0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x63,0x00,0x00,0x00,0x00,0x00}, // 4b K
    {0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0x00,0x00,0x00,0x00,0x00}, // 4c L
    {0x00,0x00,0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,0x00,0x00}, // 4d M
    {0x00,0x00,0x63,0x63,0x73,0x7b,0x6f,0x67,0x63,0x63,0x63,0x00,0x00,0x00,0x00,0x00}, // 4e N
    {0x00,0x00,0x3e,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,0x00,0x00}, // 5f O
    {0x00,0x00,0x7e,0x63,0x63,0x7e,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00}, // 50 P
    {0x00,0x00,0x3e,0x63,0x63,0x63,0x63,0x6b,0x6b,0x3e,0x0f,0x07,0x00,0x00,0x00,0x00}, // 51 Q
    {0x00,0x00,0x7e,0x63,0x63,0x7e,0x6c,0x66,0x63,0x63,0x63,0x00,0x00,0x00,0x00,0x00}, // 52 R
    {0x00,0x00,0x3e,0x63,0x60,0x3c,0x03,0x03,0x03,0x63,0x3e,0x00,0x00,0x00,0x00,0x00}, // 53 S
    {0x00,0x00,0x7f,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x00,0x00,0x00,0x00,0x00}, // 54 T
    {0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x3e,0x00,0x00,0x00,0x00,0x00}, // 55 U
    {0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x36,0x36,0x1c,0x08,0x00,0x00,0x00,0x00,0x00}, // 56 V
    {0x00,0x00,0x63,0x63,0x63,0x6b,0x6b,0x7f,0x36,0x36,0x22,0x00,0x00,0x00,0x00,0x00}, // 57 W
    {0x00,0x00,0x63,0x63,0x36,0x1c,0x08,0x1c,0x36,0x63,0x63,0x00,0x00,0x00,0x00,0x00}, // 58 X
    {0x00,0x00,0x63,0x63,0x63,0x36,0x1c,0x08,0x08,0x08,0x08,0x00,0x00,0x00,0x00,0x00}, // 59 Y
    {0x00,0x00,0x7f,0x63,0x06,0x0c,0x18,0x30,0x60,0x63,0x7f,0x00,0x00,0x00,0x00,0x00}, // 5a Z
    {0x00,0x00,0x3f,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3f,0x00,0x00,0x00,0x00,0x00}, // 5b [
    {0x00,0x00,0x80,0xc0,0x60,0x30,0x18,0x0c,0x06,0x03,0x01,0x00,0x00,0x00,0x00,0x00}, // 5c backslash
    {0x00,0x00,0xfc,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0xfc,0x00,0x00,0x00,0x00,0x00}, // 5d ]
    {0x00,0x08,0x1c,0x36,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 5e ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00}, // 5f _
    {0x00,0x18,0x18,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 60 `
    {0x00,0x00,0x00,0x00,0x00,0x3c,0x46,0x06,0x3e,0x66,0x3f,0x00,0x00,0x00,0x00,0x00}, // 61 a
    {0x00,0x00,0x60,0x60,0x60,0x7c,0x66,0x66,0x66,0x66,0x7c,0x00,0x00,0x00,0x00,0x00}, // 62 b
    {0x00,0x00,0x00,0x00,0x00,0x3e,0x62,0x60,0x60,0x62,0x3e,0x00,0x00,0x00,0x00,0x00}, // 63 c
    {0x00,0x00,0x06,0x06,0x06,0x3e,0x66,0x66,0x66,0x66,0x3f,0x00,0x00,0x00,0x00,0x00}, // 64 d
    {0x00,0x00,0x00,0x00,0x00,0x3e,0x66,0x7e,0x60,0x62,0x3e,0x00,0x00,0x00,0x00,0x00}, // 65 e
    {0x00,0x00,0x1e,0x30,0x30,0x78,0x30,0x30,0x30,0x30,0x30,0x00,0x00,0x00,0x00,0x00}, // 66 f
    {0x00,0x00,0x00,0x00,0x00,0x3f,0x66,0x66,0x66,0x3e,0x06,0x66,0x3c,0x00,0x00,0x00}, // 67 g
    {0x00,0x00,0x60,0x60,0x60,0x7c,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00}, // 68 h
    {0x00,0x00,0x0c,0x0c,0x00,0x0c,0x0c,0x0c,0x0c,0x0c,0x3f,0x00,0x00,0x00,0x00,0x00}, // 69 i
    {0x00,0x00,0x03,0x03,0x00,0x03,0x03,0x03,0x03,0x03,0x43,0x63,0x3c,0x00,0x00,0x00}, // 6a j
    {0x00,0x00,0x60,0x60,0x60,0x66,0x6c,0x78,0x6c,0x66,0x63,0x00,0x00,0x00,0x00,0x00}, // 6b k
    {0x00,0x00,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3f,0x00,0x00,0x00,0x00,0x00}, // 6c l
    {0x00,0x00,0x00,0x00,0x00,0x66,0x7f,0x6b,0x6b,0x63,0x63,0x00,0x00,0x00,0x00,0x00}, // 6d m
    {0x00,0x00,0x00,0x00,0x00,0x7c,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00}, // 6e n
    {0x00,0x00,0x00,0x00,0x00,0x3e,0x66,0x66,0x66,0x66,0x3e,0x00,0x00,0x00,0x00,0x00}, // 6f o
    {0x00,0x00,0x00,0x00,0x00,0x7c,0x66,0x66,0x66,0x7c,0x60,0x60,0xf0,0x00,0x00,0x00}, // 70 p
    {0x00,0x00,0x00,0x00,0x00,0x3e,0x66,0x66,0x66,0x3e,0x06,0x06,0x0f,0x00,0x00,0x00}, // 71 q
    {0x00,0x00,0x00,0x00,0x00,0x5e,0x30,0x30,0x30,0x30,0x30,0x00,0x00,0x00,0x00,0x00}, // 72 r
    {0x00,0x00,0x00,0x00,0x00,0x3e,0x60,0x3c,0x06,0x06,0x3c,0x00,0x00,0x00,0x00,0x00}, // 73 s
    {0x00,0x00,0x30,0x30,0x7c,0x30,0x30,0x30,0x30,0x30,0x1c,0x00,0x00,0x00,0x00,0x00}, // 74 t
    {0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x3f,0x00,0x00,0x00,0x00,0x00}, // 75 u
    {0x00,0x00,0x00,0x00,0x00,0x63,0x63,0x36,0x36,0x1c,0x08,0x00,0x00,0x00,0x00,0x00}, // 76 v
    {0x00,0x00,0x00,0x00,0x00,0x63,0x6b,0x6b,0x7f,0x36,0x22,0x00,0x00,0x00,0x00,0x00}, // 77 w
    {0x00,0x00,0x00,0x00,0x00,0x63,0x36,0x1c,0x1c,0x36,0x63,0x00,0x00,0x00,0x00,0x00}, // 78 x
    {0x00,0x00,0x00,0x00,0x00,0x63,0x63,0x36,0x1c,0x08,0x08,0x30,0x60,0xc0,0x00,0x00}, // 79 y
    {0x00,0x00,0x00,0x00,0x00,0x7f,0x46,0x0c,0x18,0x32,0x7f,0x00,0x00,0x00,0x00,0x00}, // 7a z
    {0x00,0x00,0x0e,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x0e,0x00,0x00,0x00,0x00,0x00}, // 7b {
    {0x00,0x00,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x00,0x00,0x00,0x00,0x00}, // 7c |
    {0x00,0x00,0x70,0x18,0x18,0x18,0x0e,0x18,0x18,0x18,0x70,0x00,0x00,0x00,0x00,0x00}, // 7d }
    {0x00,0x3b,0x6e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}  // 7e ~
};

/* Helper function to swap bytes for ST7789 */
static inline uint16_t us_swap_color(uint16_t color)
{
    return (color >> 8) | (color << 8);
}

/***********************************************************************************************************************
 * 函数功能    : 填充矩形区域
 * 说明(备注)  : none
 * 传入参数    : x, y: 起始坐标; w, h: 宽高; color: 颜色值
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_DrawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0 || h == 0) return;
    
    if (x >= dispTFT_WIDTH) return;
    if (y >= dispTFT_HEIGHT) return;
    if (x + w > dispTFT_WIDTH) w = dispTFT_WIDTH - x;
    if (y + h > dispTFT_HEIGHT) h = dispTFT_HEIGHT - y;
    
    v_disp_set_window(x, y, x + w - 1, y + h - 1);
    
    uint16_t swapped = us_swap_color(color);
    uint16_t row_buf[320];
    
    for (uint16_t i = 0; i < w; i++)
    {
        row_buf[i] = swapped;
    }
    
    for (uint16_t j = 0; j < h; j++)
    {
        vDisp_TftWriteBuffer((uint8_t *)row_buf, w * 2);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 绘制文本
 * 说明(备注)  : 按字符绘制8x16大小的文本，支持等比例缩放
 * 传入参数    : x, y: 起始坐标; str: 文本字符串; color: 文本颜色; bg_color: 背景颜色; scale: 缩放倍数
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_DrawText(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t scale)
{
    if (str == NULL || scale == 0) return;
    
    uint16_t len = strlen(str);
    uint16_t w = len * 8 * scale;
    uint16_t h = 16 * scale;
    
    if (x >= dispTFT_WIDTH || y >= dispTFT_HEIGHT) return;
    if (x + w > dispTFT_WIDTH) w = dispTFT_WIDTH - x;
    if (y + h > dispTFT_HEIGHT) h = dispTFT_HEIGHT - y;
    
    v_disp_set_window(x, y, x + w - 1, y + h - 1);
    
    uint16_t color_swapped = us_swap_color(color);
    uint16_t bg_color_swapped = us_swap_color(bg_color);
    uint16_t row_buf[320];
    
    for (uint16_t r = 0; r < h; r++)
    {
        uint16_t font_row = r / scale;
        uint16_t buf_idx = 0;
        
        for (uint16_t c_idx = 0; c_idx < len; c_idx++)
        {
            char c = str[c_idx];
            uint8_t font_byte = 0;
            if (c >= 32 && c <= 126)
            {
                font_byte = S_ucaAscii8x16[c - 32][font_row];
            }
            
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                uint16_t pixel_color = (font_byte & (0x80 >> bit)) ? color_swapped : bg_color_swapped;
                for (uint8_t s = 0; s < scale; s++)
                {
                    if (buf_idx < 320)
                    {
                        row_buf[buf_idx++] = pixel_color;
                    }
                }
            }
        }
        vDisp_TftWriteBuffer((uint8_t *)row_buf, w * 2);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 绘制环形进度条
 * 说明(备注)  : 使用atan2f与距离运算画圆环以及弧度代表的进度值
 * 传入参数    : x0, y0: 圆心坐标; r: 半径; thickness: 线宽; progress: 进度(0-100)
 *               active_color: 激活状态颜色; inactive_color: 待命状态颜色; bg_color: 背景色
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_DrawProgressCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t thickness, uint8_t progress, uint16_t active_color, uint16_t inactive_color, uint16_t bg_color)
{
    uint16_t r_outer = r + thickness / 2;
    uint16_t r_inner = r - thickness / 2;
    uint16_t w = r_outer * 2 + 1;
    uint16_t h = w;
    
    int16_t x1 = x0 - r_outer;
    int16_t y1 = y0 - r_outer;
    
    if (x1 >= dispTFT_WIDTH || y1 >= dispTFT_HEIGHT) return;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x1 + w > dispTFT_WIDTH) w = dispTFT_WIDTH - x1;
    if (y1 + h > dispTFT_HEIGHT) h = dispTFT_HEIGHT - y1;
    
    v_disp_set_window(x1, y1, x1 + w - 1, y1 + h - 1);
    
    uint16_t active_swapped = us_swap_color(active_color);
    uint16_t inactive_swapped = us_swap_color(inactive_color);
    uint16_t bg_swapped = us_swap_color(bg_color);
    uint16_t row_buf[320];
    
    uint32_t r_in_sq = r_inner * r_inner;
    uint32_t r_out_sq = r_outer * r_outer;
    
    #ifndef M_PI
    #define M_PI 3.14159265f
    #endif
    
    for (uint16_t y = y1; y < y1 + h; y++)
    {
        int16_t dy = y - y0;
        uint32_t dy_sq = dy * dy;
        uint16_t buf_idx = 0;
        
        for (uint16_t x = x1; x < x1 + w; x++)
        {
            int16_t dx = x - x0;
            uint32_t d2 = dx * dx + dy_sq;
            
            if (d2 >= r_in_sq && d2 <= r_out_sq)
            {
                float angle_rad = atan2f((float)dx, (float)(-dy));
                float angle_deg = angle_rad * 180.0f / M_PI;
                if (angle_deg < 0.0f)
                {
                    angle_deg += 360.0f;
                }
                
                if (angle_deg < (progress * 3.6f))
                {
                    row_buf[buf_idx++] = active_swapped;
                }
                else
                {
                    row_buf[buf_idx++] = inactive_swapped;
                }
            }
            else
            {
                row_buf[buf_idx++] = bg_swapped;
            }
        }
        vDisp_TftWriteBuffer((uint8_t *)row_buf, w * 2);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 绘制胶囊状进度条
 * 说明(备注)  : none
 * 传入参数    : x, y: 进度条坐标; w, h: 宽高; progress: 进度(0-100)
 *               active_color: 激活状态颜色; inactive_color: 边框颜色; bg_color: 背景色
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_DrawPillProgress(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t progress, uint16_t active_color, uint16_t inactive_color, uint16_t bg_color)
{
    if (x >= dispTFT_WIDTH || y >= dispTFT_HEIGHT) return;
    if (x + w > dispTFT_WIDTH) w = dispTFT_WIDTH - x;
    if (y + h > dispTFT_HEIGHT) h = dispTFT_HEIGHT - y;
    
    v_disp_set_window(x, y, x + w - 1, y + h - 1);
    
    uint16_t active_swapped = us_swap_color(active_color);
    uint16_t inactive_swapped = us_swap_color(inactive_color);
    uint16_t bg_swapped = us_swap_color(bg_color);
    uint16_t row_buf[320];
    
    uint16_t r = h / 2;
    uint16_t active_w = (w * progress) / 100;
    
    for (uint16_t j = 0; j < h; j++)
    {
        int16_t dy = j - r;
        uint32_t dy_sq = dy * dy;
        uint16_t buf_idx = 0;
        
        for (uint16_t i = 0; i < w; i++)
        {
            bool is_inside = false;
            if (i < r)
            {
                int16_t dx = i - r;
                if ((uint32_t)(dx * dx + dy_sq) <= (uint32_t)(r * r))
                {
                    is_inside = true;
                }
            }
            else if (i >= w - r)
            {
                int16_t dx = i - (w - r - 1);
                if ((uint32_t)(dx * dx + dy_sq) <= (uint32_t)(r * r))
                {
                    is_inside = true;
                }
            }
            else
            {
                is_inside = true;
            }
            
            if (is_inside)
            {
                bool is_border = false;
                if (j == 0 || j == h - 1)
                {
                    is_border = true;
                }
                else if (i < r)
                {
                    int16_t dx = i - r;
                    uint32_t d2 = dx * dx + dy_sq;
                    if (d2 >= (r-1)*(r-1) && d2 <= r*r)
                    {
                        is_border = true;
                    }
                }
                else if (i >= w - r)
                {
                    int16_t dx = i - (w - r - 1);
                    uint32_t d2 = dx * dx + dy_sq;
                    if (d2 >= (r-1)*(r-1) && d2 <= r*r)
                    {
                        is_border = true;
                    }
                }
                
                if (is_border)
                {
                    row_buf[buf_idx++] = inactive_swapped;
                }
                else
                {
                    if (i < active_w)
                    {
                        row_buf[buf_idx++] = active_swapped;
                    }
                    else
                    {
                        row_buf[buf_idx++] = bg_swapped;
                    }
                }
            }
            else
            {
                row_buf[buf_idx++] = bg_swapped;
            }
        }
        vDisp_TftWriteBuffer((uint8_t *)row_buf, w * 2);
    }
}

/* ====== 分段式圆环辅助函数 ====== */

/* 整数开方（二分法，适用于嵌入式） */
static uint16_t us_isqrt(uint32_t n)
{
    if (n == 0) return 0;
    uint16_t lo = 0, hi = 255;
    while (lo < hi)
    {
        uint16_t mid = (lo + hi + 1) >> 1;
        if ((uint32_t)mid * mid <= n)
            lo = mid;
        else
            hi = mid - 1;
    }
    return lo;
}

/* sin 查表 0-90 度，Q15 格式（32767 = 1.0），参考 LVGL lv_trigo_sin 实现 */
static const int16_t s_sin_tbl[91] = {
    0, 572, 1144, 1715, 2286, 2856, 3425, 3993, 4560, 5126,
    5690, 6252, 6813, 7371, 7927, 8480, 9032, 9580, 10126, 10668,
    11207, 11743, 12275, 12803, 13328, 13848, 14365, 14876, 15384, 15886,
    16384, 16877, 17364, 17847, 18323, 18795, 19261, 19720, 20174, 20622,
    21063, 21498, 21926, 22348, 22762, 23170, 23571, 23965, 24351, 24730,
    25101, 25465, 25822, 26170, 26510, 26842, 27166, 27482, 27789, 28088,
    28377, 28659, 28931, 29194, 29448, 29692, 29927, 30152, 30368, 30574,
    30770, 30956, 31132, 31298, 31454, 31599, 31734, 31859, 31974, 32078,
    32271, 32366, 32450, 32525, 32587, 32642, 32687, 32722, 32747, 32762,
    32767
};

/* 整数 sin（Q15 格式），角度 0-359 度，12点钟方向为0度顺时针 */
static int16_t isin_q15(uint16_t deg)
{
    deg = deg % 360;
    if (deg <= 90)  return s_sin_tbl[deg];
    if (deg <= 180) return s_sin_tbl[180 - deg];
    if (deg <= 270) return -s_sin_tbl[deg - 180];
    return -s_sin_tbl[360 - deg];
}

/* 整数 cos（Q15 格式），角度 0-359 度 */
static int16_t icos_q15(uint16_t deg)
{
    return isin_q15((deg + 90) % 360);
}

/* 段边界信息（Q15 sin/cos 预计算值） */
typedef struct {
    int16_t sin_val;    /* 边界角度 sin（Q15） */
    int16_t cos_val;    /* 边界角度 cos（Q15） */
} SegBound_T;

/***********************************************************************************************************************
 * 函数功能    : RGB565 颜色混合函数
 * 说明(备注)  : 5-bit alpha (0~32) 实现高效颜色混合
 ************************************************************************************************************************/
static uint16_t us_blend_color(uint16_t color1, uint16_t color2, uint8_t alpha)
{
    if (alpha == 0) return color2;
    if (alpha >= 32) return color1;

    uint32_t r1 = (color1 >> 11) & 0x1F;
    uint32_t g1 = (color1 >> 5) & 0x3F;
    uint32_t b1 = color1 & 0x1F;

    uint32_t r2 = (color2 >> 11) & 0x1F;
    uint32_t g2 = (color2 >> 5) & 0x3F;
    uint32_t b2 = color2 & 0x1F;

    uint32_t r = (r1 * alpha + r2 * (32 - alpha)) >> 5;
    uint32_t g = (g1 * alpha + g2 * (32 - alpha)) >> 5;
    uint32_t b = (b1 * alpha + b2 * (32 - alpha)) >> 5;

    return (uint16_t)((r << 11) | (g << 5) | b);
}

/***********************************************************************************************************************
 * 函数功能    : 绘制分段式圆环（仅环形像素，不填充内部）
 * 说明(备注)  : 参考 energy_ring.c 设计理念优化：
 *               1. 三档颜色（off/on/head）实现视觉层次，头部段高亮
 *               2. 段间间隙实现分段视觉效果
 *               3. sin/cos 查表 + 叉积法精确分类像素所属段，替代正切查表
 *               4. 仅绘制环形像素，不填充圆环内部，减少 SPI 传输量
 *               角度系统：12点钟方向为0度，顺时针增加
 * 传入参数    : cx, cy: 圆心坐标; r: 半径; thickness: 线宽
 *               lit_segs: 已点亮段数; total_segs: 总段数; gap_angle: 段间间隙角度
 *               active_color: 已点亮段颜色; head_color: 头部段高亮颜色; inactive_color: 未点亮段颜色
 *               bg_color: 间隙区域颜色（背景色，形成可见切口）
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_DrawSegmentedRing(uint16_t cx, uint16_t cy, uint16_t r, uint8_t thickness,
                             uint8_t lit_segs, uint8_t total_segs, uint8_t gap_angle,
                             uint16_t active_color, uint16_t head_color, uint16_t inactive_color,
                             uint16_t bg_color)
{
    if (thickness == 0 || total_segs == 0 || r == 0) return;

    /* 参数范围限制 */
    if (lit_segs > total_segs) lit_segs = total_segs;
    uint16_t max_step = 360 / total_segs;
    if (gap_angle >= max_step) gap_angle = 0;

    uint16_t r_outer = r + thickness / 2;
    uint16_t r_inner = (thickness / 2 < r) ? (r - thickness / 2) : 0;
    uint32_t r_in_sq = (uint32_t)r_inner * r_inner;
    uint32_t r_out_sq = (uint32_t)r_outer * r_outer;

    /* 预计算段角度参数 */
    uint16_t gap_total = (total_segs > 1) ? (uint16_t)(total_segs - 1) * gap_angle : 0;
    uint16_t seg_span = (360 - gap_total) / total_segs;
    if (seg_span == 0) seg_span = 1;
    uint16_t seg_step = seg_span + gap_angle;

    /* 预计算每段起止边界的 sin/cos（Q15），避免运行时重复查表 */
    SegBound_T bound_start[20];
    SegBound_T bound_end[20];
    uint8_t max_segs = (total_segs <= 20) ? total_segs : 20;

    for (uint8_t i = 0; i < max_segs; i++)
    {
        uint16_t start_ang = (uint16_t)(i * seg_step);
        uint16_t end_ang = (uint16_t)(start_ang + seg_span);
        bound_start[i].sin_val = isin_q15(start_ang);
        bound_start[i].cos_val = icos_q15(start_ang);
        bound_end[i].sin_val = isin_q15(end_ang);
        bound_end[i].cos_val = icos_q15(end_ang);
    }

    uint16_t active_swapped = us_swap_color(active_color);
    uint16_t head_swapped = us_swap_color(head_color);
    uint16_t inactive_swapped = us_swap_color(inactive_color);
    uint16_t bg_swapped = us_swap_color(bg_color);
    uint16_t row_buf[60];  /* 最大弧段宽度约 r_outer ≈ 48，60足够 */

    int16_t y_start = (int16_t)cy - (int16_t)r_outer;
    int16_t y_end = (int16_t)cy + (int16_t)r_outer;

    if (y_start < 0) y_start = 0;
    if (y_end >= dispTFT_HEIGHT) y_end = dispTFT_HEIGHT - 1;

    for (int16_t y = y_start; y <= y_end; y++)
    {
        int16_t dy = y - (int16_t)cy;
        int32_t dy_s32 = (int32_t)dy;
        uint32_t dy_sq = (uint32_t)(dy_s32 * dy_s32);

        if (dy_sq > r_out_sq)
            continue;

        uint16_t x_outer = us_isqrt(r_out_sq - dy_sq);
        uint16_t x_inner = (dy_sq < r_in_sq) ? us_isqrt(r_in_sq - dy_sq) : 0;

        if (x_outer <= x_inner)
            continue;

        /* 右侧弧段和左侧弧段 */
        int16_t arc_xs[2] = { (int16_t)cx + (int16_t)x_inner, (int16_t)cx - (int16_t)x_outer };
        int16_t arc_xe[2] = { (int16_t)cx + (int16_t)x_outer, (int16_t)cx - (int16_t)x_inner };

        for (uint8_t side = 0; side < 2; side++)
        {
            int16_t xs = arc_xs[side];
            int16_t xe = arc_xe[side];
            if (xs < 0) xs = 0;
            if (xe >= dispTFT_WIDTH) xe = dispTFT_WIDTH - 1;

            if (xs > xe)
                continue;

            uint16_t width = (uint16_t)(xe - xs + 1);
            v_disp_set_window((u16)xs, (u16)y, (u16)xe, (u16)y);

            for (uint16_t i = 0; i < width; i++)
            {
                int16_t x = xs + (int16_t)i;
                int16_t dx = x - (int16_t)cx;

                /* 叉积法查找像素所属段并进行亚像素抗锯齿混合 */
                uint8_t max_alpha = 0;
                uint16_t target_color = bg_color;

                for (uint8_t seg = 0; seg < max_segs; seg++)
                {
                    int32_t cross_start = (int32_t)bound_start[seg].sin_val * dy
                                        + (int32_t)bound_start[seg].cos_val * dx;
                    int32_t cross_end = (int32_t)bound_end[seg].sin_val * dy
                                      + (int32_t)bound_end[seg].cos_val * dx;

                    /* 边缘过渡带宽设为1像素 (Q15 格式中 1.0 = 32768, -0.5px 到 0.5px 为 [-16384, 16384]) */
                    int16_t alpha_start = 32;
                    if (cross_start < -16384) {
                        alpha_start = 0;
                    } else if (cross_start < 16384) {
                        alpha_start = (int16_t)((cross_start + 16384) >> 10);
                    }

                    int16_t alpha_end = 32;
                    if (cross_end > 16384) {
                        alpha_end = 0;
                    } else if (cross_end > -16384) {
                        alpha_end = (int16_t)((16384 - cross_end) >> 10);
                    }

                    uint8_t alpha_seg = (uint8_t)((alpha_start < alpha_end) ? alpha_start : alpha_end);

                    if (alpha_seg > max_alpha)
                    {
                        max_alpha = alpha_seg;
                        if (lit_segs > 0 && seg < lit_segs - 1)
                            target_color = active_color;
                        else if (lit_segs > 0 && seg == lit_segs - 1)
                            target_color = head_color;
                        else
                            target_color = inactive_color;
                    }
                }

                uint16_t blended = us_blend_color(target_color, bg_color, max_alpha);
                row_buf[i] = us_swap_color(blended);
            }
            vDisp_TftWriteBuffer((uint8_t *)row_buf, width * 2);
        }
    }
}

#endif  /*boardDISPLAY_EN*/
