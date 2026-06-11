#include "Dc/dc_iface.h"

#if(boardDC_EN)

/*****************************************************************************************************************
-----函数功能    DC相关IO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/

static void v_dc_gpio_init(void)
{
	rcu_periph_clock_enable(dcPOWER_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(dcPOWER_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, dcPOWER_EN_PIN);
	gpio_output_options_set(dcPOWER_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, dcPOWER_EN_PIN);
	#else
	gpio_init(dcPOWER_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, dcPOWER_EN_PIN);
	#endif
	dcPOWER_EN_OFF();
}


/***********************************************************************************************************************
-----函数功能    LED初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDc_IfaceInit(void)
{
	v_dc_gpio_init();
}

#endif  //boardDC_EN
