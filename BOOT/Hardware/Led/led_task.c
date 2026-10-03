/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Led
 * File    : led_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 面板按键与状态指示灯控制任务实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Led/led_task.h"

#if (boardLED_EN)
#include "Led/led_iface.h"
#include "Sys/sys_task.h"
#include "Update/update_main.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			LED_TASK_PRIO							1		/* 任务优先级 */
#define			LED_TASK_STK_SIZE						128		/* 任务堆栈 (512B，达到 configMINIMAL_STACK_SIZE 防溢出标准) */
static TaskHandle_t s_t_led_task_handler = NULL;				/* 任务句柄 */
void        	vLed_Task(void *p_v_parameters);				/* 任务函数 */
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			ledTASK_CYCLE_TIME						10		/* 任务周期(ms) */

//****************************************************Function Declaration******************************************************//
static void v_led_breathing(void);

/***********************************************************************************************************************
 * 函数功能    : 指示灯任务初始化
 * 说明(备注)  : 初始化硬件 IO 并创建指示灯控制任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; -1: 任务创建失败
 ************************************************************************************************************************/
s8 cLed_TaskInit(void)
{
	vLed_IfaceInit();

	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t )vLed_Task,
	                (const char*    )"LedTask",
	                (uint16_t       )LED_TASK_STK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )LED_TASK_PRIO,
	                (TaskHandle_t*  )&s_t_led_task_handler) != pdPASS)
		return -1;
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 指示灯循环任务
 * 说明(备注)  : 根据系统状态驱动指示灯常开/常关/闪烁/呼吸
 * 传入参数    : pvParameters: 任务参数
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vLed_Task(void *p_v_parameters)
{
	static int l_delay_cnt = 0;
	
	if (tpSysTask == NULL)
		return;
	
	switch (tpSysTask->ucID)
	{
		case STI_INIT:
		{
			ledPWR_SW_OFF();
		}
		break;
		
		case STI_ENTER_APP:
		{
			ledPWR_SW_OFF();
		}
		break;
		
		case STI_ERR:
		case STI_RESET:
		{
			l_delay_cnt++;
			if (l_delay_cnt < (200 / ledTASK_CYCLE_TIME))	/* 闪烁 */
				ledPWR_SW_ON();
			else if (l_delay_cnt < (400 / ledTASK_CYCLE_TIME))
				ledPWR_SW_OFF();
			else if (l_delay_cnt >= (400 / ledTASK_CYCLE_TIME))
				l_delay_cnt = 0;
		}
		break;
		
		#if (boardUPDATE)
		case STI_UPDATE:
		{
			l_delay_cnt++;
			if (l_delay_cnt > 0)	/* 快闪 */
			{
				l_delay_cnt = 0;
				v_led_breathing();
			}
		}
		break;
		#endif  /* boardUPDATE */
		
		#if (boardDISPLAY_EN)
		case STI_DISPLAY:
		{
			l_delay_cnt++;
			if (l_delay_cnt > (200 / ledTASK_CYCLE_TIME))	/* 慢闪 */
			{
				l_delay_cnt = 0;
				v_led_breathing();
			}
		}
		break;
		#endif  /* boardDISPLAY_EN */
		
		#if (boardLOW_POWER)
		case STI_LOW_POWER:
		{  
			led_1_GPIO_OFF();
			ledPWR_SW_OFF();
		}
		break;
		#endif  /* boardLOW_POWER */
		
		default:
		{
		}
		break;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 电源指示灯呼吸效果
 * 说明(备注)  : 50ms 周期调用，调整电源按键 PWM 占空比实现呼吸灯渐变
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_led_breathing(void)
{
	static uint8_t s_uc_breath_cnt = 0;
	static bool    s_b_breath_dec  = false;

	ledPWR_SW_PWM_SET(s_uc_breath_cnt * 50);

	if (!s_b_breath_dec)
		s_uc_breath_cnt++;
	else if (s_uc_breath_cnt > 0)
		s_uc_breath_cnt--;

	if (s_uc_breath_cnt >= 20)
		s_b_breath_dec = true;
	else if (s_uc_breath_cnt == 0)
		s_b_breath_dec = false;
}
#endif  /* boardLED_EN */

