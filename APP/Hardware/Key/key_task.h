/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Key
 * File    : key_task.h
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : 按键任务头文件及对外业务接口定义
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef KEY_TASK_H_
#define KEY_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "Key/key_iface.h"

#if (boardKEY_EN)

//****************************************************Macros********************************************************************//
#define			keyTASK_CYCLE_TIME						10		/* 按键任务更新时间 (ms) */
#define			keyGROUP_NUM							10		/* 组合按键种类/最大事件缓冲深度 */
#define			keySHORT_PRESS_TIME						2		/* 短按按键的最小时间 * 10ms = 20ms */
#define			keyLONG_PRESS_TIME						90		/* 长按按键的最小时间 * 10ms = 900ms */
#define			keySUPER_LONG_PRESS_TIME				250		/* 超长按按键的最小时间 * 10ms = 2500ms */
#define			keyNUPRESS_MAX_TIME						35		/* 组合按键最大的等待时间 * 10ms = 350ms */
#define			keyADD_SPACE_TIME						20		/* 长按累加间隔 * 10ms = 200ms */

//****************************************************Types*********************************************************************//
/* 触发事件枚举 */
typedef enum
{
	KTE_FUN_NULL = 0,		/* 空事件 */
	KTE_POWER_LONG,			/* 电源键长按 */
	KTE_POWER_SHORT,		/* 电源键短按 */
	KTE_POWER_SUPER_LONG,	/* 电源键超长按 */
	KTE_AC_LONG,			/* AC键长按 */
	KTE_AC_SHORT,			/* AC键短按 */
	KTE_AC_SUPER_LONG,		/* AC键超长按 */
	KTE_LIGHT_LONG,			/* 照明灯长按 */
	KTE_LIGHT_SHORT,		/* 照明灯短按 */
	KTE_LIGHT_SUPER_LONG,	/* 照明灯超长按 */
	KTE_USB_LONG,			/* USB键长按 */
	KTE_USB_SHORT,			/* USB键短按 */
	KTE_USB_SUPER_LONG,		/* USB键超长按 */
	KTE_DC_LONG,			/* DC键长按 */
	KTE_DC_SHORT,			/* DC键短按 */
	KTE_DC_SUPER_LONG,		/* DC键超长按 */
}KeyTriEvent_E;

//****************************************************Globals*******************************************************************//
typedef KeyTriEvent_E KeyTriEvent_e;

//****************************************************Extern********************************************************************//
s8   cKey_TaskInit(void);
void vKey_PowerIsTri(void);
void vKey_ParamInit(void);
bool bKey_IsAnyPress(void);
bool bKey_IsFactoryModePress(void);
bool bKey_IsEngModePress(void);

#if (!boardUSE_OS)
void vKey_Task(void *p_v_parameters);
#endif  /* !boardUSE_OS */

#if (boardLOW_POWER)
void vKey_EnterLowPower(void);
void vKey_ExitLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardKEY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* KEY_TASK_H_ */
