#ifndef TIMER_TASK_H_
#define TIMER_TASK_H_

#include "board_config.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#endif

#if(boardBMS_EN)
#include "MD_Bms/md_bms_iface.h"
#endif  //boardBMS_EN

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_iface.h"
#endif  //boardDCAC_EN

#if(boardBMS_485_IFACE_EN)
extern	TimerHandle_t 	tBmsRxEnTimer; 	//单次定时器
#endif  //boardBMS_485_IFACE_EN

#if(boardMPPT_485_IFACE_EN)
extern TimerHandle_t 	tMpptRxEnTimer; //单次定时器
#endif  //boardMPPT_485_IFACE_EN

#if(boardBMS_EN)
extern TimerHandle_t 	tWakeUpBmsTimer;
#endif  //boardBMS_EN

#if(boardDCAC_485_IFACE_EN)
extern TimerHandle_t 	tDcacRxEnTimer;
#endif  //boardDCAC_485_IFACE_EN

extern TimerHandle_t 	tRepetTimer;   //重复定时器  repetition

void vTimer_TaskInit(void);
#if(boardLOW_POWER)
void vCount_EnterLowPower(void);
void vCount_ExitLowPower(void);
#endif  //boardLOW_POWER

#endif  //TIMER_TASK_H_
