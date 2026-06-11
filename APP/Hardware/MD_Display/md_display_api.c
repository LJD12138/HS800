/*****************************************************************************************************************
 *                                                                                                                *
 *                                         Display API - TFT + LVGL                                              *
 *                                                                                                                *
 ******************************************************************************************************************/
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
    vDisp_TftWriteData8(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_BGR);

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
    lv_obj_t *active_screen;

    /* 初始化LVGL核心库（如果尚未初始化） */
    if(!lv_is_initialized())
        lv_init();

    /* 初始化显示驱动（如果尚未初始化） */
    if(lv_display_get_default() == NULL)
        lv_port_disp_init();

    /* 非内置 ST7789 路径下由本地驱动完成面板初始化并清屏。 */
    #if(!LV_USE_ST7789)
    lcd_send_init_commands();
    vDisp_FastDrawColor(0U, 0U, dispTFT_WIDTH, dispTFT_HEIGHT, 0x0000U);
    #else
    /* 内置 ST7789 路径由 lv_port_disp_init() 完成面板初始化，首帧通过 LVGL 全屏刷新覆盖显存。 */
    #endif  //LV_USE_ST7789

    // ui_init();

    active_screen = lv_screen_active();
    if(active_screen != NULL)
        lv_obj_invalidate(active_screen);

    /* 开启显示屏电源 */
    v_disp_tft_set_power(true);

    /* 关闭背光 */
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
    if(!lv_is_initialized())
        return;

    ui_tick();
    
    vImgAnim_ManualTick();

    lv_timer_handler();
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
    return lv_is_initialized();
}

/***********************************************************************************************************************
-----函数功能    填充矩形区域
-----说明(备注)  用指定颜色填充矩形区域
-----传入参数    x:起始X坐标  y:起始Y坐标  w:宽度  h:高度  color:RGB565颜色
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if(!LV_USE_ST7789)
void vDisp_FastDrawColor(u16 x, u16 y, u16 w, u16 h, u16 *color)
{
    uint32_t draw_size = (w - x + 1) * (h - y  + 1);

    //设置需要画的窗口大小
    v_disp_set_window(x, y, w, h);
    
    //批量发送颜色数据，底层接口会根据配置选择DMA或软件SPI发送
    vDisp_TftWriteBuffer((u8 *)color, draw_size * 2);

}
#endif  //LV_USE_ST7789

#endif  /*boardDISPLAY_EN*/
