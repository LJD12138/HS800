/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统总任务调度与安全状态机管理实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task.h"
#include "Print/print_task.h"
#include "..\..\BOOT\Application\flash_allot_table.h"

#include "gpio_init.h"
#include "main.h"
#include "timer_task.h"
#include "app_info.h"
#include "function.h"
#include <stdbool.h>

#if (boardADC_EN)
#include "Adc/adc_task.h"
#endif  //(boardADC_EN)

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //(boardBUZ_EN)

#if (boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //(boardUSB_EN)

#if (boardDC_EN)
#include "Dc/dc_task.h"
#endif  //(boardDC_EN)

#if (boardKEY_EN)
#include "Key/key_task.h"
#endif  //(boardKEY_EN)

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //(boardDISPLAY_EN)

#if (boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif  //(boardLIGHT_EN)

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  //(boardBMS_EN)

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#endif  //(boardMPPT_EN)

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#endif  //(boardDCAC_EN)

#if (boardENG_MODE_EN)
#include "Sys/sys_queue_task_eng.h"
#endif  //(boardENG_MODE_EN)

#if (boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_hm_task.h"
#endif  //boardHEAT_MANAGE_EN


//****************************************************Macros********************************************************************//
#if (boardUSE_OS)
#define			SYS_TASK_PRIO							4		//任务优先级(安全决策层:保护链源头)
#define			SYS_TASK_STK_SIZE						256		//任务堆栈 实际字节数 *4
TaskHandle_t tSysTaskHandler = NULL;
void        vSys_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) SysInfo_T tSysInfo;
static Task_T *s_tp_task = NULL;
bool G_TestMode = false;

/* 系统记忆参数步进配置表 */
typedef struct
{
	void				*pParam;
	int32_t				lMin;
	int32_t				lMax;
	uint8_t				ucType;				/* 0: uint16_t, 1: int8_t */
}SysParamStep_T;

static const SysParamStep_T s_tSysParamTable[] =
{
	{NULL,                                      0,     0, 0}, /* item 0: 保留 */
	{NULL,                                      0,     0, 0}, /* item 1: 保留 */
	{(void *)&tAppMemParam.tSYS.usAutoOffTime,   0,  3600, 0}, /* item 2: 自动关机时间 */
	{(void *)&tAppMemParam.tSYS.sMaxTemp,     -127,   127, 1}, /* item 3: 最大温度 */
	{(void *)&tAppMemParam.tSYS.sMinTemp,     -127,   127, 1}, /* item 4: 最小温度 */
	{(void *)&tAppMemParam.tSYS.usMinOpenVolt,   0, 60000, 0}, /* item 5: 最小开机电压 */
};

//****************************************************Function Declaration******************************************************//
static bool b_task_param_init(void);
static void v_sys_check_prote(void);
static void v_sys_get_perm(void);


/***********************************************************************************************************************
 * 函数功能    : 系统总任务创建初始化
 * 说明(备注)  : 初始化主任务队列对象并创建 FreeRTOS 系统调度任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
s8 cSys_TaskInit(void)
{
	if (bSys_QueueInit() == false)
		return -1;

	if (b_task_param_init() == false)
		return -2;

	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t)vSys_Task,              //任务函数
	            (const char *)"bSysTask",              //任务名称
	            (uint16_t)SYS_TASK_STK_SIZE,            //任务堆栈大小
	            (void *)NULL,                          //传递给任务函数的参数
	            (UBaseType_t)SYS_TASK_PRIO,            //任务优先级
	            (TaskHandle_t *)&tSysTaskHandler) != pdPASS)      //任务句柄
		return -3;

	vQueue_BindTaskHandler(tpSysTask, tSysTaskHandler);
	#endif  //boardUSE_OS

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 系统任务参数初始化
 * 说明(备注)  : 重置系统信息结构体、设置默认上下限温度及初始化状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 成功, false 失败
 ************************************************************************************************************************/
