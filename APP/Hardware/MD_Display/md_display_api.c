/*****************************************************************************************************************
 *                                                                                                                *
 *                                         Display API - TFT + LVGL                                              *
 *                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_api.h"

#if(boardDISPLAY_EN)

#include "MD_Display/lv_port_disp.h"
#include "MD_Display/lv_port_timer.h"
#include "Middlewares/LVGL/lvgl.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

/** @defgroup ST7789_Commands ST7789控制器命令定义 */
/** @{ */
#define ST7789_SWRESET                      0x01U   /**< 软件复位命令 */
#define ST7789_SLPOUT                       0x11U   /**< 退出睡眠模式 */
#define ST7789_NORON                        0x13U   /**< 正常显示模式开启 */
#define ST7789_INVON                        0x21U   /**< 显示反转开启 */
#define ST7789_DISPOFF                      0x28U   /**< 关闭显示 */
#define ST7789_DISPON                       0x29U   /**< 开启显示 */
#define ST7789_COLMOD                       0x3AU   /**< 颜色模式设置 */
#define ST7789_MADCTL                       0x36U   /**< 内存访问控制 */
/** @} */

/** @defgroup ST7789_MADCTL_Bits ST7789内存访问控制位定义 */
/** @{ */
#define ST7789_MADCTL_MY                    0x80U   /**< Y轴镜像 */
#define ST7789_MADCTL_MX                    0x40U   /**< X轴镜像 */
#define ST7789_MADCTL_BGR                   0x08U   /**< BGR颜色顺序 */
/** @} */

/** @brief LVGL库初始化状态标志 */
static bool s_bLvglReady = false;

static void v_disp_delay_ms(u16 ms);
static void v_disp_lvgl_init_once(void);
static void v_disp_tft_set_power(bool on);
static void v_disp_lvgl_bind_tick(void);

/**
 * @brief  毫秒级延时函数
 * @param  ms: 延时毫秒数
 * @note   支持FreeRTOS操作系统和裸机两种模式
 *         - OS模式：使用vTaskDelay进行任务延时
 *         - 裸机模式：使用CPU空循环实现延时
 */
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

/**
 * @brief  TFT控制器初始化函数
 * @note   按照ST7789 datasheet要求的时序初始化显示屏
 *         包括硬件复位、软件复位、颜色模式设置等
 *         初始化完成后清屏为黑色
 */
void vDisp_TftInitController(void)
{
    /* 初始化显示接口（SPI/并行等） */
    vDisp_IfaceInit();

    /* 硬件复位时序：高-低-高，确保控制器可靠复位 */
    DISP_TFT_RES_H();
    v_disp_delay_ms(10U);
    DISP_TFT_RES_L();
    v_disp_delay_ms(20U);
    DISP_TFT_RES_H();
    v_disp_delay_ms(120U);

    /* 软件复位命令 */
    vDisp_TftWriteCommand(ST7789_SWRESET);
    v_disp_delay_ms(150U);

    /* 退出睡眠模式 */
    vDisp_TftWriteCommand(ST7789_SLPOUT);
    v_disp_delay_ms(120U);

    /* 设置颜色模式为16位RGB565 (0x55) */
    vDisp_TftWriteCommand(ST7789_COLMOD);
    vDisp_TftWriteData8(0x55U);

    /* 设置内存访问控制：
     * - MX: X轴镜像
     * - MY: Y轴镜像  
     * - BGR: 使用BGR颜色顺序（适配硬件连接）
     */
    vDisp_TftWriteCommand(ST7789_MADCTL);
    vDisp_TftWriteData8(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_BGR);

    /* 开启显示反转（改善色彩表现） */
    vDisp_TftWriteCommand(ST7789_INVON);
    v_disp_delay_ms(10U);

    /* 开启正常显示模式 */
    vDisp_TftWriteCommand(ST7789_NORON);
    v_disp_delay_ms(10U);

    /* 开启显示 */
    vDisp_TftWriteCommand(ST7789_DISPON);
    v_disp_delay_ms(100U);

    /* 开启背光 */
    vDisp_TftSetBacklight(true);

    /* 使用黑色填充整个屏幕，清除可能的随机数据 */
    vDisp_TftFillRect(0U, 0U, DISP_TFT_WIDTH, DISP_TFT_HEIGHT, 0x0000U);
}

