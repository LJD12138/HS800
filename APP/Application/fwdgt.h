/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : fwdgt.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 独立看门狗驱动与系统复位原因捕获头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef FWDGT_H_
#define FWDGT_H_

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardWDGT_EN)

//****************************************************Types*********************************************************************//
typedef struct
{
	uint8_t				ext_pin;			//外部管脚复位
	uint8_t				por;				//上电/掉电复位
	uint8_t				sw;					//软件复位
	uint8_t				fwdgt;				//独立看门狗复位
	uint8_t				wwdgt;				//窗口看门狗复位
	uint8_t				low_power;			//低功耗复位
}ResetReason_T;
typedef ResetReason_T reset_reason_t;

//****************************************************Globals*******************************************************************//
extern ResetReason_T G_tResetReason;
#define			g_reset_reason							G_tResetReason

//****************************************************Extern********************************************************************//
void vFwdgt_Init(void);
void vFwdgt_Reload(void);
void vFwdgt_EnterLowPower(void);
void vFwdgt_ExitLowPower(void);
void vResetReason_Capture(void);
void vFwdgt_PrintResetReason(void);

#endif  //boardWDGT_EN

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  /* FWDGT_H_ */
