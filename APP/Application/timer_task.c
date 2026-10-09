/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : timer_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : FreeRTOS 软件定时器管理及各外设节拍/超时回调实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "timer_task.h"
#include "gpio_init.h"
#include "Sys/sys_task.h"

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE

#if (boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //boardUSB_EN

#if (boardDC_EN)
#include "Dc/dc_task.h"
#endif  //boardDC_EN

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  //boardPRINT_IFACE

#if (boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#endif  //boardBMS_EN

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_rec_task.h"
#endif  //boardMPPT_EN

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#endif  //boardDCAC_EN

#if (boardWDGT_EN)
#include "fwdgt.h"
#endif  //boardWDGT_EN

//****************************************************Parameter Initialization**************************************************//
#if (boardBMS_485_IFACE_EN)
TimerHandle_t    tBmsRxEnTimer = NULL;     //单次定时器,BMS的485发送延时切换
#endif  //boardBMS_485_IFACE_EN

#if (boardMPPT_485_IFACE_EN)
TimerHandle_t    tMpptRxEnTimer = NULL;    //单次定时器,MPPT的485发送延时切换
#endif  //boardMPPT_485_IFACE_EN

#if (boardDCAC_485_IFACE_EN)
TimerHandle_t    tDcacRxEnTimer = NULL;    //单次定时器,DCAC的485发送延时切换
#endif  //boardDCAC_485_IFACE_EN

#if (boardBMS_EN)
TimerHandle_t    tWakeUpBmsTimer = NULL;   //单次定时器,BMS的唤醒使能延时关闭
#endif  //boardBMS_EN

static TimerHandle_t s_t_repet_timer = NULL; //重复定时器
static vu8           s_uc_timer_cnt = 0;

//****************************************************Function Declaration******************************************************//
static void v_timer_signal_callback(TimerHandle_t xTimer);
static void v_timer_repet_callback(TimerHandle_t xTimer);

/***********************************************************************************************************************
 * 函数功能    : 软件定时器任务及各单次/重复定时器初始化
 * 说明(备注)  : 创建并启动 485 收发切换、BMS 辅助开启及系统周期性心跳定时器
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; 负数: 定时器创建失败
 ************************************************************************************************************************/
