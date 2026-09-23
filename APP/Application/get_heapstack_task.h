/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : get_heapstack_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统健康巡检与 FreeRTOS 堆栈水位监控头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef GET_HEAPSTACK_TASK_H_
#define GET_HEAPSTACK_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardUSE_OS && boardHEALTH_MONITOR_EN)
#include "freertos.h"
#include "task.h"

//****************************************************Globals*******************************************************************//
/* 空闲循环计数器: vApplicationIdleHook 每次空闲循环递增,
 * 健康巡检任务周期采样换算 CPU 占用率(与 RUN_TIME_STATS 交叉验证) */
extern volatile uint32_t ulHealthIdleLoopCnt;

//****************************************************Extern********************************************************************//
s8   cHealth_TaskInit(void);
#endif  //boardUSE_OS && boardHEALTH_MONITOR_EN

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  /* GET_HEAPSTACK_TASK_H_ */
