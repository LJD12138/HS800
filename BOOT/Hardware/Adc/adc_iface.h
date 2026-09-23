/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Adc
 * File    : adc_iface.h
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : ADC硬件驱动底层接口定义头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef ADC_IFACE_H
#define ADC_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardADC_EN)

//****************************************************Macros********************************************************************//
#define			ADC_DMAX								2

#if (ADC_DMAX == 1)
#define			ADCX_RCU								RCU_ADC0
#define			ADCX									ADC0
#define			adcDMA_RCU								RCU_DMA1
#define			adcDMA									DMA1
#define			adcDMA_CH								DMA_CH4

#define			DMA_SUBPERIX							DMA_SUBPERI0
#elif (ADC_DMAX == 2)
#define			ADCX_RCU								RCU_ADC0
#define			ADCX									ADC0
#define			adcDMA_RCU								RCU_DMA0
#define			adcDMA									DMA0
#define			adcDMA_CH								DMA_CH0
#if (boardIC_TYPE == boardIC_GD32F50X)
#define			adcDMA_REQUEST							DMA_REQUEST_ADC0_ROUTINE
#endif  /* boardIC_TYPE */
#endif  /* ADC_DMAX */

//****************************************************Types*********************************************************************//
/* ADC通道枚举: 枚举序与硬件配置表、DMA缓存及常规通道序列严格一致 */
typedef enum
{
	adcSYS_IN_VOLT = 0,	/* 电源输入电压 BAT_ADC */
	adcCHANNEL_NUM		/* DMA缓存大小 */
}AdcChannel_E;


//****************************************************Globals*******************************************************************//
extern vu16 s_usa_adc_value[adcCHANNEL_NUM];

//****************************************************Extern********************************************************************//
void vAdc_Init(void);
void vAdc_DeInit(void);
u16  usAdc_GetChannelValue(AdcChannel_E e_channel);

#if (boardLOW_POWER)
void vAdc_IoEnterLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardADC_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* ADC_IFACE_H */
