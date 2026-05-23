#ifndef MD_DCAC_REC_TASK_H
#define MD_DCAC_REC_TASK_H

#include "board_config.h"

#if(boardDCAC_EN)
#include "lwrb.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

#define       	dcacREC_TASK_CYCLE_TIME               	30

#if(boardUSE_OS)
extern       	TaskHandle_t                          	tDcacRecTaskHandle;
#endif  //boardUSE_OS

//*****************************返回参数*************************************
#pragma pack (1)   //强制进行1字节对齐
typedef struct
{
	vu16    			usOutVolt;			//0.1V       
	vs16    			sOutCurr;    		//G3604 0.1A     G2404 0.01A
	vs16    			usOutPwr;     		//W
	vu16    			usOutFreq;     		//0.01HZ
	vu16    			usState;        	//状态
	vu16				usOtherState;		//其他状态
	vu16    			temp1;
	vs16     			sTemp1;          	//0.1℃ 逆变温度
	vu16    			temp2;
	vs16     			sTemp2;          	//0.1℃ 温升温度
	vs16     			sTemp3;          	//0.1℃ 变压器温度
	vu16    			usFan;				//风扇
}DCAC_Param1_t;  
#pragma pack()   //取消进行1字节对齐

#pragma pack (1)   //强制进行1字节对齐
typedef struct
{
	vu16				uDcErrCode;			//后端错误
	vu16				uAcErrCode;			//前端错误
	vu16				uInErrCode;			//电网错误
	vu16				usSysErr;			//系统故障表
}DCAC_Param2_t;  
#pragma pack()   //取消进行1字节对齐

#pragma pack (1)   //强制进行1字节对齐
typedef struct
{
	vs16    			sBatInPwr;     		//W
	vu16    			temp1;
	vu16    			usAcInVolt;			//0.1V
	vs16    			sAcInCurr;			//G3604 0.1A     G2404 0.01A
	vs16    			sAcInPwr;			//W
	vs16    			sAcChgPwr;			//W
}DCAC_Param3_t;  
#pragma pack()   //取消进行1字节对齐

//*****************************工作状态*************************************
typedef union
{
	struct
	{								
		vu16 bInit:1;			//初始化中
		vu16 bDcMode:1;			//适配器模式
		vu16 bPvMode:1;			//太阳能模式
		vu16 bPvOff:1;			//PV关闭
		vu16 bPvErr:1;			//PV错误
		vu16 :3;
		
		vu16 bAcDisChg:1;		//初始化中
		vu16 bAcChg:1;			//适配器模式
		vu16 bAcUpf:1;			//太阳能模式
		vu16 bAcErr:1;			//太阳能模式
		vu16 :4;
	}tState;
	vu16 usState;   
}DcacWorkState_U;

//*****************************错误状态*************************************
typedef union
{
	struct
	{
		struct
		{
			vu16 bAc:1;  		//AC侧返回故障
			vu16 bOC:1;			//
			vu16 bOT:1;			//
			vu16 bOV:1;			//
			vu16 bUV:1;			//
			vu16 bNtcErr:1;		//NTC故障
			vu16 bMsgErr:1;		//内部通讯故障
			vu16 bLost:1;		//主控通讯丢失
			
			vu16 bBootErr:1;	//软启失败
			vu16 :7;
		}tDc;
		
		struct
		{
			vu16 bBusOV:1;  	//BUS过压
			vu16 bBusUV:1;		//
			vu16 bOV:1;			//逆变过压
			vu16 bUV:1;			//
			vu16 bOL:1;			//输出过载
			vu16 bSC:1;			//逆变短路
			vu16 bBootErr:1;	//软启失败
			vu16 bOC:1;			//过流
			
			vu16 bOT:1;			//逆变散热器过温
			vu16 bMsgErr:1;		//通讯故障
			vu16 bSysErr:1;		//系统故障
			vu16 :5;
		}tAc;
		
		struct
		{
			vu16 bOV1:1;		//瞬时值过压
			vu16 bUV1:1;		//瞬时值欠压
			vu16 bOV2:1;		//有效值过压
			vu16 bUV2:1;		//有效值欠压
			vu16 bOF1:1;		//频率过频
			vu16 bUF1:1;		//频率欠频
			vu16 bLost:1;		//电网缺失
			vu16 :9;
		}tIn;
		
		vu16 usSysErr;			//系统故障表
		
	}tCode;
	
	vu16 usCode[4];   
}DcacErrState_U;

//*********************************任务对象**********************************
typedef struct
{
	//输入
	vu16				usInVolt;     		//0.1V
	vu16    			usInCurr;			//0.1A
	vu16    			usInPwr;			//W
	vu16    			usInChgPwr;			//W
	vu16    			usInFreq;     		//0.1HZ
	//输出
	vu16    			usOutVolt;    		//0.1V
	vu16    			usOutCurr;    		//0.1A
	vu16    			usOutPwr;     		//W
	vu16    			usOutFreq;    		//0.1HZ
	//并网
	vu16    			usParaInMaxPwr;     //并网最大功率 W
	vu16    			usParaInPwr;        //并网功率 W
	vu16    			usParaInMode;       //并网模式
	//状态
	vs16    			usChgPwr;     		//充电功率 W
	vu16    			usMaxInPwr;        	//最大输入功率 W
	vs16             	sMaxTemp;           //1摄氏度
	vs16             	sMinTemp;           //1摄氏度
	DcacWorkState_U    	uState;        		//状态
	DcacErrState_U		uErrCode;			//错误代码
}DcacRx_T;
extern DcacRx_T   		tDcacRx; 

bool bDcac_RecTaskInit(void);
void vDcac_RecTickTimer(void);

#if(!boardUSE_OS)
void vDcac_RecTask(void *pvParameters);
#endif  //boardUSE_OS

#if(boardLOW_POWER)
void vDcac_EnterLowPower(void);
void vDcac_ExitLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardDCAC_EN

#endif  //MD_DCAC_PROCESS_H


