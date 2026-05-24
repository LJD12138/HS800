/**
 * @file lv_conf.h
 * @brief LVGL v9.4.0-dev 配置文件
 * 
 * 此文件用于配置LVGL图形库的各项参数，包括：
 * - 显示设置（分辨率、颜色深度）
 * - 内存管理
 * - 渲染引擎配置
 * - 组件使能
 * - 主题和布局
 * - 第三方库集成
 */

/*
 * 复制此文件为 `lv_conf.h`
 * 1. 直接放在 `lvgl` 文件夹旁边
 * 2. 或者放在其他位置，然后：
 *    - 定义 `LV_CONF_INCLUDE_SIMPLE`;
 *    - 添加路径作为包含路径。
 */

/* clang-format off */
#if 1 /* 设置为 "1" 以启用配置内容 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* 如果需要在此处包含其他头文件，请在 `__ASSEMBLY__` 保护内进行 */
#if  0 && defined(__ASSEMBLY__)
#include "my_include.h"
#endif

/*====================
   颜色设置
 *====================*/

#define MY_DISP_HOR_RES    240    /**< 水平分辨率（像素） */
#define MY_DISP_VER_RES    320    /**< 垂直分辨率（像素） */

/** 颜色深度选项：
 * 1 (I1)   - 单色，每个像素1位
 * 8 (L8)   - 灰度，每个像素8位
 * 16 (RGB565) - 16位色，每个像素2字节，适合大多数嵌入式LCD
 * 24 (RGB888) - 24位真彩色
 * 32 (XRGB8888) - 32位色，包含Alpha通道
 */
#define LV_COLOR_DEPTH 16

/*=========================
   标准库包装设置
 *=========================*/

/** 内存分配函数选择：
 * - LV_STDLIB_BUILTIN:     LVGL内置实现（推荐用于资源受限系统）
 * - LV_STDLIB_CLIB:        标准C函数（malloc, free等）
 * - LV_STDLIB_MICROPYTHON: MicroPython实现
 * - LV_STDLIB_RTTHREAD:    RT-Thread实现
 * - LV_STDLIB_CUSTOM:      外部自定义实现
 */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN

/** 字符串处理函数选择：
 * 选项同上，用于strlen, strcpy等字符串操作
 */
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN

/** 格式化输出函数选择：
 * 选项同上，用于sprintf等格式化操作
 */
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

/** 标准头文件路径配置 */
#define LV_STDINT_INCLUDE       <stdint.h>      /**< 标准整数类型定义 */
#define LV_STDDEF_INCLUDE       <stddef.h>      /**< 标准定义（size_t等） */
#define LV_STDBOOL_INCLUDE      <stdbool.h>     /**< 布尔类型定义 */
#define LV_INTTYPES_INCLUDE     <inttypes.h>    /**< 整数格式化宏 */
#define LV_LIMITS_INCLUDE       <limits.h>      /**< 数据类型限制 */
#define LV_STDARG_INCLUDE       <stdarg.h>      /**< 可变参数支持 */

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
    /** LVGL可用内存池大小（字节），最小建议2KB */
    #define LV_MEM_SIZE (20 * 1024U)          /**< [字节] 默认20KB */

    /** 内存池扩展大小（字节），0表示不可扩展 */
    #define LV_MEM_POOL_EXPAND_SIZE 0

    /** 内存池起始地址，可用于指定外部SRAM地址
     *  0表示不使用，由系统自动分配 */
    #define LV_MEM_ADR 0     /**< 0: 不使用指定地址 */
    
    /* 如果未指定地址，则清除内存池相关配置 */
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif
#endif  /*LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN*/

/*====================
   硬件抽象层(HAL)设置
 *====================*/

/** 默认显示刷新、输入设备读取和动画步进周期（毫秒）
 *  较小的值 = 更流畅的动画，但CPU占用更高
 *  建议值：20-50ms */
#define LV_DEF_REFR_PERIOD  33      /**< [ms] 约30FPS */

/** 默认DPI（每英寸点数），用于初始化默认尺寸
 *  如控件大小、样式内边距等
 *  可根据实际屏幕调整 */
#define LV_DPI_DEF 130              /**< [像素/英寸] */

/*=================
 * 操作系统配置
 *=================*/

/** 选择使用的操作系统：
 * - LV_OS_NONE       无操作系统（裸机）
 * - LV_OS_PTHREAD    POSIX线程
 * - LV_OS_FREERTOS   FreeRTOS（常用嵌入式RTOS）
 * - LV_OS_CMSIS_RTOS2 CMSIS-RTOS2
 * - LV_OS_RTTHREAD   RT-Thread
 * - LV_OS_WINDOWS    Windows
 * - LV_OS_MQX        MQX RTOS
 * - LV_OS_SDL2       SDL2
 * - LV_OS_CUSTOM     自定义操作系统 */
#define LV_USE_OS   LV_OS_NONE

#if LV_USE_OS == LV_OS_CUSTOM
    #define LV_OS_CUSTOM_INCLUDE <stdint.h>  /**< 自定义OS头文件 */
#endif

#if LV_USE_OS == LV_OS_FREERTOS
    /*
     * 使用RTOS任务通知比二进制信号量快45%，且占用更少RAM
     * 仅当只有一个任务可以接收事件时可用
     */
    #define LV_USE_FREERTOS_TASK_NOTIFY 1
#endif

/*========================
 * 渲染配置
 *========================*/

/** 所有图层和图像的行对齐字节数 */
#define LV_DRAW_BUF_STRIDE_ALIGN                1

/** 绘制缓冲区起始地址对齐字节数 */
#define LV_DRAW_BUF_ALIGN                       4

/** 使用矩阵进行变换
 *  要求：
 * - `LV_USE_MATRIX = 1`
 * - 渲染引擎需支持3x3矩阵变换 */
#define LV_DRAW_TRANSFORM_USE_MATRIX            0

/* 如果控件设置了 `style_opa < 255` 或非NORMAL混合模式，
 * 会在渲染前被缓冲到"简单"图层中。
 * "变换图层"（设置了transform_angle/zoom时）使用更大的缓冲区，
 * 且不能分块绘制。 */

/** 简单图层分块的目标缓冲区大小 */
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE    (24 * 1024)    /**< [字节] 默认24KB */

/** 限制简单图层和变换图层的最大内存分配
 *  至少应为 `LV_DRAW_LAYER_SIMPLE_BUF_SIZE` 大小
 *  如果使用变换图层，应能存储最大的控件（宽x高x4字节）
 *  设置为0表示无限制 */
#define LV_DRAW_LAYER_MAX_MEMORY 0  /**< 默认无限制 [字节] */

/** 绘制线程的栈大小
 *  注意：如果启用FreeType或ThorVG，建议设置为32KB或更大 */
#define LV_DRAW_THREAD_STACK_SIZE    (8 * 1024)         /**< [字节] 默认8KB */

/** 绘制任务的线程优先级
 *  数值越大优先级越高
 *  可使用lv_os.h中的lv_thread_prio_t枚举值：
 *  LV_THREAD_PRIO_LOWEST, LV_THREAD_PRIO_LOW, LV_THREAD_PRIO_MID,
 *  LV_THREAD_PRIO_HIGH, LV_THREAD_PRIO_HIGHEST
 *  确保优先级值与操作系统特定的优先级级别对齐 */
#define LV_DRAW_THREAD_PRIO LV_THREAD_PRIO_HIGH

