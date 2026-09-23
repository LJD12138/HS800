/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Dc
 * File    : dc_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : DC 12V 供电控制与保护处理任务实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Dc/dc_task.h"
#include "Dc/dc_queue_task.h"

#if (boardDC_EN)
#include "Dc/dc_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

#if (boardADC_EN)
#include "Adc/adc_task.h"
#endif  /* boardADC_EN */

//****************************************************Macros********************************************************************//
#define			dcOVER_CURR_PEAK_THRESH					130		/* 峰值过流阈值: 13.0A (单位: 0.1A) */
#define			dcOVER_CURR_CONT_THRESH					116		/* 持续过载阈值: 11.6A (单位: 0.1A) */

#if (boardUSE_OS)
#define			DC_TASK_PRIO							1		/* 任务优先级 */
#define			DC_TASK_SIZE							256		/* 任务堆栈大小 */
TaskHandle_t tDcTaskHandler = NULL;
void vDc_Task(void *p_v_parameters);
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Dc_T tDc;

/* DC 记忆参数步进配置表 */
typedef struct
{
	void				*pParam;
	int16_t				sMin;
	int16_t				sMax;
	uint8_t				ucType;				/* 0: uint16_t, 1: int8_t */
}DcParamStep_T;

static const DcParamStep_T s_t_dc_param_table[] = 
{
	{(void*)&tAppMemParam.tDC.usAutoOffTime, 0,    3600,  0},
	{(void*)&tAppMemParam.tDC.usMaxOutVolt,  0,    30000, 0},
	{(void*)&tAppMemParam.tDC.usMinOutVolt,  0,    30000, 0},
	{(void*)&tAppMemParam.tDC.usOverLoadPwr, 0,    30000, 0},
	{(void*)&tAppMemParam.tDC.usMinOpenVolt, 0,    30000, 0},
	{(void*)&tAppMemParam.tDC.sMaxTemp,     -127,  127,   1},
};

//****************************************************Function Declaration******************************************************//
static s8 c_dc_check_in_volt(void);


/***********************************************************************************************************************
 * 函数功能    : 参数初始化
 * 说明(备注)  : 重置结构体并载入记忆关闭时间
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_ParamInit(void)
{
	memset((u8*)&tDc, 0, sizeof(tDc));
	tDc.usAutoOffTime = tAppMemParam.tDC.usAutoOffTime;
	vDc_SetWorkState(DS_INIT);
}

/***********************************************************************************************************************
 * 函数功能    : DC 任务初始化
 * 说明(备注)  : 初始化 IO、参数、队列对象并创建 OS 任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; -1: 队列初始化失败; -2: 任务创建失败
 ************************************************************************************************************************/
s8 cDc_TaskInit(void)
{
	vDc_IfaceInit();
	vDc_ParamInit();

	if (bDc_QueueInit() == false)
		return -1;

	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t )vDc_Task,
	                (const char*    )"bDcTask",
	                (uint16_t       )DC_TASK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )DC_TASK_PRIO,
	                (TaskHandle_t*  )&tDcTaskHandler) != pdPASS)
		return -2;
	vQueue_BindTaskHandler(tpDcTask, tDcTaskHandler);
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : DC 任务主循环
 * 说明(备注)  : 周期刷新物理量与联动逻辑；状态机由队列任务函数驱动
 * 传入参数    : p_v_parameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_Task(void *p_v_parameters)
{
	static vu16 s_us_boot_delay    = 0;
	static vu16 s_us_pwr_exist_cnt = 0;
	static vu16 s_us_syn_cnt       = 0;

	#if (boardUSE_OS)
	for (;;)
	#endif  /* boardUSE_OS */
	{
		/* 关机状态下关闭 DC */
		if (tSysInfo.uPerm.tPerm.bDisChgPerm == false)
		{
			if (tDc.eDevState >= DS_BOOTING)
				cQueue_AddQueueTask(tpDcTask, DCTI_CLOSING, 0, false);
		}



		/* 采集物理量提前刷新，确保状态机和保护逻辑使用最新采样 */
		tDc.usInVolt = tAdcSamp.usSysInVolt;
		tDc.sMaxTemp = tAdcSamp.sDcOutTemp;

		if (tDc.eDevState >= DS_BOOTING)
		{
			tDc.usOutVolt = tAdcSamp.usDcOutVolt;
			tDc.usOutCurr = (uint16_t)(tAdcSamp.fDcOutCurr * 10.0f); /* 0.1A */
			tDc.usOutPwr  = (uint16_t)(((uint32_t)tDc.usOutCurr * tDc.usOutVolt) / 100);

			/* 开启的前 2 秒抑制功率显示，消除浪涌毛刺 */
			if (s_us_boot_delay < (2000 / dcTASK_CYCLE_TIME))
			{
				s_us_boot_delay++;
				tDc.usOutPwr = 0;
			}

			if (tDc.usOutPwr > 1)
				s_us_pwr_exist_cnt++;
			else
				s_us_pwr_exist_cnt = 0;

			if (s_us_pwr_exist_cnt < 3)
				tDc.usOutPwr = 0;

			/* 有功率输出时刷新自动关机计时 */
			if (tDc.usOutPwr > 1)
				vDc_RefreshOffTime();
		}
		else
		{
			tDc.usOutVolt      = 0;
			tDc.usOutCurr      = 0;
			tDc.usOutPwr       = 0;
			s_us_boot_delay    = 0;
			s_us_pwr_exist_cnt = 0;
		}

		/* 队列任务轮询: 命令队列 + 状态机任务执行，队列空闲时按周期休眠 */
		vQueue_TaskPoll(tpDcTask, dcTASK_CYCLE_TIME);
	}
}

