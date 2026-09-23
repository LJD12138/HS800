/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_HeatManage
 * File    : md_hm_task.c
 * Date    : 2026-09-12
 * Author  : LJD(291483914@qq.com)
 * Desc    : 风扇与热管理状态机实现(温控多级挡位与防卡滞启动)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_HeatManage/md_hm_task.h"
#include "MD_HeatManage/md_hm_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "MD_Dcac/md_dcac_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#if (boardUSE_OS)
#define			HM_TASK_PRIO							1		/* 任务优先级 */
#define			HM_TASK_STK_SIZE						128		/* 任务堆栈 (512B，达到 configMINIMAL_STACK_SIZE 防溢出标准) */
static TaskHandle_t tHeatManageHandler = NULL;
void vHM_Task(void *pvParameters);
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
HM_T tHM;

static bool    s_b_fan_stop_to_run_flag = false;
static uint8_t s_uc_update_delay        = 0;
static int16_t s_s_temper               = 0;

//****************************************************Function Declaration******************************************************//
static void     v_fan_pwm_set(uint16_t level);
static uint16_t us_fan_set_work_mode(FanWorkMode_E mode);

/***********************************************************************************************************************
 * 函数功能    : 散热管理任务初始化
 * 说明(备注)  : 初始化风扇硬件接口并创建 OS 任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 初始化成功, -1: 任务创建失败
 ************************************************************************************************************************/
s8 cHM_TaskInit(void)
{
	vFan_IfaceInit();

	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t )vHM_Task,
	                (const char*    )"bHeatManage",
	                (uint16_t       )HM_TASK_STK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )HM_TASK_PRIO,
	                (TaskHandle_t*  )&tHeatManageHandler) != pdPASS)
		return -1;
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 散热控制任务
 * 说明(备注)  : 周期检测系统功率与最高温度，自动调节风扇档位与堵转保护
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vHM_Task(void *pvParameters)
{
	bool b_open_fan_flag = false;

	#if (boardUSE_OS)
	for (;;)
	#endif  /* boardUSE_OS */
	{
		if (bSys_IsWorkState() == true || tSysInfo.eDevState == DS_ERR)
		{
			/* 3 秒更新一次采样温度 */
			if (++s_uc_update_delay >= 3)
			{
				s_s_temper        = tHM.sMaxTemp;
				s_uc_update_delay = 0;
			}

			/* 功率联动判断 */
			if (tSysInfo.usOutPwr > 100)
				b_open_fan_flag = true;
			if (tSysInfo.usOutPwr < 50)
				b_open_fan_flag = false;

			/* 逆变充电或大功率输出时提升等效温控判定线 */
			if (s_s_temper < 41 &&
			    (
			        #if (boardDCAC_EN)
			        tDcac.eChgState == IOS_WORK ||
			        #endif  /* boardDCAC_EN */
			        b_open_fan_flag == true
			    ))
			{
				s_s_temper = 41;
			}

			/* 挡位滞环控制 */
			switch (tHM.eWorkMode)
			{
				default:
				case FWM_OFF:
				{
					if (s_s_temper > 40)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_1);
				}
				break;

				case FWM_GEAR_1:
				{
					if (s_s_temper < 38)
						tHM.usValue = us_fan_set_work_mode(FWM_OFF);
					else if (s_s_temper > 44)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_2);
				}
				break;

				case FWM_GEAR_2:
				{
					if (s_s_temper < 42)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_1);
					else if (s_s_temper > 48)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_3);
				}
				break;

				case FWM_GEAR_3:
				{
					if (s_s_temper < 46)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_2);
					else if (s_s_temper > 52)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_FULL);
				}
				break;

				case FWM_GEAR_FULL:
				{
					if (s_s_temper < 50)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_3);
				}
				break;
			}

			/* 从停止启动并低于二挡时，短暂以第 2 挡启动避免风扇堵转卡滞 */
			if (tHM.eWorkMode != FWM_OFF && s_b_fan_stop_to_run_flag == false)
			{
				if (tHM.eWorkMode < FWM_GEAR_2)
					v_fan_pwm_set(500);
				else
					v_fan_pwm_set(tHM.usValue);

				#if (boardUSE_OS)
				vTaskDelay(100);
				#endif  /* boardUSE_OS */

				s_b_fan_stop_to_run_flag = true;
			}
			else
				v_fan_pwm_set(tHM.usValue);
		}
		else
		{
			/* 关机或休眠关闭风扇 */
			if (tHM.eWorkMode != FWM_OFF || tHM.usValue != 0)
			{
				tHM.usValue = us_fan_set_work_mode(FWM_OFF);
				v_fan_pwm_set(tHM.usValue);
			}
		}

		#if (boardUSE_OS)
		vTaskDelay(1000);
		#endif  /* boardUSE_OS */
	}
}

