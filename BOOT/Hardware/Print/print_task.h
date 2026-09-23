/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Print
 * File    : print_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统调试输出与上位机通信任务头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef PRINT_TASK_H_
#define PRINT_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"
#include "queue_task.h"
#include "Print/print_api.h"

#if (boardPRINT_IFACE)
#include "Baiku/baiku_proto.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			printTASK_CYCLE_TIME					10

#define			printCONSOLE_MASTER_ADDR				0xEF	/* 用户地址 */
#define			printCONSOLE_SLAVE_ADDR					0xEE	/* 用户地址 */

//****************************************************Types*********************************************************************//
/* 任务ID枚举 */
typedef enum
{										
	PTI_NULL = 0,		/* 空任务函数 */
	PTI_MAIN,			/* 主任务 */
	PTI_REPLY_APP_INFO,	/* 回复信息 */
	PTI_REPLY_CALI,		/* 回复校准 */
	PTI_UPDATE,			/* 更新任务 */
}PrintTaskId_E;
#endif  /* boardPRINT_IFACE */

/* 调试输出使能标志联合体 */
typedef union 
{
	struct
	{
		/* 0 */
		u32				bSysTask:1;
		u32				bBootInfo:1;
		u32				bOperFlash:1;
		u32				bImportant:1;
		
		u32				bAfeTask:1;
		u32				bDispTask:1;
		u32				bKeyTask:1;
		u32				bAdcTask:1;
		
		/* 8 */
		u32				bBmsTask:1;
		u32				bBmsRecTask:1;
		u32				bConsoleTask:1;
		u32				bConsoleRecTask:1;
		
		u32				bMpptTask:1;
		u32				bMpptRecTask:1;
		u32				bParaTask:1;
		u32				bParaRecTask:1;
		
		/* 16 */
		u32				bDcTask:1;
		u32				bUsbTask:1;
		u32				bUpdate:1;
		u32				bXmodem:1;
		
		u32				bBaiKuProto:1;
		u32				bW25Q128:1;
		u32				bHeatManage:1;
		u32				bFreeRTOS:1;
		
		u32				bExRtc:1;
	}tFlag;
	u32					ulFlag;
}DebugPrint_U;

//****************************************************Globals*******************************************************************//
extern DebugPrint_U     uPrint; 

#if (boardPRINT_IFACE)
extern Task_T *tpPrintTask;
extern lwrb_t tPrintTxBuff;

#if (boardUSE_OS)
extern TaskHandle_t     tPrintTaskHandler;
#endif  /* boardUSE_OS */

//****************************************************Extern********************************************************************//
s8   cPrint_TaskInit(void);
bool bPrint_SendDataToUsart(void);
void vPrint_RecTickTimer(void);

#if (!boardUSE_OS)
void vPrint_Task(void *pvParameters);
#endif  /* !boardUSE_OS */

#endif  /* boardPRINT_IFACE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PRINT_TASK_H_ */

