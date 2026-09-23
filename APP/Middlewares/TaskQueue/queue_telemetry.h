/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\TaskQueue
 * File    : queue_telemetry.h
 * Date    : 2026-09-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 队列任务遥测旁挂模块头文件(仅调试编译存在)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef QUEUE_TELEMETRY_H
#define QUEUE_TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "queue_task.h"

#if (boardHEALTH_MONITOR_EN)
//****************************************************Extern********************************************************************//
void vQueue_UpdatePeak(Task_T *tp_task);
void vQueue_TelAttach(Task_T *tp_task);
#endif  /* boardHEALTH_MONITOR_EN */

#ifdef __cplusplus
}
#endif

#endif  /* QUEUE_TELEMETRY_H */
