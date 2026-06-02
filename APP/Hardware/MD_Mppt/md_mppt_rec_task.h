#ifndef MD_MPPT_REC_TASK_H
#define MD_MPPT_REC_TASK_H

#include "board_config.h"
#if(boardMPPT_EN)
#include "lwrb.h"

#if(boardUSE_OS)
#include "FreeRTOS.h"
#include "task.h"
#endif  //boardUSE_OS


#if(boardUSE_OS)
extern TaskHandle_t tMpptRecTaskHandle;
#endif  //boardUSE_OS

extern vs16 sMpptMaxTemp;


//*****************************返回参数*************************************
#pragma pack (1)   //强制进行1字节对齐
typedef struct
{
	//MPPT
	u16 				usInVolt;   		//输入电压 0.1V
	u16 				usInCurr;   		//输入电流 0.1A
	u16 				usInPwr;   			//输入电流 1W
	u16 				usInState;  		//输入状态 1:PV输入  0无输入
	u16 				usErrCode;   		//故障状态
}MpptParam_T;  
#pragma pack()   //取消进行1字节对齐

//*********************************输入类型**********************************
typedef enum 
{
    MIT_NULL = 0,
    MIT_DC,       //适配器      
    MIT_PV,       //太阳能
}MpptInType_U;

//*****************************错误状态*************************************
typedef union
{
	struct
	{
		vu16 bInOV:1;		//输入过压
		vu16 bInUV:1;		//输入欠压
		vu16 bInUP:1;		//输入欠功率
		vu16 bOutOV:1;		//输出过压
		vu16 bReserved:1;	//预留
		vu16 bInSC:1;		//输入短路
		vu16 bInOC:1;		//输入过流
		vu16 bOutUV:1;		//输出欠压

		// vu16 bOutOC:1;		//输出过流
		// vu16 bOutSC:1;		//输出短路
		// vu16 bOL:1;			//过载
		// vu16 bOT:1;			//过温
		// vu16 bEnFault:1;	//使能故障
		vu16 :8;
	}tCode;
	vu16 usCode;   
}MpptRecErrCode_U;

//*********************************任务对象**********************************
#pragma pack (1)   //强制进行1字节对齐
typedef struct
{											
	MpptInType_U		uInType;			//输入类型
	MpptRecErrCode_U	uErrCode;          	//错误状态
	u16           		usInVolt;      		//输入电压 0.1V
    u16           		usInCurr;      		//输入电流 0.01A
	u16 				usInPwr;			//输入功率 0.1W
    u16           		usOutVolt;      	//输出电压 0.1V
    u16           		usOutCurr;      	//输出电流 0.01A
	u16 				usOutPwr;			//输入功率 0.1W
	u16			  		usMaxInPwr;			//最大输入功率 0.1W
	s16					sMaxTemp;			//最大温度 摄氏度
}MpptRx_T;              
extern MpptRx_T     	tMpptRx; 
#pragma pack()   //取消进行1字节对齐


bool bMppt_RecTaskInit(void);
void vMppt_RecTickTimer(void);

#if(!boardUSE_OS)
void vMppt_RecTask(void *pvParameters);
#endif  //boardUSE_OS

#endif  //boardMPPT_EN

#endif  //MD_MPPT_REC_TASK_H


