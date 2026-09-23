/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_queue_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器队列任务管理与子任务分发接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DCAC_QUEUE_TASK_H_
#define MD_DCAC_QUEUE_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDCAC_EN)
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Extern********************************************************************//
extern Task_T *tpDcacTask;


bool bDcac_QueueInit(void);

/* 队列子任务函数 */
void v_dcac_queue_task_init(Task_T *p_task);
void v_dcac_queue_task_main(Task_T *p_task);
void v_dcac_queue_task_dcac_out(Task_T *p_task);
void v_dcac_queue_task_dcac_in(Task_T *p_task);
void v_dcac_queue_task_para_in(Task_T *p_task);
void v_dcac_queue_task_err_proc(Task_T *p_task);
void v_dcac_queue_task_update(Task_T *p_task);

#endif  /* boardDCAC_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DCAC_QUEUE_TASK_H_ */
