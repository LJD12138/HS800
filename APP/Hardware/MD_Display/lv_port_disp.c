/***********************************************************************************************************************
 -----文件说明    LVGL显示端口适配层
 -----说明(备注)  将LVGL显示刷新回调与底层TFT控制器绑定，支持FreeRTOS信号量保护和双缓冲渲染模式
 -----文件版本    V1.1
 -----作者        HS800开发团队
 -----日期        2024
 ************************************************************************************************************************/
#include "lv_port_disp.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "Middlewares/LVGL/lv_conf.h"
#include "Middlewares/LVGL/src/misc/lv_color.h"

/* LVGL 内置 ST7789 驱动支持 */
#if (LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
#include "Middlewares/LVGL/src/drivers/display/lcd/lv_lcd_generic_mipi.h"
#include "Middlewares/LVGL/src/drivers/display/st7789/lv_st7789.h"

#define DISP_ST7789_FLAGS (LV_LCD_FLAG_MIRROR_X | LV_LCD_FLAG_MIRROR_Y | LV_LCD_FLAG_BGR)

//****************************************************参数初始化**************************************************//
/* ZJY240KP-IF10 对通用 ST7789 缺省寄存器不稳定，创建后覆盖为项目原有已验证参数。 */
static const uint8_t disp_st7789_panel_cmds[] = {
    LV_LCD_CMD_SET_ADDRESS_MODE, 1, 0xA0,
    LV_LCD_CMD_SET_PIXEL_FORMAT, 1, LV_LCD_PIXEL_FORMAT_RGB565,
    0xB0, 2, 0x00, 0xE8,
    0xB2, 5, 0x0C, 0x0C, 0x00, 0x33, 0x33,
    0xB7, 1, 0x35,
    0xBB, 1, 0x32,
    0xC2, 1, 0x01,
    0xC3, 1, 0x15,
    0xC4, 1, 0x20,
    0xC6, 1, 0x0F,
    0xD0, 2, 0xA4, 0xA1,
    0xE0, 14, 0xD0, 0x08, 0x0E, 0x09, 0x09, 0x05, 0x31, 0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34,
    0xE1, 14, 0xD0, 0x08, 0x0E, 0x09, 0x09, 0x15, 0x31, 0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34,
    LV_LCD_CMD_ENTER_INVERT_MODE, 0,
    LV_LCD_CMD_ENTER_NORMAL_MODE, 0,
    LV_LCD_CMD_EXIT_SLEEP_MODE, 0,
    LV_LCD_CMD_DELAY_MS, 12,
    LV_LCD_CMD_SET_DISPLAY_ON, 0,
    LV_LCD_CMD_DELAY_MS, 1,
    LV_LCD_CMD_EOF, LV_LCD_CMD_EOF
};
#endif

/* 显示访问的二值信号量（用于串行化对 TFT 的访问） */
#if (boardUSE_OS)
SemaphoreHandle_t DispSemaphoreBinary = NULL;
SemaphoreHandle_t DispFlushDoneSemaphore = NULL;
#endif

lv_display_t *disp;

#define DISP_DRAW_BUF_LINE_COUNT 15U

#if (boardUSE_OS)
static bool b_disp_bus_take(TickType_t wait_ticks);
static void v_disp_bus_give(void);
static void v_disp_flush_done_reset(void);
static void v_disp_flush_wait(lv_display_t *disp);
#endif

#if (LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
static void st7789_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size);
static void st7789_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size);
#endif

#if !(LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static bool b_disp_flush_area_is_valid(const lv_area_t *area);
#endif

