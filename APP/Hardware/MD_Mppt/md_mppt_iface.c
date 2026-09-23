/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_iface.c
 * Date    : 2026-09-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 串口底层硬件接口驱动与收发实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Mppt/md_mppt_iface.h"

#if (boardMPPT_IFACE)
#if (boardDCAC_IFACE)
#include "MD_Dcac/md_dcac_iface.h"
#endif  /* boardDCAC_IFACE */

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//
static void v_mppt_io_init(void);

/***********************************************************************************************************************
 * 函数功能    : MPPT 相关 IO 初始化
 * 说明(备注)  : 配置 DC 使能及 XT60 端口控制引脚
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_mppt_io_init(void)
{
	rcu_periph_clock_enable(mpptGPIO_DC_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(mpptGPIO_DC_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, mpptGPIO_DC_EN_PIN);
	gpio_output_options_set(mpptGPIO_DC_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, mpptGPIO_DC_EN_PIN);
	#else
	gpio_init(mpptGPIO_DC_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, mpptGPIO_DC_EN_PIN);
	#endif  /* boardIC_TYPE */
	mpptGPIO_DC_EN_OFF();	/* 默认关闭 */

	rcu_periph_clock_enable(mpptGPIO_XT60_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(mpptGPIO_XT60_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, mpptGPIO_XT60_EN_PIN);
	gpio_output_options_set(mpptGPIO_XT60_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, mpptGPIO_XT60_EN_PIN);
	#else
	gpio_init(mpptGPIO_XT60_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, mpptGPIO_XT60_EN_PIN);
	#endif  /* boardIC_TYPE */
	mpptGPIO_XT60_EN_OFF();	/* 默认关闭 */
}

/***********************************************************************************************************************
 * 函数功能    : MPPT 接口初始化
 * 说明(备注)  : 初始化相关 IO 控制引脚
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vMppt_IfaceInit(void)
{
	v_mppt_io_init();
}

/***********************************************************************************************************************
 * 函数功能    : MPPT 接口去初始化
 * 说明(备注)  : 暂无特定操作
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vMppt_IfaceDeInit(void)
{
}

/***********************************************************************************************************************
 * 函数功能    : MPPT 发送数据开始
 * 说明(备注)  : 复用逆变串口发送通道进行数据传输
 * 传入参数    : p_data: 发送数据地址, us_len: 数据长度
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bMppt_DataSendStart(uint8_t *p_data, uint16_t us_len)
{
	#if (boardMPPT_485_IFACE_EN)
	vMppt_485TransEnable(true);
	#endif  /* boardMPPT_485_IFACE_EN */

	#if (boardDCAC_IFACE)
	return bDcac_DataSendStart(p_data, us_len);
	#else
	return false;
	#endif  /* boardDCAC_IFACE */
}

#if (boardMPPT_485_IFACE_EN)
/***********************************************************************************************************************
 * 函数功能    : MPPT 485 收发切换
 * 说明(备注)  : 控制 RS485 收发方向
 * 传入参数    : b_en: true-发送模式, false-接收模式
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vMppt_485TransEnable(bool b_en)
{
	#if (boardDCAC_485_IFACE_EN)
	vDcac_485TransEnable(b_en);
	#endif  /* boardDCAC_485_IFACE_EN */
}
#endif  /* boardMPPT_485_IFACE_EN */

#endif  /* boardMPPT_IFACE */
