/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Adc
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

#if (1)
//****************************************************Macros********************************************************************//

#if (boardUSE_OS)
#define			adcTASK_PRIO							4		/* 任务优先级(安全采集层:保护链源头) */
#define			adcTASK_STK_SIZE						256		/* 任务堆栈(字) */
#endif  /* boardUSE_OS */

/* 滤波器缓冲区长度配置 */
#define			adcSYS_IN_VOLT_FILTER_BUFF_SIZE			6		/* 电池/系统输入电压滤波深度 */
#define			adcDC_OUT_TEMP_FILTER_BUFF_SIZE				6		/* DC温度滤波深度 */
#define			adcDC_OUT_CURR_FILTER_BUFF_SIZE				12		/* DC输出电流滤波深度 */
#define			adcDC_OUT_VOLT_FILTER_BUFF_SIZE				6		/* DC输出电压滤波深度 */
#define			adcDC_IN_1_FILTER_BUFF_SIZE				6		/* DC输入电压1滤波深度 */
#define			adcDC_IN_2_FILTER_BUFF_SIZE				6		/* DC输入电压2滤波深度 */

//****************************************************Parameter Initialization**************************************************//

#if (boardUSE_OS)
static TaskHandle_t s_t_adc_task_handler = NULL;
#endif  /* boardUSE_OS */

/* 全局物理量采样结构体 */
AdcSamp_T tAdcSamp;