/***********************************************************************************************************************
 * 函数功能    : 设置风扇 PWM 输出
 * 说明(备注)  : 限幅并配置占空比与使能引脚
 * 传入参数    : level: PWM 占空比 (0~1000)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_fan_pwm_set(uint16_t level)
{
	level = LIMIT_MAX(level, fanPWM_MAX_VALUE);
	fanPWM_SET(level);

	if (level == 0)
		fanPWM_EN_OFF();
	else
		fanPWM_EN_ON();
}

/***********************************************************************************************************************
 * 函数功能    : 设置风扇工作挡位
 * 说明(备注)  : 根据挡位映射 PWM 输出值
 * 传入参数    : mode: 目标挡位
 * 输出参数    : 无
 * 返回值      : 对应模式的 PWM 占空比数值
 ************************************************************************************************************************/
static uint16_t us_fan_set_work_mode(FanWorkMode_E mode)
{
	uint16_t us_pwm = 0;

	if (mode == FWM_GEAR_1)
		us_pwm = 200;
	else if (mode == FWM_GEAR_2)
		us_pwm = 500;
	else if (mode == FWM_GEAR_3)
		us_pwm = 800;
	else if (mode == FWM_GEAR_FULL)
		us_pwm = 1000;
	else
	{
		us_pwm                   = 0;
		s_b_fan_stop_to_run_flag = false;
		mode                     = FWM_OFF;
	}

	tHM.eWorkMode = mode;
	return us_pwm;
}

/***********************************************************************************************************************
 * 函数功能    : 获取风扇当前工作挡位
 * 说明(备注)  : 提供给外部模块查询
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : FanWorkMode_E
 ************************************************************************************************************************/
FanWorkMode_E eFan_GetWorkMode(void)
{
	return tHM.eWorkMode;
}

/***********************************************************************************************************************
 * 函数功能    : 强制打开风扇
 * 说明(备注)  : 用于测试或特殊模式下强制调整等效温度基准
 * 传入参数    : en: true 强制开启, false 恢复常温
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFan_ForceOpenFan(bool en)
{
	if (en == true)
	{
		if (s_s_temper < 41)
		{
			s_s_temper        = 41;
			s_uc_update_delay = 0;
		}
	}
	else
		s_s_temper = 25;
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 风扇进入低功耗
 * 说明(备注)  : 挂起任务并配置 GPIO 为低功耗模拟输入
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFan_EnterLowPower(void)
{
	vFan_IoEnterLowPower();
	if (tHeatManageHandler != NULL)
		vTaskSuspend(tHeatManageHandler);
}

/***********************************************************************************************************************
 * 函数功能    : 风扇退出低功耗
 * 说明(备注)  : 重新初始化硬件并恢复任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vFan_ExitLowPower(void)
{
	vFan_IfaceInit();
	if (tHeatManageHandler != NULL)
		vTaskResume(tHeatManageHandler);
}
#endif  /* boardLOW_POWER */
