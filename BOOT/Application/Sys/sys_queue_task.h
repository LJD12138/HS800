/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application\Sys
 * File    : sys_queue_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统总任务队列管理头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef SYS_QUEUE_TASK_H_
#define SYS_QUEUE_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			sysTASK_CYCLE_TIME						10		/* 任务周期时间 (ms) */

//****************************************************Globals*******************************************************************//
extern Task_T *tpSysTask;

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
bool bSys_QueueInit(void);
s8   cSys_JumpToApp(void);

/* 队列任务函数声明 */
void v_sys_queue_task_init(Task_T *p_task);
void v_sys_queue_task_enter_app(Task_T *p_task);
void v_sys_queue_task_err(Task_T *p_task);
void v_sys_queue_task_reset(Task_T *p_task);

#if (boardDISPLAY_EN)
void v_sys_queue_task_disp(Task_T *p_task);
#endif  /* boardDISPLAY_EN */

#if (boardUPDATE)
void v_sys_queue_task_update(Task_T *p_task);
#endif  /* boardUPDATE */

#if (boardLOW_POWER)
void v_sys_queue_task_low_power(Task_T *p_task);
#endif  /* boardLOW_POWER */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* SYS_QUEUE_TASK_H_ */