/** 启用软件绘制引擎 */
#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW == 1
    /*
     * 选择性禁用颜色格式支持以减小代码大小
     * 注意：某些功能在内部使用特定颜色格式，例如：
     * - 渐变使用RGB888
     * - 带透明度的位图可能使用ARGB8888
     */
    #define LV_DRAW_SW_SUPPORT_RGB565       1       /**< 支持RGB565格式 */
    #define LV_DRAW_SW_SUPPORT_RGB565_SWAPPED 1     /**< 支持字节交换的RGB565 */
    #define LV_DRAW_SW_SUPPORT_RGB565A8     1       /**< 支持RGB565+A8格式 */
    #define LV_DRAW_SW_SUPPORT_RGB888       1       /**< 支持RGB888格式 */
    #define LV_DRAW_SW_SUPPORT_XRGB8888     1       /**< 支持XRGB8888格式 */
    #define LV_DRAW_SW_SUPPORT_ARGB8888     1       /**< 支持ARGB8888格式 */
    #define LV_DRAW_SW_SUPPORT_ARGB8888_PREMULTIPLIED 1 /**< 支持预乘Alpha的ARGB8888 */
    #define LV_DRAW_SW_SUPPORT_L8           1       /**< 支持8位灰度格式 */
    #define LV_DRAW_SW_SUPPORT_AL88         1       /**< 支持AL88格式 */
    #define LV_DRAW_SW_SUPPORT_A8           1       /**< 支持8位Alpha格式 */
    #define LV_DRAW_SW_SUPPORT_I1           1       /**< 支持1位索引色格式 */

    /** 索引色格式中，判断像素为"活动"的亮度阈值 */
    #define LV_DRAW_SW_I1_LUM_THRESHOLD 127

    /** 绘制单元数量设置：
     *  - 1: 单线程渲染
     *  - >1: 需要在 `LV_USE_OS` 中启用操作系统
     *         多线程并行渲染屏幕 */
    #define LV_DRAW_SW_DRAW_UNIT_CNT    1

    /** 使用Arm-2D加速软件渲染 */
    #define LV_USE_DRAW_ARM2D_SYNC      0

    /** 启用原生Helium汇编优化 */
    #define LV_USE_NATIVE_HELIUM_ASM    0

    /**
     * 渲染器复杂度选择：
     * - 0: 简单渲染器，只能绘制简单矩形（渐变）、图像、文本和直线
     * - 1: 复杂渲染器，还能绘制圆角、阴影、斜线和弧形 */
    #define LV_DRAW_SW_COMPLEX          1

    #if LV_DRAW_SW_COMPLEX == 1
        /** 阴影计算缓存大小
         *  最大可缓存的阴影尺寸 = shadow_width + radius
         *  缓存会占用 LV_DRAW_SW_SHADOW_CACHE_SIZE^2 的RAM */
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0

        /** 圆形数据最大缓存数量
         *  保存1/4圆周用于抗锯齿
         *  每个圆占用 `radius * 4` 字节（保存最常用的半径）
         *  0: 禁用缓存 */
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
    #endif

    /** 软件绘制汇编优化选项 */
    #define  LV_USE_DRAW_SW_ASM     LV_DRAW_SW_ASM_NONE

    #if LV_USE_DRAW_SW_ASM == LV_DRAW_SW_ASM_CUSTOM
        #define  LV_DRAW_SW_ASM_CUSTOM_INCLUDE ""  /**< 自定义汇编头文件 */
    #endif

    /** 启用软件绘制复杂渐变：角度线性渐变、径向渐变、锥形渐变 */
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS    0

#endif

/* 使用TSi（Think Silicon）的NemaGFX GPU */
#define LV_USE_NEMA_GFX 0

#if LV_USE_NEMA_GFX
    /** 选择NemaGFX HAL类型：
     * - LV_NEMA_HAL_CUSTOM 自定义HAL
     * - LV_NEMA_HAL_STM32  STM32系列 */
    #define LV_USE_NEMA_HAL LV_NEMA_HAL_CUSTOM
    #if LV_USE_NEMA_HAL == LV_NEMA_HAL_STM32
        #define LV_NEMA_STM32_HAL_INCLUDE <stm32u5xx_hal.h>
    #endif

    /** 启用矢量图形操作（需NemaVG库） */
    #define LV_USE_NEMA_VG 0
    #if LV_USE_NEMA_VG
        /** 用于VG相关缓冲区分配的分辨率 */
        #define LV_NEMA_GFX_MAX_RESX 800
        #define LV_NEMA_GFX_MAX_RESY 600
    #endif
#endif

/* 使用NXP iMX RTxxx平台的VG-Lite GPU */
#define LV_USE_DRAW_VGLITE 0

#if LV_USE_DRAW_VGLITE
    /** 启用blit质量降级方案（屏幕尺寸>352像素时推荐） */
    #define LV_USE_VGLITE_BLIT_SPLIT 0

    #if LV_USE_OS
        /** 为VG-Lite处理使用额外的绘制线程 */
        #define LV_USE_VGLITE_DRAW_THREAD 1

        #if LV_USE_VGLITE_DRAW_THREAD
            /** 启用VGLite异步绘制，批量处理多个任务后一次性发送到GPU */
            #define LV_USE_VGLITE_DRAW_ASYNC 1
        #endif
    #endif

    /** 启用VGLite断言 */
    #define LV_USE_VGLITE_ASSERT 0

    /** 启用VGLite错误检查 */
    #define LV_USE_VGLITE_CHECK_ERROR 0
#endif

/* 使用NXP iMX RTxxx平台的PXP（像素处理流水线） */
#define LV_USE_PXP 0

#if LV_USE_PXP
    /** 使用PXP进行绘制 */
    #define LV_USE_DRAW_PXP 1

    /** 使用PXP旋转显示 */
    #define LV_USE_ROTATE_PXP 0

    #if LV_USE_DRAW_PXP && LV_USE_OS
        /** 为PXP处理使用额外的绘制线程 */
        #define LV_USE_PXP_DRAW_THREAD 1
    #endif

    /** 启用PXP断言 */
    #define LV_USE_PXP_ASSERT 0
#endif

/* 使用NXP MPU平台的G2D图形加速 */
#define LV_USE_DRAW_G2D 0

#if LV_USE_DRAW_G2D
    /** G2D绘制单元可存储的最大缓冲区数量（包括帧缓冲和资源） */
    #define LV_G2D_HASH_TABLE_SIZE 50

    #if LV_USE_OS
        /** 为G2D处理使用额外的绘制线程 */
        #define LV_USE_G2D_DRAW_THREAD 1
    #endif

    /** 启用G2D断言 */
    #define LV_USE_G2D_ASSERT 0
#endif

/* 使用Renesas RA平台的Dave2D图形加速 */
#define LV_USE_DRAW_DAVE2D 0

/* 使用缓存的SDL纹理进行绘制 */
#define LV_USE_DRAW_SDL 0

/* 使用VG-Lite GPU */
#define LV_USE_DRAW_VG_LITE 0

#if LV_USE_DRAW_VG_LITE
    /** 启用VG-Lite自定义外部 `gpu_init()` 函数 */
    #define LV_VG_LITE_USE_GPU_INIT 0

    /** 启用VG-Lite断言 */
    #define LV_VG_LITE_USE_ASSERT 0

    /** VG-Lite刷新提交触发阈值，GPU会尝试批量处理这些绘制任务 */
    #define LV_VG_LITE_FLUSH_MAX_COUNT 8

    /** 启用边框模拟阴影
     *  注意：通常可提高性能，但不能保证与软件渲染相同的渲染质量 */
    #define LV_VG_LITE_USE_BOX_SHADOW 1

    /** VG-Lite渐变最大缓存数量
     *  注意：单个渐变图像内存占用为4KB */
    #define LV_VG_LITE_GRAD_CACHE_CNT 32

    /** VG-Lite描边最大缓存数量 */
    #define LV_VG_LITE_STROKE_CACHE_CNT 32

    /** 移除VLC_OP_CLOSE路径指令（NXP平台的解决方案） */
    #define LV_VG_LITE_DISABLE_VLC_OP_CLOSE 0

    /** 禁用线性渐变扩展（针对某些旧版本驱动） */
    #define LV_VG_LITE_DISABLE_LINEAR_GRADIENT_EXT 0
#endif

/* 使用STM32 DMA2D加速混合、填充等操作 */
#define LV_USE_DRAW_DMA2D 0