static bool b_task_param_init(void)
{
	if (tpSysTask == NULL)
		return false;

	//系统任务参数
	memset(&tSysInfo, 0, sizeof(tSysInfo));

	tSysInfo.sMaxTemp = 25;             //设置默认最高温度
	tSysInfo.sMinTemp = 25;             //设置默认最低温度
	bSys_SetAutoOffTime(tAppMemParam.tSYS.usAutoOffTime);
	bSys_SetDevState(DS_INIT, false);   //进入初始化

	s_tp_task = tpSysTask;

	#if (boardUPDATE)
	bUpdate_Init();
	#endif  //boardUPDATE

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 系统总任务调度主循环
 * 说明(备注)  : 周期检测各模块过压/欠压/过温/过载故障保护链，评估充放电许可，轮询队列事件
 * 传入参数    : pvParameters: FreeRTOS 任务入参
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vSys_Task(void *pvParameters)
{
	(void)pvParameters;

	#if (boardUSE_OS)
	for (;;)
	#endif  //(boardUSE_OS)
	{
		if (s_tp_task == NULL)
		{
			b_task_param_init();

			#if (boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif  //(boardUSE_OS)
		}

		if (tSysInfo.eDevState != DS_UPDATE_MODE)
		{
			v_sys_check_prote();
			v_sys_get_perm();
		}

		vQueue_TaskPoll(s_tp_task, sysTASK_CYCLE_TIME);
	}
}

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 系统故障保护状态综合检查
 * 说明(备注)  : 聚合采集整机温度与板温，核算整机输入输出功率，评估放电过载、过压、欠压、过温、低温与低SOC
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_sys_check_prote(void)
{
	vu16 us_delay_time = 0;

	if ((tpSysTask->ucID == STI_INIT) ||
	    (tpSysTask->ucID == STI_CLOSING) ||
	    (tpSysTask->ucID == STI_SHUT_DOWN))
		return;
	else if ((tpSysTask->ucID == STI_BOOTING) ||
	         (tpSysTask->ucID == STI_WORK))
		us_delay_time = 10;
	else
		us_delay_time = sysTASK_CYCLE_TIME;

	//------------------------------------------------温度获取-----------------------------------------------
	s16 s_min_temp = 255;
	s16 s_max_temp = 0;
	s16 s_board_max_temp = 0;
	s16 s_hm_temp = 0;

	#if (boardUSB_EN)
	if (tUsb.eDevState >= DS_WORK)
	{
		if (tUsb.sMaxTemp >= 45)
			s_hm_temp = MAX2(s_hm_temp, 45);
		else
			s_hm_temp = MAX2(s_hm_temp, tUsb.sMaxTemp);

		s_board_max_temp = MAX2(s_board_max_temp, tUsb.sMaxTemp);
	}
	#endif  //boardUSB_EN

	#if (boardDC_EN)
	if (tDc.eDevState >= DS_WORK)
	{
		if (tDc.sMaxTemp >= 45)
			s_hm_temp = MAX2(s_hm_temp, 45);
		else
			s_hm_temp = MAX2(s_hm_temp, tDc.sMaxTemp);

		s_board_max_temp = MAX2(s_board_max_temp, tDc.sMaxTemp);
	}
	#endif  //boardDC_EN

	#if (boardBMS_EN)
	if (tBmsRx.tDevInfo[0].sMaxTemp >= 45)
		s_hm_temp = MAX2(s_hm_temp, 45);
	else
		s_hm_temp = MAX2(s_hm_temp, tBmsRx.tDevInfo[0].sMaxTemp);

	s_max_temp = MAX2(s_max_temp, tBms.sMaxTemp);
	s_min_temp = MIN2(s_min_temp, tBms.sMinTemp);
	#endif  //boardBMS_EN

	#if (boardDCAC_EN)
	s_hm_temp = MAX2(s_hm_temp, tDcac.sMaxTemp);
	s_max_temp = MAX2(s_max_temp, tDcac.sMaxTemp);
	#endif  //boardDCAC_EN

	#if (boardMPPT_EN)
	s_hm_temp = MAX2(s_hm_temp, tMppt.sMaxTemp);
	s_max_temp = MAX2(s_max_temp, tMppt.sMaxTemp);
	#endif  //boardMPPT_EN

	tSysInfo.sBoardTempMax = s_board_max_temp;
	s_max_temp = MAX2(tSysInfo.sBoardTempMax, s_max_temp);

	tSysInfo.sMinTemp = s_min_temp;
	tSysInfo.sMaxTemp = s_max_temp;
	tHM.sMaxTemp = s_hm_temp;

	//------------------------------------------------功率计算-----------------------------------------------
	tSysInfo.usOutPwr = 0;
	tSysInfo.usInPwr = 0;

	#if (boardBMS_EN && boardDCAC_EN)
	if (ucBms_GetSoc() == 100)
	{
		if ((tDcac.eDisChgState == IOS_WORK) && (tDcac.eChgState == IOS_WORK))
			tDcacRx.usInPwr = tDcacRx.usOutPwr;
		else if (tDcac.eChgState == IOS_WORK)
			tDcacRx.usInPwr = 0;
	}
	#endif  //boardBMS_EN

	#if (boardDC_EN)
	if (tDc.eDevState == DS_WORK)
		tSysInfo.usOutPwr += tDc.usOutPwr;
	#endif  //boardDC_EN

	#if (boardUSB_EN)
	if (tUsb.eDevState == DS_WORK)
		tSysInfo.usOutPwr += tUsb.usOutPwr;
	#endif  //boardUSB_EN

	#if (boardLIGHT_EN)
	if (tLight.eDevState == DS_WORK)
		tSysInfo.usOutPwr += tLight.usPower;
	#endif  //boardLIGHT_EN

	#if (boardDCAC_EN)
	if (tDcac.eChgState == IOS_WORK)
		tSysInfo.usInPwr += tDcacRx.usInPwr;

	if (tDcac.eDisChgState == IOS_WORK)
		tSysInfo.usOutPwr += tDcacRx.usOutPwr;

	if (tDcac.eParanInState == IOS_WORK)
		tSysInfo.usOutPwr += tDcacRx.usParaInPwr;
	#endif  //boardDCAC_EN

	#if (boardMPPT_EN)
	if (tMppt.eDevState == DS_WORK)
		tSysInfo.usInPwr += tMppt.usInPwr;
	#endif  //boardMPPT_EN

	//-----------------------------------------放电过功率保护(BMS许可功率限制)----------------------------------------
	#if (boardBMS_EN)
	static u16 s_us_dischg_ol_set_cnt = 0;
	static u16 s_us_dischg_ol_clr_cnt = 0;
	u16 us_perm_dischg = tBmsRx.usPermMaxDisChgPwr;
	vs16 s_bat_out_pwr = tSysInfo.usOutPwr - tSysInfo.usInPwr;

	if ((us_perm_dischg > 0) && (s_bat_out_pwr > us_perm_dischg))
	{
		s_us_dischg_ol_clr_cnt = 0;
		if (tSysInfo.uErrCode.tCode.bDisChgOL == 0)
		{
			if (++s_us_dischg_ol_set_cnt >= (500 / us_delay_time)) //0.5s防抖
			{
				s_us_dischg_ol_set_cnt = 0;
				bSys_SetErrCode(SEC_DISCHG_OL, true);
			}
		}
	}
	else
	{
		s_us_dischg_ol_set_cnt = 0;
		if (tSysInfo.uErrCode.tCode.bDisChgOL == 1)
		{
			if (++s_us_dischg_ol_clr_cnt >= (2000 / us_delay_time)) //2s
			{
				s_us_dischg_ol_clr_cnt = 0;
				bSys_SetErrCode(SEC_DISCHG_OL, false);
			}
		}
	}
	#endif  //boardBMS_EN

	//------------------------------------------------系统输入过压-----------------------------------------------
	#if (boardADC_EN)
	static u16 s_us_over_volt_cnt = 0;
	if (sSys_CheckInVolt() == 0)
	{
		if (tSysInfo.uErrCode.tCode.bOV == 0)
		{
			s_us_over_volt_cnt++;
			if (s_us_over_volt_cnt > (1000 / us_delay_time))
			{
				s_us_over_volt_cnt = 0;
				bSys_SetErrCode(SEC_OV, true);
			}
		}
	}
	else
	{
		s_us_over_volt_cnt = 0;
		if (tSysInfo.uErrCode.tCode.bOV == 1)
		{
			tSysInfo.uErrCode.tCode.bOV = 0;
			bSys_SetErrCode(SEC_OV, false);
		}
	}

	//-----------------------------------------------系统输入欠压--------------------------------------------------
	static u16 s_us_low_volt_cnt = 0;
	if ((sSys_CheckInVolt() < 0
	#if (boardBMS_EN)
	     || tBms.uErrCode.tCode.uBmsCode.tCode.bCellUV
	#endif  //(boardBMS_EN)
	    )
	    && (bSys_ExistInVolt() == false)
	)
	{
		if (tSysInfo.uErrCode.tCode.bUV == 0)
		{
			s_us_low_volt_cnt++;
			if (s_us_low_volt_cnt > (1000 / us_delay_time))
			{
				s_us_low_volt_cnt = 0;
				bSys_SetErrCode(SEC_UV, true);
			}
		}
		else if (tpSysTask->ucID == STI_WORK)
		{
			cQueue_AddQueueTask(tpSysTask, STI_ERR, SEC_UV, false);
			s_us_low_volt_cnt = 0;
		}
	}
	else
	{
		s_us_low_volt_cnt = 0;
		if (tSysInfo.uErrCode.tCode.bUV == 1)
		{
			tSysInfo.uErrCode.tCode.bUV = 0;
			bSys_SetErrCode(SEC_UV, false);
		}
	}
	#endif  //(boardADC_EN)

	//------------------------------------------------系统输入过温-----------------------------------------------------
	if (
	#if (boardDCAC_EN)
		tDcac.uErrCode.tCode.bDcacOT == 1
		|| tDcac.uErrCode.tCode.bSysOT == 1
	#else
		false
	#endif  //boardDCAC_EN

	#if (boardBMS_EN)
		|| tBms.uErrCode.tCode.bSysChgOT == 1
		|| tBms.uErrCode.tCode.bSysDisChgOT == 1
		|| tBms.uErrCode.tCode.uBmsCode.tCode.bDCOT == 1
		|| tBms.uErrCode.tCode.uBmsCode.tCode.bCOT == 1
	#endif  //boardBMS_EN

	#if (boardDC_EN)
		|| tDc.uErrCode.tCode.bOT == 1
	#endif  //boardDC_EN

	#if (boardUSB_EN)
		|| tUsb.uErrCode.tCode.bOT == 1
	#endif  //boardUSB_EN
	)
	{
		if (tSysInfo.uErrCode.tCode.bOT == 0)
			bSys_SetErrCode(SEC_OT, true);
	}
	else
	{
		if (tSysInfo.uErrCode.tCode.bOT == 1)
			bSys_SetErrCode(SEC_OT, false);
	}

	//---------------------------------------------系统输入低温 ---------------------------------------------------
	if (
	#if (boardBMS_EN)
		tBms.uErrCode.tCode.bSysChgUT == 1
		|| tBms.uErrCode.tCode.bSysDisChgUT == 1
		|| tBms.uErrCode.tCode.uBmsCode.tCode.bDCUT == 1
		|| tBms.uErrCode.tCode.uBmsCode.tCode.bCUT == 1
	#else
		false
	#endif  //boardBMS_EN
	)
	{
		if (tSysInfo.uErrCode.tCode.bUT == 0)
			bSys_SetErrCode(SEC_UT, true);
	}
	else
	{
		if (tSysInfo.uErrCode.tCode.bUT == 1)
			bSys_SetErrCode(SEC_UT, false);
	}

	//-----------------------------------------系统过载保护---------------------------------------------------
	if (
	#if (boardDCAC_EN)
		tDcac.uErrCode.tCode.bDcacOL == 1
		|| tDcac.uErrCode.tCode.bSysOutOL == 1
	#else
		false
	#endif  //boardDCAC_EN

	#if (boardDC_EN)
		|| tDc.uErrCode.tCode.bOL == 1
	#endif  //boardDC_EN
	)
	{
		if (tSysInfo.uErrCode.tCode.bOL == 0)
			bSys_SetErrCode(SEC_OL, true);
	}
	else
	{
		if (tSysInfo.uErrCode.tCode.bOL == 1)
			bSys_SetErrCode(SEC_OL, false);
	}

	//--------------------------------------低SOC自动关机--------------------------------------------------------
	#if (boardBMS_EN)
	static u16 s_us_soc_low_cnt = 0;
	if ((ucBms_GetSoc() == 0) &&          //SOC = 0%
	    (tBms.eDevState != DS_LOST) &&    //BMS非离线
	    (bSys_ExistInVolt() == false) &&  //非充电状态
	    (G_TestMode == false))            //非测试模式
	{
		if (tSysInfo.uErrCode.tCode.b0SOC == false)
			s_us_soc_low_cnt++;
		else if (tpSysTask->ucID == STI_WORK)
		{
			cQueue_AddQueueTask(tpSysTask, STI_ERR, SEC_0_SOC, false);
			s_us_soc_low_cnt = 0;
		}

		if (s_us_soc_low_cnt >= (2000 / us_delay_time))
		{
			s_us_soc_low_cnt = 0;
			bSys_SetErrCode(SEC_0_SOC, true);
		}
	}
	else if (tSysInfo.uErrCode.tCode.b0SOC == 1)
		bSys_SetErrCode(SEC_0_SOC, false);
	#endif  //boardBMS_EN
}

/***********************************************************************************************************************
 * 函数功能    : 获取并更新系统的充放电许可
 * 说明(备注)  : 综合考虑强制关机、过压、低温、欠压、过温及 BMS 许可状态计算充电和放电许可
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_sys_get_perm(void)
{
	//测试模式
	if (G_TestMode == true)
	{
		bSys_SetPerm(SPO_CHG, true);
		bSys_SetPerm(SPO_DISCHG, true);
		return;
	}
	//初始化状态或关机
	else if (tpSysTask->ucID == STI_INIT)
	{
		bSys_SetPerm(SPO_ALL, false);
		return;
	}

	//------------------------------------------充电许可------------------------------------------------------
	if ((tSysInfo.uPerm.tPerm.bForceClose == 1)  ||  //强制关闭
	    (tSysInfo.eDevState == DS_CLOSING)        ||  //开始关闭
	    (tSysInfo.uErrCode.tCode.bOV == 1)        ||  //系统过压
	    (tSysInfo.uErrCode.tCode.bUT == 1)            //系统低温(严禁充电)
	#if (boardBMS_EN)
	    || (tBms.uPerm.tPerm.bChgPerm == 0)          //BMS不许可充电
	#endif  //(boardBMS_EN)
	)
	{
		if (tSysInfo.uPerm.tPerm.bChgPerm == true)
			bSys_SetPerm(SPO_CHG, false);
	}
	else
	{
		if (tSysInfo.uPerm.tPerm.bChgPerm == false)
			bSys_SetPerm(SPO_CHG, true);
	}

	//------------------------------------------放电许可------------------------------------------------------
	if ((tSysInfo.uPerm.tPerm.bForceClose == 1)  ||  //强制关闭
	    (tSysInfo.eDevState == DS_CLOSING)        ||  //开始关闭
	    (tSysInfo.uErrCode.tCode.b0SOC == 1)      ||  //低SOC
	    (tSysInfo.uErrCode.tCode.bDisChgOL == 1)  ||  //放电过载
	    (tSysInfo.uErrCode.tCode.bUV == 1)        ||  //欠压
	    (tSysInfo.uErrCode.tCode.bOT == 1)            //系统过温(强制停止放电)
	#if (boardBMS_EN)
	    || (tBms.uPerm.tPerm.bDisChgPerm == 0)        //BMS不许可放电
	#endif  //(boardBMS_EN)
	)
	{
		if (tSysInfo.uPerm.tPerm.bDisChgPerm == true)
			bSys_SetPerm(SPO_DISCHG, false);
	}
	else
	{
		if (tSysInfo.uPerm.tPerm.bDisChgPerm == false)
			bSys_SetPerm(SPO_DISCHG, true);
	}
}

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 系统时钟节拍计时器
 * 说明(备注)  : 处理自动关机倒计时与工作状态监测
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vSys_TickTimer(void)
{
	static u8 s_uc_cnt = 0;

	s_uc_cnt++;
	if (s_uc_cnt >= 2)
	{
		s_uc_cnt = 0;
		switch (tSysInfo.eDevState)
		{
			case DS_INIT:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_INIT !\r\n");
			}break;

			case DS_CLOSING:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_CLOSING !\r\n");
			}break;

			case DS_SHUT_DOWN:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_SHUT_DOWN !\r\n");
			}break;

			case DS_ERR:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_ERR !\r\n");
			}break;

			case DS_BOOTING:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_BOOTING !\r\n");
			}break;

			case DS_WORK:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_WORK !\r\n");
			}break;

			#if (boardENG_MODE_EN)
			case DS_ENG_MODE:
				if (uPrint.tFlag.bSysTask)
				{
					sMyPrint("bSysTask = DS_ENG_MODE !\r\n");
				#if printSEGGER
					SEGGER_RTT_printf(0, "bSysTask = DS_INIT !\r\n");
				#endif  //printSEGGER
				}
				break;
			#endif  //boardENG_MODE_EN

			default:
			{
			}
			break;
		}
	}

	if (bSys_IsWorkState() == false) //非工作状态不检测
		return;

	//***************************************************关机倒计时*****************************************************
	if (tSysInfo.usAutoOffTime)
	{
		if (tSysInfo.usAutoOffCnt)
		{
			tSysInfo.usAutoOffCnt--;
			if (tSysInfo.usAutoOffCnt == 0) //倒计时为0进入
			{
				#if (boardDISPLAY_EN)
				bDisp_SwitchBacklight(DISP_BKL_ON, false);
				#endif  //boardDISPLAY_EN

				cSys_Switch(SO_KEY, ST_OFF, false);
				if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
					sMyPrint("bSysTask:====倒计时结束,进入关机 时间=%dS====\r\n", tSysInfo.usAutoOffTime);
			}
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 刷新自动关机倒计时
 * 说明(备注)  : 将倒计时计数器恢复为设定的自动关闭时间
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vSys_RefreshOffTime(void)
{
	if (tSysInfo.usAutoOffTime)
		tSysInfo.usAutoOffCnt = tSysInfo.usAutoOffTime; //更新倒计时
}

/***********************************************************************************************************************
 * 函数功能    : 刷新系统全部的关机倒计时
 * 说明(备注)  : 统一重置系统自动关机时间
 * 传入参数    : BLON: 背光使能
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vSys_RefreshAllOffTime(bool BLON)
{
	(void)BLON;
	vSys_RefreshOffTime(); //系统关机倒计时
}

/***********************************************************************************************************************
 * 函数功能    : 设置自动关闭功能时间
 * 说明(备注)  : 设置 0 为关闭此功能
 * 传入参数    : time: 自动关机时间 (秒)
 * 输出参数    : 无
 * 返回值      : bool: true 成功
 ************************************************************************************************************************/
bool bSys_SetAutoOffTime(u16 time)
{
	tSysInfo.usAutoOffTime = time;
	vSys_RefreshOffTime();
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置系统运行状态
 * 说明(备注)  : 切换系统主状态机并可选择鸣叫蜂鸣器
 * 传入参数    : state: 目标设备状态, bz: 是否鸣叫蜂鸣器
 * 输出参数    : 无
 * 返回值      : bool: true 成功
 ************************************************************************************************************************/
bool bSys_SetDevState(DevState_E state, bool bz)
{
	if (tSysInfo.eDevState != state)
	{
		tSysInfo.eDevState = state;
		if (tSysInfo.eDevState == DS_INIT) //初始化
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为初始化\r\n");
		}
		else if (tSysInfo.eDevState == DS_CLOSING) //关闭中
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为关闭中\r\n");
		}
		else if (tSysInfo.eDevState == DS_SHUT_DOWN) //关闭
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为关闭\r\n");
		}
		else if (tSysInfo.eDevState == DS_ERR) //错误
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为错误\r\n");
		}
		else if (tSysInfo.eDevState == DS_BOOTING) //启动中
		{
			vSys_RefreshAllOffTime(true);
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为启动中\r\n");
		}
		else if (tSysInfo.eDevState == DS_WORK) //工作
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为工作\r\n");
		}
		#if (boardENG_MODE_EN)
		else if (tSysInfo.eDevState == DS_ENG_MODE) //工程模式
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为工程模式----\r\n");
		}
		#endif  //boardENG_MODE_EN
		else if (tSysInfo.eDevState == DS_UPDATE_MODE) /* 升级 */
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为升级模式----\r\n");
		}

		#if (boardDISPLAY_EN)
		vDisp_PortWakeTask(); /* 状态切换即时唤醒显示任务响应 */
		#endif  //(boardDISPLAY_EN)
	}

	#if (boardBUZ_EN)
	if (bz)
		bBuz_Tweet(LONG_1);
	#endif  //(boardBUZ_EN)

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 判断系统是否处于工作状态
 * 说明(备注)  : 包含工作态与启动中态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 工作中, false 未工作
 ************************************************************************************************************************/
