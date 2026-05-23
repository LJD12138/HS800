#ifndef MD_DCAC_TASK_H
#define MD_DCAC_TASK_H

#include "board_config.h"

#if(boardDCAC_EN)
#include "queue_task.h"

#if(boardUSE_OS)
#include "FreeRTOS.h"
#include "task.h"
#endif  //boardUSE_OS

#define       	dcacTASK_CYCLE_TIME               		500

extern  		Task_T									*tpDcacTask;
#if(boardUSE_OS)
extern  		TaskHandle_t                          	tDcacTaskHandler;
#endif  //boardUSE_OS

//*******任务 ID  ************************************************
typedef enum
{
	DTI_NULL = 0,      				//空任务函数
    DTI_INIT,          				//初始化逆变
    DTI_MAIN,      					//循环获取逆变的数据
    DTI_CTRL_DCAC_OUT,				//控制交流输出
    DTI_CTRL_DCAC_IN,				//控制交流输入
	DTI_CTRL_PARA_IN,   			//控制并网放电
	DTI_ERR_PROC,   				//错误处理
	DTI_UPDATE,   					//升级
}DcacTaskId_E;



//错误标志
typedef enum 
{
    DEC_CLEAR_ALL = 0,				//清所有错误	

	DEC_DCAC_IN_VOLT = 1,			//逆变器报错
	DEC_DCAC_IN_FREQ,				//逆变器报错
	DEC_DCAC_IN_OTHER,				//逆变器报错
	DEC_DCAC_OUT_VOLT,				//逆变器报错
	DEC_DCAC_OUT_OTHER,				//逆变器报错
	DEC_DCAC_HIGH_VOLT,				//逆变器报错
	DEC_DCAC_BAT_OV,				//逆变器报错
	DEC_DCAC_BAT_UV,				//逆变器报错
	
	DEC_DCAC_OT,					//逆变器报错
	DEC_DCAC_OC,					//逆变器报错
	DEC_DCAC_OL,					//逆变器报错
	DEC_DCAC_SC,					//逆变器报错
	DEC_DCAC_FUSE,					//逆变器报错
	DEC_DCAC_RELAY,					//逆变器报错
	DEC_DCAC_PARA,					//逆变器报错
	DEC_DCAC_NTC,					//逆变器报错
	
	DEC_DCAC_OTHER = 17,			//逆变器报错
	DEC_DCAC_EEPROM,				//逆变器报错
	DEC_DCAC_2,						//逆变器报错
	DEC_DCAC_3,						//逆变器报错
	
	DEC_SYS_DEV_LOST = 21,			//设备丢失
	DEC_SYS_OT,						//过温
	DEC_SYS_UT,						//低温
	DEC_SYS_OV,						//过压
	DEC_SYS_UV,                		//欠压
	DEC_SYS_SET_IN_PROTE,			//输入保护
	DEC_SYS_OUT_OL,         		//系统过载
	DEC_SYS_OUT_ERR,              	//输出错误
	
	DEC_SYS_IN_OC = 29,    			//过流
}DCAC_ErrCode_E;


//错误状态集合
typedef union
{
	struct
	{
		//逆变器上报错误(开始)
		vu32 bDcacInVolt:1;
		vu32 bDcacInFreq:1;
		vu32 bDcacInOther:1;
		vu32 bDcacOutVolt:1;
		vu32 bDcacOutOther:1;
		vu32 bDcacHighVolt:1;
		vu32 bDcacBatOV:1;
		vu32 bDcacBatUV:1;
		
		vu32 bDcacOT:1;
		vu32 bDcacOC:1;
		vu32 bDcacOL:1;
		vu32 bDcacSC:1;
		vu32 bDcacFuse:1;
		vu32 bDcacRelay:1;
		vu32 bDcacPara:1;
		vu32 bDcacNtc:1;
		
		vu32 bDcacOther:1;
		vu32 bDcacEeprom:1;
		vu32 bDcactemp:2;
		//逆变器上报错误(结束)
		
		//系统判断的错误
		vu32 bSysDevLost:1;
		vu32 bSysOT:1;
		vu32 bSysUT:1;
		vu32 bSysOV:1;
		vu32 bSysLV:1;
		vu32 bSysSetInProte:1;
		vu32 bSysOutOL:1;
		vu32 bSysOutErr:1;
		
		vu32 bSysInOC:1;
		vu32 bSystemp:3;
	}tCode;
	vu32 ulCode;   
}DCAC_ErrState_U;

