/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
 * File    : md_bms_queue_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 任务队列管理与子任务分发接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_BMS_QUEUE_TASK_H_
#define MD_BMS_QUEUE_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"   

#if (boardBMS_EN)
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Globals*******************************************************************//
extern Task_T *tpBmsTask;  /* 队列任务指针 */

//****************************************************Extern********************************************************************//
bool bBms_QueueInit(void);

/* 队列子任务函数 */
void v_bms_queue_task_main(Task_T *p_task);

#if (boardUPDATE)
void v_bms_queue_task_update(Task_T *p_task);
#endif  /* boardUPDATE */

#endif  /* boardBMS_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_BMS_QUEUE_TASK_H_ */