bool bSys_IsWorkState(void)
{
	if ((tSysInfo.eDevState == DS_BOOTING) || (tSysInfo.eDevState == DS_WORK))
		return true;
	else
		return false;
}

/***********************************************************************************************************************
 * 函数功能    : 判断系统是否处于关机状态
 * 说明(备注)  : 包含关机态与关机进行中态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 关机状态, false 非关机状态
 ************************************************************************************************************************/
bool bSys_IsShutDownState(void)
{
	if ((tSysInfo.eDevState == DS_CLOSING) || (tSysInfo.eDevState == DS_SHUT_DOWN))
		return true;
	else
		return false;
}

/***********************************************************************************************************************
 * 函数功能    : 检查系统当前是否处于活跃状态
 * 说明(备注)  : 判断是否有充电输入、USB/DC/照明/逆变等外设在工作
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 处于活跃状态, false 不活跃
 ************************************************************************************************************************/
bool bSys_CheckActState(void)
{
	if ((cSys_IsChgState() > 0) ||          //充电状态
	    (bSys_ExistInVolt() == true)        //还插着电
	#if (boardUSB_EN)
	    || (tUsb.eDevState >= DS_BOOTING)   //USB工作
	#endif  //(boardUSB_EN)
	#if (boardLIGHT_EN)
	    || (tLight.eDevState >= DS_WORK)    //照明工作
	#endif  //(boardLIGHT_EN)
	#if (boardDC_EN)
	    || (tDc.eDevState >= DS_BOOTING)    //DC工作
	#endif  //(boardDC_EN)
	#if (boardDCAC_EN)
	    || (tDcac.eDisChgState >= IOS_WORK) //逆变开启
	#endif  //(boardDCAC_EN)
	)
		return true;
	else
		return false;
}

