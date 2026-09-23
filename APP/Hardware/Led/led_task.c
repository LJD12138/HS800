/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Led
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

#if (boardUSB_EN)
#include "Usb/usb_task.h"
#endif  /* boardUSB_EN */

#if (boardDC_EN)
#include "Dc/dc_task.h"
#endif  /* boardDC_EN */

#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif  //boardLIGHT_EN

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  //boardBMS_EN

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  /* boardDCAC_EN */

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			ledTASK_CYCLE_TIME						1000	/* 普通状态扫描周期 (ms) */

//****************************************************Parameter Initialization**************************************************//
#if (boardUSE_OS)
#define			LED_TASK_PRIO							1		/* 任务优先级 */
#define			LED_TASK_STK_SIZE						128		/* 任务堆栈 (512B，达到 configMINIMAL_STACK_SIZE 防溢出标准) */
static TaskHandle_t s_t_led_task_handler = NULL;
void        vLed_Task(void *p_v_parameters);
#endif  /* boardUSE_OS */

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
 * 说明(备注)  : 根据当前系统运行状态与外设开关状态刷新各按键指示灯
 * 传入参数    : p_v_parameters: 任务创建参数指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLed_Task(void *p_v_parameters)
{
	static vu16 led_breath_cnt = 0;

	#if (boardUSE_OS)
	for (;;)
	#endif  /* boardUSE_OS */
	{
		switch (tSysInfo.eDevState)
		{
			default:
			case DS_INIT:
			case DS_SHUT_DOWN:
			{
				ledPWR_SW_OFF();
				ledAC_SW_OFF();
				ledLight_SW_OFF();
				ledUSB_SW_OFF();
				ledDC_SW_OFF();
				#if (boardUSE_OS)
				vTaskDelay(ledTASK_CYCLE_TIME);
				#endif  //boardUSE_OS
			}
			break;
			
			case DS_BOOTING:
			{
				ledPWR_SW_ON();
				led_breath_cnt = 0;
				#if (boardUSE_OS)
				vTaskDelay(ledTASK_CYCLE_TIME);
				#endif  /* boardUSE_OS */
			}
			break;
			
			case DS_CLOSING:
			case DS_ERR:
			case DS_WORK:
			{
				#if (boardDISPLAY_EN)
				if(led_breath_cnt < 0xffff) led_breath_cnt++;
				if(bDisp_IsBacklightOn() == true || led_breath_cnt < (40))
					ledPWR_SW_ON();
				else
				#endif  /* boardDISPLAY_EN */
				{
					v_led_breathing();
				}

				#if (boardDCAC_EN)
				if (tDcac.eDisChgState >= IOS_STARTING)
					ledAC_SW_ON();
				else
					ledAC_SW_OFF();
				
				if(tLight.eDevState == DS_WORK)
					ledLight_SW_ON();
				else 
					ledLight_SW_OFF();
				
				#if(boardDCAC_PARA_IN)
				//USB DC
				if(tUsb.eDevState >= DS_BOOTING || tDc.eDevState >= DS_BOOTING)
					ledUSB_SW_ON();
				else 
					ledUSB_SW_OFF();
				//并网
				if(tDcac.eParanInState >= IOS_STARTING)
					ledDC_SW_ON();
				else 
					ledDC_SW_OFF();
				#else
				//USB
				if(tUsb.eDevState >= DS_BOOTING)
					ledUSB_SW_ON();
				else 
					ledUSB_SW_OFF();
				//DC
				if(tDc.eDevState >= DS_BOOTING)
					ledDC_SW_ON();
				else 
					ledDC_SW_OFF();
				#endif  /* boardDCAC_PARA_IN */
				
				#if (boardUSE_OS)
				vTaskDelay(50);
				#endif  /* boardUSE_OS */
				#endif  //boardDCAC_EN
			}
			break;
			
			#if(boardENG_MODE_EN)
			case DS_ENG_MODE:
			{
				static bool s_b_twinkle_flag = false;
				if (s_b_twinkle_flag)
				{
					ledPWR_SW_ON();
					ledAC_SW_ON();
					ledUSB_SW_ON();
					ledLight_SW_ON();
					ledDC_SW_ON();
					s_b_twinkle_flag = false;
				}
				else
				{
					ledPWR_SW_OFF();
					ledAC_SW_OFF();
					ledUSB_SW_OFF();
					ledLight_SW_OFF();
					ledDC_SW_OFF();
					s_b_twinkle_flag = true;
				}
				#if (boardUSE_OS)
				vTaskDelay(ledTASK_CYCLE_TIME);
				#endif  /* boardUSE_OS */
			}
			break;
			#endif  /* boardENG_MODE_EN */
		}
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

