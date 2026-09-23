/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : timer_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 软件定时器任务及各外设超时回调声明头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef TIMER_TASK_H_
#define TIMER_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#endif  //boardUSE_OS

#if (boardBMS_EN)
#include "MD_Bms/md_bms_iface.h"
#endif  //boardBMS_EN

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_iface.h"
#endif  //boardDCAC_EN

//****************************************************Globals*******************************************************************//
#if (boardBMS_485_IFACE_EN)
extern TimerHandle_t    tBmsRxEnTimer;      //单次定时器,BMS的485发送延时切换
#endif  //boardBMS_485_IFACE_EN

#if (boardMPPT_485_IFACE_EN)
extern TimerHandle_t    tMpptRxEnTimer;     //单次定时器,MPPT的485发送延时切换
#endif  //boardMPPT_485_IFACE_EN

#if (boardBMS_EN)
extern TimerHandle_t    tWakeUpBmsTimer;    //唤醒BMS定时器
#endif  //boardBMS_EN

#if (boardDCAC_485_IFACE_EN)
extern TimerHandle_t    tDcacRxEnTimer;     //单次定时器,DCAC的485发送延时切换
#endif  //boardDCAC_485_IFACE_EN

//****************************************************Extern********************************************************************//
s8   cTimer_TaskInit(void);

#if (boardLOW_POWER)
void vCount_EnterLowPower(void);
void vCount_ExitLowPower(void);
#endif  //boardLOW_POWER

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  /* TIMER_TASK_H_ */
