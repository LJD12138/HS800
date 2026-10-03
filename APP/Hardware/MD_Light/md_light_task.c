/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Light
 * File    : md_light_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 照明灯控制驱动与状态机实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Light/md_light_task.h"

#if (boardLIGHT_EN)
#include "MD_Light/md_light_iface.h"
#include "Buz/buz_task.h"
#include "Sys/sys_task.h"
#include "MD_Display/md_display_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			LIGHT_TASK_PRIO							1		/* 任务优先级 */
#define			LIGHT_TASK_STK_SIZE						128		/* 任务堆栈 (512B，达到 configMINIMAL_STACK_SIZE 防溢出标准) */
TaskHandle_t tLightTaskHandler = NULL;							/* 任务句柄 */
void vLight_Task(void *p_v_parameters);							/* 任务函数 */
#endif  /* boardUSE_OS */


//****************************************************Parameter Initialization**************************************************//
Light_T tLight;

//****************************************************Function Declaration******************************************************//
static void v_light_pwm_set(uint16_t level);
static void v_light_set_state(LightWorkMode_E mode);

/***********************************************************************************************************************
 * 函数功能    : 照明任务初始化
 * 说明(备注)  : 初始化底层 PWM 并创建任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; -1: 任务创建失败
 ************************************************************************************************************************/
s8 cLight_TaskInit(void)
{
	vLight_IfaceInit();
	v_light_set_state(LWM_OFF);

	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t )vLight_Task,
	                (const char*    )"LightTask",
	                (uint16_t       )LIGHT_TASK_STK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )LIGHT_TASK_PRIO,
	                (TaskHandle_t*  )&tLightTaskHandler) != pdPASS)
		return -1;
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 照明任务
 * 说明(备注)  : 依据放电权限和工作模式刷新 PWM 输出与闪烁/SOS 逻辑
 * 传入参数    : p_v_parameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLight_Task(void *p_v_parameters)
{
	(void)p_v_parameters;

	#if (lightSIMPLE_MODE)
	static uint8_t s_uc_twinkle_step = 0;
	static uint8_t s_uc_sos_step     = 0;
	const uint8_t  uc_short_unit     = 2;
	const uint8_t  uc_long_unit      = 4;
	#endif  /* lightSIMPLE_MODE */

	#if (boardUSE_OS)
	for (;;)
	#endif  /* boardUSE_OS */
	{
		if (tSysInfo.eDevState == DS_SHUT_DOWN && tLight.eWorkMode != LWM_OFF)
			v_light_set_state(LWM_OFF);

		if (tSysInfo.uPerm.tPerm.bDisChgPerm == true)
		{
			if (tLight.eWorkMode == LWM_HALF)
			{
				tLight.usPower = 2;
				#if (lightSIMPLE_MODE)
				s_uc_twinkle_step = 0;
				s_uc_sos_step     = 0;
				#endif  /* lightSIMPLE_MODE */
			}
			else if (tLight.eWorkMode == LWM_FULL)
			{
				tLight.usPower = 4;
				#if (lightSIMPLE_MODE)
				s_uc_twinkle_step = 0;
				s_uc_sos_step     = 0;
				#endif  /* lightSIMPLE_MODE */
			}
			#if (lightSIMPLE_MODE)
			else if (tLight.eWorkMode == LWM_SOS)
			{
				tLight.usPower    = 2;
				s_uc_twinkle_step = 0;

				if (s_uc_sos_step++ == 0)
					tLight.usValue = lightPWM_FULL_VALUE;
				else if (s_uc_sos_step == uc_short_unit * 1)
					tLight.usValue = 0;
				else if (s_uc_sos_step == (uc_short_unit * 2))
					tLight.usValue = lightPWM_FULL_VALUE;
				else if (s_uc_sos_step == (uc_short_unit * 3))
					tLight.usValue = 0;
				else if (s_uc_sos_step == (uc_short_unit * 4))
					tLight.usValue = lightPWM_FULL_VALUE;
				else if (s_uc_sos_step == (uc_short_unit * 5))
					tLight.usValue = 0;
				else if (s_uc_sos_step == (uc_long_unit * 6))
					tLight.usValue = lightPWM_FULL_VALUE;
				else if (s_uc_sos_step == (uc_long_unit * 7))
					tLight.usValue = 0;
				else if (s_uc_sos_step == (uc_long_unit * 8))
					tLight.usValue = lightPWM_FULL_VALUE;
				else if (s_uc_sos_step == (uc_long_unit * 9))
					tLight.usValue = 0;
				else if (s_uc_sos_step == (uc_long_unit * 10))
					tLight.usValue = lightPWM_FULL_VALUE;
				else if (s_uc_sos_step == (uc_long_unit * 11))
					tLight.usValue = 0;
				else if (s_uc_sos_step >= (uc_long_unit * 12))
				{
					tLight.usValue = lightPWM_FULL_VALUE;
					s_uc_sos_step  = 0;
				}
			}
			else if (tLight.eWorkMode == LWM_TWINKLE)
			{
				tLight.usPower = 2;
				s_uc_sos_step  = 0;
				if (s_uc_twinkle_step == 0)
				{
					tLight.usValue    = lightPWM_FULL_VALUE;
					s_uc_twinkle_step = 1;
				}
				else
				{
					tLight.usValue    = 0;
					s_uc_twinkle_step = 0;
				}
			}
			else
			{
				tLight.usValue = 0;
				tLight.usPower = 0;
			}
			#endif  /* lightSIMPLE_MODE */

			v_light_pwm_set(tLight.usValue);
		}
		else
		{
			tLight.usPower = 0;
			if (tLight.eWorkMode != LWM_OFF)
			{
				tLight.usLastValue = tLight.usValue;
				v_light_set_state(LWM_OFF);
				v_light_pwm_set(tLight.usValue);
			}
		}

		#if (boardUSE_OS)
		vTaskDelay(100);
		#endif  /* boardUSE_OS */
	}
}