#if LV_USE_DRAW_DMA2D
    #define LV_DRAW_DMA2D_HAL_INCLUDE "stm32h7xx_hal.h"  /**< DMA2D HAL头文件 */

    /* 启用后，用户需要在收到DMA2D全局中断时调用
     * `lv_draw_dma2d_transfer_complete_interrupt_handler` */
    #define LV_USE_DRAW_DMA2D_INTERRUPT 0
#endif

/* 使用缓存的OpenGLES纹理绘制（需要LV_USE_OPENGLES） */
#define LV_USE_DRAW_OPENGLES 0

#if LV_USE_DRAW_OPENGLES
    #define LV_DRAW_OPENGLES_TEXTURE_CACHE_COUNT 64  /**< 纹理缓存数量 */
#endif

/* 使用乐鑫PPA加速器 */
#define LV_USE_PPA  0
#if LV_USE_PPA
    #define LV_USE_PPA_IMG 0  /**< 启用PPA图像处理 */
#endif

/* 使用EVE FT81X GPU */
#define LV_USE_DRAW_EVE 0

#if LV_USE_DRAW_EVE
    /* EVE代次：2, 3, 或 4 */
    #define LV_DRAW_EVE_EVE_GENERATION 4
#endif

/*=======================
 * 功能配置
 *=======================*/

/*-------------
 * 日志系统
 *-----------*/

/** 启用日志模块 */
#define LV_USE_LOG 0
#if LV_USE_LOG
    /** 日志级别设置：
     *  - LV_LOG_LEVEL_TRACE    记录详细信息（调试用）
     *  - LV_LOG_LEVEL_INFO     记录重要事件
     *  - LV_LOG_LEVEL_WARN     记录潜在问题（未造成错误）
     *  - LV_LOG_LEVEL_ERROR    仅记录可能导致系统故障的严重问题
     *  - LV_LOG_LEVEL_USER     仅记录用户自定义日志
     *  - LV_LOG_LEVEL_NONE     不记录任何日志 */
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /** 日志输出方式：
     *  - 1: 使用printf打印日志
     *  - 0: 用户需要通过 `lv_log_register_print_cb()` 注册回调 */
    #define LV_LOG_PRINTF 0

    /** 设置日志打印回调函数
     *  例如 `my_print`，原型为 `void my_print(lv_log_level_t level, const char * buf)`
     *  可被 `lv_log_register_print_cb` 覆盖 */
    //#define LV_LOG_PRINT_CB

    /** 是否打印时间戳：
     *  - 1: 启用时间戳
     *  - 0: 禁用时间戳 */
    #define LV_LOG_USE_TIMESTAMP 1

    /** 是否打印文件名和行号：
     *  - 1: 打印文件和行号
     *  - 0: 不打印文件和行号 */
    #define LV_LOG_USE_FILE_LINE 1

    /* 各模块的TRACE日志开关（产生大量日志时可关闭） */
    #define LV_LOG_TRACE_MEM        1   /**< 内存操作的TRACE日志 */
    #define LV_LOG_TRACE_TIMER      1   /**< 定时器操作的TRACE日志 */
    #define LV_LOG_TRACE_INDEV      1   /**< 输入设备操作的TRACE日志 */
    #define LV_LOG_TRACE_DISP_REFR  1   /**< 显示刷新操作的TRACE日志 */
    #define LV_LOG_TRACE_EVENT      1   /**< 事件分发逻辑的TRACE日志 */
    #define LV_LOG_TRACE_OBJ_CREATE 1   /**< 对象创建（核心obj和所有控件）的TRACE日志 */
    #define LV_LOG_TRACE_LAYOUT     1   /**< Flex和Grid布局操作的TRACE日志 */
    #define LV_LOG_TRACE_ANIM       1   /**< 动画逻辑的TRACE日志 */
    #define LV_LOG_TRACE_CACHE      1   /**< 缓存操作的TRACE日志 */
#endif  /*LV_USE_LOG*/

/*-------------
 * 断言检查
 *-----------*/

/* 启用断言检查，操作失败或发现无效数据时触发
 * 如果启用了LV_USE_LOG，失败时会打印错误信息 */
#define LV_USE_ASSERT_NULL          1   /**< 检查参数是否为NULL（快速，推荐） */
#define LV_USE_ASSERT_MALLOC        1   /**< 检查内存分配是否成功（快速，推荐） */
#define LV_USE_ASSERT_STYLE         0   /**< 检查样式是否正确初始化（快速，推荐） */
#define LV_USE_ASSERT_MEM_INTEGRITY 0   /**< 检查关键操作后lv_mem的完整性（慢） */
#define LV_USE_ASSERT_OBJ           0   /**< 检查对象类型和存在性（如未被删除）（慢） */

/** 自定义断言处理函数（如重启MCU） */
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);     /**< 默认：死循环暂停 */

/*-------------
 * 调试功能
 *-----------*/

/** 在重绘区域上绘制随机颜色的矩形（用于可视化刷新区域） */
#define LV_USE_REFR_DEBUG 0

/** 为ARGB图层绘制红色覆盖层，为RGB图层绘制绿色覆盖层 */
#define LV_USE_LAYER_DEBUG 0

/** 启用并行绘制调试功能：
 *  - 为每个绘制单元的任务绘制不同颜色的覆盖层
 *  - 在白色背景上绘制绘制单元的索引号
 *  - 对于图层，在黑色背景上绘制绘制单元的索引号 */
#define LV_USE_PARALLEL_DRAW_DEBUG 0

/*-------------
 * 其他功能
 *-----------*/

/** 启用全局自定义数据结构 */
#define LV_ENABLE_GLOBAL_CUSTOM 0
#if LV_ENABLE_GLOBAL_CUSTOM
    /** 自定义 'lv_global' 函数的头文件 */
    #define LV_GLOBAL_CUSTOM_INCLUDE <stdint.h>
#endif

/** 默认缓存大小（字节）
 *  用于lv_lodepng等图像解码器，在内存中保留解码后的图像
 *  如果大小不为0，缓存满时解码会失败
 *  如果大小为0，禁用缓存功能，解码内存使用后立即释放 */
#define LV_CACHE_DEF_SIZE       0

/** 图像头缓存条目数
 *  用于存储图像头信息，逻辑类似 `LV_CACHE_DEF_SIZE` */
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0

/** 每个渐变允许的最大色标数量
 *  每增加一个色标会增加 (sizeof(lv_color_t) + 1) 字节 */
#define LV_GRADIENT_MAX_STOPS   2

/** 颜色混合函数舍入调整
 *  GPU可能以不同方式计算颜色混合
 *  - 0:   向下取整
 *  - 64:  从x.75开始向上取整
 *  - 128: 从一半开始向上取整
 *  - 192: 从x.25开始向上取整
 *  - 254: 向上取整 */
#define LV_COLOR_MIX_ROUND_OFS  0

/** 为每个 `lv_obj_t` 添加2个32位变量以加速获取样式属性 */
#define LV_OBJ_STYLE_CACHE      0

/** 为 `lv_obj_t` 添加 `id` 字段 */
#define LV_USE_OBJ_ID           0

/** 启用控件名称支持 */
#define LV_USE_OBJ_NAME         0

/** 对象创建时自动分配ID */
#define LV_OBJ_ID_AUTO_ASSIGN   LV_USE_OBJ_ID

/** 使用内置的对象ID处理函数：
 * - lv_obj_assign_id:       控件创建时调用，为每个控件类使用独立计数器作为ID
 * - lv_obj_id_compare:      比较ID以决定是否匹配请求值
 * - lv_obj_stringify_id:    返回字符串化的标识符，如 "button3"
 * - lv_obj_free_id:         不执行任何操作，因为ID没有内存分配
 * 禁用时，用户需要自行实现这些函数 */
#define LV_USE_OBJ_ID_BUILTIN   1

/** 使用对象属性设置/获取API */
#define LV_USE_OBJ_PROPERTY 0

/** 启用属性名称支持 */
#define LV_USE_OBJ_PROPERTY_NAME 1