/***********************************************************************************************************************
 * 函数功能    : 检测是否存在有效输入电源
 * 说明(备注)  : 检测 DCAC 或 MPPT 输入电压是否超过最小设定门限
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 插着充电, false 断开充电
 ************************************************************************************************************************/
bool bSys_ExistInVolt(void)
{
	if (
	#if (boardDCAC_EN)
		tDcacRx.usInVolt > tAppMemParam.tDCAC.usMinInVolt
	#else
		false
	#endif  //(boardDCAC_EN)
	#if (boardMPPT_EN)
		|| tMpptRx.usInVolt > tAppMemParam.tMPPT.usMinInVolt
	#endif  //(boardMPPT_EN)
	)
		return true;
	else
		return false;
}

/***********************************************************************************************************************
 * 函数功能    : 获取当前系统充电状态
 * 说明(备注)  : 检测 DCAC 与 MPPT 是否处于启动/工作态，并检测 BMS 是否有充电电流
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : s8: 2 充电中且有电流, 1 充电状态但无电流, 0 非充电状态
 ************************************************************************************************************************/
s8 cSys_IsChgState(void)
{
	if (
	#if (boardDCAC_EN)
		tDcac.eChgState >= IOS_STARTING
	#else
		false
	#endif  //(boardDCAC_EN)
	#if (boardMPPT_EN)
		|| tMppt.eDevState >= DS_BOOTING
	#endif  //(boardMPPT_EN)
	)
	{
		#if (boardBMS_EN)
		if (tBms.eWorkState == BWS_CHG)
			return 2;
		#endif  //boardBMS_EN

		return 1;
	}
	else
		return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 外部充电接入唤醒开机
 * 说明(备注)  : 在关机状态下检测到充电接入时触发系统开机
 * 传入参数    : obj: 触发对象
 * 输出参数    : 无
 * 返回值      : bool: true 唤醒成功, false 条件不符
 ************************************************************************************************************************/
bool bSys_ChgWakeUp(SwitchObject_E obj)
{
	//关机状态下 && 非强制关机
	if (((bSys_IsShutDownState() == true) || (tSysInfo.eDevState == DS_INIT)) &&
	    (tSysInfo.uPerm.tPerm.bForceClose == false))
	{
		cSys_Switch(obj, ST_ON, false); //开机
		if (uPrint.tFlag.bSysTask)
			sMyPrint("bSysTask:开启充电唤醒\r\n");
		return true;
	}
	else
		return false;
}

/***********************************************************************************************************************
 * 函数功能    : 系统开关机控制函数
 * 说明(备注)  : 控制系统启动与关机队列任务调度
 * 传入参数    : obj: 开关对象, type: 开关类型 (ST_NULL 取反, ST_ON 开机, ST_OFF 关机), fore_en: 强制执行
 * 输出参数    : 无
 * 返回值      : s8: <0 错误, 0 无操作, >0 成功
 ************************************************************************************************************************/
s8 cSys_Switch(SwitchObject_E obj, SwitchType_E type, bool fore_en)
{
	TaskInParam_U u_param;
	bool b_switch_target = false;

	u_param.tTaskParam.ucObj = obj;

	switch (type)
	{
		case ST_ON:
		{
			if ((bSys_IsWorkState() == true) && (fore_en == false))
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:当前状态为工作,不允许开机.对象:%d\r\n", u_param.tTaskParam.ucObj);
				return 0;
			}
			b_switch_target = true;
		}break;

		case ST_OFF:
		{
			if ((bSys_IsShutDownState() == true) && (fore_en == false))
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:当前状态为关机,不允许关机.对象:%d\r\n", u_param.tTaskParam.ucObj);
				return 0;
			}
			b_switch_target = false;
		}break;

		default:
		{
			if (bSys_IsShutDownState() == true)
				b_switch_target = true;
			else
				b_switch_target = false;
		}break;
	}

	if (b_switch_target == true)
	{
		u_param.tTaskParam.ucParam = ST_ON;
		bSys_SetDevState(DS_BOOTING, true);
		vGPIO_AssistBmsOpen(true);
		if (tSysInfo.eDevState == DS_INIT)
			cQueue_AddQueueTask(tpSysTask, STI_BOOTING, u_param.usTaskInParam, false);
		else
			cQueue_AddQueueTask(tpSysTask, STI_BOOTING, u_param.usTaskInParam, fore_en);

		if (uPrint.tFlag.bSysTask)
			sMyPrint("bSysTask:开机\r\n");
	}
	else
	{
		// 插着充电线, 不关机
		if ((bSys_ExistInVolt() == true) && (fore_en == false))
		{
			#if (boardBUZ_EN)
			bBuz_Tweet(LONG_2);
			#endif  //(boardBUZ_EN)

			if (uPrint.tFlag.bSysTask)
				log_w("bSysTask:插着充电线,不允许关机");
			return -2;
		}

		if (fore_en == true
		#if (boardDISPLAY_EN)
		    || bDisp_IsBacklightOn()
		#endif  /* boardDISPLAY_EN */
		)
		{
			u_param.tTaskParam.ucParam = ST_OFF;
			cQueue_AddQueueTask(tpSysTask, STI_CLOSING, u_param.usTaskInParam, fore_en);

			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:关机\r\n");
		}
		else
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:屏幕休眠,开始唤醒屏幕\r\n");
		}
	}

	#if (boardDISPLAY_EN)
	bDisp_SwitchBacklight(DISP_BKL_ON, false);
	#endif  //(boardDISPLAY_EN)

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化系统记忆参数
 * 说明(备注)  : 恢复出厂默认值
 * 传入参数    : p_sys_mem: 记忆参数指针
 * 输出参数    : p_sys_mem: 填充默认配置
 * 返回值      : bool: true 成功
 ************************************************************************************************************************/
