#include "Buz/buz_iface.h"

/***********************************************************************************************************************
-----函数功能    蜂鸣器IO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_buz_gpio_init(void)
{
    rcu_periph_clock_enable(buzPWM_GPIO_RCU);    /*使能端口时钟*/
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(buzPWM_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, buzPWM_GPIO_PIN);
	gpio_output_options_set(buzPWM_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, buzPWM_GPIO_PIN);
	gpio_af_set(buzPWM_GPIO_PORT, buzTIMER_AF, buzPWM_GPIO_PIN);
	#else
	gpio_init(buzPWM_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, buzPWM_GPIO_PIN);  //配置为外设引脚
	#endif
}

/***********************************************************************************************************************
-----函数功能    蜂鸣器定时器初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_buz_timer_init(void)
{
    timer_oc_parameter_struct timer_ocinitpara;       /*声明结构体*/
    timer_parameter_struct timer_initpara;            /*声明结构体*/

    rcu_periph_clock_enable(buzTIMER_RCU);              /*使能时钟*/

    timer_deinit(buzTIMER);                             /*默认定时器x*/

    timer_struct_para_init(&timer_initpara);                 /*默认值初始化*/
    /* TIMER0 configuration */
    timer_initpara.prescaler         = 49;                   /*分频数*/
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;   /*边沿对齐模式*/
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;     /*向上计时*/
    timer_initpara.period            = 999;                  /*重装值*/
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;     /*分频系数*/
    timer_initpara.repetitioncounter = 0;                    /*初始计数器值*/
    timer_init(buzTIMER, &timer_initpara);

	
    /* CH03 configuration in PWM mode */
    timer_ocinitpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocinitpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocinitpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
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
    /* enable a TIMER */
    timer_enable(buzTIMER);

    buzTIMER_PWM_SET(0);
}

/***********************************************************************************************************************
-----函数功能    蜂鸣器初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vBuz_Init(void)
{    
	v_buz_gpio_init();
	v_buz_timer_init();
}

#if(boardLOW_POWER)
/***********************************************************************************************************************
-----函数功能    按键任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vBuz_IoEnterLowPower(void)
{
	rcu_periph_clock_enable(buzPWM_GPIO_RCU);    /*使能端口时钟*/
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(buzPWM_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, buzPWM_GPIO_PIN);
	#else
	gpio_init(buzPWM_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, buzPWM_GPIO_PIN);  //配置为外设引脚
	#endif

	rcu_periph_clock_disable(buzTIMER_RCU);

	timer_disable(buzTIMER);
}
#endif