/* 使用VG-Lite模拟器
 * 要求：LV_USE_THORVG_INTERNAL 或 LV_USE_THORVG_EXTERNAL */
#define LV_USE_VG_LITE_THORVG  0

#if LV_USE_VG_LITE_THORVG
    /** 启用LVGL混合模式支持 */
    #define LV_VG_LITE_THORVG_LVGL_BLEND_SUPPORT 0

    /** 启用YUV颜色格式支持 */
    #define LV_VG_LITE_THORVG_YUV_SUPPORT 0

    /** 启用线性渐变扩展支持 */
    #define LV_VG_LITE_THORVG_LINEAR_GRADIENT_EXT_SUPPORT 0

    /** 启用16像素对齐 */
    #define LV_VG_LITE_THORVG_16PIXELS_ALIGN 1

    /** 缓冲区地址对齐 */
    #define LV_VG_LITE_THORVG_BUF_ADDR_ALIGN 64

    /** 启用多线程渲染 */
    #define LV_VG_LITE_THORVG_THREAD_RENDER 0
#endif

/* 启用LVGL的vg_lite规格驱动 */
#define LV_USE_VG_LITE_DRIVER  0

#if LV_USE_VG_LITE_DRIVER
    /* GPU系列选择：gc255, gc355, gc555 */
    #define LV_VG_LITE_HAL_GPU_SERIES gc255

    /* GPU修订版本（依赖于供应商） */
    #define LV_VG_LITE_HAL_GPU_REVISION 0x40

    /* GPU IP基地址（依赖于SoC，默认值适用于NXP设备） */
    #define LV_VG_LITE_HAL_GPU_BASE_ADDRESS 0x40240000
#endif

/* 启用多点触控手势识别功能 */
/* 手势识别需要使用浮点数 */
#define LV_USE_GESTURE_RECOGNITION 0

/*=====================
 * 编译器设置
 *====================*/

/** 大端系统设置为1 */
#define LV_BIG_ENDIAN_SYSTEM 0

/** `lv_tick_inc` 函数的自定义属性 */
#define LV_ATTRIBUTE_TICK_INC

/** `lv_timer_handler` 函数的自定义属性 */
#define LV_ATTRIBUTE_TIMER_HANDLER

/** `lv_display_flush_ready` 函数的自定义属性 */
#define LV_ATTRIBUTE_FLUSH_READY

/** VG_LITE缓冲区对齐字节数 */
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1

/** 内存对齐属性（如-Os优化时数据可能不对齐）
 *  例如 __attribute__((aligned(4))) */
#define LV_ATTRIBUTE_MEM_ALIGN

/** 大常量数组属性（如字体位图） */
#define LV_ATTRIBUTE_LARGE_CONST

/** RAM中大数组声明的编译器前缀 */
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/** 将性能关键函数放入更快的内存（如RAM） */
#define LV_ATTRIBUTE_FAST_MEM

/** 导出整数常量到绑定API
 *  用于LV_<CONST>形式的常量，使其出现在MicroPython等绑定API中 */
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning

/** 全局外部数据前缀 */
#define LV_ATTRIBUTE_EXTERN_DATA

/** 使用 `float` 作为 `lv_value_precise_t` 类型 */
#define LV_USE_FLOAT            0

/** 启用矩阵支持
 *  要求 `LV_USE_FLOAT = 1` */
#define LV_USE_MATRIX           0

/** 在 `lvgl.h` 中包含 `lvgl_private.h` 以访问内部数据和函数 */
#ifndef LV_USE_PRIVATE_API
    #define LV_USE_PRIVATE_API  0
#endif

/*==================
 * 字体使用配置
 *===================*/

/* Montserrat字体（ASCII范围+部分符号，bpp=4）
 * https://fonts.google.com/specimen/Montserrat */
#define LV_FONT_MONTSERRAT_8  0   /**< 8号字体 */
#define LV_FONT_MONTSERRAT_10 0   /**< 10号字体 */
#define LV_FONT_MONTSERRAT_12 0   /**< 12号字体 */
#define LV_FONT_MONTSERRAT_14 1   /**< 14号字体（默认启用） */
#define LV_FONT_MONTSERRAT_16 0   /**< 16号字体 */
#define LV_FONT_MONTSERRAT_18 0   /**< 18号字体 */
#define LV_FONT_MONTSERRAT_20 0   /**< 20号字体 */
#define LV_FONT_MONTSERRAT_22 0   /**< 22号字体 */
#define LV_FONT_MONTSERRAT_24 0   /**< 24号字体 */
#define LV_FONT_MONTSERRAT_26 0   /**< 26号字体 */
#define LV_FONT_MONTSERRAT_28 0   /**< 28号字体 */
#define LV_FONT_MONTSERRAT_30 0   /**< 30号字体 */
#define LV_FONT_MONTSERRAT_32 0   /**< 32号字体 */
#define LV_FONT_MONTSERRAT_34 0   /**< 34号字体 */
#define LV_FONT_MONTSERRAT_36 0   /**< 36号字体 */
#define LV_FONT_MONTSERRAT_38 0   /**< 38号字体 */
#define LV_FONT_MONTSERRAT_40 0   /**< 40号字体 */
#define LV_FONT_MONTSERRAT_42 0   /**< 42号字体 */
#define LV_FONT_MONTSERRAT_44 0   /**< 44号字体 */
#define LV_FONT_MONTSERRAT_46 0   /**< 46号字体 */
#define LV_FONT_MONTSERRAT_48 0   /**< 48号字体 */

/* 特殊功能字体演示 */
#define LV_FONT_MONTSERRAT_28_COMPRESSED    0  /**< 压缩字体，bpp=3 */
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW    0  /**< 希伯来语、阿拉伯语、波斯语字母及其所有形式 */
#define LV_FONT_SOURCE_HAN_SANS_SC_14_CJK   0  /**< 1338个最常用的CJK部首 */
#define LV_FONT_SOURCE_HAN_SANS_SC_16_CJK   0  /**< 1338个最常用的CJK部首 */

/** 像素完美的等宽字体 */
#define LV_FONT_UNSCII_8  0   /**< 8号等宽字体 */
#define LV_FONT_UNSCII_16 0   /**< 16号等宽字体 */

/** 可在此处声明自定义字体
 *  这些字体可作为默认字体全局使用
 *  示例：
 *  @code
 *  #define LV_FONT_CUSTOM_DECLARE   LV_FONT_DECLARE(my_font_1) LV_FONT_DECLARE(my_font_2)
 *  @endcode
 */
#define LV_FONT_CUSTOM_DECLARE

/** 必须设置默认字体 */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/** 启用大字体和/或多字符字体的支持
 *  限制取决于字体大小、字体面和bpp
 *  如果字体需要，将触发编译器错误 */
#define LV_FONT_FMT_TXT_LARGE 0

/** 启用/禁用压缩字体支持 */
#define LV_USE_FONT_COMPRESSED 0

/** 当找不到字形描述时绘制占位符 */
#define LV_USE_FONT_PLACEHOLDER 1

/*=================
 * 文本设置
 *=================*/

/**
 * 选择字符串的字符编码
 * IDE或编辑器应使用相同的字符编码
 * - LV_TXT_ENC_UTF8   UTF-8编码（推荐）
 * - LV_TXT_ENC_ASCII  ASCII编码
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/** 渲染文本时，在这些字符处换行 */
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}"

/** 如果单词至少这么长，会在"最合适"的位置断开
 *  设置为<=0的值可禁用此功能 */
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/** 长单词在换行前放在一行的最小字符数
 *  依赖 LV_TXT_LINE_BREAK_LONG_LEN */
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/** 长单词在换行后放在一行的最小字符数
 *  依赖 LV_TXT_LINE_BREAK_LONG_LEN */
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/** 支持双向文本（混合从左到右和从右到左文本）
 *  方向将根据Unicode双向算法处理：
 *  https://www.w3.org/International/articles/inline-bidi-markup/uba-basics */
