/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_queue_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print模块任务队列管理及各子任务函数声明
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef __PRINT_QUEUE_TASK_H
#define __PRINT_QUEUE_TASK_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardPRINT_IFACE)
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
bool bPrint_QueueInit(void);

void v_print_queue_task_main(Task_T *tp_task);
void v_print_queue_task_reply_app_info(Task_T *tp_task);
void v_print_queue_task_reply_cali(Task_T *tp_task);

#if (boardBMS_EN && boardRUN_LOG_EN)
void v_print_queue_task_reply_run_log(Task_T *tp_task);
#endif  /* boardBMS_EN && boardRUN_LOG_EN */

#if (boardUPDATE)
void v_print_queue_task_update(Task_T *tp_task);
#endif  /* boardUPDATE */

#endif  /* boardPRINT_IFACE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __PRINT_QUEUE_TASK_H */
