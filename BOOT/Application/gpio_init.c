/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application
 * File    : gpio_init.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : GPIO引脚配置与低功耗/跳转控制实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "gpio_init.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : GPIO引脚初始配置
 * 说明(备注)  : 将所有端口默认配置为模拟输入以降低功耗，并配置必要的外设引脚
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vGPIO_Init(void)
{
//	//	//初始化所有IO,已达到最低功耗.
//	rcu_periph_clock_enable(RCU_GPIOA);
//	rcu_periph_clock_enable(RCU_GPIOB);
//	rcu_periph_clock_enable(RCU_GPIOC);
//	rcu_periph_clock_enable(RCU_GPIOD);
//	rcu_periph_clock_enable(RCU_GPIOE);
//	#if (boardIC_TYPE != boardIC_GD32F50X)
//	rcu_periph_clock_enable(RCU_GPIOF);
//	rcu_periph_clock_enable(RCU_GPIOG);
//	#endif
//	
//	#if (boardIC_TYPE == boardIC_GD32F50X)
//	/* GD32F50x: gpio_mode_set + gpio_output_options_set */
//	gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_ALL);
//	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, GPIO_PIN_ALL);
//	gpio_mode_set(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_ALL);
//	gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, GPIO_PIN_ALL);
//	gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_ALL);
//	gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, GPIO_PIN_ALL);
//	gpio_mode_set(GPIOD, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_ALL);
//	gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, GPIO_PIN_ALL);
//	gpio_mode_set(GPIOE, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_ALL);
//	gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, GPIO_PIN_ALL);
//	#else
//	/* GD32F30x: gpio_init */
//	gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_ALL);
//	gpio_init(GPIOB, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_ALL);
//	gpio_init(GPIOC, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_ALL);
//	gpio_init(GPIOD, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_ALL);
//	gpio_init(GPIOE, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_ALL);
//	gpio_init(GPIOF, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_ALL);
//	gpio_init(GPIOG, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, GPIO_PIN_ALL);
//	#endif
	
	rcu_periph_clock_enable(KEY_POWER_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(KEY_POWER_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_POWER_PIN);
	#else
	gpio_init(KEY_POWER_GPIO, GPIO_MODE_IPU, GPIO_OSPEED_2MHZ, KEY_POWER_PIN);
	#endif  /* boardIC_TYPE */
	
	rcu_periph_clock_enable(gpioASSIST_OPEN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(gpioASSIST_OPEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, gpioASSIST_OPEN_PIN);
	gpio_output_options_set(gpioASSIST_OPEN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, gpioASSIST_OPEN_PIN);
	#else
	gpio_init(gpioASSIST_OPEN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, gpioASSIST_OPEN_PIN);
	#endif  /* boardIC_TYPE */
	gpioASSIST_OPEN_ON();
}

/***********************************************************************************************************************
 * 函数功能    : 获取跳转到APP程序信号
 * 说明(备注)  : 1ms周期刷新判定
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 跳转APP, false: 留在Boot
 ************************************************************************************************************************/
bool bGPIO_BootJumpApp(void)
{
//	static u8 uc_key_tri_cnt;
//	if (KEY_POWER_IsPress() == true)
//	{
//		if (++uc_key_tri_cnt >= 30)
//		{
//			uc_key_tri_cnt = 0;
//			return true;
//		}
//	}
//	else 
//		uc_key_tri_cnt = 0;
//	return false;
	return true;
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 配置GPIO进入低功耗状态
 * 说明(备注)  : 关闭非必要外设时钟并配置唤醒按键外部中断
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_EnterLowPower(void)
{
	rcu_periph_clock_disable(RCU_GPIOA);
	rcu_periph_clock_disable(RCU_GPIOB);
	rcu_periph_clock_disable(RCU_GPIOD); 
	rcu_periph_clock_disable(RCU_GPIOE);
	#if (boardIC_TYPE != boardIC_GD32F50X)
	rcu_periph_clock_disable(RCU_GPIOF);
	rcu_periph_clock_disable(RCU_GPIOG);
	#endif  /* boardIC_TYPE */
	
	/* enable clock */
    rcu_periph_clock_enable(RCU_PMU);
	rcu_periph_clock_enable(KEY_POWER_RCU);     //C
	rcu_periph_clock_enable(KEY_WP_RCU);        //A
	rcu_periph_clock_enable(attiSENSOR_INT_RCU);//A
	rcu_periph_clock_enable(PRINT_RX_RCU);      //D
	rcu_periph_clock_enable(RCU_AF);
	
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(KEY_POWER_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_NONE, KEY_POWER_PIN);
	gpio_mode_set(KEY_WP_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_NONE, KEY_WP_PIN);
	gpio_mode_set(KEY_KEY1_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, KEY_KEY1_PIN);
	gpio_mode_set(attiSENSOR_INT_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_NONE, attiSENSOR_INT_PIN);
	gpio_mode_set(PRINT_RX_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_NONE, PRINT_RX_PIN);
	#else
	gpio_init(KEY_POWER_GPIO, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, KEY_POWER_PIN);             //中断 电源按键 PC13
	gpio_init(KEY_WP_GPIO, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, KEY_WP_PIN);                   //中断 唤醒脚 PA0
	gpio_init(KEY_KEY1_GPIO, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, KEY_KEY1_PIN);
	gpio_init(attiSENSOR_INT_GPIO, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, attiSENSOR_INT_PIN);   //中断 状态传感器 PA9
	gpio_init(PRINT_RX_GPIO, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, PRINT_RX_PIN);               //中断 串口接收   PD2
	#endif  /* boardIC_TYPE */
	
	/* enable and set key EXTI interrupt to the lowest priority */
	nvic_irq_enable(EXTI10_15_IRQn, 2U, 0U);
	nvic_irq_enable(EXTI5_9_IRQn, 2U, 0U);
	nvic_irq_enable(EXTI2_IRQn, 2U, 0U);
	nvic_irq_enable(EXTI0_IRQn, 2U, 0U);

	/* connect key EXTI line to key GPIO pin */
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOC, GPIO_PIN_SOURCE_13); //PC13
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_9);  //PA9
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOD, GPIO_PIN_SOURCE_2);  //PD2
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_0);  //PA0

	/* configure key EXTI line */
	exti_init(EXTI_13, EXTI_INTERRUPT, EXTI_TRIG_FALLING); //下降沿触发
	exti_init(EXTI_9, EXTI_INTERRUPT, EXTI_TRIG_RISING);   //上升沿触发
	exti_init(EXTI_2, EXTI_INTERRUPT, EXTI_TRIG_RISING);   //上升沿触发
	exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_RISING);   //上升沿触发
	exti_interrupt_flag_clear(EXTI_13);
	exti_interrupt_flag_clear(EXTI_9);
	exti_interrupt_flag_clear(EXTI_2);
	exti_interrupt_flag_clear(EXTI_0);
	exti_interrupt_enable(EXTI_13);
	exti_interrupt_enable(EXTI_9);
	exti_interrupt_enable(EXTI_2);
	exti_interrupt_enable(EXTI_0);
}

