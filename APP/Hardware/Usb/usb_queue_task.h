/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_queue_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB 任务队列管理模块头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef USB_QUEUE_TASK_H_
#define USB_QUEUE_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardUSB_EN)
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			usbTASK_CYCLE_TIME						100		/* 任务时间 (ms) */

//****************************************************Globals*******************************************************************//
extern s32 us_usb_total_out_pwr;
extern s16 s_max_temp;

//****************************************************Extern********************************************************************//
bool bUsb_QueueInit(void);

/* 队列任务函数 */
void v_usb_queue_task_init(Task_T *p_task);
void v_usb_queue_task_closing(Task_T *p_task);
void v_usb_queue_task_booting(Task_T *p_task);
void v_usb_queue_task_err(Task_T *p_task);
void v_usb_queue_task_work(Task_T *p_task);

#endif  /* boardUSB_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* USB_QUEUE_TASK_H_ */
