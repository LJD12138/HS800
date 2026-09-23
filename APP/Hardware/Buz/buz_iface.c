/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Buz
 * File    : buz_iface.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 蜂鸣器底层硬件驱动接口实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Buz/buz_iface.h"

#if (boardBUZ_EN)

//****************************************************Function Declaration******************************************************//
static void v_buz_gpio_init(void);
static void v_buz_timer_init(void);

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 蜂鸣器 GPIO 初始化
 * 说明(备注)  : 配置蜂鸣器 PWM 引脚复用推挽输出
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_buz_gpio_init(void)
{
	rcu_periph_clock_enable(buzPWM_GPIO_RCU);                                       /* 使能端口时钟 */
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(buzPWM_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, buzPWM_GPIO_PIN);
	gpio_output_options_set(buzPWM_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, buzPWM_GPIO_PIN);
	gpio_af_set(buzPWM_GPIO_PORT, buzTIMER_AF, buzPWM_GPIO_PIN);
	#else
	gpio_init(buzPWM_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, buzPWM_GPIO_PIN); /* 配置为外设复用推挽 */
	#endif  /* boardIC_TYPE */
}

/***********************************************************************************************************************
 * 函数功能    : 蜂鸣器定时器初始化
 * 说明(备注)  : 配置硬件定时器通道 PWM 模式输出
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_buz_timer_init(void)
{
	timer_oc_parameter_struct timer_ocinitpara;
	timer_parameter_struct    timer_initpara;

	rcu_periph_clock_enable(buzTIMER_RCU);                                          /* 使能定时器时钟 */

	timer_deinit(buzTIMER);

	timer_struct_para_init(&timer_initpara);
	/* TIMER0 configuration */
    timer_initpara.prescaler         = 49;                   /*分频数*/
	timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;                          /* 边沿对齐模式 */
	timer_initpara.counterdirection  = TIMER_COUNTER_UP;                            /* 向上计数 */
	timer_initpara.period            = 999;                                         /* 重装载值 */
	timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;                            /* 分频系数 */
	timer_initpara.repetitioncounter = 0;
	timer_init(buzTIMER, &timer_initpara);

	/* CH03 configuration in PWM mode */
	timer_ocinitpara.outputstate = TIMER_CCX_ENABLE;
	timer_ocinitpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
	timer_ocinitpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
	timer_channel_output_config(buzTIMER, buzTIMER_CH, &timer_ocinitpara);

	/* configure TIMER channel 3 output pulse value */
	timer_channel_output_pulse_value_config(buzTIMER, buzTIMER_CH, 0);
	/* configure TIMER channel 3 PWM0 mode */
	timer_channel_output_mode_config(buzTIMER, buzTIMER_CH, TIMER_OC_MODE_PWM0);
	/* disable TIMER channel output shadow function */
	timer_channel_output_shadow_config(buzTIMER, buzTIMER_CH, TIMER_OC_SHADOW_DISABLE);

	/* enable TIMER primary output function */
	timer_primary_output_config(buzTIMER, ENABLE);

	/* auto-reload preload enable */
	timer_auto_reload_shadow_enable(buzTIMER);
	/* enable TIMER */
	timer_enable(buzTIMER);

	buzTIMER_PWM_SET(0);
}

/***********************************************************************************************************************
 * 函数功能    : 蜂鸣器硬件初始化
 * 说明(备注)  : 初始化引脚与定时器
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBuz_Init(void)
{
	v_buz_gpio_init();
	v_buz_timer_init();
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 蜂鸣器进入低功耗
 * 说明(备注)  : 引脚配置为模拟输入，关闭定时器时钟
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBuz_IoEnterLowPower(void)
{
	rcu_periph_clock_enable(buzPWM_GPIO_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(buzPWM_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, buzPWM_GPIO_PIN);
	#else
	gpio_init(buzPWM_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, buzPWM_GPIO_PIN);
	#endif  /* boardIC_TYPE */

	rcu_periph_clock_disable(buzTIMER_RCU);
	timer_disable(buzTIMER);
}
#endif  /* boardLOW_POWER */

#endif  /* boardBUZ_EN */