/***********************************************************************************************************************
 * 函数功能    : GPIO退出低功耗
 * 说明(备注)  : 重新初始化系统GPIO引脚
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vGPIO_ExitLowPower(void)
{
	vGPIO_Init();
}

/***********************************************************************************************************************
 * 函数功能    : 配置GPIO进入APP运行状态
 * 说明(备注)  : 禁用并清理低功耗唤醒中断线
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vGPIO_EnterApp(void)
{
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(KEY_POWER_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, KEY_POWER_PIN);
	gpio_mode_set(KEY_WP_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, KEY_WP_PIN);
	gpio_mode_set(KEY_KEY1_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, KEY_KEY1_PIN);
	gpio_mode_set(attiSENSOR_INT_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, attiSENSOR_INT_PIN);
//	gpio_mode_set(PRINT_RX_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, PRINT_RX_PIN);
	#else
	gpio_init(KEY_POWER_GPIO, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, KEY_POWER_PIN);             //模拟输入 电源按键 PC13
	gpio_init(KEY_WP_GPIO,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,KEY_WP_PIN);                   //中断 唤醒脚     PA0
	gpio_init(KEY_KEY1_GPIO,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,KEY_KEY1_PIN);
	gpio_init(attiSENSOR_INT_GPIO,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,attiSENSOR_INT_PIN);   //中断 状态传感器 PA9
//	gpio_init(PRINT_RX_GPIO,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,PRINT_RX_PIN);               //中断 串口接收   PD2
	#endif  /* boardIC_TYPE */
	
	nvic_irq_disable(EXTI10_15_IRQn);
	nvic_irq_disable(EXTI5_9_IRQn);
	nvic_irq_disable(EXTI2_IRQn);
	nvic_irq_disable(EXTI0_IRQn);
	
	exti_interrupt_flag_clear(EXTI_13);
	exti_interrupt_flag_clear(EXTI_9);
	exti_interrupt_flag_clear(EXTI_2);
	exti_interrupt_flag_clear(EXTI_0);
	exti_interrupt_disable(EXTI_13);
	exti_interrupt_disable(EXTI_9);
	exti_interrupt_disable(EXTI_2);
	exti_interrupt_disable(EXTI_0);
}
#endif  /* boardLOW_POWER */