#define LV_USE_BIDI 0
#if LV_USE_BIDI
    /*设置默认方向，支持的值：
    *`LV_BASE_DIR_LTR` 从左到右
    *`LV_BASE_DIR_RTL` 从右到左
    *`LV_BASE_DIR_AUTO` 自动检测文本基础方向*/
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/** 启用阿拉伯语/波斯语字符处理
 *  在这些语言中，字符应根据其在文本中的位置替换为另一种形式 */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*用于文本重新着色的控制字符*/
#define LV_TXT_COLOR_CMD "#"

/*==================
 * 控件配置
 *================*/

/** 1: 使控件在创建时具有默认值
 *  - lv_buttonmatrix_t:  获取默认映射：{"Btn1", "Btn2", "Btn3", "\n", "Btn4", "Btn5", ""}
 *  - lv_checkbox_t    :  标签字符串设为 "Check box"，否则为空字符串
 *  - lv_dropdown_t    :  选项设为 "Option 1", "Option 2", "Option 3"
 *  - lv_roller_t      :  选项设为 "Option 1" 到 "Option 5"
 *  - lv_label_t       :  文本设为 "Text"，否则为空字符串
 *  - lv_arclabel_t    :  文本设为 "Arced Text"，否则为空字符串 */
#define LV_WIDGETS_HAS_DEFAULT_VALUE  1

#define LV_USE_ANIMIMG    1   /**< 动画图像控件 */

#define LV_USE_ARC        1   /**< 弧形控件 */

#define LV_USE_ARCLABEL   0   /**< 弧形标签控件 */

#define LV_USE_BAR        1   /**< 进度条控件 */

#define LV_USE_BUTTON     1   /**< 按钮控件 */

#define LV_USE_BUTTONMATRIX  0   /**< 按钮矩阵控件 */

#define LV_USE_CALENDAR   0   /**< 日历控件 */
#if LV_USE_CALENDAR
    #define LV_CALENDAR_WEEK_STARTS_MONDAY 0  /**< 周一为一周开始 */
    #if LV_CALENDAR_WEEK_STARTS_MONDAY
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
    #else
        #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
    #endif

    #define LV_CALENDAR_DEFAULT_MONTH_NAMES {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}
    #define LV_USE_CALENDAR_HEADER_ARROW 1    /**< 使用箭头样式日历头 */
    #define LV_USE_CALENDAR_HEADER_DROPDOWN 1 /**< 使用下拉样式日历头 */
    #define LV_USE_CALENDAR_CHINESE 0         /**< 中文日历支持 */
#endif  /*LV_USE_CALENDAR*/

#define LV_USE_CANVAS     0   /**< 画布控件 */

#define LV_USE_CHART      0   /**< 图表控件 */

#define LV_USE_CHECKBOX   0   /**< 复选框控件 */

#define LV_USE_DROPDOWN   0   /**< 下拉列表控件（需要lv_label） */

#define LV_USE_IMAGE      1   /**< 图像控件（需要lv_label） */

#define LV_USE_IMAGEBUTTON  0   /**< 图像按钮控件 */

#define LV_USE_KEYBOARD   0   /**< 键盘控件 */

#define LV_USE_LABEL      1   /**< 标签控件（文本显示） */
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1   /**< 启用标签文本选择 */
    #define LV_LABEL_LONG_TXT_HINT 1    /**< 在标签中存储额外信息以加速绘制长文本 */
    #define LV_LABEL_WAIT_CHAR_COUNT 3  /**< 等待字符数量 */
#endif

#define LV_USE_LED        0   /**< LED指示灯控件 */

#define LV_USE_LINE       0   /**< 线条控件 */

#define LV_USE_LIST       0   /**< 列表控件 */

#define LV_USE_LOTTIE     0   /**< Lottie动画控件（需要lv_canvas, thorvg） */

#define LV_USE_MENU       0   /**< 菜单控件 */

#define LV_USE_MSGBOX     0   /**< 消息框控件 */

#define LV_USE_ROLLER     0   /**< 滚轮选择器控件（需要lv_label） */

#define LV_USE_SCALE      0   /**< 刻度尺控件 */

#define LV_USE_SLIDER     0   /**< 滑块控件（需要lv_bar） */

#define LV_USE_SPAN       0   /**< 文本片段控件 */
#if LV_USE_SPAN
    /** 一行文本可包含的最大片段描述符数量 */
    #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif

#define LV_USE_SPINBOX    0   /**< 数字输入框控件 */

#define LV_USE_SPINNER    0   /**< 旋转器控件 */

#define LV_USE_SWITCH     0   /**< 开关控件 */

#define LV_USE_TABLE      0   /**< 表格控件 */

#define LV_USE_TABVIEW    0   /**< 标签页视图控件 */

#define LV_USE_TEXTAREA   0   /**< 文本区域控件（需要lv_label） */
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500    /**< [ms] 密码显示时间 */
#endif

#define LV_USE_TILEVIEW   0   /**< 瓦片视图控件 */

#define LV_USE_WIN        0   /**< 窗口控件 */

#define LV_USE_3DTEXTURE  0   /**< 3D纹理控件 */

/*==================
 * 主题配置
 *==================*/

/** 简单、美观且功能完整的主题 */
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    /** 0: 浅色模式; 1: 深色模式 */
    #define LV_THEME_DEFAULT_DARK 0

    /** 1: 启用按下时放大效果 */
    #define LV_THEME_DEFAULT_GROW 1

    /** 默认过渡动画时间（毫秒） */
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif /*LV_USE_THEME_DEFAULT*/

/** 非常简单的主题，是自定义主题的良好起点 */
#define LV_USE_THEME_SIMPLE 1

/** 为单色显示器设计的主题 */
#define LV_USE_THEME_MONO 1

/*==================
 * 布局配置
 *==================*/

/** 类似CSS Flexbox的布局 */
#define LV_USE_FLEX 1

/** 类似CSS Grid的布局 */
#define LV_USE_GRID 1

/*====================
 * 第三方库配置
 *====================*/

/** 设置默认驱动器字母可跳过文件路径中的驱动器前缀
 *  文档：https://docs.lvgl.io/master/details/main-modules/fs.html#lv-fs-identifier-letters */
#define LV_FS_DEFAULT_DRIVER_LETTER '\0'

/** fopen, fread等API接口 */
#define LV_USE_FS_STDIO 0
#if LV_USE_FS_STDIO
    #define LV_FS_STDIO_LETTER '\0'     /**< 设置大写驱动器标识字母（如 'A'） */
    #define LV_FS_STDIO_PATH ""         /**< 设置工作目录 */
    #define LV_FS_STDIO_CACHE_SIZE 0    /**< >0时缓存指定字节数 */
#endif

/** open, read等POSIX API接口 */
#define LV_USE_FS_POSIX 0
#if LV_USE_FS_POSIX
    #define LV_FS_POSIX_LETTER '\0'     /**< 驱动器标识字母 */
    #define LV_FS_POSIX_PATH ""         /**< 工作目录 */
    #define LV_FS_POSIX_CACHE_SIZE 0    /**< 缓存大小 */
#endif

/** CreateFile, ReadFile等Windows API接口 */
#define LV_USE_FS_WIN32 0
#if LV_USE_FS_WIN32
    #define LV_FS_WIN32_LETTER '\0'     /**< 驱动器标识字母 */
    #define LV_FS_WIN32_PATH ""         /**< 工作目录 */
    #define LV_FS_WIN32_CACHE_SIZE 0    /**< 缓存大小 */
#endif

/** FATFS API接口（需单独添加） */
#define LV_USE_FS_FATFS 0
#if LV_USE_FS_FATFS
    #define LV_FS_FATFS_LETTER '\0'     /**< 驱动器标识字母 */
    #define LV_FS_FATFS_PATH ""         /**< 工作目录 */
    #define LV_FS_FATFS_CACHE_SIZE 0    /**< 缓存大小 */
#endif

