/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Led
 * File    : led_iface.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 指示灯底层驱动接口实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Led/led_iface.h"

#if (boardLED_EN)

//****************************************************Function Declaration******************************************************//
static void v_led_gpio_init(void);

/***********************************************************************************************************************
 * 函数功能    : LED GPIO 初始化
 * 说明(备注)  : 配置 AC、DC 按键指示灯 GPIO 为推挽输出模式并默认关闭
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_led_gpio_init(void)
{
	rcu_periph_clock_enable(RCU_AF);	/* 开启复用外设时钟使能 */

	rcu_periph_clock_enable(ledPWR_SW_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(ledPWR_SW_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, ledPWR_SW_PIN);
	gpio_output_options_set(ledPWR_SW_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, ledPWR_SW_PIN);
	gpio_af_set(ledPWR_SW_PORT, ledTIMER_AF, ledPWR_SW_PIN); /* TIM0_CH0 = AF1 */
	#else
//    gpio_pin_remap_config(GPIO_TIMER2_FULL_REMAP,ENABLE);//重映射T2_H0
	gpio_init(ledPWR_SW_PORT,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,ledPWR_SW_PIN);
	#endif  /* boardIC_TYPE */
}

/***********************************************************************************************************************
 * 函数功能    : 定时器PWM初始化
 * 说明(备注)  : 通用定时器PWM频率配置为1KHz；通用定时器时钟取自APB1且在其2分频时为APB1的2倍(240MHz)，
 *               溢出时间 Tout = ((arr + 1) * (psc + 1)) / Ft
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_led_pwm_init(void)
{
	timer_parameter_struct timer0_init;
	timer_oc_parameter_struct timer0_ocintpara;
	
	/* Enable TIMER0 clock */
	rcu_periph_clock_enable(ledTIMER_RCU);
	/* TIMER0 reset */
	timer_deinit(ledTIMER);
 
	/* TIMER2 configuration PWM frequency is 1KHZ */
	timer0_init.prescaler         = 119;
	timer0_init.alignedmode       = TIMER_COUNTER_EDGE;
	timer0_init.counterdirection  = TIMER_COUNTER_UP;
	timer0_init.period            = 999;
	timer0_init.clockdivision     = TIMER_CKDIV_DIV1;
	timer0_init.repetitioncounter = 0;
	timer_init(ledTIMER, &timer0_init);
 
	/* Output channel_0 is configured as PWM mode */
	timer0_ocintpara.outputstate  = TIMER_CCX_ENABLE;
	timer0_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
	timer0_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
	timer0_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
	timer0_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
	timer0_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
	timer_channel_output_config(ledTIMER, ledTIMER_CH, &timer0_ocintpara);
 
	/* Set the comparison register value */
	timer_channel_output_pulse_value_config(ledTIMER, ledTIMER_CH, 0);
	timer_channel_output_mode_config(ledTIMER, ledTIMER_CH, TIMER_OC_MODE_PWM0);
	timer_channel_output_shadow_config(ledTIMER, ledTIMER_CH, TIMER_OC_SHADOW_DISABLE);
 
	/* Enable TIMER output */
	timer_primary_output_config(ledTIMER, ENABLE);
	/* Enable timer auto reload shadow */
	timer_auto_reload_shadow_enable(ledTIMER);
	/* Enable TIMER */
	timer_enable(ledTIMER);

	ledPWR_SW_PWM_SET(0);
}

/***********************************************************************************************************************
 * 函数功能    : LED 硬件接口初始化
 * 说明(备注)  : 电源指示灯 PWM 共用 fanTIMER (TIMER2 通道1)，已在 md_hm_iface 中统一配置
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLed_IfaceInit(void)
{
	v_led_gpio_init();
	v_led_pwm_init();
}

/***********************************************************************************************************************
 * 函数功能    : LED 硬件接口反初始化
 * 说明(备注)  : 释放定时器外设时钟
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLed_IfaceDeInit(void)
{
	rcu_periph_clock_disable(ledTIMER_RCU);
	timer_deinit(ledTIMER);
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 指示灯进入低功耗
 * 说明(备注)  : 将所有指示灯引脚重置为模拟输入模式
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLed_IoEnterLowPower(void)
{
	rcu_periph_clock_enable(ledPWR_SW_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
    gpio_mode_set(TIMRT_LED_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, TIMRT_LED_PIN);
	#else
	gpio_init(ledPWR_SW_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, ledPWR_SW_PIN);
	#endif  /* boardIC_TYPE */

    rcu_periph_clock_disable(LED_TIMTER_RCU);

    timer_disable(LED_TIMRT);
}
#endif  /* boardLOW_POWER */

#endif  /* boardLED_EN */

