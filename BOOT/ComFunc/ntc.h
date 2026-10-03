/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\ComFunc
 * File    : ntc.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : NTC 温度查表与转换计算模块头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef NTC_H
#define NTC_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if (1)
//****************************************************Macros********************************************************************//
                
#define			ntc10K_B3950_RES_TABLE_SIZE				161
#define			ntc10K_B3950_INDEX_ZERO_TEMP			(-40)

#define			ntc100K_B3950_RES_TABLE_SIZE			161
#define			ntc100K_B3950_INDEX_ZERO_TEMP			(-40)


//****************************************************Types*********************************************************************//
typedef struct
{
	float				sys_vol;			//电压
	u16					volt_res;			//ntc分压电阻
	u16					ntc_res;			//ntc额定电阻
	u16					hex_x;				//ADC分辨率 -12Bit_4096 10Bit_1025 8Bit_256
	u16					b_x;				//B值
}NtcVal_T;

//****************************************************Globals*******************************************************************//
typedef NtcVal_T ntc_val_t;                 /* 兼容旧类型定义 */

//****************************************************Extern********************************************************************//
void vNtc_Init(ntc_val_t *p_val, float sys_vol, u16 volt_res, u16 ntc_res, u16 hex_x, u16 b_x);
s16  sNtc_GetTempByRes(const u32 *p_buff, const s16 zero_index_temp, const u16 len, const u32 res);
int16_t sNtc_CalcTempByAd(uint16_t us_ad_val);
#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* NTC_H */