bool bSys_MemParamInit(SysMemParam_T *p_sys_mem)
{
	p_sys_mem->usAutoOffTime = boardSYS_OFF_TIME;
	p_sys_mem->sMaxTemp = boardSYS_MAX_TEMP;
	p_sys_mem->sMinTemp = boardSYS_MIN_TEMP;
	p_sys_mem->usMinOpenVolt = boardSYS_OPEN_MIN_VOLT;
	p_sys_mem->bBuzSwitchOff = 0;
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置系统记忆参数 (表驱动版)
 * 说明(备注)  : 支持 uint16 与 int8 类型，自动进行上下限防越界检查
 * 传入参数    : item: 参数索引, add: true-增加, false-减少
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vSys_MemParamSet(u8 item, bool add)
{
	if ((item >= mainARRAY_SIZE(s_tSysParamTable)) || (s_tSysParamTable[item].pParam == NULL))
		return;

	const SysParamStep_T *p = &s_tSysParamTable[item];
	if (p->ucType == 1)
	{
		int8_t *p_val = (int8_t *)p->pParam;
		if (add && (*p_val < (int8_t)p->lMax))
			(*p_val)++;
		else if (!add && (*p_val > (int8_t)p->lMin))
			(*p_val)--;
	}
	else
	{
		uint16_t *p_val = (uint16_t *)p->pParam;
		if (add && (*p_val < (uint16_t)p->lMax))
			(*p_val)++;
		else if (!add && (*p_val > (uint16_t)p->lMin))
			(*p_val)--;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 设置充放电许可
 * 说明(备注)  : 设置系统各回路充电、放电或强制关机许可
 * 传入参数    : obj: 许可对象, en: true 许可, false 禁止
 * 输出参数    : 无
 * 返回值      : bool: true 设置成功, false 失败
 ************************************************************************************************************************/
bool bSys_SetPerm(SysPermObject_E obj, bool en)
{
	switch (obj)
	{
		case SPO_CHG:
		{
			//状态变化
			if (en != tSysInfo.uPerm.tPerm.bChgPerm)
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置充电许可: 设置=%d 当前状态=%d \r\n", en, tSysInfo.uPerm.tPerm.bChgPerm);

				tSysInfo.uPerm.tPerm.bChgPerm = en;
			}
		}break;

		case SPO_DISCHG:
		{
			//状态变化
			if (en != tSysInfo.uPerm.tPerm.bDisChgPerm)
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置放电许可: 设置=%d 当前状态=%d \r\n", en, tSysInfo.uPerm.tPerm.bDisChgPerm);

				tSysInfo.uPerm.tPerm.bDisChgPerm = en;
			}
		}break;

		case SPO_FORCE_CLOSE:
		{
			//状态变化
			if (en != tSysInfo.uPerm.tPerm.bForceClose)
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置强制关机: 设置=%d 当前状态=%d \r\n", en, tSysInfo.uPerm.tPerm.bForceClose);

				tSysInfo.uPerm.tPerm.bForceClose = en;
			}
		}break;

		case SPO_ALL:
		{
			//状态变化
			if (en != tSysInfo.uPerm.tPerm.bChgPerm)
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置充电许可: 设置=%d 当前状态=%d \r\n", en, tSysInfo.uPerm.tPerm.bChgPerm);

				tSysInfo.uPerm.tPerm.bChgPerm = en;
			}

			//状态变化
			if (en != tSysInfo.uPerm.tPerm.bDisChgPerm)
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置放电许可: 设置=%d 当前状态=%d \r\n", en, tSysInfo.uPerm.tPerm.bDisChgPerm);

				tSysInfo.uPerm.tPerm.bDisChgPerm = en;
			}

			//状态变化
			if (en != tSysInfo.uPerm.tPerm.bForceClose)
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置强制关机: 设置=%d 当前状态=%d \r\n", en, tSysInfo.uPerm.tPerm.bForceClose);

				tSysInfo.uPerm.tPerm.bForceClose = en;
			}
		}break;

		default:
			return false;
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置系统设备错误代码
 * 说明(备注)  : 置位或清除指定错误位，并向系统任务队列投递错误处理任务
 * 传入参数    : code: 错误代码枚举, set: true 置位错误, false 清除错误
 * 输出参数    : 无
 * 返回值      : bool: true 成功
 ************************************************************************************************************************/