//开关的对象
typedef enum
{
	DSO_AC_OUT=0,  		//输出
	DSO_AC_IN,    		//充电
	DSO_PARA_IN,    	//并网
	DSO_OFF_ALL,     	//关闭所有
}DACD_SwitchObject_E;

//*********************************许可*************************************
typedef union
{
	struct 
	{
		u8 				bChgPerm:1;			//充电许可
		u8 				bDisChgPerm:1;		//放电许可
		u8				bParaInPerm:1;		//并网
		u8 				bForceClose:1;		//强制关闭
		u8 				temp:4;
	}tPerm;
	u8 ucPerm;
}DcacPerm_U;

typedef enum
{
	DPO_CHG = 0,		//充电
	DPO_DISCHG,    		//放电
	DPO_PARA_IN,    	//并网
	DPO_ALL,			//
}DcacPermObject_E;

//*********************************任务对象**********************************
#pragma pack(1)
typedef struct
{
	DevState_E  		eDevState;         	//设备状态
	DCAC_ErrState_U 	uErrCode;           //错误状态
	InOutState_E		eChgState;			//充电状态
	InOutState_E		eDisChgState;		//放电状态
	InOutState_E		eParanInState;		//并网状态
	DcacPerm_U			uPerm;				//许可
	vu16                usAutoOffCnt;    	//时间戳大于这个值就关闭逆变器
	vu16                usAutoOffTime;   	//关闭逆变器的时间,0为不开启  
	s16                 sMaxTemp;           //任务最高温度
}
Dcac_T;
#pragma pack()
extern Dcac_T 			tDcac;

#pragma pack(1)//强制一个字节对齐
typedef struct
{
	vu16             	usAutoOffTime;  	//自动关闭时间  0为关闭此功能
	vu16             	usMinOpenVolt;      //最小开启电压
	vu16             	usVoltRating;       //额定电压
	vu16             	usMaxInVolt;		//最大输入电压
	vu16             	usMinInVolt;		//最小输入电压
	vu16             	usInPwrRating;		//输入额定功率
	vu16             	usMinInPwr;			//最小输入功率
	vu16             	usMaxInCurr;		//最大输入电流 0.1A
	vu16             	usOutPwrRating;		//输出额定功率
	vu16             	usOverLoadPwr;      //过载功率
	vu16             	usParaInPwr;      	//并网功率
	vu16             	usAcOutFreq;      	//逆变输出频率
	s8               	sMaxTemp;      		//允许的最大温度
}DcacMemParam_T;
#pragma pack() //取消一个字节对齐

bool bDcac_TaskInit(void);
s8 cDCAC_Switch(DACD_SwitchObject_E obj, SwitchType_E sw,bool buz_en);
bool bDcac_SetAcState(OperaObject_E obj, InOutState_E state);
bool bDcac_SetDevState(DevState_E state);
bool bDcac_SetErrCode(DCAC_ErrCode_E code, bool set);
void vDcac_TickTimer(void);
void vDcac_RefreshOffTime(void);
bool bDcac_SetAutoOffTime(u16 time);
bool bDcac_InProteFuncSwitch(bool sw);
bool bDcac_GetOverLoadState(void);
bool bDcac_SetPerm(DcacPermObject_E obj, bool en);
bool bDcac_MemParamInit(DcacMemParam_T* p_dcac_mem);
void vDcac_MemParamSet(u8 item, bool add);
//bool bDcac_DynAdjustChgCurr(vu16 curr);

#endif  //boardDCAC_EN

#endif  //MD_DCAC_TASK_H


