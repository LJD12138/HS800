/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application
 * File    : fwdgt.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 独立看门狗驱动及芯片复位原因检测头文件
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
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardWDGT_EN)
//****************************************************Macros********************************************************************//



//****************************************************Types*********************************************************************//
typedef struct
{
	uint8_t				ext_pin;
	uint8_t				por;
	uint8_t				sw;
	uint8_t				fwdgt;
	uint8_t				wwdgt;
	uint8_t				low_power;
}ResetReason_T;

//****************************************************Globals*******************************************************************//
typedef ResetReason_T reset_reason_t;

//****************************************************Extern********************************************************************//
extern reset_reason_t g_reset_reason;

void vFwdgt_Init(void);
void vFwdgt_Reload(void);
void vFwdgt_EnterLowPower(void);
void vFwdgt_ExitLowPower(void);
void vResetReason_Capture(void);
void vFwdgt_PrintResetReason(void);

#endif  /* boardWDGT_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* FWDGT_H_ */
