#ifndef MD_BMS_REC_TASK_H
#define MD_BMS_REC_TASK_H

#include "board_config.h"

#if(boardBMS_EN)
#include "lwrb.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

/****************************************************宏定义**************************************************/
#define     	bmsDEV_NUM								6

#if(boardUSE_OS)
extern TaskHandle_t tBmsRecTaskHandle;
#endif  //boardUSE_OS

//*********************************任务参数**********************************

#pragma pack(2)
typedef struct
{						
	vu16				bPermChg:1;			//充电许可   0:不许可充电  非0:许可充电
	vu16				bImpermDisChg:1;	//放电许可   0:许可放电    非0:不许可放电
	vu16 				bTemp:6;			//预留
	vu16				ucSysState:8;		//0:丢失 1:初始化 2:关闭中 3:关闭状态 4:错误状态 5:启动中 6:工作中
}State_T;
#pragma pack()

#pragma pack(2)
typedef union
{
	struct 
	{
		vu16			bCellOV:1;			//单体过压
		vu16			bCellUV:1;			//单体欠压
		vu16			bEnvOT:1;			//环境过温
		vu16			bEnvUT:1;			//环境低温
		vu16			bCOT:1;				//充电过温
		vu16			bCUT:1;				//充电低温
		vu16			bDCOT:1;			//放电过温
		vu16			bDCUT:1;			//放电低温

		vu16			bCOC:1;				//充电过流
		vu16			bDCOC:1;			//放电过流
		vu16			bSC:1;				//短路保护
		vu16			bBatFull:1;			//充满状态
		vu16			bAfeLost:1;			//AFE丢失
		vu16			bCurrErr:1;			//电流异常
		vu16			bPerchgFault:1;		//预存异常
		vu16			bLowVoltOL:1;		//低电压过载报警
	}tCode;
	vu16   usCode;
}ErrCode_U;
#pragma pack()

#pragma pack(2)
typedef struct
{
	vu16				usSOC;				//1%
	vu16				usVolt;				//0.01V
	vs16				sCurr;				//0.01A
	vu16				usCalcCapAH;		//估算容量 0.1AH
	vu16				usCycleCnt;    		//循环次数
	vs16				sMaxTemp;			//主机最高温度 1摄氏度
	vs16				sMinTemp;			//主机最低温度 1摄氏度
	vs16             	sBoardTempMax;      //板载最高温 (主控)
	ErrCode_U           uErrCode;			//错误代码
}DevInfo_T;
#pragma pack()

#pragma pack(1)
typedef struct
{
	vu8					ucOnlineNum;   		//在线设备数   最大6台码
	vu8					ucMasterNum;		//选中的数量
}DevNum_T;
#pragma pack()

#pragma pack (1)   //强制进行1字节对齐
typedef struct
{
	vu16				usSOC;				//总的SOC      1%
	s16					sTotalCurr;			//总的电流     0.01A
	vu16				usChgFullTime;		//总的充满时间 1min
	vu16				usDisChgEmptyTime;	//总的放空时间 1min
	vu16				usPermMaxChgPwr;	//许可的最大充电功率 W
	DevNum_T			tDevNum;   		//电池包数量   最大6台
	State_T				tState;				//主机系统状态
	DevInfo_T           tDevInfo[bmsDEV_NUM];//错误代码     tDevInfo[0]为主机
}BmsParam_T;  
#pragma pack()   //取消进行1字节对齐

//*********************************任务对象**********************************
typedef struct
{
	vu16				usErrCode;
	BmsParam_T     		tParam;
}BmsRx_T;              
extern	BmsRx_T   		tBmsRx;

bool bBms_RecTaskInit(void);
void vBms_RecTickTimer(void);

#if(!boardUSE_OS)
void vBms_RecTask(void *pvParameters);
#endif  //boardUSE_OS

#endif  //boardBMS_EN

#endif  //MD_BMS_REC_TASK_H