/** 内存映射文件访问API */
#define LV_USE_FS_MEMFS 0
#if LV_USE_FS_MEMFS
    #define LV_FS_MEMFS_LETTER '\0'     /**< 驱动器标识字母 */
#endif

/** LittleFs API接口 */
#define LV_USE_FS_LITTLEFS 0
#if LV_USE_FS_LITTLEFS
    #define LV_FS_LITTLEFS_LETTER '\0'  /**< 驱动器标识字母 */
    #define LV_FS_LITTLEFS_PATH ""      /**< 工作目录 */
#endif

/** Arduino LittleFs API接口 */
#define LV_USE_FS_ARDUINO_ESP_LITTLEFS 0
#if LV_USE_FS_ARDUINO_ESP_LITTLEFS
    #define LV_FS_ARDUINO_ESP_LITTLEFS_LETTER '\0'
    #define LV_FS_ARDUINO_ESP_LITTLEFS_PATH ""
#endif

/** Arduino SD卡API接口 */
#define LV_USE_FS_ARDUINO_SD 0
#if LV_USE_FS_ARDUINO_SD
    #define LV_FS_ARDUINO_SD_LETTER '\0'
    #define LV_FS_ARDUINO_SD_PATH ""
#endif

/** UEFI文件系统API */
#define LV_USE_FS_UEFI 0
#if LV_USE_FS_UEFI
    #define LV_FS_UEFI_LETTER '\0'
#endif

/** LODEPNG解码库 */
#define LV_USE_LODEPNG 0

/** PNG解码库（libpng） */
#define LV_USE_LIBPNG 0

/** BMP解码库 */
#define LV_USE_BMP 0

/** JPG + 分割JPG解码库
 *  分割JPG是为嵌入式系统优化的自定义格式 */
#define LV_USE_TJPGD 0

/** libjpeg-turbo解码库
 *  支持完整JPEG规范和高性能JPEG解码 */
#define LV_USE_LIBJPEG_TURBO 0

/** GIF解码库 */
#define LV_USE_GIF 0
#if LV_USE_GIF
    /** GIF解码加速 */
    #define LV_GIF_CACHE_DECODE_DATA 0
#endif

/** GStreamer多媒体库 */
#define LV_USE_GSTREAMER 0

/** 将二进制图像解码到RAM */
#define LV_BIN_DECODER_RAM_LOAD 0

/** RLE解压缩库 */
#define LV_USE_RLE 0

/** QR码库 */
#define LV_USE_QRCODE 0

/** 条形码库 */
#define LV_USE_BARCODE 0

/** FreeType字体库 */
#define LV_USE_FREETYPE 0
#if LV_USE_FREETYPE
    /** 让FreeType使用LVGL内存和文件移植 */
    #define LV_FREETYPE_USE_LVGL_PORT 0

    /** FreeType中缓存的字形数量
     *  值越大，使用的内存越多 */
    #define LV_FREETYPE_CACHE_FT_GLYPH_CNT 256
#endif

/** 内置TTF解码器（轻量级） */
#define LV_USE_TINY_TTF 0
#if LV_USE_TINY_TTF
    /* 启用从文件加载TTF数据 */
    #define LV_TINY_TTF_FILE_SUPPORT 0
    #define LV_TINY_TTF_CACHE_GLYPH_CNT 128     /**< 字形缓存数量 */
    #define LV_TINY_TTF_CACHE_KERNING_CNT 256   /**< 字距调整缓存数量 */
#endif

/** Rlottie动画库 */
#define LV_USE_RLOTTIE 0

/** GLTF 3D模型库（需要 `LV_USE_3DTEXTURE = 1`） */
#define LV_USE_GLTF  0

/** 启用矢量图形API（需要 `LV_USE_MATRIX = 1`） */
#define LV_USE_VECTOR_GRAPHIC  0

/** 启用src/libs文件夹中的ThorVG矢量图形库 */
#define LV_USE_THORVG_INTERNAL 0

/** 启用已安装并链接到项目的ThorVG */
#define LV_USE_THORVG_EXTERNAL 0

/** 使用LVGL内置的LZ4压缩库 */
#define LV_USE_LZ4_INTERNAL  0

/** 使用外部LZ4压缩库 */
#define LV_USE_LZ4_EXTERNAL  0

/** SVG矢量图形库（需要 `LV_USE_VECTOR_GRAPHIC = 1`） */
#define LV_USE_SVG 0
#define LV_USE_SVG_ANIMATION 0  /**< SVG动画支持 */
#define LV_USE_SVG_DEBUG 0      /**< SVG调试功能 */

/** FFmpeg多媒体库（图像解码和视频播放）
 *  支持所有主要图像格式，启用后无需其他图像解码器 */
#define LV_USE_FFMPEG 0
#if LV_USE_FFMPEG
    /** 将输入信息转储到stderr */
    #define LV_FFMPEG_DUMP_FORMAT 0
    /** 在FFmpeg播放器控件中使用lvgl文件路径
     *  启用此功能后将无法打开URL
     *  FFmpeg图像解码器始终使用lvgl文件系统 */
    #define LV_FFMPEG_PLAYER_USE_LV_FS 0
#endif

/*==================
 * 其他功能
 *==================*/

/** 1: 启用对象快照API */
#define LV_USE_SNAPSHOT 0

/** 1: 启用系统监控组件 */
#define LV_USE_SYSMON   0
#if LV_USE_SYSMON
    /** 获取空闲百分比函数，例如 uint32_t my_get_idle(void); */
    #define LV_SYSMON_GET_IDLE lv_os_get_idle_percent
    /** 1: 启用lv_os_get_proc_idle_percent */
    #define LV_SYSMON_PROC_IDLE_AVAILABLE 0
    #if LV_SYSMON_PROC_IDLE_AVAILABLE
        /** 获取应用程序空闲百分比（需要 `LV_USE_OS == LV_OS_PTHREAD`） */
        #define LV_SYSMON_GET_PROC_IDLE lv_os_get_proc_idle_percent
    #endif

    /** 1: 显示CPU使用率和FPS计数（需要 `LV_USE_SYSMON = 1`） */
    #define LV_USE_PERF_MONITOR 0
    #if LV_USE_PERF_MONITOR
        #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT  /**< 显示位置 */

        /** 0: 在屏幕上显示性能数据; 1: 使用日志打印性能数据 */
        #define LV_USE_PERF_MONITOR_LOG_MODE 0
    #endif

    /** 1: 显示已使用内存和内存碎片
     *     要求：`LV_USE_STDLIB_MALLOC = LV_STDLIB_BUILTIN`
     *     要求：`LV_USE_SYSMON = 1` */
    #define LV_USE_MEM_MONITOR 0
    #if LV_USE_MEM_MONITOR
        #define LV_USE_MEM_MONITOR_POS LV_ALIGN_BOTTOM_LEFT  /**< 显示位置 */
    #endif
#endif /*LV_USE_SYSMON*/

