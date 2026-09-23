/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_HeatManage
 * File    : md_hm_iface.c
 * Date    : 2026-09-12
 * Author  : LJD(291483914@qq.com)
 * Desc    : 风扇与热管理硬件接口实现(GPIO与定时器PWM初始化)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "MD_HeatManage/md_hm_iface.h"
#include "MD_HeatManage/md_hm_task.h"

//****************************************************Function Declaration******************************************************//
static void v_fan_gpio_init(void);
static void v_fan_timer_init(uint16_t arr, uint16_t psc);


/***********************************************************************************************************************
 * 函数功能    : 风扇与相关 GPIO 初始化
 * 说明(备注)  : 重映射定时器引脚并初始化 GPIO
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_fan_gpio_init(void)
{
	//IO config
	rcu_periph_clock_enable(RCU_AF);                          //开启复用外设时钟使能

	rcu_periph_clock_enable(fanPWM_GPIO_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(fanPWM_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, fanPWM_PIN);
	gpio_output_options_set(fanPWM_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, fanPWM_PIN);
	gpio_af_set(fanPWM_GPIO_PORT, fanTIMER_AF, fanPWM_PIN);
	#else
	gpio_pin_remap_config(GPIO_TIMER1_FULL_REMAP, ENABLE);   //重映射
	gpio_init(fanPWM_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, fanPWM_PIN);
	#endif  /* boardIC_TYPE */

	rcu_periph_clock_enable(fanPWM_EN_GPIO_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(fanPWM_EN_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, fanPWM_EN_PIN);
	gpio_output_options_set(fanPWM_EN_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, fanPWM_EN_PIN);
	#else
	gpio_init(fanPWM_EN_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, fanPWM_EN_PIN);
	#endif  /* boardIC_TYPE */
	fanPWM_EN_OFF();
}

/***********************************************************************************************************************
 * 函数功能    : 风扇定时器 PWM 初始化
 * 说明(备注)  : 配置硬件定时器通道与输出模式
 * 传入参数    : arr: 重装载值, psc: 预分频系数
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_fan_timer_init(uint16_t arr, uint16_t psc)
{
	/* -----------------------------------------------------------------------
	TIMER1 configuration: generate 2 PWM signals with 2 different duty cycles:
	TIMER1CLK = SystemCoreClock / 120 / 1000 = 1KHz
	----------------------------------------------------------------------- */
	timer_oc_parameter_struct timer_ocintpara;
	timer_parameter_struct timer_initpara;

	rcu_periph_clock_enable(fanTIMER_RCU);

	timer_deinit(fanTIMER);

	/* TIMER configuration */
	timer_initpara.prescaler         = psc;
	timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection  = TIMER_COUNTER_UP;
	timer_initpara.period            = arr;
	timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(fanTIMER, &timer_initpara);

	/* CH0 configuration in PWM mode0 */
	timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
	timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
	timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
	timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
	timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
	timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

	timer_channel_output_config(fanTIMER, fanTIMER_CH, &timer_ocintpara);

	/* CH0 configuration in PWM mode0 */
	timer_channel_output_pulse_value_config(fanTIMER, fanTIMER_CH, 0);
	timer_channel_output_mode_config(fanTIMER, fanTIMER_CH, TIMER_OC_MODE_PWM0);
	timer_channel_output_shadow_config(fanTIMER, fanTIMER_CH, TIMER_OC_SHADOW_DISABLE);

	/* Enable timer auto reload shadow */
	timer_auto_reload_shadow_enable(fanTIMER);

	/* Enable TIMER */
	timer_enable(fanTIMER);

	fanPWM_SET(0);
}

/***********************************************************************************************************************
 * 函数功能    : 风扇硬件接口初始化
 * 说明(备注)  : 初始化 GPIO 和定时器 PWM
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFan_IfaceInit(void)
{
	v_fan_gpio_init();
	v_fan_timer_init(fanPWM_MAX_VALUE - 1, fanPWM_PSC - 1);  /* SystemCoreClock / 120 / 1000 = 1KHz */
}

/***********************************************************************************************************************
 * 函数功能    : 风扇DeInit
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vFan_IfaceDeInit(void)
{
	rcu_periph_clock_disable(fanTIMER_RCU);
	timer_deinit(fanTIMER);
}

#if(boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 风扇 IO 进入低功耗模拟输入配置
 * 说明(备注)  : 关闭定时器并配置 GPIO 为模拟输入
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFan_IoEnterLowPower(void)
{
	//IO config
	rcu_periph_clock_enable(fanPWM_GPIO_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(fanPWM_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, fanPWM_PIN);
	#else
	gpio_init(fanPWM_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, fanPWM_PIN);
	#endif  /* boardIC_TYPE */

	rcu_periph_clock_enable(fanPWM_EN_GPIO_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(fanPWM_EN_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, fanPWM_EN_PIN);
	#else
	gpio_init(fanPWM_EN_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, fanPWM_EN_PIN);
	#endif  /* boardIC_TYPE */

	rcu_periph_clock_disable(fanTIMER_RCU);
	timer_disable(fanTIMER);
}
#endif  /* boardLOW_POWER */

