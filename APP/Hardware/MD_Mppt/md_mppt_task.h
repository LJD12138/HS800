#ifndef MD_MPPT_TASK_H
#define MD_MPPT_TASK_H

#include "board_config.h"
#if(boardMPPT_EN)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS


extern Task_T *tpMpptTask;

#if(boardUSE_OS)
extern TaskHandle_t tMpptTaskHandler;
#endif  //boardUSE_OS


//*********************************任务ID***********************************
typedef enum
{
	MTI_NULL = 0,       	//空任务函数
    MTI_INIT,           	//初始化电池包
    MTI_MAIN,      			//循环获取电池包的数据
    MTI_SET_CHG_PWR,   		//控制MPPT开启
	MTI_ERR_PROCESS,    	//错误处理
}MpptTaskId_E;

//*********************************工作状态***********************************
typedef enum
{ 
	MWM_NULL = 0, 			//清除所有
	MWM_PV,
	MWM_DC,
}MpptWorkMode_E;

//*****************************错误状态*************************************
typedef enum 
{
    MEC_CLEAR_ALL = 0,   	//清所有错误	
	
	MEC_MPPT_IN_OV = 1,		//输入过压
	MEC_MPPT_IN_UV,			//输入欠压
	MEC_MPPT_IN_OC,			//输入过流
	MEC_MPPT_IN_SC,			//输入短路
	MEC_MPPT_OUT_OV,		//输出过压
	MEC_MPPT_OUT_UV,		//输出欠压
	MEC_MPPT_OUT_OC,		//输出过流
	MEC_MPPT_OUT_SC,		//输出短路

	MEC_MPPT_OT = 9,		//过载
	MEC_MPPT_OL,			//过温
	MEC_MPPT_EN_FAULT,		//使能故障
	
	MEC_SYS_DEV_LOST= 17,   //设备丢失
	MEC_SYS_OVER_TEMP,      //过温
	MEC_SYS_LOW_TEMP,       //低温
	MEC_SYS_OVER_VOLT,      //过压
	MEC_SYS_OVER_LOAD,      //过载
	
}MpptErrCode_E;

typedef union
{
	struct
	{
		vu32 			bMpptInOV:1;		//输入过压
		vu32 			bMpptInUV:1;		//输入欠压
		vu32 			bMpptInOC:1;		//输入过流
		vu32 			bMpptInSC:1;		//输入短路
		vu32 			bMpptOutOV:1;		//输出过压
		vu32 			bMpptOutUV:1;		//输出欠压
		vu32 			bMpptOutOC:1;		//输出过流
		vu32 			bMpptOutSC:1;		//输出短路

		vu32 			bMpptOL:1;			//过载
		vu32 			bMpptOT:1;			//过温
		vu32 			bMpptEnFault:1;		//使能故障
		vu32 			:5;

		vu32 			bDevLost:1;
		vu32 			bSysOT:1;
		vu32 			bSysUT:1;
		vu32	 		bSysOV:1;
		vu32 			bSysOL:1;
	}tCode;
	vu32 ulCode;  
}MpptErrState_U;

//*********************************任务对象**********************************
#pragma pack(1)
typedef struct
{
	DevState_E  		eDevState;          //设备状态
    MpptWorkMode_E  	eWorkMode;          //工作模式
	MpptErrState_U 		uErrCode;           //tMppt错误状态
	vu16             	usAutoOffTime;   	//关闭逆变器的时间,0为不开启  
    vu16             	usAutoOffCnt;    	//时间戳大于这个值就关闭逆变器
	vu16             	usInPwr;         	//输入的功率 W
	bool             	bChgPerm;         	//充电许可
	s16             	sMaxTemp;    
}
Mppt_T;
#pragma pack()
extern Mppt_T 			tMppt;

//*********************************记忆参数**********************************
#pragma pack(1)//强制一个字节对齐
typedef struct
{
	s8               	cAllowMaxTemp;      //允许的最高温度 1摄氏度
	vu16             	usAutoOffTime;   	//自动关闭时间  0为关闭此功能
	vu16             	usMaxInVolt;		//最大输入电压 0.1V
	vu16             	usMinInVolt;		//最小输入电压 0.1V
	vu16             	usMaxInCurr;		//最大输入电压 0.01A
	vu16             	usInPwrRating;     	//最大输入功率 0.1W
}MpptMemParam_T;
#pragma pack() //取消一个字节对齐


bool bMppt_TaskInit(void);
bool bMppt_SetDevState(DevState_E state);
bool bMppt_SetErrCode(MpptErrCode_E code, bool set);
s8 cMppt_SetChgPwr(u16 pwr);
bool bMppt_SetChgPerm(bool en);
bool bMppt_MemParamInit(MpptMemParam_T* p_mppt_mem);
void vMppt_MemParamSet(u8 item, bool add);

#if(!boardUSE_OS)
void vMppt_Task(void *pvParameters);
#endif  //boardUSE_OS

#endif  //boardMPPT_EN

#endif  //MD_MPPT_TASK_H