bool bSys_SetErrCode(SysErrCode_E code, bool set)
{
	static SysErrCode_E s_e_next_code;
	static bool         s_b_next_set;

	if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
	{
		if ((s_e_next_code != code) || (s_b_next_set != set))
		{
			if (tSysInfo.uErrCode.tCode.bOV)
			{
				#if (boardADC_EN)
				log_e("bSysTask:输入过压 电压=%dV", tAdcSamp.usSysInVolt / 10);
				#else
				log_e("bSysTask:输入过压");
				#endif  //(boardADC_EN)
			}
			else if (tSysInfo.uErrCode.tCode.bUV)
			{
				#if (boardADC_EN)
				log_e("bSysTask:输入欠压 电压=%dV", tAdcSamp.usSysInVolt / 10);
				#else
				log_e("bSysTask:输入欠压");
				#endif  //(boardADC_EN)
			}
			else if (tSysInfo.uErrCode.tCode.bOT)
				log_e("bSysTask:系统过温 %d摄氏度", tSysInfo.sMaxTemp);
			else if (tSysInfo.uErrCode.tCode.bUT)
				log_e("bSysTask:系统低温,%d摄氏度", tSysInfo.sMinTemp);
			else if (tSysInfo.uErrCode.tCode.bOL)
				log_e("bSysTask:系统过载");
			else if (tSysInfo.uErrCode.tCode.b0SOC)
				log_e("bSysTask:SOC = 0%");
			else if (tSysInfo.uErrCode.tCode.bBootFault)
				log_w("bSysTask:启动任务等待超时,开始关闭系统");
			else if (tSysInfo.uErrCode.tCode.bCloseFault)
				log_w("bSysTask:关闭系统任务等待超时,退出");
			else if (tSysInfo.uErrCode.tCode.bTaskFault)
				log_e("bSysTask:任务初始化/运行故障");
			else
				log_e("bSysTask:系统错误 代码%d 类型%d", code, set);

			s_e_next_code = code;
			s_b_next_set = set;
		}
	}

	//有错误
	if (code > SEC_CLEAR_ALL)
	{
		if (set)
			ERR_SET(tSysInfo.uErrCode.usCode, (code - 1));
		else
			ERR_CLR(tSysInfo.uErrCode.usCode, (code - 1));
	}
	else
		tSysInfo.uErrCode.usCode = 0;

	if (tSysInfo.uErrCode.usCode)
	{
		if (tSysInfo.uErrCode.tCode.bUV || tSysInfo.uErrCode.tCode.b0SOC)
			cQueue_AddQueueTask(tpSysTask, STI_ERR, code, false);
	}
	else
	{
		//清除错误,重新启动
		if (tSysInfo.eDevState == DS_ERR)
		{
			bSys_SetDevState(DS_WORK, true);

			if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
				log_i("bSysTask:清除错误,重新进入工作状态");
		}
	}

	return false;
}

