/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : lv_port_disp.c
 * Date    : 2026-09-22
 * Author  : LJD(291483914@qq.com)
 * Desc    : LVGL显示端口适配层
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "lv_port_disp.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "Middlewares/LVGL/lv_conf.h"
#include "Middlewares/LVGL/src/misc/lv_color.h"

/* LVGL 内置 ST7789 驱动支持 */
#if (LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
#include "Middlewares/LVGL/src/drivers/display/lcd/lv_lcd_generic_mipi.h"
#include "Middlewares/LVGL/src/drivers/display/st7789/lv_st7789.h"

#define			DISP_ST7789_FLAGS						(LV_LCD_FLAG_MIRROR_X | LV_LCD_FLAG_MIRROR_Y | LV_LCD_FLAG_BGR)

//****************************************************Parameter Initialization**************************************************//
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

/* 非内置 ST7789 路径：本地驱动使用的初始化命令表与发送函数 */
#if (!LV_USE_ST7789)
/* ST7789 初始化命令表项结构体 */
typedef struct {
    uint8_t  u8Cmd;             /* 命令寄存器地址 */
    uint8_t  u8Len;             /* 参数数据长度(字节数) */
    uint8_t  u8DelayMs;         /* 发送后延时毫秒数 */
    uint8_t  aucData[14];       /* 参数数据缓冲区 (ST7789最大单条参数为Gamma的14字节) */
} LcdInitCmd_T;

/* ST7789 初始化命令配置表 (适配 ZJY240KP-IF10 320x240 横屏) */
static const LcdInitCmd_T S_atSt7789InitCmds[] = {
    /* 命令,          长度, 延时(ms), 参数数据 */
    { ST7789_SLPOUT,  0U,   120U,    { 0 } },
    { ST7789_MADCTL,  1U,     0U,    { ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_BGR } },
    { ST7789_COLMOD,  1U,     0U,    { 0x55U } },
    { 0xB2U,          5U,     0U,    { 0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U } },
    { 0xB7U,          1U,     0U,    { 0x35U } },
    { 0xBBU,          1U,     0U,    { 0x32U } },
    { 0xC2U,          1U,     0U,    { 0x01U } },
    { 0xC3U,          1U,     0U,    { 0x15U } },
    { 0xC4U,          1U,     0U,    { 0x20U } },
    { 0xC6U,          1U,     0U,    { 0x0FU } },
    { 0xD0U,          2U,     0U,    { 0xA4U, 0xA1U } },
    { 0xE0U,         14U,     0U,    { 0xD0U, 0x08U, 0x0EU, 0x09U, 0x09U, 0x05U, 0x31U, 0x33U, 0x48U, 0x17U, 0x14U, 0x15U, 0x31U, 0x34U } },
    { 0xE1U,         14U,     0U,    { 0xD0U, 0x08U, 0x0EU, 0x09U, 0x09U, 0x15U, 0x31U, 0x33U, 0x48U, 0x17U, 0x14U, 0x15U, 0x31U, 0x34U } },
    { ST7789_INVON,   0U,    10U,    { 0 } },
    { ST7789_NORON,   0U,    10U,    { 0 } },
    { ST7789_DISPON,  0U,   100U,    { 0 } },
};
#endif  /* !LV_USE_ST7789 */

/* 显示访问的二值信号量（用于串行化对 TFT 的访问） */
#if (boardUSE_OS)
SemaphoreHandle_t DispSemaphoreBinary = NULL;
SemaphoreHandle_t DispFlushDoneSemaphore = NULL;
#endif

lv_display_t *tp_disp;

#define			DISP_DRAW_BUF_LINE_COUNT				15U

#if (boardUSE_OS)
static bool b_disp_bus_take(TickType_t wait_ticks);
static void v_disp_bus_give(void);
static void v_disp_flush_done_reset(void);
static void v_disp_flush_wait(lv_display_t *tp_disp);
#endif

#if (LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
static void st7789_send_cmd(lv_display_t *tp_disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size);
static void st7789_send_color(lv_display_t *tp_disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size);
#endif

#if !(LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
static void disp_flush(lv_display_t *tp_disp, const lv_area_t *area, uint8_t *px_map);
static bool b_disp_flush_area_is_valid(const lv_area_t *area);
#endif

