/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_queue_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 请求队列与任务调度相关定义头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_MPPT_QUEUE_TASK_H_
#define MD_MPPT_QUEUE_TASK_H_

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
#define			mpptTASK_CYCLE_TIME						1000	/* 任务时间 */

//****************************************************Extern********************************************************************//
bool bMppt_QueueInit(void);

/* 队列任务函数 */
void v_mppt_queue_task_init(Task_T *tp_task);
void v_mppt_queue_task_main(Task_T *tp_task);
void v_mppt_queue_task_set_chg_pwr(Task_T *tp_task);
void v_mppt_queue_task_err_process(Task_T *tp_task);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_MPPT_QUEUE_TASK_H_ */