/***********************************************************************************************************************
 * 函数功能    : 工作状态设置
 * 说明(备注)  : 切换状态并在启动/关闭时复位错误状态
 * 传入参数    : e_stat: 目标状态
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_SetWorkState(DevState_E e_stat)
{
	tDc.eDevState = e_stat;

	if (e_stat == DS_CLOSING || e_stat == DS_BOOTING)
	{
		if (tDc.uErrCode.ucErrCode)
			vDc_SetErrCode(DC_EC_CLEAR_ALL, false);
	}
}

/***********************************************************************************************************************
 * 函数功能    : 设置错误代码
 * 说明(备注)  : 设置/清除故障位并驱动蜂鸣器、状态机联动与错误任务投递
 * 传入参数    : e_code: 错误码; b_set: true-设置, false-清除
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_SetErrCode(DcErrCode_E e_code, bool b_set)
{
	static DcErrCode_E s_e_next_code;
	static bool        s_b_next_set;

	if (uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
	{
		if (s_e_next_code != e_code || s_b_next_set != b_set)
		{
			log_e("bDcTask:任务错误 代码%d 类型%d", e_code, b_set);
			s_e_next_code = e_code;
			s_b_next_set  = b_set;
		}
	}

	/* 清除所有错误 */
	if (e_code == DC_EC_CLEAR_ALL)
	{
		tDc.uErrCode.ucErrCode = 0;
		return;
	}

	if (b_set)
	{
		ERR_SET(tDc.uErrCode.ucErrCode, (e_code - 1));

		#if (boardBUZ_EN)
		bBuz_Tweet(LONG_3);
		#endif  /* boardBUZ_EN */
	}
	else
		ERR_CLR(tDc.uErrCode.ucErrCode, (e_code - 1));

	if (tDc.uErrCode.ucErrCode)
	{
		if (tDc.eDevState != DS_ERR)
			vDc_SetWorkState(DS_ERR);

		/* 投递错误任务立即切换到错误处理(已在错误任务中则不重复投递) */
		if (tpDcTask->ucID != DCTI_ERR)
			cQueue_AddQueueTask(tpDcTask, DCTI_ERR, 0, true);
	}
	else
	{
		if (e_code == DC_EC_OUT_LOW)
		{
			cDc_Switch(ST_ON, false);
			if (uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
				log_i("bDcTask:错误清除,重新开启");
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 保护处理
 * 说明(备注)  : 综合检测 NTC、电源电压、过温、过流、输出丢失与关断失败
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_ProtectProcess(void)
{
	static u8 s_uc_pwr_err_cnt          = 0;
	static u8 s_uc_clear_pwr_err_cnt    = 0;
	static u8 s_uc_over_temp_cnt        = 0;
	static u8 s_uc_clear_over_temp_cnt  = 0;
	static u8 s_uc_overload_cnt         = 0;
	static u8 s_uc_over_curr_cnt        = 0;
	static u8 s_uc_output_low_cnt       = 0;
	static u8 s_uc_output_high_cnt      = 0;
	static u8 s_uc_clear_output_err_cnt = 0;
	static u8 s_uc_close_fail_cnt       = 0;
	static u8 s_uc_clear_close_fail_cnt = 0;
	static u8 s_uc_ntc_lost_cnt         = 0;
	u8 uc_temp                          = 0;

	/* NTC 检测: 0 度或负温持续 3 秒判定为传感器掉线/丢失 */
	if (tDc.eDevState == DS_WORK)
	{
		if (tDc.sMaxTemp <= 0 && tDc.uErrCode.tCode.bNtcLost == 0)
		{
			s_uc_ntc_lost_cnt++;
			if (s_uc_ntc_lost_cnt >= (3000 / dcTASK_CYCLE_TIME))
			{
				s_uc_ntc_lost_cnt = 0;
				vDc_SetErrCode(DC_EC_NTC_LOST, true);
			}
		}
		else if (tDc.sMaxTemp > 5)
		{
			s_uc_ntc_lost_cnt = 0;
			if (tDc.uErrCode.tCode.bNtcLost == 1)
				vDc_SetErrCode(DC_EC_NTC_LOST, false);
		}
	}
	else
		s_uc_ntc_lost_cnt = 0;

	/* 电源供电电压检查 (仅工作/启动状态) */
	if (tDc.eDevState == DS_BOOTING || tDc.eDevState == DS_WORK)
	{
		if (c_dc_check_in_volt() != 0)
		{
			s_uc_clear_pwr_err_cnt = 0;
			if (tDc.uErrCode.tCode.bPowerErr == 0)
			{
				s_uc_pwr_err_cnt++;
				if (s_uc_pwr_err_cnt >= 10)
				{
					s_uc_pwr_err_cnt = 0;
					vDc_SetErrCode(DC_EC_PWR_ERR, true);
				}
			}
		}
		else
		{
			s_uc_pwr_err_cnt = 0;
			if (tDc.uErrCode.tCode.bPowerErr == 1)
			{
				s_uc_clear_pwr_err_cnt++;
				if (s_uc_clear_pwr_err_cnt >= 10)
				{
					s_uc_clear_pwr_err_cnt = 0;
					vDc_SetErrCode(DC_EC_PWR_ERR, false);
				}
			}
		}
	}
	else
	{
		s_uc_pwr_err_cnt       = 0;
		s_uc_clear_pwr_err_cnt = 0;
	}

	/* 过温检查 */
	if (tDc.sMaxTemp > tAppMemParam.tDC.sMaxTemp)
	{
		s_uc_clear_over_temp_cnt = 0;
		if (tDc.uErrCode.tCode.bOT == 0)
		{
			s_uc_over_temp_cnt++;
			if (s_uc_over_temp_cnt >= 20)
			{
				vDc_SetErrCode(DC_EC_OT, true);
				s_uc_over_temp_cnt = 0;
			}
		}
	}
	else if (tDc.sMaxTemp < (tAppMemParam.tDC.sMaxTemp - 10))
	{
		s_uc_over_temp_cnt = 0;
		/* NTC断线时 sMaxTemp 恒为 0, 恢复条件恒成立, 禁止自动清除过温故障 */
		if (tDc.uErrCode.tCode.bOT == 1 && tDc.uErrCode.tCode.bNtcLost == 0)
		{
			s_uc_clear_over_temp_cnt++;
			if (s_uc_clear_over_temp_cnt >= 20)
			{
				vDc_SetErrCode(DC_EC_OT, false);
				s_uc_clear_over_temp_cnt = 0;
			}
		}
	}

	/* 过流检查 (采用定点整型比较，彻底消除浮点计算) */
	if (tDc.usOutCurr > dcOVER_CURR_PEAK_THRESH)
	{
		if (tDc.uErrCode.tCode.bOL == 0)
		{
			s_uc_over_curr_cnt++;
			if (s_uc_over_curr_cnt >= 5)                             /* 1S */
			{
				vDc_SetErrCode(DC_EC_OL, true);
				s_uc_over_curr_cnt = 0;
			}
		}
		else
			s_uc_over_curr_cnt = 0;
	}
	else
		s_uc_over_curr_cnt = 0;

	/* 持续过载检查 */
	if (tDc.usOutCurr > dcOVER_CURR_CONT_THRESH)
	{
		if (tDc.uErrCode.tCode.bOL == 0)
		{
			s_uc_overload_cnt++;
			if (s_uc_overload_cnt >= 25)                            /* 5S */
			{
				vDc_SetErrCode(DC_EC_OL, true);
				s_uc_overload_cnt = 0;
			}
		}
		else
			s_uc_overload_cnt = 0;
	}
	else
		s_uc_overload_cnt = 0;

	/* 输出电压异常/丢失检查 */
	if (tDc.eDevState == DS_WORK)
	{
		if (cDc_CheckOutVolt() < 0)
		{
			uc_temp                     = (tDc.eDevState == DS_WORK) ? 4 : 15;
			s_uc_clear_output_err_cnt   = 0;
			s_uc_output_high_cnt        = 0;
			if (tDc.uErrCode.tCode.bOutLow == 0)
			{
				s_uc_output_low_cnt++;
				if (s_uc_output_low_cnt >= uc_temp)
				{
					s_uc_output_low_cnt = 0;
					vDc_SetErrCode(DC_EC_OUT_LOW, true);
				}
			}
		}
		else if (cDc_CheckOutVolt() > 0)
		{
			s_uc_clear_output_err_cnt = 0;
			s_uc_output_low_cnt       = 0;
			if (tDc.uErrCode.tCode.bOutHigh == 0)
			{
				s_uc_output_high_cnt++;
				if (s_uc_output_high_cnt >= 3)
				{
					s_uc_output_high_cnt = 0;
					vDc_SetErrCode(DC_EC_OUT_HIGH, true);
				}
			}
		}
		else
		{
			s_uc_output_low_cnt  = 0;
			s_uc_output_high_cnt = 0;
			if (tDc.uErrCode.tCode.bOutHigh == 1 || tDc.uErrCode.tCode.bOutLow == 1)
			{
				s_uc_clear_output_err_cnt++;
				if (s_uc_clear_output_err_cnt >= 4)
				{
					s_uc_clear_output_err_cnt = 0;
					if (tDc.uErrCode.tCode.bOutHigh == 1)
						vDc_SetErrCode(DC_EC_OUT_HIGH, false);
					if (tDc.uErrCode.tCode.bOutLow == 1)
						vDc_SetErrCode(DC_EC_OUT_LOW, false);
				}
			}
		}
	}
	else
	{
		s_uc_output_low_cnt       = 0;
		s_uc_output_high_cnt      = 0;
		s_uc_clear_output_err_cnt = 0;
	}

	/* 关闭输出失败检查 (关闭中或错误状态) */
	if (tDc.eDevState == DS_CLOSING || tDc.eDevState == DS_ERR)
	{
		if (cDc_CheckOutVolt() >= 0)
		{
			s_uc_clear_close_fail_cnt = 0;
			if (tDc.uErrCode.tCode.bCloseFail == 0)
			{
				s_uc_close_fail_cnt++;
				if (s_uc_close_fail_cnt >= 50)
				{
					s_uc_close_fail_cnt = 0;
					vDc_SetErrCode(DC_EC_CLOSE_FAIL, true);
				}
			}
		}
		else
		{
			s_uc_close_fail_cnt = 0;
			if (tDc.uErrCode.tCode.bCloseFail == 1)
			{
				s_uc_clear_close_fail_cnt++;
				if (s_uc_clear_close_fail_cnt >= 10)
				{
					s_uc_clear_close_fail_cnt = 0;
					vDc_SetErrCode(DC_EC_CLOSE_FAIL, false);
				}
			}
		}
	}
	else
		s_uc_close_fail_cnt = 0;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化 DC 记忆信息
 * 说明(备注)  : 获取或重新初始化 Flash 存储参数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 小于0:失败; 0:未完成; 大于0:完成
 ************************************************************************************************************************/
s8 cDc_InfoInit(void)
{
	s8 ret                = 0;
	const char *p_obj_str = tDcMemParamStr;
	static bool s_b_ret   = true;

	if (tSysInfo.uInit.tFinish.bIF_SysInit == true)
	{
		ret = cApp_GetMemParam(p_obj_str);
		if (ret > 0)
			return 1;

		if ((uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant) && s_b_ret == true)
		{
			log_e("bDcTask:当前系统已经初始化完成,但是tDC读取依旧为空,准备重置");
			s_b_ret = false;
		}
	}

	ret = cApp_MemParamInit(p_obj_str);
	if (ret <= 0)
		return -1;

	ret = cApp_UpdateMemParam(p_obj_str);
	if (ret <= 0)
		return -2;

	s_b_ret = true;
	return 2;
}

/***********************************************************************************************************************
 * 函数功能    : 检查 DC 输出状态
 * 说明(备注)  : 比对当前输出电压与参数阈值
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 小于0:欠压; 0:电压正常; 1:过压
 ************************************************************************************************************************/
s8 cDc_CheckOutVolt(void)
{
	if (RANGE(tDc.usOutVolt, tAppMemParam.tDC.usMinOutVolt, tAppMemParam.tDC.usMaxOutVolt))
		return 0;
	else if (tDc.usOutVolt > tAppMemParam.tDC.usMaxOutVolt)
		return 1;
	else
		return -1;
}

/***********************************************************************************************************************
 * 函数功能    : 检查 DC 供电状态
 * 说明(备注)  : 比对输入供电电压与最低开启门限
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 0:正常; -1:欠压
 ************************************************************************************************************************/
static s8 c_dc_check_in_volt(void)
{
	if (tDc.usInVolt >= tAppMemParam.tDC.usMinOpenVolt)
		return 0;
	else
		return -1;
}

/***********************************************************************************************************************
 * 函数功能    : DC 开关控制
 * 说明(备注)  : 同步执行权限/状态前置检查并保留返回值语义；开关命令投递至队列
 * 传入参数    : e_tri_type: 开关类型; b_fore_en: 强制执行(队列抢占模式)
 * 输出参数    : 无
 * 返回值      : 0:维持现状; 1:成功; 负数:权限/故障禁止
 ************************************************************************************************************************/
s8 cDc_Switch(SwitchType_E e_tri_type, bool b_fore_en)
{
	bool b_turn_on = false;

	if (e_tri_type == ST_ON)
	{
		if ((tDc.eDevState == DS_WORK || tDc.eDevState == DS_BOOTING) && b_fore_en == false)
		{
			if (uPrint.tFlag.bDcTask)
				sMyPrint("bDcTask:当前状态为工作,不允许开机\r\n");
			return 0;
		}
		b_turn_on = true;
	}
	else if (e_tri_type == ST_OFF)
	{
		if ((tDc.eDevState == DS_SHUT_DOWN || tDc.eDevState == DS_CLOSING) && b_fore_en == false)
		{
			if (uPrint.tFlag.bDcTask)
				sMyPrint("bDcTask:当前状态为关闭,不允许关机\r\n");
			return 0;
		}
		b_turn_on = false;
	}
	else
		b_turn_on = (tDc.eDevState <= DS_SHUT_DOWN);

	if (b_turn_on)
	{
		if (tSysInfo.uPerm.tPerm.bDisChgPerm == false)
		{
			#if (boardBUZ_EN)
			bBuz_Tweet(SHORT_2);
			#endif  /* boardBUZ_EN */

			if (uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
				log_w("bDcTask:系统不允许开启放电");
			return -1;
		}

		#if (boardBUZ_EN)
		bBuz_Tweet(LONG_1);
		#endif  /* boardBUZ_EN */

		if (uPrint.tFlag.bDcTask)
			sMyPrint("bDcTask:----DC 开启----\r\n");

		/* 开机命令投递至队列，由 DcTask 上下文串行执行 */
		cQueue_AddQueueTask(tpDcTask, DCTI_BOOTING, 0, b_fore_en);
	}
	else
	{
		#if (boardBUZ_EN)
		bBuz_Tweet(LONG_1);
		#endif  /* boardBUZ_EN */

		if (uPrint.tFlag.bDcTask)
			sMyPrint("bDcTask:----DC 关闭----\r\n");

		/* 已处于关机完成状态则无需重复执行 */
		if (tDc.eDevState != DS_SHUT_DOWN)
			cQueue_AddQueueTask(tpDcTask, DCTI_CLOSING, 0, b_fore_en);
	}

	#if (boardSYS_DATA_UPADATA)
	Sys_Update_Mod(DC_Mod, true);
	#endif  /* boardSYS_DATA_UPADATA */

	return 1;
}

#if (boardADC_EN)
/***********************************************************************************************************************
 * 函数功能    : ADC 越限哨兵
 * 说明(备注)  : AdcTask 滤波更新后调用；峰值过流越限时投递保护扫描任务提前唤醒
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_AdcWatchdog(void)
{
	if (tpDcTask == NULL)
		return;

	/* 峰值过流阈值越限: 0.1A 定点比较 */
	if ((uint16_t)(tAdcSamp.fDcOutCurr * 10.0f) > dcOVER_CURR_PEAK_THRESH)
		cQueue_AddQueueTask(tpDcTask, DCTI_PROT, 0, false);
}
#endif  /* boardADC_EN */

/***********************************************************************************************************************
 * 函数功能    : 自动关闭计时
 * 说明(备注)  : 1秒周期心跳递减
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_TickTimer(void)
{
	if (bSys_IsWorkState() == false || tDc.eDevState != DS_WORK)
		return;

	if (tDc.usAutoOffTime)
	{
		if (tDc.usAutoOffCnt)
		{
			tDc.usAutoOffCnt--;
			if (tDc.usAutoOffCnt == 0)
			{
				cDc_Switch(ST_OFF, false);
				if (uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
					sMyPrint("Dc_Task:====倒计时结束,关闭DC  时间=%dS====\r\n", tDc.usAutoOffTime);
			}
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 刷新关闭时间
 * 说明(备注)  : 重新将倒计时装载为设定值
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_RefreshOffTime(void)
{
	if (tDc.usAutoOffTime)
		tDc.usAutoOffCnt = tDc.usAutoOffTime;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化参数
 * 说明(备注)  : 装载 board 配置默认值
 * 传入参数    : p_dc_mem: DC 记忆参数结构体指针
 * 输出参数    : 无
 * 返回值      : true: 成功
 ************************************************************************************************************************/
bool bDc_MemParamInit(DcMemParam_T *p_dc_mem)
{
	p_dc_mem->usAutoOffTime = boardDC_OFF_TIME;
	p_dc_mem->usMaxOutVolt  = boardDC_MAX_OUT_VOLT;
	p_dc_mem->usMinOutVolt  = boardDC_MIN_OUT_VOLT;
	p_dc_mem->usOverLoadPwr = boardDC_OVERLOAD_PWR;
	p_dc_mem->usMinOpenVolt = boardDC_OPEN_MIN_VOLT;
	p_dc_mem->sMaxTemp      = boardDC_MAX_TEMP;
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置记忆参数 (表驱动精简版)
 * 说明(备注)  : 支持 uint16 与 int8 类型，自动进行安全上下限防越界检查
 * 传入参数    : uc_item: 参数索引; b_add: true-增加, false-减少
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_MemParamSet(uint8_t uc_item, bool b_add)
{
	if (uc_item >= mainARRAY_SIZE(s_t_dc_param_table))
		return;

	const DcParamStep_T *p = &s_t_dc_param_table[uc_item];
	if (p->ucType == 1)
	{
		int8_t *p_val = (int8_t*)p->pParam;
		if (b_add && *p_val < (int8_t)p->sMax)
			(*p_val)++;
		else if (!b_add && *p_val > (int8_t)p->sMin)
			(*p_val)--;
	}
	else
	{
		uint16_t *p_val = (uint16_t*)p->pParam;
		if (b_add && *p_val < (uint16_t)p->sMax)
			(*p_val)++;
		else if (!b_add && *p_val > (uint16_t)p->sMin)
			(*p_val)--;
	}
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 进入低功耗
 * 说明(备注)  : 配置 IO 为模拟输入并挂起任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_EnterLowPower(void)
{
	rcu_periph_clock_enable(dcPOWER_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(dcPOWER_EN_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, dcPOWER_EN_PIN);
	#else
	gpio_init(dcPOWER_EN_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, dcPOWER_EN_PIN);
	#endif  /* boardIC_TYPE */

	#if (boardUSE_OS)
	vTaskSuspend(tDcTaskHandler);
	#endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 退出低功耗
 * 说明(备注)  : 重新初始化 IO 并恢复任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDc_ExitLowPower(void)
{
	vDc_IfaceInit();

	#if (boardUSE_OS)
	vTaskResume(tDcTaskHandler);
	#endif  /* boardUSE_OS */
}
#endif  /* boardLOW_POWER */

#endif  /* boardDC_EN */

