/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : gpio_init.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 板载通用 GPIO 初始化及电源辅助控制实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "gpio_init.h"
#include "freertos.h"
#include "task.h"

#include "Sys/sys_task.h"
#include "timer_task.h"
#include "Print/print_task.h"

/***********************************************************************************************************************
 * 函数功能    : 板载通用 IO 口初始化
 * 说明(备注)  : 初始化辅助开机控制管脚为推挽输出并默认拉低
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vGPIO_Init(void)
{
	rcu_periph_clock_enable(gpioASSIST_OPEN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(gpioASSIST_OPEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, gpioASSIST_OPEN_PIN);
	gpio_output_options_set(gpioASSIST_OPEN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, gpioASSIST_OPEN_PIN);
	#else
	gpio_init(gpioASSIST_OPEN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, gpioASSIST_OPEN_PIN);
	#endif  /* boardIC_TYPE */
	gpioASSIST_OPEN_OFF();
}

/***********************************************************************************************************************
 * 函数功能    : 协助 BMS 开启控制
 * 说明(备注)  : 输出开机脉冲拉高辅助管脚并重置 2S 定时器，超时后自动拉低
 * 传入参数    : en: true 开启辅助脉冲, false 结束辅助脉冲
 * 输出参数    : 无
 * 返回值      : bool: true 操作成功, false 失败
 ************************************************************************************************************************/
bool vGPIO_AssistBmsOpen(bool en)
{
	if (en == true)
	{
		tSysInfo.uInit.tFinish.bIF_Gpio = 0;  //关闭初始化,预防按键误触发
		gpioASSIST_OPEN_ON();

		#if (boardBMS_EN)
		xTimerReset(tWakeUpBmsTimer, 0);      //开启软件定时,2S后关闭
		#endif  //boardBMS_EN

		if (uPrint.tFlag.bSysTask)
			sMyPrint("BMS电源辅助开启打开\r\n");

		return true;
	}
	else
	{
		tSysInfo.uInit.tFinish.bIF_Gpio = 1;  //初始化完成
		gpioASSIST_OPEN_OFF();

		if (uPrint.tFlag.bSysTask)
			sMyPrint("定时2S到达,关闭BMS辅助开启\r\n");

		return true;
	}
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : IO 口进入低功耗模式
 * 说明(备注)  : 配置未用引脚为模拟输入以降低待机漏电流
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vGPIO_EnterLowPower(void)
{
	rcu_periph_clock_enable(gpioASSIST_OPEN_RCU);
	gpio_init(gpioASSIST_OPEN_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, gpioASSIST_OPEN_PIN);
}

/***********************************************************************************************************************
 * 函数功能    : IO 口退出低功耗模式
 * 说明(备注)  : 重新初始化通用 GPIO 为正常运行状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vGPIO_ExitLowPower(void)
{
	vGPIO_Init();
}
#endif  //boardLOW_POWER

