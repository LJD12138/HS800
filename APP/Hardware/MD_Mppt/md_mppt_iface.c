/*******************************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS803\APP\Hardware\MD_Mppt
 * File    : md_mppt_iface.c
 * Date    : 2026-06-04
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT interface file
 * -------------------------------------------------------
 * todo    :
 * 1. None
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Mppt/md_mppt_iface.h"

#if(boardMPPT_IFACE)
#if(boardDCAC_IFACE)
#include "MD_Dcac/md_dcac_iface.h"
#endif

//****************************************************Function Declaration****************************************************//
static void v_mppt_io_init(void);

/***********************************************************************************************************************
 * 函数功能    : MPPT IO初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_mppt_io_init(void)
{
	rcu_periph_clock_enable(mpptGPIO_DC_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(mpptGPIO_DC_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, mpptGPIO_DC_EN_PIN);
	gpio_output_options_set(mpptGPIO_DC_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, mpptGPIO_DC_EN_PIN);
	#else
	gpio_init(mpptGPIO_DC_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, mpptGPIO_DC_EN_PIN);
	#endif
	mpptGPIO_DC_EN_OFF();  // 默认关闭

	rcu_periph_clock_enable(mpptGPIO_XT60_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(mpptGPIO_XT60_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, mpptGPIO_XT60_EN_PIN);
	gpio_output_options_set(mpptGPIO_XT60_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, mpptGPIO_XT60_EN_PIN);
	#else
	gpio_init(mpptGPIO_XT60_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, mpptGPIO_XT60_EN_PIN);
	#endif
	mpptGPIO_XT60_EN_OFF();  // 默认关闭
}

/***********************************************************************************************************************
 * 函数功能    : MPPT接口初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vMppt_IfaceInit(void)
{
    v_mppt_io_init();
}

/***********************************************************************************************************************
 * 函数功能    : MPPT接口去初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vMppt_IfaceDeInit(void)
{
    
}

/***********************************************************************************************************************
 * 函数功能    : MPPT发送数据开始
 * 说明(备注)  : none
 * 传入参数    : data: 待发送数据指针, len: 长度
 * 输出参数    : none
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bMppt_DataSendStart(u8* data, u16 len)
{
	#if(boardMPPT_485_IFACE_EN)
	vMppt_485TransEnable(true);
	#endif
	
	#if(boardDCAC_IFACE)
	return bDcac_DataSendStart(data, len);
	#else
	return false;
	#endif
}

#if(boardMPPT_485_IFACE_EN)
/***********************************************************************************************************************
 * 函数功能    : MPPT 485收发切换
 * 说明(备注)  : none
 * 传入参数    : en: true发送, false接收
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vMppt_485TransEnable(bool en)
{
	#if(boardDCAC_485_IFACE_EN)
	vDcac_485TransEnable(en);
	#endif
}
#endif

#endif  /* boardMPPT_IFACE */