/***********************************************************************************************************************
 -----函数功能    LVGL显示端口初始化
 -----说明(备注)  配置LVGL显示对象、分配双缓冲区并设置刷新回调，使用对齐缓冲区以满足DMA/控制器要求
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void lv_port_disp_init(void)
{
    /* 按硬件分辨率与行数分配两块对齐缓存（双缓冲），提高渲染效率 */
    __ALIGNED(4) static lv_color_t buf_1[DISP_HOR_RES * DISP_DRAW_BUF_LINE_COUNT];
    __ALIGNED(4) static lv_color_t buf_2[DISP_HOR_RES * DISP_DRAW_BUF_LINE_COUNT];

    /* 初始化底层显示控制器与同步对象 */
    #if (boardUSE_OS)
    if (DispSemaphoreBinary == NULL)
    {
        DispSemaphoreBinary = xSemaphoreCreateBinary();
        if (DispSemaphoreBinary != NULL)
            xSemaphoreGive(DispSemaphoreBinary);
    }

    if (DispFlushDoneSemaphore == NULL)
        DispFlushDoneSemaphore = xSemaphoreCreateBinary();
    #endif

    #if (LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
    disp = lv_st7789_create(DISP_HOR_RES, DISP_VER_RES, DISP_ST7789_FLAGS, st7789_send_cmd, st7789_send_color);
    if (disp != NULL)
        lv_st7789_send_cmd_list(disp, disp_st7789_panel_cmds);
    #else
    disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    if (disp != NULL)
        lv_display_set_flush_cb(disp, disp_flush);
    #endif

    if (disp != NULL)
    {
        lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

        #if (boardUSE_OS)
        if (DispFlushDoneSemaphore != NULL)
            lv_display_set_flush_wait_cb(disp, v_disp_flush_wait);
        #endif
    }
}

#if (boardUSE_OS)
/***********************************************************************************************************************
 -----函数功能    获取信号量
 -----说明(备注)  尝试获取显示控制器的信号量，用于在启用RTOS时保护TFT访问
 -----传入参数    wait_ticks:等待信号量的最大时间（Tick数）
 -----输出参数    none
 -----返回值      true:获取成功  false:获取失败
 ************************************************************************************************************************/
static bool b_disp_bus_take(TickType_t wait_ticks)
{
    if (DispSemaphoreBinary == NULL)
        return false;

    return (xSemaphoreTake(DispSemaphoreBinary, wait_ticks) == pdPASS);
}

/***********************************************************************************************************************
 -----函数功能    释放信号量
 -----说明(备注)  释放显示控制器的信号量，用于在启用RTOS时保护TFT访问
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_disp_bus_give(void)
{
    if (DispSemaphoreBinary != NULL)
        xSemaphoreGive(DispSemaphoreBinary);
}

/***********************************************************************************************************************
 -----函数功能    复位刷新完成信号量
 -----说明(备注)  清空DispFlushDoneSemaphore中的所有计数，使其恢复到"未完成"状态
                  在发起新的异步刷新传输前调用，确保等待的是本次刷新完成事件
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_disp_flush_done_reset(void)
{
    if (DispFlushDoneSemaphore == NULL)
        return;

    while (xSemaphoreTake(DispFlushDoneSemaphore, 0U) == pdPASS)
    {
    }
}

/***********************************************************************************************************************
 -----函数功能    等待刷新完成
 -----说明(备注)  阻塞等待DispFlushDoneSemaphore信号量，直到底层异步刷新传输完成
                  作为LVGL的flush_wait回调，在渲染下一帧前确保上一帧数据已全部写入TFT控制器
 -----传入参数    disp:LVGL显示对象指针（未使用，仅为匹配回调签名）
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_disp_flush_wait(lv_display_t *disp)
{
    LV_UNUSED(disp);

    if (DispFlushDoneSemaphore != NULL)
        (void)xSemaphoreTake(DispFlushDoneSemaphore, portMAX_DELAY);
}
#endif

#if (LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
/***********************************************************************************************************************
 -----函数功能    ST7789 发送命令回调函数
 -----说明(备注)  LVGL内置ST7789驱动的命令发送回调，用于向TFT控制器发送命令和参数
 -----传入参数    disp:显示对象  cmd:命令缓冲区  cmd_size:命令大小  param:参数数据  param_size:参数大小
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void st7789_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size)
{
    LV_UNUSED(disp);

    #if (boardUSE_OS)
    if (!b_disp_bus_take(pdMS_TO_TICKS(100U)))
        return;
    #endif

    /* 发送命令字节 */
    for (size_t i = 0; i < cmd_size; i++)
        vDisp_TftWriteCommand(cmd[i]);

    /* 发送参数 */
    if ((param != NULL) && (param_size != 0U))
        vDisp_TftWriteBuffer(param, (u32)param_size);

    #if (boardUSE_OS)
    v_disp_bus_give();
    #endif
}