#if (!LV_USE_ST7789)
static void v_disp_delay_ms(u16 ms);
static void lcd_send_init_commands(void);
#endif  /* !LV_USE_ST7789 */

/***********************************************************************************************************************
 * 函数功能    : LVGL显示端口初始化
 * 说明(备注)  : 配置LVGL显示对象、分配双缓冲区并设置刷新回调，使用对齐缓冲区以满足DMA/控制器要求
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
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
    #endif  /* boardUSE_OS */

    #if (LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
    tp_disp = lv_st7789_create(DISP_HOR_RES, DISP_VER_RES, DISP_ST7789_FLAGS, st7789_send_cmd, st7789_send_color);
    if (tp_disp != NULL)
        lv_st7789_send_cmd_list(tp_disp, disp_st7789_panel_cmds);
    #else
    tp_disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    if (tp_disp != NULL)
        lv_display_set_flush_cb(tp_disp, disp_flush);

        #if (!LV_USE_ST7789)
    /* 本地驱动路径：发送面板初始化命令表并整屏清黑 */
    lcd_send_init_commands();
    vDisp_FastDrawColor(0U, 0U, dispTFT_WIDTH, dispTFT_HEIGHT, 0x0000U);
        #endif  /* !LV_USE_ST7789 */
    #endif  /* LV_USE_ST7789 */

    if (tp_disp != NULL)
    {
        lv_display_set_buffers(tp_disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

        #if (boardUSE_OS)
        if (DispFlushDoneSemaphore != NULL)
            lv_display_set_flush_wait_cb(tp_disp, v_disp_flush_wait);
        #endif  /* boardUSE_OS */
    }
}

#if (!LV_USE_ST7789)
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
 * 函数功能    : 发送初始化命令序列
 * 说明(备注)  : 按照ST7789 datasheet要求的时序初始化显示屏
 *               包括硬件复位、软件复位、颜色模式设置等
 *               初始化完成后清屏为黑色
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void lcd_send_init_commands(void)
{
    /* 硬件复位时序：高-低-高，确保控制器可靠复位 */
    dispTFT_RES_H();
    v_disp_delay_ms(10U);
    dispTFT_RES_L();
    v_disp_delay_ms(50U);
    dispTFT_RES_H();
    v_disp_delay_ms(50U);

    /* 遍历表驱动初始化序列 */
    for (uint32_t i = 0U; i < (sizeof(S_atSt7789InitCmds) / sizeof(S_atSt7789InitCmds[0])); i++)
    {
        const LcdInitCmd_T *pt_cmd = &S_atSt7789InitCmds[i];

        vDisp_WriteCommand(pt_cmd->u8Cmd);
        for (uint8_t j = 0U; j < pt_cmd->u8Len; j++)
        {
            vDisp_WriteData8(pt_cmd->aucData[j]);
        }

        if (pt_cmd->u8DelayMs > 0U)
        {
            v_disp_delay_ms((u16)pt_cmd->u8DelayMs);
        }
    }

    /* 开启背光 */
    vDisp_SetBacklight(false);
}
#endif  /* !LV_USE_ST7789 */

#if (boardUSE_OS)
/***********************************************************************************************************************
 * 函数功能    : 获取信号量
 * 说明(备注)  : 尝试获取显示控制器的信号量，用于在启用RTOS时保护TFT访问
 * 传入参数    : wait_ticks:等待信号量的最大时间（Tick数）
 * 输出参数    : none
 * 返回值      : true:获取成功  false:获取失败
 ************************************************************************************************************************/
static bool b_disp_bus_take(TickType_t wait_ticks)
{
    if (DispSemaphoreBinary == NULL)
        return false;

    return (xSemaphoreTake(DispSemaphoreBinary, wait_ticks) == pdPASS);
}