/**
 * @brief  TFT显示屏电源控制函数
 * @param  on: true-开启显示，false-关闭显示
 * @note   通过发送ST7789显示开/关命令控制显示状态
 *         开启显示需要较长延时(120ms)，关闭显示延时较短(20ms)
 */
static void v_disp_tft_set_power(bool on)
{
    /* 根据电源状态选择对应的显示命令 */
    vDisp_TftWriteCommand(on ? ST7789_DISPON : ST7789_DISPOFF);
    /* 开启显示需要更长的稳定时间 */
    v_disp_delay_ms(on ? 120U : 20U);
}

/**
 * @brief  LVGL系统时钟绑定函数
 * @note   将LVGL的系统时钟回调函数绑定到自定义的tick获取函数
 *         用于LVGL内部的时间管理和动画定时
 */
static void v_disp_lvgl_bind_tick(void)
{
    /* 设置LVGL系统时钟回调，用于获取毫秒级时间戳 */
    lv_tick_set_cb(ulLV_GetTickMs);
}

/**
 * @brief  LVGL库一次性初始化函数
 * @note   确保LVGL只初始化一次，包括：
 *         - LVGL核心库初始化
 *         - 定时器初始化
 *         - 系统时钟绑定
 *         - 显示驱动初始化
 */
static void v_disp_lvgl_init_once(void)
{
    /* 检查是否已经初始化，避免重复初始化 */
    if(s_bLvglReady)
        return;

    /* 初始化LVGL核心库（如果尚未初始化） */
    if(!lv_is_initialized())
    {
        lv_init();           /* 初始化LVGL图形库 */
        vLV_TimerInit();     /* 初始化LVGL定时器 */
    }

    /* 绑定系统时钟回调函数 */
    v_disp_lvgl_bind_tick();

    /* 初始化显示驱动（如果尚未初始化） */
    if(lv_display_get_default() == NULL)
    {
        lv_port_disp_init(); /* 初始化显示端口 */
    }

    /* 标记LVGL初始化完成 */
    s_bLvglReady = true;
}

/**
 * @brief  显示模块初始化函数
 * @note   这是显示模块的主初始化函数，应优先调用
 *         完成LVGL初始化、电源开启和首次刷新
 */
void vDisp_Init(void)
{
    /* 初始化LVGL库 */
    v_disp_lvgl_init_once();
    /* 开启显示屏电源 */
    vDisp_SetPower(true);
    /* 执行首次刷新，更新显示内容 */
    vDisp_Refresh();
}

/**
 * @brief  显示刷新函数
 * @note   调用LVGL的定时器处理函数，更新显示内容
 *         需要定期调用以支持LVGL的动画和UI更新
 * @warning 此函数应在主循环或专用任务中定期调用
 */
void vDisp_Refresh(void)
{
    /* 检查LVGL是否已初始化 */
    if(!s_bLvglReady)
        return;

    /* 调用LVGL定时器处理函数，更新UI和动画 */
    (void)lv_timer_handler();
}

/**
 * @brief  显示电源控制函数
 * @param  on: true-开启电源，false-关闭电源
 * @note   同时控制背光和TFT控制器电源状态
 *         背光控制用于节能，控制器电源控制用于保护屏幕
 */
void vDisp_SetPower(bool on)
{
    /* 控制背光开关 */
    vDisp_TftSetBacklight(on);

    /* 如果LVGL已初始化，控制TFT控制器电源 */
    if(s_bLvglReady)
    {
        v_disp_tft_set_power(on);
    }
}

/**
 * @brief  显示对比度设置函数
 * @param  value: 对比度值（当前未使用）
 * @note   当前硬件仅支持背光开关控制，不支持亮度调节
 *         此函数预留用于未来可能的PWM背光调光功能
 */
void vDisp_SetContrast(u8 value)
{
    /* 当前硬件仅支持背光开关，忽略对比度参数 */
    (void)value;
}

/**
 * @brief  显示就绪状态检查函数
 * @retval true: 显示模块已初始化并就绪
 * @retval false: 显示模块未初始化
 * @note   用于在调用其他显示函数前检查模块状态
 */
bool bDisp_IsReady(void)
{
    return s_bLvglReady;
}

#endif  /*boardDISPLAY_EN*/