s8 cTimer_TaskInit(void)
{
	/* 创建单次定时器 */
	#if (boardBMS_485_IFACE_EN)
	tBmsRxEnTimer = xTimerCreate("bms_exit_485_tx_timer",
	                             2,
	                             pdFALSE,
	                             (void *)1,
	                             v_timer_signal_callback);
	if (tBmsRxEnTimer == NULL)
		return -1;
	#endif  /* boardBMS_485_IFACE_EN */

	#if (boardBMS_EN)
	tWakeUpBmsTimer = xTimerCreate("wake_up_bms_timer",
	                               3000,
	                               pdFALSE,
	                               (void *)2,
	                               v_timer_signal_callback);
	if (tWakeUpBmsTimer == NULL)
		return -2;
	#endif  /* boardBMS_EN */

	#if (boardDCAC_485_IFACE_EN)
	tDcacRxEnTimer = xTimerCreate("acdc_exit_485_tx_timer",
	                              3,
	                              pdFALSE,
	                              (void *)3,
	                              v_timer_signal_callback);
	if (tDcacRxEnTimer == NULL)
		return -3;
	#endif  /* boardDCAC_485_IFACE_EN */

	#if (boardMPPT_485_IFACE_EN)
	tMpptRxEnTimer = xTimerCreate("mppt_exit_485_tx_timer",
	                              2,
	                              pdFALSE,
	                              (void *)4,
	                              v_timer_signal_callback);
	if (tMpptRxEnTimer == NULL)
		return -4;
	#endif  /* boardMPPT_485_IFACE_EN */

	/* 创建重复定时器 */
	s_t_repet_timer = xTimerCreate("repet_timer",
	                               boardREPET_TIMER_CYCLE_TMIE,
	                               pdTRUE,
	                               (void *)1,
	                               v_timer_repet_callback);
	if (s_t_repet_timer == NULL)
		return -5;

	// 启动定时器
	#if (boardBMS_485_IFACE_EN)
	xTimerStart(tBmsRxEnTimer, 0);
	#endif  /* boardBMS_485_IFACE_EN */

	#if (boardMPPT_485_IFACE_EN)
	xTimerStart(tMpptRxEnTimer, 0);
	#endif  /* boardMPPT_485_IFACE_EN */

	#if (boardBMS_EN)
	xTimerStart(tWakeUpBmsTimer, 0);
	#endif  /* boardBMS_EN */

	#if (boardDCAC_485_IFACE_EN)
	xTimerStart(tDcacRxEnTimer, 0);
	#endif  /* boardDCAC_485_IFACE_EN */

	xTimerStart(s_t_repet_timer, 0);

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 单次定时器超时回调函数
 * 说明(备注)  : 处理 485 发送使能撤回或 BMS 辅助开机脉冲拉低
 * 传入参数    : xTimer: 定时器句柄
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_timer_signal_callback(TimerHandle_t xTimer)
{
	if (pvTimerGetTimerID(xTimer) == ((void *)1))
	{
		#if (boardBMS_485_IFACE_EN)
		vBms_485TransEnable(false);
		#endif  //boardBMS_485_IFACE_EN
	}
	else if (pvTimerGetTimerID(xTimer) == ((void *)2))
		vGPIO_AssistBmsOpen(false);
	else if (pvTimerGetTimerID(xTimer) == ((void *)3))
	{
		#if (boardDCAC_485_IFACE_EN)
		vDcac_485TransEnable(false);
		#endif  //boardDCAC_485_IFACE_EN
	}
	else if (pvTimerGetTimerID(xTimer) == ((void *)4))
	{
		#if (boardMPPT_485_IFACE_EN)
		vMppt_485TransEnable(false);
		#endif  //boardMPPT_485_IFACE_EN
	}
}

/***********************************************************************************************************************
 * 函数功能    : 重复周期定时器回调函数
 * 说明(备注)  : 定周期调度各模块接收超时、升级超时及 1S 系统心跳管理
 * 传入参数    : xTimer: 定时器句柄
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_timer_repet_callback(TimerHandle_t xTimer)
{
	(void)xTimer;

	#if (boardUPDATE)
	if (tSysInfo.eDevState == DS_UPDATE_MODE)
	{
		vUpdate_TickTimer();
		vBms_RecTickTimer();
		vPrint_RecTickTimer();

		#if (boardDCAC_EN && (!boardDEBUG))
		vDcac_RecTickTimer();
		#endif  //boardDCAC_EN && (!boardDEBUG)

		return;
	}
	#endif  //boardUPDATE

	s_uc_timer_cnt++;
	if (s_uc_timer_cnt >= (1000 / boardREPET_TIMER_CYCLE_TMIE)) //1S计时
	{
		s_uc_timer_cnt = 0;
		vSys_TickTimer();

		#if (boardDCAC_EN)
		vDcac_TickTimer();
		#endif  //boardDCAC_EN

		#if (boardUSB_EN)
		vUsb_TickTimer();
		#endif  //boardUSB_EN

		#if (boardDC_EN)
		vDc_TickTimer();
		#endif  //boardDC_EN

		#if (boardDISPLAY_EN)
		vDisp_TickTimer();
		#endif  //boardDISPLAY_EN

		#if (boardWDGT_EN && boardPRINT_IFACE == 0)
		vFwdgt_Reload();
		#endif  //boardWDGT_EN && boardPRINT_IFACE == 0
	}

	#if (boardBMS_EN && (!boardDEBUG))
	vBms_RecTickTimer();
	#endif  //boardBMS_EN && (!boardDEBUG)

	#if (boardMPPT_EN && (!boardDEBUG))
	vMppt_RecTickTimer();
	#endif  //boardMPPT_EN && (!boardDEBUG)

	#if (boardDCAC_EN && (!boardDEBUG))
	vDcac_RecTickTimer();
	#endif  //boardDCAC_EN && (!boardDEBUG)

	#if (boardWIFI_IFACE && (!boardDEBUG))
	vWiFi_RecTickTimer();
	#endif  //boardWIFI_IFACE && (!boardDEBUG)

	#if (boardPRINT_IFACE && (!boardDEBUG))
	vPrint_RecTickTimer();
	#endif  //boardPRINT_IFACE && (!boardDEBUG)
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 定时器进入低功耗模式
 * 说明(备注)  : 删除处于运行状态的软件定时器以避免休眠期间唤醒
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vCount_EnterLowPower(void)
{
	#if (boardBMS_485_IFACE_EN)
	xTimerDelete(tBmsRxEnTimer, 100);
	#endif  //boardBMS_485_IFACE_EN

	#if (boardBMS_EN)
	xTimerDelete(tWakeUpBmsTimer, 100);
	#endif  //boardBMS_EN

	#if (boardDCAC_485_IFACE_EN)
	xTimerDelete(tDcacRxEnTimer, 100);
	#endif  //boardDCAC_485_IFACE_EN

	#if (boardMPPT_485_IFACE_EN)
	xTimerDelete(tMpptRxEnTimer, 100);
	#endif  //boardMPPT_485_IFACE_EN

	xTimerDelete(s_t_repet_timer, 100);
}

/***********************************************************************************************************************
 * 函数功能    : 定时器退出低功耗模式
 * 说明(备注)  : 唤醒后重新初始化并启动软件定时器
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vCount_ExitLowPower(void)
{
	cTimer_TaskInit();
}
#endif  //boardLOW_POWER
