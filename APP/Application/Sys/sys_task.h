#ifndef SYS_TASK_H_
#define SYS_TASK_H_

#include "board_config.h"
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS


//充电功率等级(总的,PV+AC)
#define  		sysCHG_PWR_LEVEL1              		100	//0.2C * 40 * 28V
#define  		sysCHG_PWR_LEVEL2              		200 //0.5C * 40 * 28V
//0-100;1-110;2-120;3-220;4-230;5-240
#if(boardDCAC_VOLT_TYPE==0)	//110V
#define  		sysCHG_PWR_LEVEL3              		1500	
#elif(boardDCAC_VOLT_TYPE==3) //230V
#define  		sysCHG_PWR_LEVEL3              		600
#else
#error "DCAC类型定义有误"
#endif
#define  		sysCHG_PWR_LEVEL4              		3000


extern bool G_TestMode;
extern Task_T *tpSysTask;
extern TaskHandle_t tSysTaskHandler;


//*********************************任务ID***********************************
typedef enum
{
	STI_NULL = 0,		//空任务函数
    STI_INIT,			//初始化电池包
	STI_CLOSING,		//关闭中
	STI_SHUT_DOWN,		//关闭完成
	STI_ERR,			//错误
	STI_RESET,			//重置
	STI_BOOTING,		//载入中
	STI_WORK,			//工作中
	STI_ENG,			//工程模式
	STI_UPDATE,			//升级
}SysTaskId_E;

//*****************************设备初始化标志位********************************
typedef union
{
	struct
	{
		u16 			bIF_SysTask:1;
		u16 			bIF_AppInfo:1;
		u16 			bIF_AdcTask:1;
		u16 			bIF_Gpio:1;
		
		u16 			bIF_Print:1;
		u16 			bIF_SysInit:1;
		u16 			bIF_MpptTask:1;
		u16 			bIF_MpptProc:1;
		
		u16 			bIF_BmsTask:1;
		u16 			bIF_BmsProt:1;
		u16 			bIF_DcacTask:1;
		u16 			bIF_DcacProt:1;
		
		u16 			bIF_UsbTask:1;
		u16 			bIF_DcTask:1;
		u16 			bIF_DispTask:1;
	}tFinish;
	u16 State;
}InitFinish_U;

//*****************************系统供电类型**********************************
typedef enum
{
	SPT_5V = 0,
	SPT_BMS,
	SPT_10V,
}SysPowerType_E;

//*****************************错误状态*************************************
typedef enum 
{
    SEC_CLEAR_ALL = 0,	//清所有错误
						
	SEC_OT,				//过温
	SEC_UT,				//低温
	SEC_OV,				//过压
	SEC_UV,				//欠压
	SEC_OL,				//过载
	SEC_0_SOC,			//0%SOC
	SEC_CLOSE_FAULT,	//关闭失败
	SEC_BOOT_FAULT,		//开启失败
}SysErrCode_E;

typedef union
{
	struct 
	{
		u16 			bOT :1;
		u16 			bUT :1;
		u16 			bOV :1;
		u16 			bUV :1;
		u16 			bOL :1;
		u16 			b0SOC :1;
		u16 			bCloseFault :1;
		u16 			bBootFault :1;
	}tCode;
	u16 usCode;
}SysErrCode_U;

//*********************************许可*************************************
typedef union
{
	struct 
	{
		u8 				bChgPerm:1;//充电许可
		u8 				bDisChgPerm:1;//放电许可
		u8 				bForceClose:1;//强制关闭
		u8 				temp:5;
	}tPerm;
	u8 ucPerm;
}SysPerm_U;

typedef enum
{
	SPO_CHG = 0,		//充电
	SPO_DISCHG,    		//放电
	SPO_FORCE_CLOSE,	//强制关闭
	SPO_ALL,			//充放电
}SysPermObject_E;

//*********************************充电功率**********************************
#pragma pack(1)
typedef struct
{
	vu16              	usMPPT;
	vu16              	usDCAC;
}SysChgPwr_T;  
#pragma pack()

//*********************************任务对象**********************************
#pragma pack(1)//强制一个字节对齐
typedef struct
{
	//1Tab				//5Tab				//5Tab
	DevState_E    		eDevState;          //设备状态
	SysErrCode_U  		uErrCode;        	//错误代码
	SysPerm_U           uPerm;				//许可
	InitFinish_U     	uInit;              //初始化完成
	SysPowerType_E   	ePowerType;         //系统供电类型
	SysChgPwr_T			tSetChgPwr;			//充电功率
    vu16             	usAutoOffCnt;       //自动关闭计时
	vu16             	usAutoOffTime;      //自动关闭时间  0为关闭此功能
	vu16             	usNeedSleepCnt;     //需要休眠计时
    vs16             	sMaxTemp;           //整机最高温 1摄氏度
	vs16             	sMinTemp;           //整机最低温 1摄氏度
	vs16             	sBoardTempMax;      //板载最高温 (主控)
	vu16				usVoltMax;			//0.01V
	vu16				usVoltMin;			//0.01V
	vu16             	usOutPwr;        	//输出功率
	vu16             	usInPwr;         	//输入功率
}SysInfo_T;         
#pragma pack() //取消一个字节对齐
extern  SysInfo_T    	tSysInfo; 
//		//2Tab			//4Tab

//*********************************记忆参数**********************************
#pragma pack(1)//强制一个字节对齐
typedef struct
{
	bool 				bBuzSwitchOff;		//关闭蜂鸣器
	s8               	sMaxTemp;      		//允许的最大温度
	s8               	sMinTemp;      		//允许的最小温度
	vu16             	usAutoOffTime;      //自动关闭时间  0为关闭此功能
	vu16             	usMinOpenVolt;      //最小开启电压
}SysMemParam_T;
#pragma pack() //取消一个字节对齐


void vSys_TaskInit(void);
void vSys_RefreshAllOffTime(bool BLON);
void vSys_RefreshOffTime(void);
void vSys_TickTimer(void);
bool bSys_SetAutoOffTime(u16 time);
bool bSys_SetDevState(DevState_E state, bool bz);
bool bSys_IsWorkState(void);
bool bSys_IsShutDownState(void);
bool bSys_CheckActState(void);
bool bSys_ExistInVolt(void);
s8 cSys_IsChgState(void);
bool bSys_ChgWakeUp(SwitchObject_E obj);
s8 cSys_Switch(SwitchObject_E obj, SwitchType_E type, bool fore_en);
bool bSys_MemParamInit(SysMemParam_T* p_sys_mem);
void vSys_MemParamSet(u8 item, bool add);
bool bSys_SetPerm(SysPermObject_E obj, bool en);
bool bSys_SetErrCode(SysErrCode_E code, bool set);
bool bSys_LowVoltReqChg(void);
s16 sSys_CheckInVolt(void);

#if(!boardUSE_OS)
void vSys_Task(void *pvParameters);
#endif  //boardUSE_OS

#endif  //SYS_TASK_H_
