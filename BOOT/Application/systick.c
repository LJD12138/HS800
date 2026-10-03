/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application
 * File    : systick.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统滴答定时器(SysTick)配置与毫秒级延时/定时周期处理实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardIC_TYPE == boardIC_GD32F50X)
#include "gd32f50x.h"
#else
#include "gd32f30x.h"
#endif  /* boardIC_TYPE == boardIC_GD32F50X */

#include "systick.h"

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  /* boardUPDATE */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

#if (boardBMS_485_IFACE_EN)
#include "MD_Bms/md_bms_iface.h"
__IO bool bSysTick_BmsSendFinish = false;
#endif  /* boardBMS_485_IFACE_EN */

#if (boardPRINT_485_IFACE_EN)
#include "Print/print_iface.h"
__IO bool bSysTick_PrintSendFinish = false;
#endif  /* boardPRINT_485_IFACE_EN */

#if (boardPRINT_IFACE == 7)
#include "Print/print_usb_iface.h"
#endif  /* boardPRINT_IFACE == 7 */

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
__IO bool bSystick_10MsFlag = false;
__IO bool bSystick_100MsFlag = false;

static vu32 delay;

//****************************************************Function Declaration******************************************************//

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 配置SysTick滴答定时器
 * 说明(备注)  : 设置为1ms周期中断(1000Hz)，并配置中断优先级
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vSys_TickConfig(void)
{
    /* setup systick timer for 1000Hz interrupts */
    if (SysTick_Config(SystemCoreClock / 1000U))
    {
        /* capture error */
        while (1)
        {
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/***********************************************************************************************************************
 * 函数功能    : 毫秒级阻塞延时
 * 说明(备注)  : 等待SysTick中断递减计数器到0
 * 传入参数    : cnt: 延时时间(ms)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vSys_MsDelay(u32 cnt)
{
    delay = cnt;

    while (0U != delay)
    {
    }
}

/***********************************************************************************************************************
 * 函数功能    : SysTick中断处理回调
 * 说明(备注)  : 递减延时变量并维护10ms与100ms节拍标志
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vSys_Tick(void)
{
    if (0U != delay)
        delay--;
	
#if (boardPRINT_IFACE == 7)
    vUsbCdc_Tick();
#endif  /* boardPRINT_IFACE == 7 */
	
	#if (boardBMS_485_IFACE_EN)
	static vu16 cnt = 0;
	//BMS485发送接口关闭倒计时
	if (bSysTick_BmsSendFinish == true)
	{
		cnt++;
		if (cnt >= 2)
		{
			cnt = 0;
			vBms_485TransEnable(false);
		}
	}
	#endif  /* boardBMS_485_IFACE_EN */
	
	#if (boardPRINT_485_IFACE_EN)
	static vu16 cnt1 = 0;
	//BMS485发送接口关闭倒计时
	if (bSysTick_PrintSendFinish == true)
	{
		cnt1++;
		if (cnt1 >= 2)
		{
			cnt1 = 0;
			vPrint_485TransEnable(false);
		}
	}
	#endif  /* boardPRINT_485_IFACE_EN */
	
	//10MS计时
	static vu16 us_10ms_cnt = 0;
	us_10ms_cnt++;
	if (us_10ms_cnt >= 10)
	{
		us_10ms_cnt = 0;
		bSystick_10MsFlag = true;
		
		#if (boardUPDATE)
		vUpdate_TickTimer();
		#endif  /* boardUPDATE */
	}
	
	//100MS计时
	static vu16 us_100ms_cnt = 0;
	us_100ms_cnt++;
	if (us_100ms_cnt >= 100)
	{
		us_100ms_cnt = 0;
		bSystick_100MsFlag = true;
	}
	
	#if (boardDISPLAY_EN)
//	vDisp_DispTask();
	#endif  /* boardDISPLAY_EN */
}
