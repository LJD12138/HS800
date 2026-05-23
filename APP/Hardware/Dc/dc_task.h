#ifndef DC_TASK_H_
#define DC_TASK_H_

#include "board_config.h"

#if(boardDC_EN)

//*****************************错误状态*************************************
typedef enum 
{ 
	DC_EC_CLEAR_ALL = 0,
    DC_EC_PWR_ERR,
	DC_EC_OT,
	DC_EC_OL,
	DC_EC_CLOSE_FAIL,
	DC_EC_OUT_LOW,
	DC_EC_OUT_HIGH,
	DC_EC_NTC_LOST,
}DcErrCode_E;

typedef union
{
	struct
	{
		u8 bPowerErr:1;     	//电源错误
		u8 bOT:1;      			//过温
		u8 bOL:1;      			//过载
		u8 bCloseFail:1;     	//关闭失败
		u8 bOutLow:1;			//输出低
		u8 bOutHigh:1;			//输出高
		u8 bNtcLost:1;			//NTC丢失
	}tCode;
	u8 ucErrCode;
}DcErrCode_U;

//*********************************任务对象**********************************
#pragma pack(1)//强制一个字节对齐
typedef struct
{
	DevState_E    		eDevState;     		//设备状态
    DcErrCode_U    		uErrCode;      		//DC任务错误状态
    vu16				usInVolt;     		//0.1V
	vu16    			usInCurr;			//0.1A
	vu16				usOutVolt;     		//0.1V
	vu16    			usOutCurr;			//0.1A
	vu16    			usOutPwr;     		//W
	vu16             	usAutoOffTime; 		//自动关机时间
	vu16             	usAutoOffCnt;  		//自动关机计时
	s16             	sMaxTemp;
}Dc_T;              
#pragma pack() //取消一个字节对齐
extern Dc_T   			tDc;

//*********************************记忆参数**********************************
#pragma pack(1)//强制一个字节对齐
typedef struct
{
	vu16             	usAutoOffTime;  	//自动关闭时间  0为关闭此功能
	vu16             	usMaxOutVolt;		//最大输入电压
	vu16             	usMinOutVolt;		//最小输入电压
	vu16             	usOverLoadPwr;      //过载功率
	vu16             	usMinOpenVolt;      //最小开启电压
	s8               	sMaxTemp;      		//允许的最大温度
}DcMemParam_T;
#pragma pack() //取消一个字节对齐


void vDc_TaskInit(void);
s8 cDc_Switch(SwitchType_E Tri_Type, bool fore_en);
void vDc_TickTimer(void);
void vDc_RefreshOffTime(void);
bool bDc_MemParamInit(DcMemParam_T* p_dc_mem);
void vDc_MemParamSet(u8 item, bool add);

#if(!boardUSE_OS)
void vDc_Task(void *pvParameters)
#endif  //boardUSE_OS

#if(boardLOW_POWER)
void vDc_EnterLowPower(void);
void vDc_ExitLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardDC_EN

#endif  //DC_TASK_H_