/***********************************************************************************************************************
 * 函数功能    : 欠压请求充电状态判断
 * 说明(备注)  : 判断电池单体是否欠压、BMS 是否允许充电并插着充电线
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 请求欠压充电, false 没有
 ************************************************************************************************************************/
bool bSys_LowVoltReqChg(void)
{
	#if (boardBMS_EN)
	if (tBms.uErrCode.tCode.uBmsCode.tCode.bCellUV == false)
		return false;

	if (tBms.uPerm.tPerm.bChgPerm == false)
		return false;
	#endif  //(boardBMS_EN)

	if (bSys_ExistInVolt() == false)
		return false;

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 检查系统总线输入供电状态
 * 说明(备注)  : 检测 ADC 采样的系统输入电压范围
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : s16: -2 接近0V, -1 小于最小输入, 0 过压, >0 电压正常值
 ************************************************************************************************************************/
s16 sSys_CheckInVolt(void)
{
	#if (boardADC_EN && boardBMS_EN)
	if (RANGE(tAdcSamp.usSysInVolt,
	          tAppMemParam.tSYS.usMinOpenVolt,
	          tAppMemParam.tBMS.usMaxVolt))
		return tAdcSamp.usSysInVolt;
	else if (tAdcSamp.usSysInVolt > tAppMemParam.tBMS.usMaxVolt)
		return 0;
	else if (tAdcSamp.usSysInVolt < (tAppMemParam.tBMS.usMaxVolt / 10))
		return -2;
	else
		return -1;
	#else
	return tAppMemParam.tSYS.usMinOpenVolt;
	#endif  //boardADC_EN
}

#if (boardUSE_OS && boardPRINT_IFACE)

#endif  //(boardUSE_OS && boardPRINT_IFACE)

