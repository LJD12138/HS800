/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\ComFunc
 * File    : i2c.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 软件模拟 I2C 通信接口实现，提供 GPIO 模拟时序、无阻塞延时与 CRC 校验封装
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "i2c.h"
#include "check.h"
#include <string.h>

#if (1)
//****************************************************Macros********************************************************************//
#define			ACK										1
#define			NACK									0

#if (boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
#define			I2C_LOW(GPIO, PIN)						GPIO_BC(GPIO) = (uint32_t)(PIN)
#define			I2C_HIGH(GPIO, PIN)						GPIO_BOP(GPIO) = (uint32_t)(PIN)
#elif (boardIC_TYPE == boardIC_STM32H7XX)
#define			I2C_LOW(GPIO, PIN)						(HAL_GPIO_WritePin(GPIO, PIN, GPIO_PIN_RESET))
#define			I2C_HIGH(GPIO, PIN)						(HAL_GPIO_WritePin(GPIO, PIN, GPIO_PIN_SET))
#elif (boardIC_TYPE == boardIC_STM32G4XX)
#error "boardIC_STM32G4XX 未定义"
#endif

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 软件延时，降低 I2C 速率
 * 说明(备注)  : 内部时序控制 (加入 nop 防止 -O2/-O3 优化消除)
 * 传入参数    : us_cnt: 延时计数值
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_delay(uint16_t us_cnt)
{
    while (us_cnt--)
    {
        __asm volatile ("nop");
    }
}

/***********************************************************************************************************************
 * 函数功能    : 软件半延时
 * 说明(备注)  : 内部时序控制 (加入 nop 防止 -O2/-O3 优化消除)
 * 传入参数    : us_cnt: 延时计数值
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_delay_low(uint16_t us_cnt)
{
    us_cnt >>= 1;
    while (us_cnt--)
    {
        __asm volatile ("nop");
    }
}

/***********************************************************************************************************************
 * 函数功能    : 设置 SCL 为推挽输出
 * 说明(备注)  : 时钟线配置
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_scl_gpio_output(const I2cObj_T *p_i2c_obj)
{
    #if (boardIC_TYPE == boardIC_GD32F50X)
    gpio_mode_set(p_i2c_obj->ulGPIO_PORT_SCL, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, p_i2c_obj->ulGPIO_PIN_SCL);
    gpio_output_options_set(p_i2c_obj->ulGPIO_PORT_SCL, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, p_i2c_obj->ulGPIO_PIN_SCL);
    #elif (boardIC_TYPE == boardIC_GD32F30X)
    gpio_init(p_i2c_obj->ulGPIO_PORT_SCL, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, p_i2c_obj->ulGPIO_PIN_SCL);
    #elif (boardIC_TYPE == boardIC_STM32H7XX)
    GPIO_InitTypeDef gpio_init_struct = {0};
    gpio_init_struct.Pin   = p_i2c_obj->ulGPIO_PIN_SCL;
    gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull  = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(p_i2c_obj->ulGPIO_PORT_SCL, &gpio_init_struct);
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#error "boardIC_STM32G4XX 未定义"
	#endif  /* boardIC_TYPE */
    
}

/***********************************************************************************************************************
 * 函数功能    : 设置 SDA 为推挽输出
 * 说明(备注)  : 数据发送时配置
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_sda_gpio_output(const I2cObj_T *p_i2c_obj)
{
    #if (boardIC_TYPE == boardIC_GD32F50X)
    gpio_mode_set(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, p_i2c_obj->ulGPIO_PIN_SDA);
    gpio_output_options_set(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, p_i2c_obj->ulGPIO_PIN_SDA);
    #elif (boardIC_TYPE == boardIC_GD32F30X)
    gpio_init(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, p_i2c_obj->ulGPIO_PIN_SDA);
    #elif (boardIC_TYPE == boardIC_STM32H7XX)
    GPIO_InitTypeDef gpio_init_struct = {0};
    gpio_init_struct.Pin   = p_i2c_obj->ulGPIO_PIN_SDA;
    gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull  = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(p_i2c_obj->ulGPIO_PORT_SDA, &gpio_init_struct);
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#error "boardIC_STM32G4XX 未定义"
	#endif  /* boardIC_TYPE */
}

