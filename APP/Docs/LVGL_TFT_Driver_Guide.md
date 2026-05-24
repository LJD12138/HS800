# LVGL TFT显示屏驱动使用指南

## 概述

本文档说明如何在HS800项目中使用LVGL图形库驱动ZJY240KP-IF10 TFT显示屏。

## 硬件规格

| 参数 | 规格 |
|------|------|
| 屏幕型号 | ZJY240KP-IF10 |
| 尺寸 | 2.4英寸 |
| 分辨率 | 240×320像素 |
| 颜色深度 | 16位(RGB565) |
| 驱动芯片 | ST7789V2 |
| 接口 | 4线SPI |

## 引脚配置

| 功能 | 引脚 | 说明 |
|------|------|------|
| CS | PC8 | 片选信号 |
| RES | PC7 | 复位信号 |
| BL | PC6 | 背光控制 |
| SDA | PB15 | SPI数据线 |
| SCK | PB13 | SPI时钟线 |
| A0 | PB14 | 数据/命令选择 |

## 文件结构

```
Hardware/MD_Display/
├── lv_port_tft_gpio.h      # GPIO控制接口头文件
├── lv_port_tft_gpio.c      # GPIO控制接口实现
├── lv_port_tft_disp.h      # TFT显示驱动头文件
├── lv_port_tft_disp.c      # TFT显示驱动实现
├── lv_port_timer.h         # LVGL定时器头文件
├── lv_port_timer.c         # LVGL定时器实现
├── tft_minimal_demo.c      # 最小演示程序
└── lv_port_disp.c          # LVGL显示接口集成
```

## 使用方法

### 1. 基本初始化

```c
#include "MD_Display/tft_minimal_demo.h"

void main(void)
{
    // 系统初始化...
    
    // 启动TFT演示
    vTFT_MinimalDemo();
}
```

### 2. 自定义LVGL应用

```c
#include "lvgl.h"
#include "MD_Display/lv_port_disp.h"
#include "MD_Display/lv_port_timer.h"

void custom_display_app(void)
{
    // 初始化LVGL
    lv_init();
    
    // 初始化定时器
    vLV_TimerInit();
    
    // 初始化显示驱动
    lv_port_disp_init();
    
    // 创建自定义UI
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello TFT!");
    lv_obj_center(label);
    
    // 主循环
    while(1)
    {
        lv_timer_handler();
        HAL_Delay(5);
    }
}
```

### 3. 直接操作TFT函数

```c
#include "MD_Display/lv_port_tft_disp.h"
#include "MD_Display/lv_port_tft_gpio.h"

void direct_tft_control(void)
{
    // 初始化TFT
    vTFT_DispIfaceInit();
    
    // 开启背光
    vTFT_BlOn();
    
    // 清屏为红色
    vTFT_Clear(0xF800);
    
    // 填充矩形区域
    vTFT_FillRect(10, 10, 100, 100, 0x07E0);
    
    // 设置显示方向
    vTFT_SetRotation(1);  // 横屏
}
```

## 配置说明

### LVGL配置 (lv_conf.h)

```c
// 屏幕分辨率配置
#define MY_DISP_HOR_RES    240    // 水平分辨率
#define MY_DISP_VER_RES    320    // 垂直分辨率

// 颜色深度
#define LV_COLOR_DEPTH 16

// 启用ST7789驱动支持
#define LV_USE_ST7789        1
```

### 内存配置

```c
// LVGL内存池大小
#define LV_MEM_SIZE (64 * 1024U)  // 64KB

// 显示缓冲区大小
// 在lv_port_disp.c中配置：
#define MY_DISP_HOR_RES * 100  // 100行缓冲
```

## 注意事项

1. **SPI通信**: 使用软件SPI实现，速度较慢但兼容性好
2. **内存使用**: LVGL需要较多内存，确保堆栈空间充足
3. **中断处理**: SysTick中断用于LVGL心跳，已在gd32f30x_it.c中配置
4. **背光控制**: 默认初始化时关闭背光，需要手动开启

## 调试技巧

1. 如果屏幕无显示，检查：
   - 背光是否开启
   - SPI引脚连接
   - 电源电压是否正常

2. 如果LVGL无响应，检查：
   - SysTick是否正常工作
   - lv_timer_handler()是否被调用
   - 内存是否充足

## 版本信息

- LVGL版本: 9.4.0-dev
- 驱动版本: 1.0
- 最后更新: 2026-05-24