/***********************************************************************************************************************
 * 函数功能    : 释放信号量
 * 说明(备注)  : 释放显示控制器的信号量，用于在启用RTOS时保护TFT访问
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_disp_bus_give(void)
{
    if (DispSemaphoreBinary != NULL)
        xSemaphoreGive(DispSemaphoreBinary);
}

/***********************************************************************************************************************
 * 函数功能    : 复位刷新完成信号量
 * 说明(备注)  : 清空DispFlushDoneSemaphore中的所有计数，使其恢复到"未完成"状态
 *               在发起新的异步刷新传输前调用，确保等待的是本次刷新完成事件
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
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
 * 函数功能    : 等待刷新完成
 * 说明(备注)  : 阻塞等待DispFlushDoneSemaphore信号量，直到底层异步刷新传输完成
 *               作为LVGL的flush_wait回调，在渲染下一帧前确保上一帧数据已全部写入TFT控制器
 * 传入参数    : tp_disp:LVGL显示对象指针（未使用，仅为匹配回调签名）
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_disp_flush_wait(lv_display_t *tp_disp)
{
    LV_UNUSED(tp_disp);

    if (DispFlushDoneSemaphore != NULL)
        (void)xSemaphoreTake(DispFlushDoneSemaphore, portMAX_DELAY);
}
#endif

#if (LV_USE_ST7789 && LV_USE_GENERIC_MIPI)
/***********************************************************************************************************************
 * 函数功能    : ST7789 发送命令回调函数
 * 说明(备注)  : LVGL内置ST7789驱动的命令发送回调，用于向TFT控制器发送命令和参数
 * 传入参数    : tp_disp:显示对象  cmd:命令缓冲区  cmd_size:命令大小  param:参数数据  param_size:参数大小
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void st7789_send_cmd(lv_display_t *tp_disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param, size_t param_size)
{
    LV_UNUSED(tp_disp);

    #if (boardUSE_OS)
    if (!b_disp_bus_take(pdMS_TO_TICKS(100U)))
        return;
    #endif  /* boardUSE_OS */

    /* 发送命令字节 */
    for (size_t i = 0; i < cmd_size; i++)
        vDisp_WriteCommand(cmd[i]);

    /* 发送参数 */
    if ((param != NULL) && (param_size != 0U))
        vDisp_WriteBuffer(param, (u32)param_size);

    #if (boardUSE_OS)
    v_disp_bus_give();
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : ST7789 发送颜色数据回调函数
 * 说明(备注)  : LVGL内置ST7789驱动的颜色数据发送回调，用于向TFT控制器发送像素数据
 * 传入参数    : tp_disp:显示对象  cmd:命令缓冲区  cmd_size:命令大小  param:像素数据  param_size:数据大小(字节数)
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void st7789_send_color(lv_display_t *tp_disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param, size_t param_size)
{
    #if (boardUSE_OS)
    if (!b_disp_bus_take(pdMS_TO_TICKS(100U)))
    {
        lv_display_flush_ready(tp_disp);
        return;
    }
    #endif  /* boardUSE_OS */

    for (size_t i = 0; i < cmd_size; i++)
        vDisp_WriteCommand(cmd[i]);

    if ((param != NULL) && (param_size != 0U))
    {
        #if (boardUSE_OS)
        v_disp_flush_done_reset();
        #endif  /* boardUSE_OS */

        if (bDisp_WriteColorAsync(param, (u32)param_size))
            return;
    }

    #if (boardUSE_OS)
    v_disp_bus_give();
    #endif  /* boardUSE_OS */

    lv_display_flush_ready(tp_disp);
}
#endif

/***********************************************************************************************************************
 * 函数功能    : 刷新区域有效性检查
 * 说明(备注)  : 检查LVGL传入的刷新区域是否有效，防止越界或空区域操作
 * 传入参数    : area: LVGL区域结构体指针
 * 输出参数    : none
 * 返回值      : true:区域有效  false:区域无效
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
 * 函数功能    : LVGL刷新回调
 * 说明(备注)  : LVGL的flush回调，将px_map中的像素数据转换/传给底层显示控制器
 *               在RTOS下使用信号量保护TFT访问（避免多个任务同时绘制）
 * 传入参数    : disp_drv:LVGL显示驱动指针  area:刷新区域  px_map:像素数据指针
 * 输出参数    : none
 * 返回值      : none
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
    #endif  /* boardUSE_OS */
}
#endif

#else  /*Enable this file at the top*/

typedef int keep_pedantic_happy;
#endif // boardDISPLAY_EN