/***********************************************************************************************************************
 * 函数功能    : 设置 SDA 为上拉输入
 * 说明(备注)  : 数据接收及 ACK 采样时配置
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_sda_gpio_input(const I2cObj_T *p_i2c_obj)
{
    #if (boardIC_TYPE == boardIC_GD32F50X)
    gpio_mode_set(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, p_i2c_obj->ulGPIO_PIN_SDA);
    #elif (boardIC_TYPE == boardIC_GD32F30X)
    gpio_init(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, p_i2c_obj->ulGPIO_PIN_SDA);
    #elif (boardIC_TYPE == boardIC_STM32H7XX)
    GPIO_InitTypeDef gpio_init_struct = {0};
    gpio_init_struct.Pin  = p_i2c_obj->ulGPIO_PIN_SDA;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(p_i2c_obj->ulGPIO_PORT_SDA, &gpio_init_struct);
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#error "boardIC_STM32G4XX 未定义"
	#endif  /* boardIC_TYPE */
}

/***********************************************************************************************************************
 * 函数功能    : 读取 SDA 电平信号
 * 说明(备注)  : 采样 SDA 引脚输入电平
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : true: 高电平; false: 低电平
 ************************************************************************************************************************/
__STATIC_INLINE bool b_i2c_read_sda_gpio(const I2cObj_T *p_i2c_obj)
{
	#if(boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
    if ((uint32_t)RESET != (GPIO_ISTAT(p_i2c_obj->ulGPIO_PORT_SDA) & (p_i2c_obj->ulGPIO_PIN_SDA)))
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
    if (HAL_GPIO_ReadPin(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA) != GPIO_PIN_RESET)
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#Error(未定义)
	#endif  /* boardIC_TYPE */
		return true;
	else
		return false;
}

/***********************************************************************************************************************
 * 函数功能    : 发送起始信号 (START)
 * 说明(备注)  : 当 SCL 为高电平时，SDA 产生由高到低的跳变
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_start(const I2cObj_T *p_i2c_obj)
{
    v_i2c_set_sda_gpio_output(p_i2c_obj);

    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    v_delay(p_i2c_obj->usDelay);
    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
    v_delay(p_i2c_obj->usDelay);
    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
}

/***********************************************************************************************************************
 * 函数功能    : 发送停止信号 (STOP)
 * 说明(备注)  : 当 SCL 为高电平时，SDA 产生由低到高的跳变
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_stop(const I2cObj_T *p_i2c_obj)
{
    v_i2c_set_sda_gpio_output(p_i2c_obj);

    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    v_delay(p_i2c_obj->usDelay);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
    v_delay(p_i2c_obj->usDelay);
	I2C_HIGH    (p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	I2C_HIGH    (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
}

/***********************************************************************************************************************
 * 函数功能    : 主机产生应答信号 (ACK)
 * 说明(备注)  : SCL 为高电平期间拉低 SDA
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_ack(const I2cObj_T *p_i2c_obj)
{
    v_i2c_set_sda_gpio_output(p_i2c_obj);

    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
    v_delay_low(p_i2c_obj->usDelay);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    v_delay_low(p_i2c_obj->usDelay);
    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
}

/***********************************************************************************************************************
 * 函数功能    : 主机产生非应答信号 (NACK)
 * 说明(备注)  : SCL 为高电平期间保持 SDA 为高
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_no_ack(const I2cObj_T *p_i2c_obj)
{
    v_i2c_set_sda_gpio_output(p_i2c_obj);

    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
    v_delay_low(p_i2c_obj->usDelay);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    v_delay_low(p_i2c_obj->usDelay);
    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
}

/***********************************************************************************************************************
 * 函数功能    : 等待从机应答 (ACK)
 * 说明(备注)  : 超时则发送停止信号退出
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 1: 应答成功; -1: 应答超时
 ************************************************************************************************************************/
