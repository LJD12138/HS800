/***********************************************************************************************************************
 -----文件说明    LVGL显示端口适配层
 -----说明(备注)  将LVGL显示刷新回调与底层TFT控制器绑定，支持FreeRTOS信号量保护和双缓冲渲染模式
 -----文件版本    V1.0
 -----作者        HS800开发团队
 -----日期        2024
 ************************************************************************************************************************/
#include "lv_port_disp.h"

#if(boardDISPLAY_EN)

/* 包含 LVGL 定时器、底层显示驱动接口及布尔类型 */
#include "lv_port_timer.h"
#include "MD_Display/md_display_api.h"
#include <stdbool.h>
#include "Middlewares/LVGL/src/misc/lv_color.h"

/* LVGL 内置 ST7789 驱动支持 */
#if(LV_USE_ST7796)
#include "Middlewares/LVGL/src/drivers/display/st7789/lv_st7789.h"
#include "Middlewares/LVGL/src/drivers/display/lcd/lv_lcd_generic_mipi.h"
#endif

#if(boardUSE_OS)
/* FreeRTOS 相关头文件：用于创建/使用二值信号量保护显示访问 */
#include "freertos.h"
#include "task.h"
#include "semphr.h"

/* 显示访问的二值信号量（用于串行化对 TFT 的访问） */
SemaphoreHandle_t DispSemaphoreBinary = NULL;
#endif  //boardUSE_OS

/*********************
 *      配置与宏
 *********************/
/* 渲染缓冲区按行数分配：每次交由 LVGL 提交的刷新行数 */
#define DISP_DRAW_BUF_LINE_COUNT            16U

/**********************
 *  静态函数原型（模块内部）
 **********************/
static void disp_init(void); /* 显示控制器及同步资源初始化 */
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map); /* LVGL 刷新回调 */
static bool b_disp_flush_area_is_valid(const lv_area_t * area); /* 刷新区域合法性检查 */

/* LVGL 内置 ST7789 驱动回调函数 */
#if(LV_USE_ST7796)
static void st7789_send_cmd(lv_lcd_send_cmd_cb_t cmd, const uint8_t *param, uint32_t param_len);
static void st7789_send_color(lv_lcd_send_color_cb_t cmd, const uint8_t *param, uint32_t param_len);
#endif

/**********************
 *  静态变量
 **********************/
/* 全局使能标志：在无 RTOS 时用于控制是否实际更新屏幕 */
static volatile bool s_bDispFlushEnabled = true;

/**********************
 *   全局函数（供外部调用）
 **********************/

/***********************************************************************************************************************
 -----函数功能    LVGL显示端口初始化
 -----说明(备注)  配置LVGL显示对象、分配双缓冲区并设置刷新回调，使用对齐缓冲区以满足DMA/控制器要求
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void lv_port_disp_init(void)
{
	lv_display_t * disp;

	/* 按硬件分辨率与行数分配两块对齐缓存（双缓冲），提高渲染效率 */
    __align(4) static lv_color_t buf_1[DISP_HOR_RES * DISP_DRAW_BUF_LINE_COUNT];
    __align(4) static lv_color_t buf_2[DISP_HOR_RES * DISP_DRAW_BUF_LINE_COUNT];

	/* 初始化底层显示控制器与同步对象 */
    disp_init();

	#if(LV_USE_ST7796) /* 使用 LVGL 内置 ST7789 驱动 */
	
    /* 创建 ST7789 LCD 驱动 */
    disp = lv_st7789_create(DISP_HOR_RES, DISP_VER_RES, 0, st7789_send_cmd, st7789_send_color);

    /* 配置双缓冲区和渲染模式 */
    lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

	#else /* 使用自定义 ST7789 驱动 */


    /* 创建 LVGL 显示驱动并绑定回调与缓冲区 */
    disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	#endif
}

/**********************
 *   静态函数实现
 **********************/

#if(LV_USE_ST7796)
/**
 * @brief  ST7789 发送命令回调函数
 * @param  cmd: 命令值
 * @param  param: 参数数据
 * @param  param_len: 参数长度
 */
static void st7789_send_cmd(lv_lcd_send_cmd_cb_t cmd, const uint8_t *param, uint32_t param_len)
{
    /* 发送命令 */
    vDisp_TftWriteCommand((u8)cmd);

    /* 发送参数 */
    for(uint32_t i = 0; i < param_len; i++) {
        vDisp_TftWriteData8(param[i]);
    }
}

/**
 * @brief  ST7789 发送颜色数据回调函数
 * @param  cmd: 命令值（通常是 RAMWR 0x2C）
 * @param  param: 像素数据
 * @param  param_len: 数据长度（字节数）
 */
static void st7789_send_color(lv_lcd_send_color_cb_t cmd, const uint8_t *param, uint32_t param_len)
{
    /* 发送内存写入命令 */
    vDisp_TftWriteCommand((u8)cmd);

    /* 使用现有接口批量发送颜色数据 */
    /* 注意：这里需要根据实际硬件接口调整，假设 vDisp_TftWriteData8 可以循环调用 */
    uint16_t pixel_count = param_len / sizeof(lv_color_t);
    const lv_color_t *pixels = (const lv_color_t *)param;

    /* 对于大块数据，建议使用现有的批量传输函数 */
    /* 这里使用逐像素方式，可根据需要优化为 DMA 传输 */
    for(uint32_t i = 0; i < pixel_count; i++) {
        u16 color = lv_color_to_u16(pixels[i]);
        vDisp_TftWriteData8((u8)(color >> 8));   /* 高字节 */
        vDisp_TftWriteData8((u8)(color & 0xFF)); /* 低字节 */
    }
}
#endif