/* 1. 系统输入电压滤波器 */
static s32 s_sa_sys_in_volt_buff[adcSYS_IN_VOLT_FILTER_BUFF_SIZE];
static FilterHandler_T s_t_adc_sys_in_volt_filter_mad_avg = {s_sa_sys_in_volt_buff, adcSYS_IN_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

#if (boardDC_EN)
/* 2. DC温度滤波器 */
static s32 s_sa_dc_temp_buff[adcDC_OUT_TEMP_FILTER_BUFF_SIZE];
static FilterHandler_T s_t_adc_dc_temp_filter_mad_avg = {s_sa_dc_temp_buff, adcDC_OUT_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

/* 3. DC电流滤波器 */
static s32 s_sa_dc_curr_buff[adcDC_OUT_CURR_FILTER_BUFF_SIZE];
static FilterHandler_T s_t_adc_dc_curr_filter_mad_avg = {s_sa_dc_curr_buff, adcDC_OUT_CURR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

/* 4. DC电压滤波器 */
static s32 s_sa_dc_volt_buff[adcDC_OUT_VOLT_FILTER_BUFF_SIZE];
static FilterHandler_T s_t_adc_dc_volt_filter_mad_avg = {s_sa_dc_volt_buff, adcDC_OUT_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};
#endif  /* boardDC_EN */

/* 5. DC输入电压1滤波器 */
static s32 s_sa_dc_in_1_buff[adcDC_IN_1_FILTER_BUFF_SIZE];
static FilterHandler_T s_t_adc_dc_in_1_filter_mad_avg = {s_sa_dc_in_1_buff, adcDC_IN_1_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

/* 6. DC输入电压2滤波器 */
static s32 s_sa_dc_in_2_buff[adcDC_IN_2_FILTER_BUFF_SIZE];
static FilterHandler_T s_t_adc_dc_in_2_filter_mad_avg = {s_sa_dc_in_2_buff, adcDC_IN_2_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

/* ---------------------------------------------------------------------------------------------------------------------
 * NTC 温度拟合定点查找表 (Q8 格式: 真实值 * 256)
 * 对应原式 T = 307 - 37 * ln(AD)，步长为 64 (覆盖 AD 0 ~ 4096，共 65 个节点)
 * 在 Cortex-M4 下通过单周期纯整数乘法插值即可完成高精计算，完全消除 log 浮点库
 * --------------------------------------------------------------------------------------------------------------------- */
static const int16_t s_sa_adc_temp_lut_q8[65] =
{
	32634,  32634,  32634,  28793,  26068,  23954,  22228,  20767,
	19503,  18387,  17389,  16486,  15662,  14904,  14202,  13548,
	12937,  12363,  11821,  11309,  10823,  10361,   9921,   9500,
	 9097,   8710,   8338,   7981,   7636,   7304,   6983,   6672,
	 6372,   6080,   5797,   5523,   5256,   4996,   4744,   4498,
	 4258,   4024,   3796,   3573,   3355,   3142,   2934,   2730,
	 2531,   2336,   2144,   1957,   1773,   1592,   1415,   1242,
	 1071,    903,    739,    577,    417,    261,    107,    -45,
	 -194
};

//****************************************************Function Declaration******************************************************//
static void v_adc_param_init(void);

#if (boardUSE_OS)
static void v_adc_task_loop(void *p_v_parameters);
#endif  /* boardUSE_OS */


/***********************************************************************************************************************
 * 函数功能    : 通过采样 AD 值计算温度 (定点查表线性插值法)
 * 说明(备注)  : 原理对应 T = 307 - 37 * ln(AD)，消除 math.h 对数库调用与除零硬件异常
 * 传入参数    : us_ad_val: 12 位采样 AD 转换值 (0~4095)
 * 输出参数    : 无
 * 返回值      : int16_t: 计算得出的温度值 (单位: 摄氏度, 范围 -128 ~ 127)
 ************************************************************************************************************************/
int16_t sAdc_CalcTempByAd(uint16_t us_ad_val)
{
	int32_t s_idx;
	int32_t s_rem;
	int32_t s_t0;
	int32_t s_t1;
	int32_t s_t_q8;
	int32_t s_temp;

	/* 1. 安全边界检查与极限情况处理 */
	if (us_ad_val <= 129)
		return 127;     /* AD过小或短路，判定为最高保护温标 127 度 */
	if (us_ad_val >= 4080)
		return 0;       /* AD满量程或开路(NTC断线/丢失)，判定为 0 度，触发 NTC 掉线保护 */

	/* 2. 定点查表与线性插值 (步长 64，右移 6 位) */
	s_idx = (int32_t)(us_ad_val >> 6);
	s_rem = (int32_t)(us_ad_val & 63);

	s_t0 = (int32_t)s_sa_adc_temp_lut_q8[s_idx];
	s_t1 = (int32_t)s_sa_adc_temp_lut_q8[s_idx + 1];

	/* 线性插值并四舍五入: T = T0 + (T1 - T0) * rem / 64 */
	s_t_q8 = s_t0 + (((s_t1 - s_t0) * s_rem) >> 6);
	s_temp = (s_t_q8 + 128) >> 8;

	/* 3. 输出门限钳位 */
	if (s_temp > 127)
		s_temp = 127;
	else if (s_temp < -128)
		s_temp = -128;

	return (int16_t)s_temp;
}

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
	if (xTaskCreate((TaskFunction_t )v_adc_task_loop,
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

	#if (boardDC_EN)
	memset((uint8_t *)&s_sa_dc_temp_buff, 0, sizeof(s_sa_dc_temp_buff));
	memset((uint8_t *)&s_sa_dc_curr_buff, 0, sizeof(s_sa_dc_curr_buff));
	memset((uint8_t *)&s_sa_dc_volt_buff, 0, sizeof(s_sa_dc_volt_buff));
	#endif  /* boardDC_EN */

	memset((uint8_t *)&s_sa_dc_in_1_buff, 0, sizeof(s_sa_dc_in_1_buff));
	memset((uint8_t *)&s_sa_dc_in_2_buff, 0, sizeof(s_sa_dc_in_2_buff));
}

/***********************************************************************************************************************
 * 函数功能    : ADC核心采样与物理量计算任务
 * 说明(备注)  : 定期拉取各通道原始值进行滑动中位值平均滤波，并换算为实际电压、电流与温度
 * 传入参数    : p_v_parameters: 任务创建参数指针
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
#if (boardUSE_OS)
static void v_adc_task_loop(void *p_v_parameters)
#else
void vAdc_Task(void *p_v_parameters)
#endif  /* boardUSE_OS */
{
	(void)p_v_parameters;

	s32      s_temp_ad = 0;
	uint16_t us_filter_sys_input_volt_ad = 0;

	#if (boardDC_EN)
	uint16_t us_filter_dc_temp_ad = 0;
	uint16_t us_filter_dc_curr_ad = 0;
	uint16_t us_filter_dc_volt_ad = 0;
	#endif  /* boardDC_EN */

	uint16_t us_filter_dc_in_1_ad = 0;
	uint16_t us_filter_dc_in_2_ad = 0;

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

		#if (boardDC_EN)
		/* 1.2 DC温度采样 */
		s_temp_ad = usAdc_GetChannelValue(adcDC_OUT_TEMP);
		us_filter_dc_temp_ad = (uint16_t)lFilter_MadianAverage(&s_t_adc_dc_temp_filter_mad_avg, &s_temp_ad);

		/* 1.3 DC输出电流 */
		s_temp_ad = usAdc_GetChannelValue(adcDC_OUT_CURR);
		us_filter_dc_curr_ad = (uint16_t)lFilter_MadianAverage(&s_t_adc_dc_curr_filter_mad_avg, &s_temp_ad);

		/* 1.4 DC输出电压 */
		s_temp_ad = usAdc_GetChannelValue(adcDC_OUT_VOLT);
		us_filter_dc_volt_ad = (uint16_t)lFilter_MadianAverage(&s_t_adc_dc_volt_filter_mad_avg, &s_temp_ad);
		#endif  /* boardDC_EN */

		/* 1.5 DC输入电压1 */
		s_temp_ad = usAdc_GetChannelValue(adcDC_IN_1);
		us_filter_dc_in_1_ad = (uint16_t)lFilter_MadianAverage(&s_t_adc_dc_in_1_filter_mad_avg, &s_temp_ad);

		/* 1.6 DC输入电压2 */
		s_temp_ad = usAdc_GetChannelValue(adcDC_IN_2);
		us_filter_dc_in_2_ad = (uint16_t)lFilter_MadianAverage(&s_t_adc_dc_in_2_filter_mad_avg, &s_temp_ad);

		/* 注意: adcKEY_POWER 不需要滤波，在Key任务中直接读取原始AD值 */

		/* ================= 2. 物理量算式换算 ================= */

		/* 系统/电池输入电压 (单位: 0.1V) */
		tAdcSamp.usSysInVolt = (uint16_t)(us_filter_sys_input_volt_ad * adcVBMS_RES_RATIO);

		#if (boardDC_EN)
		/* DC温度换算: 高效定点查表，彻底消除 log 库调用 */
		tAdcSamp.sDcOutTemp = sAdc_CalcTempByAd(us_filter_dc_temp_ad);

		/* DC输出电流 (单位: A) */
		tAdcSamp.fDcOutCurr = (float)us_filter_dc_curr_ad * adcDC_OUT_CURR_SCALE_FACTOR;

		/* DC输出电压 (单位: 0.1V) */
		tAdcSamp.usDcOutVolt = (uint16_t)(us_filter_dc_volt_ad * adcDC_OUT_VOLT_RES_RATIO);

		/* DC 峰值过流越限哨兵: 提前唤醒 DcTask 执行保护扫描 */
		vDc_AdcWatchdog();
		#endif  /* boardDC_EN */

		/* 按键电源 (AD原始值，直接存储) */
		tAdcSamp.usKeyPower = usAdc_GetChannelValue(adcKEY_POWER);

		/* DC输入电压1 (单位: 0.1V) */
		tAdcSamp.usDcIn1Volt = (uint16_t)(us_filter_dc_in_1_ad * adcDC_IN_1_RES_RATIO);

		/* DC输入电压2 (单位: 0.1V) */
		tAdcSamp.usDcIn2Volt = (uint16_t)(us_filter_dc_in_2_ad * adcDC_IN_2_RES_RATIO);

		/* ================= 3. 调试信息打印 ================= */
		if (uPrint.tFlag.bAdcTask)
		{
			s_uc_delay_cnt++;
			if (s_uc_delay_cnt >= 100)
			{
				s_uc_delay_cnt = 0;
				sMyPrint("电池电压 = %.2fV\r\n", tAdcSamp.usSysInVolt / 10.0f);
				#if (boardDC_EN)
				sMyPrint("DC温度 = %d 摄氏度\r\n", tAdcSamp.sDcOutTemp);
				sMyPrint("DC电流 = %.2fA\r\n", tAdcSamp.fDcOutCurr);
				sMyPrint("DC电压 = %.2fV\r\n", tAdcSamp.usDcOutVolt / 10.0f);
				#endif  /* boardDC_EN */
				sMyPrint("按键电源AD = %d\r\n", tAdcSamp.usKeyPower);
				sMyPrint("DC输入电压1 = %.2fV\r\n", tAdcSamp.usDcIn1Volt / 10.0f);
				sMyPrint("DC输入电压2 = %.2fV\r\n", tAdcSamp.usDcIn2Volt / 10.0f);
			}
		}

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

#endif  /* 1 */

#endif  /* boardADC_EN */