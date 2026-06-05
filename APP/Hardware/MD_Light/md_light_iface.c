#include "MD_Light/md_light_iface.h"

#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"

/***********************************************************************************************************************
 *-----函数功能    照明GPIO初始化
 *-----说明(备注)  none
 *-----传入参数    none
 *-----输出参数    none
 *-----返回值      none
 ************************************************************************************************************************/
static void v_light_gpio_init(void)
{
	//IO config
	rcu_periph_clock_enable(lightPWM_GPIO_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(lightPWM_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, lightPWM_PIN);
	gpio_output_options_set(lightPWM_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, lightPWM_PIN);
	gpio_af_set(lightPWM_GPIO_PORT, lightTIMER_AF, lightPWM_PIN); /* TIM0_CH0 = AF1 */
	#else
	gpio_init(lightPWM_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, lightPWM_PIN);
	#endif
}

/***********************************************************************************************************************
 *-----函数功能    照明定时器初始化
 *-----说明(备注)  
 *				通用定时器的时钟来自APB1,当D2PPRE1≥2分频的时候
 *				通用定时器的时钟为APB1时钟的2倍, 而APB1为120M, 所以定时器时钟 = 240Mhz
 *				定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *				Ft=定时器工作频率,单位:Mhz
 *-----传入参数    arr: 自动重装载值, psc: 预分频值
 *-----输出参数    none
 *-----返回值      none
 ************************************************************************************************************************/
static void v_light_timer_init(uint16_t arr, uint16_t psc)
{
	timer_oc_parameter_struct timer_ocintpara;
	timer_parameter_struct timer_initpara;

	rcu_periph_clock_enable(lightTIMER_RCU);

	timer_deinit(lightTIMER);

	/* TIMER configuration */
	timer_initpara.prescaler         = psc;
	timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection  = TIMER_COUNTER_UP;
	timer_initpara.period            = arr;
	timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(lightTIMER, &timer_initpara);

	/* CH0 configuration in PWM mode0 */
	timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
	timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
	timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
	timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
	timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
	timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

	timer_channel_output_config(lightTIMER, lightTIMER_CH, &timer_ocintpara);

	/* CH0 configuration in PWM mode0 */
	timer_channel_output_pulse_value_config(lightTIMER, lightTIMER_CH, 0);
	timer_channel_output_mode_config(lightTIMER, lightTIMER_CH, TIMER_OC_MODE_PWM0);
	timer_channel_output_shadow_config(lightTIMER, lightTIMER_CH, TIMER_OC_SHADOW_DISABLE);

	/* Enable TIMER0 output */
	timer_primary_output_config(lightTIMER, ENABLE);

	/* Enable timer auto reload shadow */
	timer_auto_reload_shadow_enable(lightTIMER);

	/* Enable TIMER */
	timer_enable(lightTIMER);

	lightPWM_SET(0);
}

/***********************************************************************************************************************
 *-----函数功能    照明初始化
 *-----说明(备注)  none
 *-----传入参数    none
 *-----输出参数    none
 *-----返回值      none
 ************************************************************************************************************************/
void vLight_IfaceInit(void)
{
	v_light_gpio_init();
	v_light_timer_init(lightPWM_MAX_VALUE - 1, lightPWM_PSC - 1);
	tLight.usLastValue = lightPWM_SEMI_VALUE;   //默认的亮度
}

/***********************************************************************************************************************
 *-----函数功能    照明DeInit
 *-----说明(备注)  none
 *-----传入参数    none
 *-----输出参数    none
 *-----返回值      none
 ************************************************************************************************************************/
void vLight_IfaceDeInit(void)
{
	rcu_periph_clock_disable(lightTIMER_RCU);
	timer_deinit(lightTIMER);
}

#if(boardLOW_POWER)
/***********************************************************************************************************************
 *-----函数功能    照明进入低功耗
 *-----说明(备注)  none
 *-----传入参数    none
 *-----输出参数    none
 *-----返回值      none
 ************************************************************************************************************************/
void vLight_IoEnterLowPower(void)
{
	//IO config
	rcu_periph_clock_enable(lightPWM_GPIO_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(lightPWM_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, lightPWM_PIN);
	#else
	gpio_init(lightPWM_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, lightPWM_PIN);
	#endif

	rcu_periph_clock_disable(lightTIMER_RCU);
	timer_disable(lightTIMER);
}
#endif

#endif  //boardLIGHT_EN