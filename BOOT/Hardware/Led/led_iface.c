#include "Led/led_iface.h"
#if(boardLED_EN)


/***********************************************************************************************************************
-----函数功能    LED GPIO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_led_gpio_init(void)
{
	rcu_periph_clock_enable(RCU_AF);//开启复用外设时钟使能
//    gpio_pin_remap_config(GPIO_TIMER2_FULL_REMAP,ENABLE);//重映射T2_H0
	rcu_periph_clock_enable(ledPWR_SW_RCU);
	gpio_init(ledPWR_SW_PORT,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,ledPWR_SW_PIN);
//	rcu_periph_clock_enable(ledPWR_SW_RCU);
//	gpio_init(ledPWR_SW_PORT,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,ledPWR_SW_PIN);

	
	rcu_periph_clock_enable(ledAC_SW_RCU);
	gpio_init(ledAC_SW_PORT,GPIO_MODE_OUT_PP,GPIO_OSPEED_2MHZ,ledAC_SW_PIN);
	ledAC_SW_OFF();
	
	rcu_periph_clock_enable(ledUSB_SW_RCU);
	gpio_init(ledUSB_SW_PORT,GPIO_MODE_OUT_PP,GPIO_OSPEED_2MHZ,ledUSB_SW_PIN);
	ledUSB_SW_OFF();
	
	rcu_periph_clock_enable(ledLight_SW_RCU);
	gpio_init(ledLight_SW_PORT,GPIO_MODE_OUT_PP,GPIO_OSPEED_2MHZ,ledLight_SW_PIN);
	ledLight_SW_OFF();
	
	rcu_periph_clock_enable(ledDC_SW_RCU);
	gpio_init(ledDC_SW_PORT,GPIO_MODE_OUT_PP,GPIO_OSPEED_2MHZ,ledDC_SW_PIN);
	ledDC_SW_OFF();
}

/***********************************************************************************************************************
-----函数功能    定时器初始化
-----说明(备注)  
				通用定时器的时钟来自APB1,当D2PPRE1≥2分频的时候
				通用定时器的时钟为APB1时钟的2倍, 而APB1为120M, 所以定时器时钟 = 240Mhz
				定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
				Ft=定时器工作频率,单位:Mhz
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_led_pwm_init(void)
{
	timer_parameter_struct timer0_init;
	timer_oc_parameter_struct timer0_ocintpara;
	// Enable TIMER0 clock
	rcu_periph_clock_enable(ledTIMER_RCU);
	// TIMER0 reset
	timer_deinit(ledTIMER);
 
	// TIMER0 configuration PWM frequency is 100HZ
	timer0_init.prescaler         = 59;
	timer0_init.alignedmode       = TIMER_COUNTER_EDGE;
	timer0_init.counterdirection  = TIMER_COUNTER_UP;
	timer0_init.period            = 999;
	timer0_init.clockdivision     = TIMER_CKDIV_DIV1;
	timer0_init.repetitioncounter = 0;
	timer_init(ledTIMER, &timer0_init);
 
	// Output channel_2 is configured as PWM mode
	timer0_ocintpara.outputstate  = TIMER_CCX_ENABLE;
	timer0_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
	timer0_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
	timer0_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
	timer0_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
	timer0_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
	timer_channel_output_config(ledTIMER, ledTIMER_CH, &timer0_ocintpara);
 
	// Set the comparison register value
	timer_channel_output_pulse_value_config(ledTIMER, ledTIMER_CH, 0);
	timer_channel_output_mode_config(ledTIMER, ledTIMER_CH, TIMER_OC_MODE_PWM0);
	timer_channel_output_shadow_config(ledTIMER, ledTIMER_CH, TIMER_OC_SHADOW_DISABLE);
 
	// Enable TIMER0 output
	timer_primary_output_config(ledTIMER, ENABLE);
	// Enable timer auto reload shadow
	timer_auto_reload_shadow_enable(ledTIMER);
	// Enable TIMER0
	timer_enable(ledTIMER);

	ledPWR_SW_PWM_SET(0);
}

/***********************************************************************************************************************
-----函数功能    LED初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vLed_IfaceInit(void)
{
	v_led_gpio_init();
	v_led_pwm_init();
}

/***********************************************************************************************************************
-----函数功能    重置
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vLed_IfaceDeInit(void)
{
	rcu_periph_clock_disable(ledTIMER_RCU);
	timer_deinit(ledTIMER);
}

#if(boardLOW_POWER)
/***********************************************************************************************************************
-----函数功能    按键任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vLed_IoEnterLowPower(void)
{
    rcu_periph_clock_enable(TIMRT_LED_RCU);    /*使能端口时钟*/
    gpio_init(TIMRT_LED_GPIO, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, TIMRT_LED_PIN); //配置为外设引脚

    rcu_periph_clock_disable(LED_TIMTER_RCU);

    timer_disable(LED_TIMRT);
}
#endif

#endif  //boardLED_EN