/** 1: 启用运行时性能分析器 */
#define LV_USE_PROFILER 0
#if LV_USE_PROFILER
    /** 1: 启用内置性能分析器 */
    #define LV_USE_PROFILER_BUILTIN 1
    #if LV_USE_PROFILER_BUILTIN
        /** 默认性能分析器跟踪缓冲区大小 */
        #define LV_PROFILER_BUILTIN_BUF_SIZE (16 * 1024)     /**< [字节] */
        #define LV_PROFILER_BUILTIN_DEFAULT_ENABLE 1
        #define LV_USE_PROFILER_BUILTIN_POSIX 0 /**< 启用POSIX性能分析器移植 */
    #endif

    /** 性能分析器头文件 */
    #define LV_PROFILER_INCLUDE "lvgl/src/misc/lv_profiler_builtin.h"

    /** 性能分析器开始点函数 */
    #define LV_PROFILER_BEGIN    LV_PROFILER_BUILTIN_BEGIN

    /** 性能分析器结束点函数 */
    #define LV_PROFILER_END      LV_PROFILER_BUILTIN_END

    /** 带自定义标签的性能分析器开始点 */
    #define LV_PROFILER_BEGIN_TAG LV_PROFILER_BUILTIN_BEGIN_TAG

    /** 带自定义标签的性能分析器结束点 */
    #define LV_PROFILER_END_TAG   LV_PROFILER_BUILTIN_END_TAG

    /* 各模块性能分析开关 */
    #define LV_PROFILER_LAYOUT 1    /**< 布局性能分析 */
    #define LV_PROFILER_REFR 1      /**< 显示刷新性能分析 */
    #define LV_PROFILER_DRAW 1      /**< 绘制性能分析 */
    #define LV_PROFILER_INDEV 1     /**< 输入设备性能分析 */
    #define LV_PROFILER_DECODER 1   /**< 解码器性能分析 */
    #define LV_PROFILER_FONT 1      /**< 字体性能分析 */
    #define LV_PROFILER_FS 1        /**< 文件系统性能分析 */
    #define LV_PROFILER_STYLE 0     /**< 样式性能分析 */
    #define LV_PROFILER_TIMER 1     /**< 定时器性能分析 */
    #define LV_PROFILER_CACHE 1     /**< 缓存性能分析 */
    #define LV_PROFILER_EVENT 1     /**< 事件性能分析 */
#endif

/** 1: 启用Monkey测试（随机输入测试） */
#define LV_USE_MONKEY 0

/** 1: 启用网格导航 */
#define LV_USE_GRIDNAV 0

/** 1: 启用 `lv_obj` 片段逻辑 */
#define LV_USE_FRAGMENT 0

/** 1: 支持在标签或片段控件中使用图像作为字体 */
#define LV_USE_IMGFONT 0

/** 1: 启用观察者模式实现 */
#define LV_USE_OBSERVER 1

/** 1: 启用拼音输入法（需要lv_keyboard） */
#define LV_USE_IME_PINYIN 0
#if LV_USE_IME_PINYIN
    /** 1: 使用默认词典
     *  注意：如果不使用默认词典，请确保在设置词典后再使用 `lv_ime_pinyin` */
    #define LV_IME_PINYIN_USE_DEFAULT_DICT 1
    /** 最大候选面板显示数量（需根据屏幕大小调整） */
    #define LV_IME_PINYIN_CAND_TEXT_NUM 6

    /** 使用9键输入模式 */
    #define LV_IME_PINYIN_USE_K9_MODE      1
    #if LV_IME_PINYIN_USE_K9_MODE == 1
        #define LV_IME_PINYIN_K9_CAND_TEXT_NUM 3  /**< 9键候选文本数量 */
    #endif /*LV_IME_PINYIN_USE_K9_MODE*/
#endif

/** 1: 启用文件浏览器（需要lv_table） */
#define LV_USE_FILE_EXPLORER                     0
#if LV_USE_FILE_EXPLORER
    /** 路径最大长度 */
    #define LV_FILE_EXPLORER_PATH_MAX_LEN        (128)
    /** 快速访问栏（需要lv_list）
     *  1: 使用, 0: 不使用 */
    #define LV_FILE_EXPLORER_QUICK_ACCESS        1
#endif

/** 1: 启用字体管理器 */
#define LV_USE_FONT_MANAGER                     0
#if LV_USE_FONT_MANAGER
    /** 字体管理器名称最大长度 */
    #define LV_FONT_MANAGER_NAME_MAX_LEN            32
#endif

/** 启用模拟输入设备、时间模拟和屏幕截图比较 */
#define LV_USE_TEST 0
#if LV_USE_TEST
    /** 启用 `lv_test_screenshot_compare`（需要lodepng和几MB额外RAM） */
    #define LV_USE_TEST_SCREENSHOT_COMPARE 0
#endif /*LV_USE_TEST*/

/** 启用运行时加载XML UI */
#define LV_USE_XML    0

/** 1: 启用文本翻译支持 */
#define LV_USE_TRANSLATION 0

/** 1: 启用颜色滤镜样式 */
#define LV_USE_COLOR_FILTER     0

/*==================
 * 设备驱动配置
 *==================*/

/** 使用SDL在PC上打开窗口并处理鼠标和键盘 */
#define LV_USE_SDL              0
#if LV_USE_SDL
    #define LV_SDL_INCLUDE_PATH     <SDL2/SDL.h>
    #define LV_SDL_RENDER_MODE      LV_DISPLAY_RENDER_MODE_DIRECT   /**< 推荐使用直接模式以获得最佳性能 */
    #define LV_SDL_BUF_COUNT        1    /**< 1或2个缓冲区 */
    #define LV_SDL_ACCELERATED      1    /**< 1: 使用硬件加速 */
    #define LV_SDL_FULLSCREEN       0    /**< 1: 默认全屏 */
    #define LV_SDL_DIRECT_EXIT      1    /**< 1: 关闭所有SDL窗口时退出应用 */
    #define LV_SDL_MOUSEWHEEL_MODE  LV_SDL_MOUSEWHEEL_MODE_ENCODER  /*LV_SDL_MOUSEWHEEL_MODE_ENCODER/CROWN*/
#endif

/** 使用X11在Linux桌面上打开窗口并处理鼠标和键盘 */
#define LV_USE_X11              0
#if LV_USE_X11
    #define LV_X11_DIRECT_EXIT         1  /**< 关闭所有X11窗口时退出应用 */
    #define LV_X11_DOUBLE_BUFFER       1  /**< 使用双缓冲渲染 */
    /* 渲染模式（仅选择1种，推荐LV_X11_RENDER_MODE_PARTIAL） */
    #define LV_X11_RENDER_MODE_PARTIAL 1  /**< 部分渲染模式（推荐） */
    #define LV_X11_RENDER_MODE_DIRECT  0  /**< 直接渲染模式 */
    #define LV_X11_RENDER_MODE_FULL    0  /**< 完全渲染模式 */
#endif

/** 使用Wayland在Linux/BSD桌面上打开窗口并处理输入 */
#define LV_USE_WAYLAND          0
#if LV_USE_WAYLAND
    #define LV_WAYLAND_BUF_COUNT            1    /**< 单缓冲用1，双缓冲用2 */
    #define LV_WAYLAND_USE_DMABUF           0    /**< 使用DMA缓冲区（需要LV_DRAW_USE_G2D） */
    #define LV_WAYLAND_RENDER_MODE          LV_DISPLAY_RENDER_MODE_PARTIAL
    #define LV_WAYLAND_WINDOW_DECORATIONS   0    /**< 绘制客户端窗口装饰（仅Mutter/GNOME需要） */
#endif

/** Linux帧缓冲设备驱动（/dev/fb） */
#define LV_USE_LINUX_FBDEV      0
#if LV_USE_LINUX_FBDEV
    #define LV_LINUX_FBDEV_BSD           0      /**< BSD系统支持 */
    #define LV_LINUX_FBDEV_RENDER_MODE   LV_DISPLAY_RENDER_MODE_PARTIAL
    #define LV_LINUX_FBDEV_BUFFER_COUNT  0      /**< 缓冲区数量（0=自动） */
    #define LV_LINUX_FBDEV_BUFFER_SIZE   60     /**< 缓冲区大小（百分比） */
    #define LV_LINUX_FBDEV_MMAP          1      /**< 使用内存映射 */
#endif

