/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Adc
 * File    : adc_task.h
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : ADC采样任务与物理量换算接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef ADC_TASK_H
#define ADC_TASK_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardADC_EN)
#include "Adc/adc_iface.h"

#if (1)
//****************************************************Macros********************************************************************//

/* 1. 电池输入电压采样分压比 (单位: 0.1V) */
#define			adcVBMS_R1								1800.0f	/* Kohm 分压上电阻 */
#define			adcVBMS_R2								100.0f	/* Kohm 分压对地电阻 */
#define			adcVBMS_RES_RATIO						((((3.3f / 4095.0f) * (adcVBMS_R1 + adcVBMS_R2)) / adcVBMS_R2) * 10.0f)

/* 2. DC输出电压采样分压比 (单位: 0.1V) */
#define			adcDC_OUT_VOLT_R1							100.0f	/* Kohm 分压上电阻 */
#define			adcDC_OUT_VOLT_R2							22.0f	/* Kohm 分压对地电阻 */
#define			adcDC_OUT_VOLT_RES_RATIO					((((3.3f / 4095.0f) * (adcDC_OUT_VOLT_R1 + adcDC_OUT_VOLT_R2)) / adcDC_OUT_VOLT_R2) * 10.0f)

/* 3. DC输入电压1采样分压比 (单位: 0.1V) */
#define			adcDC_IN_1_R1							300.0f	/* Kohm 分压上电阻 */
#define			adcDC_IN_1_R2							10.0f	/* Kohm 分压对地电阻 */
#define			adcDC_IN_1_RES_RATIO					((((3.3f / 4095.0f) * (adcDC_IN_1_R1 + adcDC_IN_1_R2)) / adcDC_IN_1_R2) * 10.0f)

/* 4. DC输入电压2采样分压比 (单位: 0.1V) */
#define			adcDC_IN_2_R1							300.0f	/* Kohm 分压上电阻 */
#define			adcDC_IN_2_R2							10.0f	/* Kohm 分压对地电阻 */
#define			adcDC_IN_2_RES_RATIO					((((3.3f / 4095.0f) * (adcDC_IN_2_R1 + adcDC_IN_2_R2)) / adcDC_IN_2_R2) * 10.0f)

/* 5. DC输出电流换算系数 (单位: A/LSB) */
#define			adcDC_OUT_CURR_SCALE_FACTOR					0.0034f


//****************************************************Types*********************************************************************//

/* 电压状态枚举 */
typedef enum
{
	VS_NORMAL = 0,		/* 正常 */
	VS_LOW,				/* 欠压 */
	VS_HIGH,			/* 过压 */
}VoltSate_E;

/* ADC 采样物理量结构体 */
typedef struct
{
	vu16				usSysInVolt;		/* 电池/系统输入电压 (单位: 0.1V) */
	s16					sDcOutTemp;			/* DC输出温度 (单位: 摄氏度) */
	float				fDcOutCurr;			/* DC输出电流 (单位: A) */
	vu16				usDcOutVolt;		/* DC输出电压 (单位: 0.1V) */
	vu16				usKeyPower;			/* Key Power按键 AD采样值 */
	vu16				usDcIn1Volt;		/* DC输入电压1 (DC-INAD, 单位: 0.1V) */
	vu16				usDcIn2Volt;		/* DC输入电压2 (XT-INAD, 单位: 0.1V) */
}AdcSamp_T;

//****************************************************Globals*******************************************************************//
extern AdcSamp_T tAdcSamp;


//****************************************************Extern********************************************************************//
s8      cAdc_TaskInit(void);
int16_t sAdc_CalcTempByAd(uint16_t us_ad_val);

#if (!boardUSE_OS)
void    vAdc_Task(void *p_v_parameters);
#endif  /* !boardUSE_OS */

#if (boardLOW_POWER)
bool    bAdc_EnterLowPower(void);
bool    bAdc_ExitLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* 1 */

#endif  /* boardADC_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* ADC_TASK_H */
