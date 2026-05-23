#include "MD_Light/md_light_iface.h"

#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"

/*****************************************************************************************************************
-----函数功能    照明GPIO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_light_gpio_init(void)
{
	//IO config
	rcu_periph_clock_enable(lightPWM_GPIO_RCU);
	gpio_init(lightPWM_GPIO_PORT,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,lightPWM_PIN);  //配置为外设引脚
	
//	rcu_periph_clock_enable(lightPWM_EN_GPIO_RCU);
//	gpio_init(lightPWM_EN_GPIO_PORT,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,lightPWM_EN_PIN);
}


/*****************************************************************************************************************
-----函数功能    照明定时器初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_light_timer_init(uint16_t arr,uint16_t psc)
{
    /* -----------------------------------------------------------------------
    TIMER1 configuration: generate 2 PWM signals with 2 different duty cycles:
    TIMER1CLK = SystemCoreClock / 120 / 1000 = 1KHz
    ----------------------------------------------------------------------- */
	  
	  //PWM config
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(lightTIMER_RCU);

    timer_deinit(lightTIMER);

    /* TIMER3 configuration */
    timer_initpara.prescaler         = psc;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = arr;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV4;
    timer_initpara.repetitioncounter = 0;
    timer_init(lightTIMER, &timer_initpara);

    /*CH0 configuration in PWM mode1 */
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

    timer_channel_output_config(lightTIMER, lightTIMER_CH, &timer_ocintpara);

    /*LED CH2 configuration in PWM mode1*/
    timer_channel_output_pulse_value_config(lightTIMER, lightTIMER_CH, 0);
    timer_channel_output_mode_config(lightTIMER, lightTIMER_CH, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(lightTIMER, lightTIMER_CH, TIMER_OC_SHADOW_DISABLE);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(lightTIMER);
	
    /* auto-reload preload enable */
//    lightPWM_EN_OFF();
	
//  lightPWM_EN_ON();
//	lightPWM_SET(100);
}

/*****************************************************************************************************************
-----函数功能    照明初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vLight_IfaceInit(void)
{    
	v_light_gpio_init();
	v_light_timer_init(lightPWM_MAX_VALUE-1,lightPWM_PSC-1);//SystemCoreClock / 120 / 1000 = 1KHz
	tLight.usLastValue=lightPWM_SEMI_VALUE; //默认的亮度
}

/*****************************************************************************************************************
-----函数功能    照明任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
#if(boardLOW_POWER)
void vLight_IoEnterLowPower(void)
{
	//IO config
	rcu_periph_clock_enable(lightPWM_GPIO_RCU);
	gpio_init(lightPWM_GPIO_PORT,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,lightPWM_PIN);  //配置为外设引脚
	
	rcu_periph_clock_enable(lightPWM_EN_GPIO_RCU);
	gpio_init(lightPWM_EN_GPIO_PORT,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,lightPWM_EN_PIN);
	
	rcu_periph_clock_disable(lightTIMER_RCU);
	timer_disable(lightTIMER);
}
#endif

#endif  //boardLIGHT_EN
