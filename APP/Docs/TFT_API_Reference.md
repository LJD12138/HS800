# TFT显示屏 API参考文档

## 概述

本文档详细说明TFT显示屏驱动的所有API函数，包括GPIO控制、SPI通信、显示控制等功能。

## 目录

1. [GPIO控制接口](#1-gpio控制接口)
2. [TFT显示接口](#2-tft显示接口)
3. [LVGL定时器接口](#3-lvgl定时器接口)
4. [LVGL显示接口](#4-lvgl显示接口)

---

## 1. GPIO控制接口

头文件: `lv_port_tft_gpio.h`

### 函数列表

#### vTFT_GpioInit

**功能**: 初始化所有TFT相关的GPIO引脚

**函数原型**:
```c
void vTFT_GpioInit(void);
```

**参数**: 无

**返回值**: 无

**说明**: 
- 配置CS、RES、BL、SDA、SCK、A0引脚为推挽输出模式
- 使能GPIOB和GPIOC时钟
- 设置初始状态：CS高、RES高、BL关闭

---

#### vTFT_CsLow / vTFT_CsHigh

**功能**: 控制片选信号

**函数原型**:
```c
void vTFT_CsLow(void);
void vTFT_CsHigh(void);
```

**参数**: 无

**返回值**: 无

**说明**: 
- CS低电平选中TFT芯片
- CS高电平取消选中

---

#### vTFT_ResLow / vTFT_ResHigh

**功能**: 控制复位信号

**函数原型**:
```c
void vTFT_ResLow(void);
void vTFT_ResHigh(void);
```

**参数**: 无

**返回值**: 无

**说明**: 
- RES低电平复位芯片
- RES高电平正常工作

---

#### vTFT_BlOff / vTFT_BlOn

**功能**: 控制背光

**函数原型**:
```c
void vTFT_BlOff(void);
void vTFT_BlOn(void);
```

**参数**: 无

**返回值**: 无

**说明**: 
- BlOff关闭背光
- BlOn开启背光

---

#### vTFT_A0Low / vTFT_A0High

**功能**: 控制数据/命令选择

**函数原型**:
```c
void vTFT_A0Low(void);
void vTFT_A0High(void);
```

**参数**: 无

**返回值**: 无

**说明**: 
- A0低电平表示发送命令
- A0高电平表示发送数据

---

#### vTFT_SdaLow / vTFT_SdaHigh

**功能**: 控制SPI数据线

**函数原型**:
```c
void vTFT_SdaLow(void);
void vTFT_SdaHigh(void);
```

**参数**: 无

**返回值**: 无

---

#### vTFT_SckLow / vTFT_SckHigh

**功能**: 控制SPI时钟线

**函数原型**:
```c
void vTFT_SckLow(void);
void vTFT_SckHigh(void);
```

**参数**: 无

**返回值**: 无

---

#### vTFT_DelayMs

**功能**: 毫秒延时

**函数原型**:
```c
void vTFT_DelayMs(uint32_t ms);
```

**参数**:
- `ms`: 延时毫秒数

**返回值**: 无

---

#### vTFT_DelayUs

**功能**: 微秒延时

**函数原型**:
```c
void vTFT_DelayUs(uint32_t us);
```

**参数**:
- `us`: 延时微秒数

**返回值**: 无

**说明**: 软件延时，精度依赖系统时钟

---

## 2. TFT显示接口

头文件: `lv_port_tft_disp.h`

### ST7789V2命令定义

| 命令 | 值 | 说明 |
|------|-----|------|
| ST7789_NOP | 0x00 | 空操作 |
| ST7789_SWRESET | 0x01 | 软件复位 |
| ST7789_SLPIN | 0x10 | 进入睡眠 |
| ST7789_SLPOUT | 0x11 | 退出睡眠 |
| ST7789_INVOFF | 0x20 | 关闭反转 |
| ST7789_INVON | 0x21 | 开启反转 |
| ST7789_DISPOFF | 0x28 | 关闭显示 |
| ST7789_DISPON | 0x29 | 开启显示 |
| ST7789_CASET | 0x2A | 列地址设置 |
| ST7789_RASET | 0x2B | 行地址设置 |
| ST7789_RAMWR | 0x2C | 写显存 |
| ST7789_RAMRD | 0x2E | 读显存 |
| ST7789_MADCTL | 0x36 | 内存访问控制 |
| ST7789_COLMOD | 0x3A | 颜色模式设置 |

### MADCTL位定义

| 位 | 值 | 说明 |
|----|-----|------|
| ST7789_MADCTL_MY | 0x80 | 行地址顺序 |
| ST7789_MADCTL_MX | 0x40 | 列地址顺序 |
| ST7789_MADCTL_MV | 0x20 | 行列交换 |
| ST7789_MADCTL_ML | 0x10 | 垂直刷新顺序 |
| ST7789_MADCTL_RGB | 0x00 | RGB顺序 |
| ST7789_MADCTL_BGR | 0x08 | BGR顺序 |

### 函数列表

#### vTFT_DispIfaceInit

**功能**: TFT显示接口初始化

**函数原型**:
```c
void vTFT_DispIfaceInit(void);
```

**参数**: 无

**返回值**: 无

**说明**: 
- 初始化GPIO
- 执行ST7789V2初始化序列
- 开启背光
- 清屏为黑色

**初始化序列**:
1. 硬件复位
2. 软件复位
3. 退出睡眠模式
4. 设置颜色模式(RGB565)
5. 设置内存访问控制
6. 设置显示区域(240x320)
7. 开启显示反转
8. 开启显示

---

#### vTFT_WriteCmd

**功能**: 向TFT发送命令

**函数原型**:
```c
void vTFT_WriteCmd(uint8_t cmd);
```

**参数**:
- `cmd`: 命令字节

**返回值**: 无

**说明**: 
- CS拉低
- A0拉低表示命令
- 通过SPI发送命令
- CS拉高

---

#### vTFT_WriteData

**功能**: 向TFT发送单字节数据

**函数原型**:
```c
void vTFT_WriteData(uint8_t data);
```

**参数**:
- `data`: 数据字节

**返回值**: 无

---

#### vTFT_WriteData16

**功能**: 向TFT发送16位数据

**函数原型**:
```c
void vTFT_WriteData16(uint16_t data);
```

**参数**:
- `data`: 16位数据

**返回值**: 无

**说明**: 高字节先发送

---

#### vTFT_WriteMultipleData

**功能**: 向TFT发送多字节数据

**函数原型**:
```c
void vTFT_WriteMultipleData(uint8_t *data, uint32_t len);
```

**参数**:
- `data`: 数据缓冲区指针
- `len`: 数据长度

**返回值**: 无

---

#### vTFT_SetWindow

**功能**: 设置显示窗口

**函数原型**:
```c
void vTFT_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
```

**参数**:
- `x1`: 起始列坐标(0-239)
- `y1`: 起始行坐标(0-319)
- `x2`: 结束列坐标(0-239)
- `y2`: 结束行坐标(0-319)

**返回值**: 无

**说明**: 设置后会自动进入RAMWR模式

---

#### vTFT_FastDrawColor

**功能**: 快速绘制颜色数据

**函数原型**:
```c
void vTFT_FastDrawColor(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *color);
```

**参数**:
- `x1`: 起始列坐标
- `y1`: 起始行坐标
- `x2`: 结束列坐标
- `y2`: 结束行坐标
- `color`: RGB565颜色数据数组

**返回值**: 无

**说明**: 
- LVGL刷新回调使用的函数
- 高效绘制矩形区域的像素数据

**示例**:
```c
uint16_t buf[240*10];  // 10行缓冲区
// 填充缓冲区数据...
vTFT_FastDrawColor(0, 0, 239, 9, buf);
```

---

#### vTFT_FillRect

**功能**: 填充矩形区域

**函数原型**:
```c
void vTFT_FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
```

**参数**:
- `x1`: 起始列坐标
- `y1`: 起始行坐标
- `x2`: 结束列坐标
- `y2`: 结束行坐标
- `color`: RGB565颜色值

**返回值**: 无

**示例**:
```c
// 填充红色矩形
vTFT_FillRect(10, 10, 100, 100, 0xF800);
```

---

#### vTFT_Clear

**功能**: 清屏

**函数原型**:
```c
void vTFT_Clear(uint16_t color);
```

**参数**:
- `color`: RGB565颜色值

**返回值**: 无

**示例**:
```c
vTFT_Clear(0x0000);  // 清屏为黑色
vTFT_Clear(0xFFFF);  // 清屏为白色
```

---

#### vTFT_SetRotation

**功能**: 设置显示方向

**函数原型**:
```c
void vTFT_SetRotation(uint8_t rotation);
```

**参数**:
- `rotation`: 旋转角度(0-3)
  - 0: 竖屏0度
  - 1: 横屏90度
  - 2: 竖屏180度
  - 3: 横屏270度

**返回值**: 无

**示例**:
```c
vTFT_SetRotation(1);  // 设置为横屏
```

---

## 3. LVGL定时器接口

头文件: `lv_port_timer.h`

#### vLV_TimerInit

**功能**: 初始化LVGL定时器

**函数原型**:
```c
void vLV_TimerInit(void);
```

**参数**: 无

**返回值**: 无

**说明**: 
- 配置SysTick为1ms中断
- 为LVGL提供心跳

---

#### ulLV_GetTickMs

**功能**: 获取当前系统时间戳

**函数原型**:
```c
uint32_t ulLV_GetTickMs(void);
```

**参数**: 无

**返回值**: 当前时间戳(毫秒)

---

#### vLV_TimerIrqCallback

**功能**: 定时器中断回调

**函数原型**:
```c
void vLV_TimerIrqCallback(void);
```

**参数**: 无

**返回值**: 无

**说明**: 需要在SysTick_Handler中调用

---

## 4. LVGL显示接口

头文件: `lv_port_disp.h`

#### lv_port_disp_init

**功能**: 初始化LVGL显示驱动

**函数原型**:
```c
void lv_port_disp_init(void);
```

**参数**: 无

**返回值**: 无

**说明**: 
- 创建LVGL显示对象
- 设置刷新回调
- 配置显示缓冲区

---

#### disp_enable_update

**功能**: 启用显示更新

**函数原型**:
```c
void disp_enable_update(u8 index);
```

**参数**:
- `index`: 信号量释放方式
  - 0: 普通释放
  - 1: 中断中释放

**返回值**: 无

---

#### disp_disable_update

**功能**: 禁用显示更新

**函数原型**:
```c
void disp_disable_update(void);
```

**参数**: 无

**返回值**: 无

---

## 颜色定义

### RGB565颜色格式

```c
// RGB565颜色计算宏
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// 常用颜色定义
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
```

---

## 错误处理

所有函数都是void类型，没有返回错误码。如果出现问题，请检查：

1. GPIO配置是否正确
2. SPI引脚连接
3. 电源电压
4. 屏幕初始化序列

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2026-05-24 | 初始版本 |