__STATIC_INLINE s8 c_i2c_wait_ack(const I2cObj_T *p_i2c_obj)
{
    uint32_t ul_err_time = 0;

    v_i2c_set_sda_gpio_input(p_i2c_obj);

    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    v_delay_low(p_i2c_obj->usDelay);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    v_delay_low(p_i2c_obj->usDelay);

    while (b_i2c_read_sda_gpio(p_i2c_obj))
    {
        ul_err_time++;
        if (ul_err_time > 1000)
        {
            v_i2c_set_stop(p_i2c_obj);
            return -1;
        }
    }

    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    v_delay_low(p_i2c_obj->usDelay);

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写入单字节数据
 * 说明(备注)  : 高位在前发送
 * 传入参数    : p_i2c_obj: I2C 对象; txd: 发送字节
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_i2c_write_byte(const I2cObj_T *p_i2c_obj, uint8_t txd)
{
    uint8_t uc_bit;

    v_i2c_set_sda_gpio_output(p_i2c_obj);

    I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    v_delay_low(p_i2c_obj->usDelay);

    for (uc_bit = 0; uc_bit < 8; uc_bit++)
    {
        if ((txd & 0x80) == 0x80)
            I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
        else
            I2C_LOW(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
        txd <<= 1;
        v_delay_low(p_i2c_obj->usDelay);
        I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
        v_delay_low(p_i2c_obj->usDelay);
        I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 读取单字节数据
 * 说明(备注)  : 读取完成后根据 ack 参数发送 ACK 或 NACK
 * 传入参数    : p_i2c_obj: I2C 对象; ack: ACK (1) 或 NACK (0)
 * 输出参数    : 无
 * 返回值      : 接收到的单字节数据
 ************************************************************************************************************************/
__STATIC_INLINE uint8_t uc_i2c_read_byte(const I2cObj_T *p_i2c_obj, uint8_t ack)
{
    uint8_t uc_bit;
    uint8_t uc_receive = 0;

    v_i2c_set_sda_gpio_input(p_i2c_obj);

    for (uc_bit = 0; uc_bit < 8; uc_bit++)
    {
        I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
        v_delay_low(p_i2c_obj->usDelay);
        I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
        v_delay_low(p_i2c_obj->usDelay);
        uc_receive <<= 1;
        if (b_i2c_read_sda_gpio(p_i2c_obj))
            uc_receive++;
        v_delay_low(p_i2c_obj->usDelay);
    }

    if (ack == ACK)
        v_i2c_set_ack(p_i2c_obj);
    else
        v_i2c_set_no_ack(p_i2c_obj);

    return uc_receive;
}

/***********************************************************************************************************************
 * 函数功能    : I2C 对象初始化
 * 说明(备注)  : 初始化 GPIO 并将总线置为空闲高电平状态
 * 传入参数    : p_i2c_obj: I2C 对象指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vI2C_ObjInit(I2cObj_T *p_i2c_obj)
{
    if (p_i2c_obj == NULL)
        return;

    if (p_i2c_obj->AddrType == AddrType_7bit)
    {
        p_i2c_obj->Addr = (p_i2c_obj->Addr << 1) & 0xFE;
        p_i2c_obj->AddrType = AddrType_8bit;
    }

    v_i2c_set_scl_gpio_output(p_i2c_obj);
    v_i2c_set_sda_gpio_output(p_i2c_obj);

    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
}

/***********************************************************************************************************************
 * 函数功能    : I2C 总线死锁恢复 (9 脉冲复位)
 * 说明(备注)  : 当从机异常拉低 SDA 时，通过 9 个时钟脉冲恢复总线
 * 传入参数    : p_i2c_obj: I2C 对象指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vI2C_BusReset(const I2cObj_T *p_i2c_obj)
{
    uint8_t uc_i;

    if (p_i2c_obj == NULL)
        return;

    v_i2c_set_sda_gpio_input(p_i2c_obj);
    v_i2c_set_scl_gpio_output(p_i2c_obj);

    for (uc_i = 0; uc_i < 9; uc_i++)
    {
        if (b_i2c_read_sda_gpio(p_i2c_obj))
            break;
        I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
        v_delay_low(p_i2c_obj->usDelay);
        I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
        v_delay_low(p_i2c_obj->usDelay);
    }

    v_i2c_set_stop(p_i2c_obj);
}

/***********************************************************************************************************************
 * 函数功能    : 连续写数据 (无寄存器地址)
 * 说明(备注)  : 适合无内部寄存器地址的器件
 * 传入参数    : p_i2c_obj: I2C 对象; p_buf: 发送缓冲区; len: 数据长度
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_WriteData(const I2cObj_T *p_i2c_obj, const u8 *p_buf, u16 len)
{
    uint16_t us_idx;

    if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
        return -128;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));

    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -1;

    for (us_idx = 0; us_idx < len; us_idx++)
    {
        v_i2c_write_byte(p_i2c_obj, p_buf[us_idx]);
        if (c_i2c_wait_ack(p_i2c_obj) < 0)
            return -2;
    }

    v_i2c_set_stop(p_i2c_obj);
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 连续读数据 (无寄存器地址)
 * 说明(备注)  : 适合无内部寄存器地址的器件
 * 传入参数    : p_i2c_obj: I2C 对象; p_buf: 接收缓冲区; len: 数据长度
 * 输出参数    : p_buf: 接收到的数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_ReadData(const I2cObj_T *p_i2c_obj, u8 *p_buf, u16 len)
{
    if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
        return -128;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));

    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -1;

    while (len)
    {
        if (len == 1)
            *p_buf = uc_i2c_read_byte(p_i2c_obj, NACK);
        else
            *p_buf = uc_i2c_read_byte(p_i2c_obj, ACK);
        len--;
        p_buf++;
    }

    v_i2c_set_stop(p_i2c_obj);
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写入单字节寄存器数据
 * 说明(备注)  : 8 位寄存器地址
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 发送数据; len: 长度
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_WriteBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *p_buf, u8 len)
{
    uint8_t uc_idx;

    if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
        return -128;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));

    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -1;

    v_i2c_write_byte(p_i2c_obj, reg_addr);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -2;

    for (uc_idx = 0; uc_idx < len; uc_idx++)
    {
        v_i2c_write_byte(p_i2c_obj, p_buf[uc_idx]);
        if (c_i2c_wait_ack(p_i2c_obj) < 0)
            return -3;
    }

    v_i2c_set_stop(p_i2c_obj);
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 读取单字节寄存器数据
 * 说明(备注)  : 8 位寄存器地址
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 接收数据; len: 长度
 * 输出参数    : p_buf: 接收到的数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_ReadBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *p_buf, u8 len)
{
    if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
        return -128;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -1;

    v_i2c_write_byte(p_i2c_obj, reg_addr);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -2;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -3;

    while (len)
    {
        if (len == 1)
            *p_buf = uc_i2c_read_byte(p_i2c_obj, NACK);
        else
            *p_buf = uc_i2c_read_byte(p_i2c_obj, ACK);
        len--;
        p_buf++;
    }

    v_i2c_set_stop(p_i2c_obj);
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写入双字节寄存器数据
 * 说明(备注)  : 16 位寄存器地址 (高位在前)
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 16 位寄存器地址; p_buf: 发送数据; len: 长度
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_WriteBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, const u8 *p_buf, u8 len)
{
    uint8_t uc_idx;

    if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
        return -128;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -1;

    /* 写寄存器高地址 */
    v_i2c_write_byte(p_i2c_obj, (uint8_t)(reg_addr >> 8));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -2;

    /* 写寄存器低地址 */
    v_i2c_write_byte(p_i2c_obj, (uint8_t)(reg_addr & 0xFF));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -2;

    for (uc_idx = 0; uc_idx < len; uc_idx++)
    {
        v_i2c_write_byte(p_i2c_obj, p_buf[uc_idx]);
        if (c_i2c_wait_ack(p_i2c_obj) < 0)
            return -3;
    }

    v_i2c_set_stop(p_i2c_obj);
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 读取双字节寄存器数据
 * 说明(备注)  : 16 位寄存器地址 (高位在前)
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 16 位寄存器地址; p_buf: 接收数据; len: 长度
 * 输出参数    : p_buf: 接收到的数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_ReadBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, u8 *p_buf, u8 len)
{
    if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
        return -128;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -1;

    v_i2c_write_byte(p_i2c_obj, (uint8_t)(reg_addr >> 8));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -2;

    v_i2c_write_byte(p_i2c_obj, (uint8_t)(reg_addr & 0xFF));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -2;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -3;

    while (len)
    {
        if (len == 1)
            *p_buf = uc_i2c_read_byte(p_i2c_obj, NACK);
        else
            *p_buf = uc_i2c_read_byte(p_i2c_obj, ACK);
        len--;
        p_buf++;
    }

    v_i2c_set_stop(p_i2c_obj);
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写数据 (带寄存器地址和 CRC8 校验)
 * 说明(备注)  : AFE 或特定外设专用，消除 VLA 变长数组以避免隐式 malloc 依赖
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 发送数据; len: 长度
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_WriteBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *p_buf, u8 len)
{
    uint8_t uca_buff[i2cCRC_BUFF_MAX_LEN];
    uint8_t uc_crc_data = 0;
    uint8_t uc_idx;

    if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
        return -128;

    if (p_i2c_obj->ucBuffLen <= 2 || p_i2c_obj->ucBuffLen > i2cCRC_BUFF_MAX_LEN)
        return -1;

    if (p_i2c_obj->ucBuffLen < (len + 2))
        return -2;

    uca_buff[0] = p_i2c_obj->Addr;
    uca_buff[1] = reg_addr;
    memcpy(&uca_buff[2], p_buf, len);

    uc_crc_data = ucCheck_CRC8cal(uca_buff, len + 2);

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -3;

    v_i2c_write_byte(p_i2c_obj, reg_addr);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -4;

    for (uc_idx = 0; uc_idx < len; uc_idx++)
    {
        v_i2c_write_byte(p_i2c_obj, p_buf[uc_idx]);
        if (c_i2c_wait_ack(p_i2c_obj) < 0)
            return -5;
    }

    v_i2c_write_byte(p_i2c_obj, uc_crc_data);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -6;

    v_i2c_set_stop(p_i2c_obj);
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 读数据 (带寄存器地址和 CRC8 校验)
 * 说明(备注)  : AFE 专用，定长栈缓冲区防护
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 接收数据; len: 长度
 * 输出参数    : p_buf: 接收到的校验后有效数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_ReadBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *p_buf, u8 len)
{
    uint8_t uca_buff[i2cCRC_BUFF_MAX_LEN];
    uint8_t uc_crc_data = 0;
    uint8_t uc_idx;

    if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
        return -128;

    if (p_i2c_obj->ucBuffLen <= 4 || p_i2c_obj->ucBuffLen > i2cCRC_BUFF_MAX_LEN)
        return -1;

    if (p_i2c_obj->ucBuffLen < (len + 4))
        return -2;

    uca_buff[0] = p_i2c_obj->Addr;
    uca_buff[1] = reg_addr;
    uca_buff[2] = len;
    uca_buff[3] = p_i2c_obj->Addr | 0x01;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -3;

    v_i2c_write_byte(p_i2c_obj, reg_addr);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -4;

    v_i2c_write_byte(p_i2c_obj, len);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -5;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -6;

    for (uc_idx = 0; uc_idx < len; uc_idx++)
        uca_buff[4 + uc_idx] = uc_i2c_read_byte(p_i2c_obj, ACK);

    uc_crc_data = uc_i2c_read_byte(p_i2c_obj, NACK);
    v_i2c_set_stop(p_i2c_obj);

    if (uc_crc_data == ucCheck_CRC8cal(uca_buff, 4 + len))
    {
        for (uc_idx = 0; uc_idx < len; uc_idx++)
            p_buf[uc_idx] = uca_buff[4 + uc_idx];
    }
    else
        return -7;

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 读取两字节数据 (带 CRC8 校验)
 * 说明(备注)  : AFE 专用
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 接收数据 (2 字节)
 * 输出参数    : p_buf: 接收到的数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_Read2BytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *p_buf)
{
    uint8_t uca_buff[i2cCRC_BUFF_MAX_LEN];
    uint8_t uc_crc_data = 0;
    uint8_t uc_idx;

    if ((p_i2c_obj == NULL) || (p_buf == NULL))
        return -128;

    if (p_i2c_obj->ucBuffLen <= 4 || p_i2c_obj->ucBuffLen > i2cCRC_BUFF_MAX_LEN)
        return -1;

    if (p_i2c_obj->ucBuffLen < (2 + 4))
        return -2;

    uca_buff[0] = p_i2c_obj->Addr;
    uca_buff[1] = reg_addr;
    uca_buff[2] = p_i2c_obj->Addr | 0x01;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -3;

    v_i2c_write_byte(p_i2c_obj, reg_addr);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -4;

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -6;

    for (uc_idx = 0; uc_idx < 2; uc_idx++)
        uca_buff[3 + uc_idx] = uc_i2c_read_byte(p_i2c_obj, ACK);

    uc_crc_data = uc_i2c_read_byte(p_i2c_obj, NACK);
    v_i2c_set_stop(p_i2c_obj);

    if (uc_crc_data == ucCheck_CRC8cal(uca_buff, 5))
    {
        for (uc_idx = 0; uc_idx < 2; uc_idx++)
            p_buf[uc_idx] = uca_buff[3 + uc_idx];
    }
    else
        return -7;

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写入单字节数据 (带 CRC8 校验)
 * 说明(备注)  : AFE 专用
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 发送数据 (1 字节)
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_Write1BytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *p_buf)
{
    uint8_t uca_buff[i2cCRC_BUFF_MAX_LEN];
    uint8_t uc_crc_data = 0;
    const uint8_t uc_len = 1;

    if ((p_i2c_obj == NULL) || (p_buf == NULL))
        return -128;

    if (p_i2c_obj->ucBuffLen <= 2 || p_i2c_obj->ucBuffLen > i2cCRC_BUFF_MAX_LEN)
        return -1;

    if (p_i2c_obj->ucBuffLen < (uc_len + 2))
        return -2;

    uca_buff[0] = p_i2c_obj->Addr;
    uca_buff[1] = reg_addr;
    memcpy(&uca_buff[2], p_buf, uc_len);

    uc_crc_data = ucCheck_CRC8cal(uca_buff, uc_len + 2);

    v_i2c_set_start(p_i2c_obj);
    v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -3;

    v_i2c_write_byte(p_i2c_obj, reg_addr);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -4;

    v_i2c_write_byte(p_i2c_obj, p_buf[0]);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -5;

    v_i2c_write_byte(p_i2c_obj, uc_crc_data);
    if (c_i2c_wait_ack(p_i2c_obj) < 0)
        return -6;

    v_i2c_set_stop(p_i2c_obj);
    return 1;
}

#endif  /* 1 */