/***********************************************************************************************************************
 -----函数功能    显示控制器初始化
 -----说明(备注)  初始化显示控制器，并在启用RTOS时创建并初始化二值信号量
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void disp_init(void)
{
	#if(boardUSE_OS)
    /* 若还未创建信号量，则创建并立即释放以使其可用 */
    if(DispSemaphoreBinary == NULL)
    {
        DispSemaphoreBinary = xSemaphoreCreateBinary();
        if(DispSemaphoreBinary != NULL)
            xSemaphoreGive(DispSemaphoreBinary);
    }
	#endif
    /* 调用底层控制器初始化接口（由 md_display_api 提供） */
    vDisp_TftInitController();
}

/***********************************************************************************************************************
 -----函数功能    刷新区域有效性检查
 -----说明(备注)  检查LVGL传入的刷新区域是否有效，防止越界或空区域操作
 -----传入参数    area: LVGL区域结构体指针
 -----输出参数    none
 -----返回值      true:区域有效  false:区域无效
 ************************************************************************************************************************/
static bool b_disp_flush_area_is_valid(const lv_area_t * area)
{
    if(area == NULL)
        return false;

    /* 坐标范围检查：x1/x2 与 y1/y2 的有效性 */
    if((area->x1 > area->x2) || (area->y1 > area->y2))
        return false;

    /* 是否超出屏幕右下边界（左上点越界亦视为非法） */
    if((area->x1 >= (lv_coord_t)DISP_HOR_RES) || (area->y1 >= (lv_coord_t)DISP_VER_RES))
        return false;

    return true;
}

/***********************************************************************************************************************
 -----函数功能    使能显示刷新
 -----说明(备注)  在RTOS下从任务或中断唤醒显示任务（释放信号量）；无RTOS时直接设置全局使能标志为true
 -----传入参数    index: 0-任务上下文 1-中断上下文(使用FromISR版本)
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
__INLINE void disp_enable_update(u8 index)
{
	#if(boardUSE_OS)
    if(DispSemaphoreBinary == NULL)
        return;

    if(index == 1U)
    {
        /* 中断上下文释放信号量并根据优先级触发调度 */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(DispSemaphoreBinary, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
        /* 任务上下文下释放信号量 */
        xSemaphoreGive(DispSemaphoreBinary);
    }
	#else
    /* 无 RTOS：通过标志位允许下一次刷新写入屏幕 */
    s_bDispFlushEnabled = true;
	#endif
}

/***********************************************************************************************************************
 -----函数功能    禁止显示刷新
 -----说明(备注)  在无RTOS的场景下停止屏幕更新（设置标志为false）
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void disp_disable_update(void)
{
    s_bDispFlushEnabled = false;
}

/***********************************************************************************************************************
 -----函数功能    LVGL刷新回调
 -----说明(备注)  LVGL的flush回调，将px_map中的像素数据转换/传给底层显示控制器
                  在RTOS下使用信号量保护TFT访问（避免多个任务同时绘制）
 -----传入参数    disp_drv:LVGL显示驱动指针  area:刷新区域  px_map:像素数据指针
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
	#if(boardUSE_OS)
    /* 非法参数直接完成 flush，避免阻塞 LVGL */
    if(!b_disp_flush_area_is_valid(area) || (px_map == NULL))
    {
        lv_display_flush_ready(disp_drv);
        return;
    }

    /* 尝试获取信号量，超时则放弃本次刷新以避免阻塞 */
    if((DispSemaphoreBinary != NULL)
    && (xSemaphoreTake(DispSemaphoreBinary, pdMS_TO_TICKS(100U)) != pdPASS))
    {
        lv_display_flush_ready(disp_drv);
        return;
    }

    /* 若刷新使能则调用底层绘图接口（RGB565 数据） */
    if(s_bDispFlushEnabled)
    {
        vDisp_TftDrawBitmapRgb565((u16)area->x1, (u16)area->y1, (u16)area->x2, (u16)area->y2, (u16 *)px_map);
    }

    /* 释放信号量并通知 LVGL 本次刷新已完成 */
    if(DispSemaphoreBinary != NULL)
    {
        xSemaphoreGive(DispSemaphoreBinary);
    }

    lv_display_flush_ready(disp_drv);
	#else
    /* 无 RTOS：仅在使能且参数合法时执行绘制 */
    if(s_bDispFlushEnabled && b_disp_flush_area_is_valid(area) && (px_map != NULL))
    {
        vDisp_TftDrawBitmapRgb565((u16)area->x1, (u16)area->y1, (u16)area->x2, (u16)area->y2, (u16 *)px_map);
    }

    lv_display_flush_ready(disp_drv);
	#endif
}

#else /*Enable this file at the top*/

typedef int keep_pedantic_happy;
#endif  //boardDISPLAY_EN
