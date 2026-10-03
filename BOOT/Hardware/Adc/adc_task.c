/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Adc
 * File    : adc_task.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : ADC采样任务与物理量计算实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Adc/adc_task.h"

#if (boardADC_EN)
#include "Adc/adc_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "filtration.h"
#include <string.h>

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

#if (boardDC_EN)
#include "Dc/dc_task.h"
#endif  /* boardDC_EN */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			adcTASK_PRIO							4		/* 任务优先级(安全采集层:保护链源头) */
#define			adcTASK_STK_SIZE						256		/* 任务堆栈(字) */
static TaskHandle_t s_t_adc_task_handler = NULL;				/* 任务句柄 */
static void 	vAdc_Task(void *p_v_parameters);				/* 任务函数 */
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//

/* 滤波器缓冲区长度配置 */
#define			adcSYS_IN_VOLT_FILTER_BUFF_SIZE			6		/* 电池/系统输入电压滤波深度 */

//****************************************************Parameter Initialization**************************************************//
/* 全局物理量采样结构体 */
AdcSamp_T tAdcSamp;

/* 1. 系统输入电压滤波器 */
static s32 s_sa_sys_in_volt_buff[adcSYS_IN_VOLT_FILTER_BUFF_SIZE];
static FilterHandler_T s_t_adc_sys_in_volt_filter_mad_avg = {s_sa_sys_in_volt_buff, adcSYS_IN_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//****************************************************Function Declaration******************************************************//
static void v_adc_param_init(void);


/***********************************************************************************************************************
 * 函数功能    : ADC任务与外设初始化
 * 说明(备注)  : 完成底层的模拟引脚、DMA以及 FreeRTOS 采样任务创建
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; -1: 任务创建失败
 ************************************************************************************************************************/
s8 cAdc_TaskInit(void)
{
	#if (boardLOW_POWER)
	vAdc_IoEnterLowPower();
	#endif  /* boardLOW_POWER */

	vAdc_Init();            /* 底层外设与DMA通道初始化 */
	v_adc_param_init();     /* 滤波参数与采样状态清零 */

	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t )vAdc_Task,
	                (const char*    )"AdcTask",
	                (uint16_t       )adcTASK_STK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )adcTASK_PRIO,
	                (TaskHandle_t*  )&s_t_adc_task_handler) != pdPASS)
		return -1;
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : ADC参数与缓冲区初始化
 * 说明(备注)  : 清空全部通道的滤波历史队列
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_adc_param_init(void)
{
	memset((uint8_t *)&s_sa_sys_in_volt_buff, 0, sizeof(s_sa_sys_in_volt_buff));
}

/***********************************************************************************************************************
 * 函数功能    : ADC核心采样与物理量计算任务
 * 说明(备注)  : 定期拉取各通道原始值进行滑动中位值平均滤波，并换算为实际电压、电流与温度
 * 传入参数    : p_v_parameters: 任务创建参数指针
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vAdc_Task(void *p_v_parameters)
{
	(void)p_v_parameters;

	s32      s_temp_ad = 0;
	uint16_t us_filter_sys_input_volt_ad = 0;

	static uint8_t s_uc_init_adc_cnt = 0;
	static uint8_t s_uc_delay_cnt = 0;

	#if (boardUSE_OS)
	for (;;)
	#endif  /* boardUSE_OS */
	{
		/* ================= 1. 各路模拟量滑动中位均值滤波 ================= */

		/* 1.1 系统/电池输入电压 */
		s_temp_ad = usAdc_GetChannelValue(adcSYS_IN_VOLT);
		us_filter_sys_input_volt_ad = (uint16_t)lFilter_MadianAverage(&s_t_adc_sys_in_volt_filter_mad_avg, &s_temp_ad);

		/* ================= 2. 物理量算式换算 ================= */

		/* 系统/电池输入电压 (单位: 0.1V) */
		tAdcSamp.usSysInVolt = (uint16_t)(us_filter_sys_input_volt_ad * adcVBMS_RES_RATIO);

		/* ================= 4. 等待ADC初次采集稳定 ================= */
		if (s_uc_init_adc_cnt < 0xFF)
			s_uc_init_adc_cnt++;

		if (s_uc_init_adc_cnt == 10)
			tSysInfo.uInit.tFinish.bIF_AdcTask = true;

		#if (boardUSE_OS)
		if (tSysInfo.eDevState == DS_INIT)
			vTaskDelay(30);
		else
			vTaskDelay(100);
		#endif  /* boardUSE_OS */
	}
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : ADC模块进入低功耗
 * 说明(备注)  : 挂起采样任务并关闭底层引脚模拟态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 成功
 ************************************************************************************************************************/
bool bAdc_EnterLowPower(void)
{
	#if (boardUSE_OS)
	if (s_t_adc_task_handler != NULL)
		vTaskSuspend(s_t_adc_task_handler);
	#endif  /* boardUSE_OS */

	vAdc_IoEnterLowPower();
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : ADC模块退出低功耗
 * 说明(备注)  : 恢复底层外设并恢复采样任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 成功
 ************************************************************************************************************************/
bool bAdc_ExitLowPower(void)
{
	vAdc_Init();
	#if (boardUSE_OS)
	if (s_t_adc_task_handler != NULL)
		vTaskResume(s_t_adc_task_handler);
	#endif  /* boardUSE_OS */

	return true;
}
#endif  /* boardLOW_POWER */

#endif  /* boardADC_EN */

