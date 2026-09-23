/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Light
 * File    : md_light_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 照明灯控制任务与工作模式定义头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_LIGHT_TASK_H_
#define MD_LIGHT_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardLIGHT_EN)

//****************************************************Macros********************************************************************//
#define			lightSIMPLE_MODE						1		/* 0:简单模式, 1:全功能(SOS/闪烁) */

//****************************************************Types*********************************************************************//
/* 照明灯工作模式 */
typedef enum
{
	LWM_OFF = 0,
	LWM_HALF,
	LWM_FULL,
	#if (lightSIMPLE_MODE)
	LWM_SOS,
	LWM_TWINKLE,
	#endif  /* lightSIMPLE_MODE */
}LightWorkMode_E;

typedef struct
{
	vu16				usValue;			/* 当前 PWM 占空比 */
	vu16				usLastValue;		/* 记忆 PWM 占空比 */
	vu16				usPower;			/* 估算功率 (W) */
	LightWorkMode_E		eWorkMode;			/* 工作模式 */
	DevState_E			eDevState;			/* 设备状态 */
}Light_T;

//****************************************************Extern********************************************************************//
extern Light_T tLight;

s8   cLight_TaskInit(void);
bool bLight_Switch(SwitchType_E type);
void vLight_CircSelectMode(void);

#if (boardLOW_POWER)
void vLight_EnterLowPower(void);
void vLight_ExitLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardLIGHT_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_LIGHT_TASK_H_ */
