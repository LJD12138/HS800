/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application
 * File    : fwdgt.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 独立看门狗驱动及芯片复位原因捕获实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "fwdgt.h"

#if(boardWDGT_EN)
#include "gd32f50x_rcu.h"

#if (boardPRINT_IFACE)
#include "Print/print_api.h"
#endif  /* boardPRINT_IFACE */

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
reset_reason_t g_reset_reason;

//****************************************************Function Declaration******************************************************//


/***********************************************************************************************************************
 * 函数功能    : 独立看门狗初始化
 * 说明(备注)  : 时钟源来自IRC40K内部低速时钟，超时时间约3.2秒
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFwdgt_Init(void)
{
	uint16_t timeout_t = 0xFFFFU;
	
	/* enable IRC40K */
	rcu_osci_on(RCU_IRC40K);
	
	/* wait till IRC40K is ready */
	while (SUCCESS != rcu_osci_stab_wait(RCU_IRC40K))
	{
		if (timeout_t > 0)
			timeout_t--;
		else
			break;
	}
	
	/* configure FWDGT counter clock: 40KHz(IRC40K) / 128 = 0.312 KHz, t = (1/0.312) * (2 * 500) = 3.2s */
	fwdgt_config(2 * 500, FWDGT_PSC_DIV128);
	
	fwdgt_write_disable();
	fwdgt_enable();
}

/***********************************************************************************************************************
 * 函数功能    : 独立看门狗重载计数器(喂狗)
 * 说明(备注)  : 任何时刻均可调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFwdgt_Reload(void)
{
	/* unlock fwdgt write protect */
	fwdgt_write_enable();
	/* feed fwdgt */
	fwdgt_counter_reload();	
}

/***********************************************************************************************************************
 * 函数功能    : 看门狗进入低功耗模式
 * 说明(备注)  : 将分频比调整为256分频，喂狗间隔延长至最大约26秒
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFwdgt_EnterLowPower(void)
{
	vFwdgt_Reload();
	/* configure FWDGT counter clock: 40KHz(IRC40K) / 256 = 0.156 KHz, t = 26s */
	fwdgt_config(0xFFF, FWDGT_PSC_DIV256);
	
	fwdgt_write_disable();
	fwdgt_enable();
}

/***********************************************************************************************************************
 * 函数功能    : 看门狗退出低功耗模式
 * 说明(备注)  : 恢复常规看门狗计数周期
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFwdgt_ExitLowPower(void)
{
	vFwdgt_Reload();
	vFwdgt_Init();
}

/***********************************************************************************************************************
 * 函数功能    : 捕获芯片复位原因
 * 说明(备注)  : 读取RCU标志寄存器并清除复位标志
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vResetReason_Capture(void)
{
	g_reset_reason.ext_pin   = (rcu_flag_get(RCU_FLAG_EPRST)    == SET);
	g_reset_reason.por       = (rcu_flag_get(RCU_FLAG_PORRST)   == SET);
	g_reset_reason.sw        = (rcu_flag_get(RCU_FLAG_SWRST)    == SET);
	g_reset_reason.fwdgt     = (rcu_flag_get(RCU_FLAG_FWDGTRST) == SET);
	g_reset_reason.wwdgt     = (rcu_flag_get(RCU_FLAG_WWDGTRST) == SET);
	g_reset_reason.low_power = (rcu_flag_get(RCU_FLAG_LPRST)    == SET);

	// 读取完马上清掉，防止下次误判
	rcu_all_reset_flag_clear();
}

/***********************************************************************************************************************
 * 函数功能    : 串口打印复位原因
 * 说明(备注)  : 依据捕获的复位状态输出调试信息
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFwdgt_PrintResetReason(void)
{
	vResetReason_Capture();

	#if (boardPRINT_IFACE)
	if (g_reset_reason.fwdgt)
		sMyPrint("[Reset] reason: FWDGT reset\r\n");
	else if (g_reset_reason.wwdgt)
		sMyPrint("[Reset] reason: WWDGT reset\r\n");
	else if (g_reset_reason.sw)
		sMyPrint("[Reset] reason: software reset\r\n");
	else if (g_reset_reason.ext_pin)
		sMyPrint("[Reset] reason: external pin reset\r\n");
	else if (g_reset_reason.por)
		sMyPrint("[Reset] reason: power on reset\r\n");
	else if (g_reset_reason.low_power)
		sMyPrint("[Reset] reason: low power reset\r\n");
	#endif  /* boardPRINT_IFACE */
}

#endif  /* boardWDGT_EN */

