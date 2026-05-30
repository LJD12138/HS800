#include "MD_Mppt/md_mppt_iface.h"

#if(boardMPPT_IFACE)



/*****************************************************************************************************************
-----函数功能    串口相关IO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_mppt_io_init(void)//IO设置
{
	rcu_periph_clock_enable(mpptGPIO_DC_EN_RCU);
	gpio_init(mpptGPIO_DC_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, mpptGPIO_DC_EN_PIN);
	mpptGPIO_DC_EN_OFF();  //默认接收

	rcu_periph_clock_enable(mpptGPIO_XT60_EN_RCU);
	gpio_init(mpptGPIO_XT60_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, mpptGPIO_XT60_EN_PIN);
	mpptGPIO_XT60_EN_OFF();  //默认接收
}



/*****************************************************************************************************************
-----函数功能    串口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vMppt_IfaceInit(void)
{
    v_mppt_io_init();
}

/*****************************************************************************************************************
-----函数功能    串口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vMppt_IfaceDeInit(void)
{
    
}

#endif  //boardMPPT_IFACE















