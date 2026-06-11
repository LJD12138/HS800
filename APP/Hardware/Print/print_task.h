#ifndef PRINT_TASK_H_
#define PRINT_TASK_H_

#include "board_config.h"
#include "queue_task.h"
#include "Print/print_api.h"

#define       	printTASK_CYCLE_TIME                  	10  

#define      	printCONSOLE_MASTER_ADDR         		0xEF  //用户地址
#define      	printCONSOLE_SLAVE_ADDR         		0xEE  //用户地址

#if(boardPRINT_IFACE)
#include "main.h"
#include "Baiku/baiku_proto.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  //boardUSE_OS


extern Task_T *tpPrintTask;
extern lwrb_t tPrintTxBuff;

#if(boardUSE_OS)
extern TaskHandle_t tPrintTaskHandler;
#endif  //boardUSE_OS


//*********************************任务ID***********************************
typedef enum
{										
	PTI_NULL = 0,      	//空任务函数
    PTI_MAIN,      		//主任务
	PTI_REPLY_APP_INFO,	//回复信息
	PTI_REPLY_CALI,		//回复校准
	PTI_UPDATE,			//更新任务
}PrintTaskId_E;
#endif	//boardPRINT_IFACE

//开启Print输出
typedef union 
{
	struct
	{
		//0
		u32 bSysTask:1;
		u32 bAppInfo:1;
		u32 bOperFlash:1;
		u32 bImportant:1;

		u32 :1;
		u32 bDispTask:1;
		u32 bKeyTask:1;
		u32 bAdcTask:1;
		
		//8
		u32 bBmsTask:1;
		u32 bBmsRecTask:1;
		u32 bDcacTask:1;
		u32 bDcacRecTask:1;

		u32 bMpptTask:1;
		u32 bMpptRecTask:1;
		u32 bDcTask:1;
		u32 bUsbTask:1;
		
		//16
		u32 bUpdate:1;
		u32 bXmodem:1;
		u32 bBaiKuProto:1;
		u32 bFreeRTOS:1;
		
		//24
		u32 bExRtc:1;
	}tFlag;
	u32 ulFlag;
}DebugPrint_U;
extern DebugPrint_U     uPrint; 

#if(boardPRINT_IFACE)
bool bPrint_TaskInit(void);
bool bPrint_SendDataToUsart(void);
void vPrint_RecTickTimer(void);

#if(!boardUSE_OS)
void vPrint_Task(void *pvParameters);
#endif  //boardUSE_OS

#endif  //boardPRINT_IFACE

#endif  //PRINT_TASK_H_

