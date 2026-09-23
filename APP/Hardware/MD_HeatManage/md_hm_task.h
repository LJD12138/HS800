/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_HeatManage
 * File    : md_hm_task.h
 * Date    : 2026-09-12
 * Author  : LJD(291483914@qq.com)
 * Desc    : 风扇与热管理任务头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_HM_TASK_H_
#define MD_HM_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

//****************************************************Macros********************************************************************//
#define			fanSIMPLE_MODE							1		/* 0:简单模式, 1:全功能 */

//****************************************************Types*********************************************************************//

/* 风扇工作挡位枚举 */
typedef enum
{
	FWM_OFF = 0,		/* 停止 */
	FWM_GEAR_1,			/* 1挡 */
	FWM_GEAR_2,			/* 2挡 */
	FWM_GEAR_3,			/* 3挡 */
	FWM_GEAR_FULL,		/* 全速 */
}FanWorkMode_E;

/* 热管理运行控制结构体 */
typedef struct
{
	__IO uint16_t		usValue;			/* 当前 PWM 占空比 (0~1000) */
	__IO int16_t		sMaxTemp;			/* 当前各路 NTC 最高温度 (℃) */
	FanWorkMode_E		eWorkMode;			/* 风扇当前工作挡位 */
}HM_T;

//****************************************************Globals*******************************************************************//
extern HM_T tHM;

//****************************************************Extern********************************************************************//
s8 cHM_TaskInit(void);
FanWorkMode_E eFan_GetWorkMode(void);
void vFan_ForceOpenFan(bool en);

#if (boardLOW_POWER)
void vFan_EnterLowPower(void);
void vFan_ExitLowPower(void);
#endif  /* boardLOW_POWER */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_HM_TASK_H_ */

