/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : fwdgt.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 独立看门狗驱动及复位源标志捕获实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "fwdgt.h"

#if (boardWDGT_EN)

#if (boardIC_TYPE == boardIC_GD32F50X)
#include "gd32f50x_rcu.h"
#else
#include "gd32f30x_rcu.h"
#endif

//****************************************************Parameter Initialization**************************************************//
ResetReason_T G_tResetReason;

/***********************************************************************************************************************
 * 函数功能    : 独立看门狗初始化
 * 说明(备注)  : 使能 IRC40K 内部低速时钟，配置 128 分频，重装载周期约 3.2 秒
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_Init(void)
{
	uint16_t us_timeout = 0xFFFFU;

	/* enable IRC40K */
	rcu_osci_on(RCU_IRC40K);

	/* wait till IRC40K is ready */
	while (SUCCESS != rcu_osci_stab_wait(RCU_IRC40K))
	{
		if (us_timeout > 0)
			us_timeout--;
		else
			break;
	}

	/* configure FWDGT counter clock: 40KHz(IRC40K) / 128 = 0.312 KHz */
	fwdgt_config(2 * 500, FWDGT_PSC_DIV128); //t = (1/0.312)x(2x500) = 3.2s

	fwdgt_write_disable();
	/* After 1.6 seconds to generate a reset */
	fwdgt_enable();
}

/***********************************************************************************************************************
 * 函数功能    : 独立看门狗喂狗
 * 说明(备注)  : 解除写保护并重装计数器
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_Reload(void)
{
	/* unlock fwdgt write protect */
	fwdgt_write_enable();
	/* feed fwdgt */
	fwdgt_counter_reload();
}

/***********************************************************************************************************************
 * 函数功能    : 看门狗进入低功耗待机配置
 * 说明(备注)  : 先行喂狗并将溢出时间延长至最大 26 秒
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_EnterLowPower(void)
{
	vFwdgt_Reload();
	/* configure FWDGT counter clock: 40KHz(IRC40K) / 256 = 0.156 KHz */
	fwdgt_config(0xfff, FWDGT_PSC_DIV256); //t = 26S

	fwdgt_write_disable();
	/* After 1.6 seconds to generate a reset */
	fwdgt_enable();
}

/***********************************************************************************************************************
 * 函数功能    : 看门狗退出低功耗
 * 说明(备注)  : 喂狗并恢复 3.2 秒正常工作看门狗周期
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_ExitLowPower(void)
{
	vFwdgt_Reload();
	vFwdgt_Init();
}

/***********************************************************************************************************************
 * 函数功能    : 获取并暂存复位源信息
 * 说明(备注)  : 读取 RCU 复位标志后立即清除，防止后续复位识别误判
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vResetReason_Capture(void)
{
	G_tResetReason.ext_pin   = (rcu_flag_get(RCU_FLAG_EPRST)    == SET);
	G_tResetReason.por       = (rcu_flag_get(RCU_FLAG_PORRST)   == SET);
	G_tResetReason.sw        = (rcu_flag_get(RCU_FLAG_SWRST)    == SET);
	G_tResetReason.fwdgt     = (rcu_flag_get(RCU_FLAG_FWDGTRST) == SET);
	G_tResetReason.wwdgt     = (rcu_flag_get(RCU_FLAG_WWDGTRST) == SET);
	G_tResetReason.low_power = (rcu_flag_get(RCU_FLAG_LPRST)    == SET);

	// 读取完马上清掉，防止下次误判
	rcu_all_reset_flag_clear();
}

/***********************************************************************************************************************
 * 函数功能    : 打印看门狗及系统复位原因
 * 说明(备注)  : 解析捕获的复位原因并输出调试日志
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_PrintResetReason(void)
{
	vResetReason_Capture();

	if (G_tResetReason.fwdgt)
		printf("[Reset] reason: FWDGT reset\r\n");
	else if (G_tResetReason.wwdgt)
		printf("[Reset] reason: WWDGT reset\r\n");
	else if (G_tResetReason.sw)
		printf("[Reset] reason: software reset\r\n");
	else if (G_tResetReason.ext_pin)
		printf("[Reset] reason: external pin reset\r\n");
	else if (G_tResetReason.por)
		printf("[Reset] reason: power on reset\r\n");
	else if (G_tResetReason.low_power)
		printf("[Reset] reason: low power reset\r\n");
}

#endif  //boardWDGT_EN