/***********************************************************************************************************************
 * 函数功能    : 照明设置状态
 * 说明(备注)  : 控制定时器启停并更新设备状态
 * 传入参数    : mode: 目标工作模式
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_light_set_state(LightWorkMode_E mode)
{
	if (mode == LWM_OFF)
	{
		/* 先拉低占空比再关定时器,防止PWM冻结在关闭时刻的高电平导致关机后灯常亮 */
		v_light_pwm_set(0);
		timer_disable(lightTIMER);
		tLight.eDevState = DS_SHUT_DOWN;
	}
	else
	{
		timer_enable(lightTIMER);
		#if (boardIC_TYPE == boardIC_GD32F50X)
		timer_primary_output_config(lightTIMER, ENABLE);
		timer_channel_primary_output_config(lightTIMER, lightTIMER_CH, ENABLE);
		#endif  /* boardIC_TYPE */
		tLight.eDevState = DS_WORK;
	}

	switch (mode)
	{
		case LWM_HALF:
		{
			tLight.usValue = lightPWM_SEMI_VALUE;
		}
		break;

		case LWM_FULL:
		{
			tLight.usValue = lightPWM_FULL_VALUE;
		}	
		break;

		#if (lightSIMPLE_MODE)
		case LWM_SOS:
		case LWM_TWINKLE:
		{
			tLight.usValue = 0;
		}	
		break;
		#endif  /* lightSIMPLE_MODE */

		case LWM_OFF:
		default:
		{
			tLight.usValue = 0;
		}
		break;
	}

	tLight.eWorkMode = mode;
}

/***********************************************************************************************************************
 * 函数功能    : 照明设置 PWM 占空比
 * 说明(备注)  : 限幅后写入定时器通道
 * 传入参数    : level: PWM 目标值 (0~1000)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_light_pwm_set(uint16_t level)
{
	level = LIMIT_MAX(level, lightPWM_MAX_VALUE);
	lightPWM_SET(level);
}

/***********************************************************************************************************************
 * 函数功能    : 照明开关控制
 * 说明(备注)  : 响应按键动作，消除 goto，条件分支结构化处理
 * 传入参数    : type: 开关类型 (ST_ON, ST_OFF, ST_NULL: 翻转)
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 无放电权限失败
 ************************************************************************************************************************/
bool bLight_Switch(SwitchType_E type)
{
	bool b_target_on = false;

	if (type == ST_ON)
	{
		if (tLight.eDevState == DS_WORK)
			return true;
		b_target_on = true;
	}
	else if (type == ST_OFF)
	{
		if (tLight.eDevState == DS_SHUT_DOWN)
			return true;
		b_target_on = false;
	}
	else
		/* 触发模式: 状态翻转 */
		b_target_on = (tLight.eDevState == DS_SHUT_DOWN);

	if (b_target_on)
	{
		if (tSysInfo.uPerm.tPerm.bDisChgPerm == false)
		{
			#if (boardBUZ_EN)
			bBuz_Tweet(SHORT_2);
			#endif  /* boardBUZ_EN */
			return false;
		}

		tLight.usValue = tLight.usLastValue;
		v_light_set_state(LWM_HALF);
	}
	else
	{
		tLight.usLastValue = tLight.usValue;
		v_light_set_state(LWM_OFF);
	}

	#if (boardSYS_DATA_UPADATA)
	Sys_Update_Element(AT_LIGHT_SWITCH_ADDR, tLight.eWorkMode, true, true);
	#endif  /* boardSYS_DATA_UPADATA */

	#if (boardBUZ_EN)
	bBuz_Tweet(LONG_1);
	#endif  /* boardBUZ_EN */

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 循环切换照明模式
 * 说明(备注)  : 运行中短按按键在 Half -> Full -> SOS -> Twinkle 之间循环
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLight_CircSelectMode(void)
{
	if (tLight.eDevState != DS_WORK)
		return;

	LightWorkMode_E mode = tLight.eWorkMode;
	mode++;

	#if (lightSIMPLE_MODE)
	if (mode > LWM_TWINKLE)
		mode = LWM_OFF;
	#else
	if (mode > LWM_FULL)
		mode = LWM_OFF;
	#endif  /* lightSIMPLE_MODE */

	v_light_set_state(mode);

	#if (boardBUZ_EN)
	bBuz_Tweet(LONG_1);
	#endif  /* boardBUZ_EN */

	#if (boardSYS_DATA_UPADATA)
	Sys_Update_Element(AT_LIGHT_SWITCH_ADDR, tLight.eWorkMode, true, true);
	#endif  /* boardSYS_DATA_UPADATA */
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 照明进入低功耗
 * 说明(备注)  : 挂起任务并配置 GPIO 为低功耗
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLight_EnterLowPower(void)
{
	vLight_IoEnterLowPower();
	if (tLightTaskHandler != NULL)
		vTaskSuspend(tLightTaskHandler);
}

/***********************************************************************************************************************
 * 函数功能    : 照明退出低功耗
 * 说明(备注)  : 恢复硬件并恢复任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLight_ExitLowPower(void)
{
	vLight_IfaceInit();
	if (tLightTaskHandler != NULL)
		vTaskResume(tLightTaskHandler);
}
#endif  /* boardLOW_POWER */

#endif  /* boardLIGHT_EN */