/** NuttX操作系统支持 */
#define LV_USE_NUTTX    0
#if LV_USE_NUTTX
    #define LV_USE_NUTTX_INDEPENDENT_IMAGE_HEAP 0  /**< 使用独立图像堆 */

    /** 为默认绘制缓冲区使用独立图像堆 */
    #define LV_NUTTX_DEFAULT_DRAW_BUF_USE_INDEPENDENT_IMAGE_HEAP    0

    #define LV_USE_NUTTX_LIBUV    0  /**< 使用libuv事件循环 */

    /** 使用NuttX自定义初始化API */
    #define LV_USE_NUTTX_CUSTOM_INIT    0

    /** LCD设备驱动（/dev/lcd） */
    #define LV_USE_NUTTX_LCD      0
    #if LV_USE_NUTTX_LCD
        #define LV_NUTTX_LCD_BUFFER_COUNT    0
        #define LV_NUTTX_LCD_BUFFER_SIZE     60
    #endif

    /** 触摸屏设备驱动（/dev/input） */
    #define LV_USE_NUTTX_TOUCHSCREEN    0

    /** 触摸屏光标大小（像素，<=0禁用光标） */
    #define LV_NUTTX_TOUCHSCREEN_CURSOR_SIZE    0

    /** 鼠标设备驱动（/dev/mouse） */
    #define LV_USE_NUTTX_MOUSE    0

    /** 鼠标移动步长（像素） */
    #define LV_USE_NUTTX_MOUSE_MOVE_STEP    1

    /** NuttX跟踪文件 */
    #define LV_USE_NUTTX_TRACE_FILE 0
    #if LV_USE_NUTTX_TRACE_FILE
        #define LV_NUTTX_TRACE_FILE_PATH "/data/lvgl-trace.log"
    #endif
#endif

/** Linux DRM显示驱动（/dev/dri/card） */
#define LV_USE_LINUX_DRM        0
#if LV_USE_LINUX_DRM
    /* 使用MESA GBM库分配可通过Linux DMA-BUF API共享的DMA缓冲区 */
    #define LV_USE_LINUX_DRM_GBM_BUFFERS 0

    #define LV_LINUX_DRM_USE_EGL     0  /**< 使用EGL加速 */
#endif

/** TFT_eSPI接口 */
#define LV_USE_TFT_ESPI         0

/** Lovyan_GFX接口 */
#define LV_USE_LOVYAN_GFX         0
#if LV_USE_LOVYAN_GFX
    #define LV_LGFX_USER_INCLUDE "lv_lgfx_user.hpp"  /**< 用户配置头文件 */
#endif /*LV_USE_LOVYAN_GFX*/

/** evdev输入设备驱动 */
#define LV_USE_EVDEV    0

/** libinput输入设备驱动 */
#define LV_USE_LIBINPUT    0
#if LV_USE_LIBINPUT
    #define LV_LIBINPUT_BSD    0  /**< BSD系统支持 */

    /** 完整键盘支持 */
    #define LV_LIBINPUT_XKB             0
    #if LV_LIBINPUT_XKB
        /** 键盘映射配置（可用 "setxkbmap -query" 查找正确值） */
        #define LV_LIBINPUT_XKB_KEY_MAP { .rules = NULL, .model = "pc101", .layout = "us", .variant = NULL, .options = NULL }
    #endif
#endif

/* SPI/并口LCD设备驱动 */
#define LV_USE_ST7735        0   /**< ST7735 LCD驱动 */
#define LV_USE_ST7796        1   /**< ST7796 LCD驱动 */
#define LV_USE_ILI9341       0   /**< ILI9341 LCD驱动 */
#define LV_USE_FT81X         0   /**< FT81X EVE驱动 */


#if (LV_USE_ST7735 | LV_USE_ST7789 | LV_USE_ST7796 | LV_USE_ILI9341)
    #define LV_USE_GENERIC_MIPI 1  /**< 启用通用MIPI接口 */
#else
    #define LV_USE_GENERIC_MIPI 0
#endif

/** Renesas GLCD显示驱动 */
#define LV_USE_RENESAS_GLCDC    0

/** ST LTDC显示驱动 */
#define LV_USE_ST_LTDC    0
#if LV_USE_ST_LTDC
    /* 仅用于部分渲染模式 */
    #define LV_ST_LTDC_USE_DMA2D_FLUSH 0
#endif

/** NXP ELCDIF显示驱动 */
#define LV_USE_NXP_ELCDIF   0

/** LVGL Windows后端 */
#define LV_USE_WINDOWS    0

/** LVGL UEFI后端 */
#define LV_USE_UEFI 0
#if LV_USE_UEFI
    #define LV_USE_UEFI_INCLUDE "myefi.h"   /**< 隐藏实际框架的头文件 */
    #define LV_UEFI_USE_MEMORY_SERVICES 0   /**< 使用引导服务表中的内存函数 */
#endif

/** 通用OpenGL驱动（可嵌入其他应用或与GLFW/EGL配合使用） */
#define LV_USE_OPENGLES   0
#if LV_USE_OPENGLES
    #define LV_USE_OPENGLES_DEBUG        1    /**< 启用OpenGL ES调试 */
#endif

/** 使用GLFW在PC上打开窗口并处理鼠标和键盘 */
#define LV_USE_GLFW   0

/** QNX Screen显示和输入驱动 */
#define LV_USE_QNX              0
#if LV_USE_QNX
    #define LV_QNX_BUF_COUNT        1    /**< 1或2个缓冲区 */
#endif

/*=====================
 * 编译选项
 *======================*/

/** 启用示例程序编译 */
#define LV_BUILD_EXAMPLES 1

/** 启用演示程序编译 */
#define LV_BUILD_DEMOS 1

/*===================
 * 演示程序配置
 ====================*/

#if LV_BUILD_DEMOS
    /** 控件演示（可能需要增加 `LV_MEM_SIZE`） */
    #define LV_USE_DEMO_WIDGETS 0

    /** 编码器和键盘使用演示 */
    #define LV_USE_DEMO_KEYPAD_AND_ENCODER 0

    /** 系统性能基准测试 */
    #define LV_USE_DEMO_BENCHMARK 0
    #if LV_USE_DEMO_BENCHMARK
        /** 使用位图16字节对齐且有Nx16字节步长的字体 */
        #define LV_DEMO_BENCHMARK_ALIGNED_FONTS 0
    #endif

    /** 各图元渲染测试（至少需要480x272显示屏） */
    #define LV_USE_DEMO_RENDER 0

    /** LVGL压力测试 */
    #define LV_USE_DEMO_STRESS 0

    /** 音乐播放器演示 */
    #define LV_USE_DEMO_MUSIC 0
    #if LV_USE_DEMO_MUSIC
        #define LV_DEMO_MUSIC_SQUARE    0    /**< 方形布局 */
        #define LV_DEMO_MUSIC_LANDSCAPE 0    /**< 横屏布局 */
        #define LV_DEMO_MUSIC_ROUND     0    /**< 圆形布局 */
        #define LV_DEMO_MUSIC_LARGE     0    /**< 大屏幕布局 */
        #define LV_DEMO_MUSIC_AUTO_PLAY 0    /**< 自动播放 */
    #endif

    /** 矢量图形演示 */
    #define LV_USE_DEMO_VECTOR_GRAPHIC  0

    /** GLTF 3D模型演示 */
    #define LV_USE_DEMO_GLTF            0

    /*---------------------------
     * lvgl/lv_demos 演示程序
      ---------------------------*/

    /** Flex布局演示 */
    #define LV_USE_DEMO_FLEX_LAYOUT     0

    /** 智能手机风格多语言演示 */
    #define LV_USE_DEMO_MULTILANG       0

    /** 控件变换演示 */
    #define LV_USE_DEMO_TRANSFORM       0

    /** 滚动设置演示 */
    #define LV_USE_DEMO_SCROLL          0

    /** 电动自行车演示（可配合Lottie动画） */
    #define LV_USE_DEMO_EBIKE           0
    #if LV_USE_DEMO_EBIKE
        #define LV_DEMO_EBIKE_PORTRAIT  0    /*0: 480x270..480x320, 1: 480x800..720x1280*/
    #endif

    /** 高分辨率演示 */
    #define LV_USE_DEMO_HIGH_RES        0

    /** 智能手表演示 */
    #define LV_USE_DEMO_SMARTWATCH      0
#endif /* LV_BUILD_DEMOS */

/*--LV_CONF_H配置文件结束--*/

#endif /*LV_CONF_H*/

#endif /*启用配置内容*/