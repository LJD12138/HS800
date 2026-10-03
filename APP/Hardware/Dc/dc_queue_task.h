/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Dc
 * File    : dc_queue_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : DC 任务队列管理模块头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef DC_QUEUE_TASK_H_
#define DC_QUEUE_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDC_EN)
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			dcTASK_CYCLE_TIME						200		/* DC 任务周期 (ms) */

//****************************************************Extern********************************************************************//
bool bDc_QueueInit(void);

/* 队列任务函数 */
void v_dc_queue_task_init(Task_T *p_task);
void v_dc_queue_task_closing(Task_T *p_task);
void v_dc_queue_task_shut_down(Task_T *p_task);
void v_dc_queue_task_err(Task_T *p_task);
void v_dc_queue_task_booting(Task_T *p_task);
void v_dc_queue_task_work(Task_T *p_task);

#endif  /* boardDC_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* DC_QUEUE_TASK_H_ */