/***********************************************************************************************************************
 -----函数功能    ST7789 发送颜色数据回调函数
 -----说明(备注)  LVGL内置ST7789驱动的颜色数据发送回调，用于向TFT控制器发送像素数据
 -----传入参数    disp:显示对象  cmd:命令缓冲区  cmd_size:命令大小  param:像素数据  param_size:数据大小(字节数)
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void st7789_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size)
{
    #if (boardUSE_OS)
    if (!b_disp_bus_take(pdMS_TO_TICKS(100U)))
    {
        lv_display_flush_ready(disp);
        return;
    }
    #endif

    for (size_t i = 0; i < cmd_size; i++)
        vDisp_TftWriteCommand(cmd[i]);

    if ((param != NULL) && (param_size != 0U))
    {
        #if (boardUSE_OS)
        v_disp_flush_done_reset();
        #endif

        if (bDisp_TftWriteColorAsync(param, (u32)param_size))
            return;
    }

    #if (boardUSE_OS)
    v_disp_bus_give();
    #endif

    lv_display_flush_ready(disp);
}
#endif

/***********************************************************************************************************************
 -----函数功能    刷新区域有效性检查
 -----说明(备注)  检查LVGL传入的刷新区域是否有效，防止越界或空区域操作
 -----传入参数    area: LVGL区域结构体指针
 -----输出参数    none
 -----返回值      true:区域有效  false:区域无效
 ************************************************************************************************************************/
#if !(LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
static bool b_disp_flush_area_is_valid(const lv_area_t *area)
{
    if (area == NULL)
        return false;

    /* 坐标范围检查：x1/x2 与 y1/y2 的有效性 */
    if ((area->x1 > area->x2) || (area->y1 > area->y2))
        return false;

    /* 是否超出屏幕右下边界（左上点越界亦视为非法） */
    if ((area->x1 >= (lv_coord_t)DISP_HOR_RES) || (area->y1 >= (lv_coord_t)DISP_VER_RES))
        return false;

    return true;
}

/***********************************************************************************************************************
 -----函数功能    LVGL刷新回调
 -----说明(备注)  LVGL的flush回调，将px_map中的像素数据转换/传给底层显示控制器
                  在RTOS下使用信号量保护TFT访问（避免多个任务同时绘制）
 -----传入参数    disp_drv:LVGL显示驱动指针  area:刷新区域  px_map:像素数据指针
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    #if (boardUSE_OS)
    if (DispSemaphoreBinary == NULL)
    {
        lv_display_flush_ready(disp_drv);
        return;
    }

    /* 非法参数直接完成 flush，避免阻塞 LVGL */
    if (!b_disp_flush_area_is_valid(area) || (px_map == NULL))
    {
        lv_display_flush_ready(disp_drv);
        return;
    }

    /* 尝试获取信号量，超时则放弃本次刷新以避免阻塞 */
    if (!b_disp_bus_take(pdMS_TO_TICKS(100U)))
    {
        lv_display_flush_ready(disp_drv);
        return;
    }

    /* 获取到显示访问权后直接调用底层绘图接口（RGB565 数据） */
    vDisp_FastDrawColor((u16)area->x1, (u16)area->y1, (u16)area->x2, (u16)area->y2, (u16 *)px_map);

    /* 释放信号量并通知 LVGL 本次刷新已完成 */
    v_disp_bus_give();

    lv_display_flush_ready(disp_drv);
    #else
    /* 无 RTOS：参数合法时直接执行绘制 */
    if (b_disp_flush_area_is_valid(area) && (px_map != NULL))
        vDisp_FastDrawColor((u16)area->x1, (u16)area->y1, (u16)area->x2, (u16)area->y2, (u16 *)px_map);

    lv_display_flush_ready(disp_drv);
    #endif
}
#endif

#else  /*Enable this file at the top*/

typedef int keep_pedantic_happy;
#endif // boardDISPLAY_EN
