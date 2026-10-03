/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Key
 * File    : key_iface.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 按键硬件驱动底层接口实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Key/key_iface.h"

#if (boardKEY_EN)

#if (boardADC_EN)
#include "Adc/adc_task.h"
#endif  /* boardADC_EN */

//****************************************************Macros********************************************************************//
#if (boardIC_TYPE == boardIC_GD32F50X)
#define			KEY_MODE_FLOATING						GPIO_PUPD_NONE
#define			KEY_MODE_PULLUP							GPIO_PUPD_PULLUP
#else
#define			KEY_MODE_FLOATING						GPIO_MODE_IN_FLOATING
#define			KEY_MODE_PULLUP							GPIO_MODE_IPU
#endif  /* boardIC_TYPE == boardIC_GD32F50X */

//****************************************************Parameter Initialization**************************************************//
typedef struct
{
	rcu_periph_enum		rcu;				/* 外设时钟 */
	uint32_t			gpio;				/* GPIO 端口 */
	uint32_t			pin;				/* GPIO 引脚 */
	uint32_t			mode;				/* GPIO 模式 */
	bool				bPressHigh;			/* 按下有效电平: true=高电平按下(浮空Power), false=低电平按下(上拉) */
	const char			*pName;				/* 按键调试名称 */
}KeyCfgGpio_T;

/* 按键引脚硬件配置表: 顺序需严格对应 KeyId_E */
static const KeyCfgGpio_T s_t_key_hw_config[keyNUM] =
{
	[keyPOWER] = {
		.rcu        = RCU_GPIOC,
		.gpio       = GPIOC,
		.pin        = GPIO_PIN_9,
		.mode       = KEY_MODE_FLOATING,
		.bPressHigh = true,
		.pName      = "Power",
	},

	#if (boardDCAC_EN)
	[keyAC] = {
		.rcu        = RCU_GPIOC,
		.gpio       = GPIOC,
		.pin        = GPIO_PIN_2,
		.mode       = KEY_MODE_PULLUP,
		.bPressHigh = false,
		.pName      = "AC",
	},
	#endif  /* boardDCAC_EN */

	#if (boardLIGHT_EN)
	[keyLIGHT] = {
		.rcu        = RCU_GPIOC,
		.gpio       = GPIOC,
		.pin        = GPIO_PIN_12,
		.mode       = KEY_MODE_PULLUP,
		.bPressHigh = false,
		.pName      = "Light",
	},
	#endif  /* boardLIGHT_EN */

	#if (boardUSB_EN)
	[keyUSB] = {
		.rcu        = RCU_GPIOB,
		.gpio       = GPIOB,
		.pin        = GPIO_PIN_4,
		.mode       = KEY_MODE_PULLUP,
		.bPressHigh = false,
		.pName      = "USB",
	},
	#endif  /* boardUSB_EN */

	#if (boardDC_EN)
	[keyDC] = {
		.rcu        = RCU_GPIOB,
		.gpio       = GPIOB,
		.pin        = GPIO_PIN_12,
		.mode       = KEY_MODE_PULLUP,
		.bPressHigh = false,
		.pName      = "DC",
	},
	#endif  /* boardDC_EN */
};

#define			KEY_HW_NUM								(sizeof(s_t_key_hw_config) / sizeof(s_t_key_hw_config[0]))

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 按键 GPIO 初始化
 * 说明(备注)  : 根据配置表统一配置外设时钟与输入模式；Power 按键由 ADC 负责时跳过 GPIO 初始化
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_IfaceInit(void)
{
	for (uint8_t i = 0; i < KEY_HW_NUM; ++i)
	{
		#if (boardADC_EN)
		/* Power 按键由 ADC 模块负责, 不初始化 GPIO */
		if (i == keyPOWER)
			continue;
		#endif  /* boardADC_EN */

		rcu_periph_clock_enable(s_t_key_hw_config[i].rcu);

		#if (boardIC_TYPE == boardIC_GD32F50X)
		gpio_mode_set(s_t_key_hw_config[i].gpio, GPIO_MODE_INPUT, s_t_key_hw_config[i].mode, s_t_key_hw_config[i].pin);
		#else
		gpio_init(s_t_key_hw_config[i].gpio, s_t_key_hw_config[i].mode, GPIO_OSPEED_2MHZ, s_t_key_hw_config[i].pin);
		#endif  /* boardIC_TYPE == boardIC_GD32F50X */
	}
}

/***********************************************************************************************************************
 * 函数功能    : 查表读取按键电平状态
 * 说明(备注)  : 硬件层唯一电平数据源，极性归一化；按下返回 true，未按下或越界返回 false
 * 传入参数    : e_id: 按键ID枚举
 * 输出参数    : 无
 * 返回值      : bool: true 按下, false 未按下
 ************************************************************************************************************************/
bool bKey_IsPressById(KeyId_E e_id)
{
	uint8_t uc_idx = (uint8_t)e_id;
	if (uc_idx >= KEY_HW_NUM)
		return false;

	#if (boardADC_EN)
	/* Power 按键由 ADC 模块负责 */
	if (uc_idx == keyPOWER)
		return (usAdc_GetChannelValue(adcKEY_POWER) > 1000);
	#endif  /* boardADC_EN */

	bool b_pin_level = (gpio_input_bit_get(s_t_key_hw_config[uc_idx].gpio, s_t_key_hw_config[uc_idx].pin) != RESET);
	return (b_pin_level == s_t_key_hw_config[uc_idx].bPressHigh);
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 按键引脚及外设进入低功耗状态
 * 说明(备注)  : 唤醒源沿用原代码 EXTI 配置 (PC13/PA0, 与 Power 按键 PC9 独立)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_IoEnterLowPower(void)
{
	rcu_periph_clock_enable(RCU_PMU);
	rcu_periph_clock_enable(RCU_AF);

	for (uint8_t i = 0; i < KEY_HW_NUM; ++i)
		rcu_periph_clock_enable(s_t_key_hw_config[i].rcu);

	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_9);
	#if (boardDCAC_EN)
	gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_2);
	#endif  /* boardDCAC_EN */
	#else
	gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, GPIO_PIN_9);
	#if (boardDCAC_EN)
	gpio_init(GPIOB, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_12);
	#endif  /* boardDCAC_EN */
	#endif  /* boardIC_TYPE */

	nvic_irq_enable(EXTI10_15_IRQn, 2U, 0U);
	nvic_irq_enable(EXTI0_IRQn, 2U, 0U);

	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOC, GPIO_PIN_SOURCE_13);
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_0);

	exti_init(EXTI_13, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
	exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_RISING);
	exti_interrupt_flag_clear(EXTI_13);
	exti_interrupt_flag_clear(EXTI_0);
}

/***********************************************************************************************************************
 * 函数功能    : 按键引脚退出低功耗状态
 * 说明(备注)  : 重新初始化按键 GPIO
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_IoExitLowPower(void)
{
	vKey_IfaceInit();
}
#endif  /* boardLOW_POWER */

#endif  /* boardKEY_EN */

