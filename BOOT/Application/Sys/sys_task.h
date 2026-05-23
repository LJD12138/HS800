#ifndef SYS_TASK_H_
#define SYS_TASK_H_

#include "main.h"
#include "board_config.h"
#include "queue_task.h"

//#define		//4Tab									//10Tab
#define			sysLOW_POWER_MODE						0      //0:不进入休眠   1:睡眠模式   2:待机模式
extern 			Task_T									*tpSysTask;

//*******任务 ID  ************************************************
typedef enum
{
	STI_NULL = 0,      				 		//空任务函数
	STI_INIT,								//初始化
	STI_ENTER_APP,							//进入APP
	STI_ERR,								//错误
	STI_RESET,								//重置
	#if(boardUPDATE)
    STI_UPDATE,           					//升级
	#endif
	#if(boardDISPLAY_EN)
	STI_DISPLAY,							//显示
	#endif
	#if(boardLOW_POWER)
	STI_LOW_POWER,							//低功耗
	#endif
}SysTaskId_E;


//设备初始化标志位
typedef union
{
	struct
	{
		u16 bIF_SysTask:1;
		u16 bIF_BootInfo:1;
		u16 bIF_AdcTask:1;
		u16 bIF_Gpio:1;
		
		u16 bIF_AT24Cxx:1;
		u16 bIF_SysInit:1;
		u16 bIF_AfeTask:1;
		u16 bIF_MpptTask:1;
		
		u16 bIF_BmsTask:1;
		u16 bIF_BmsProt:1;
		u16 bIF_Print:1;
	}tFinish;
	u16 State;
}InitFinish_U;

//系统供电类型
typedef enum
{
	SPT_5V = 0,
	SPT_BMS,
	SPT_10V,
}SysPowerType_E;

//错误
typedef enum 
{
    SEC_CLEAR_ALL = 0,		//清所有错误
	
	SEC_PARA_LOST,			//并机丢失
	SEC_CONSOLE_LOST,		//面板丢失
	SEC_DISCHG_MOS_ERR,		//放电MOS错误
    SEC_CHG_MOS_ERR,		//充电MOS错误

	SEC_MOS_ERR1,			//MOS错误1
	SEC_MOS_ERR2,			//MOS错误2
	SEC_MOS_ERR3,			//MOS错误3
}SysErrCode_E;

typedef union
{
	struct 
	{
		uint16_t bParaLost			:1;
		uint16_t bConsoleLost		:1;
		uint16_t bDisChgMosErr		:1;
		uint16_t bChgMosErr			:1;

		uint16_t bMosErr1			:1;
		uint16_t bMosErr2			:1;
		uint16_t bMosErr3			:1;
	}tCode;
	uint16_t usCode;
}SysErrCode_N;

#pragma pack(1)//强制一个字节对齐
typedef struct
{
	//1Tab				//5Tab				//5Tab
    vu16             	usAutoOffCnt;       //自动关闭计时
	vu16             	usAutoOffTime;      //自动关闭时间  0为关闭此功能
    vs16             	sMaxTemp;           //整机最高温 1摄氏度
	vs16             	sMinTemp;           //整机最低温 1摄氏度
	vs16             	sBoardTempMax;      //板载最高温 (主控)
	vu16				usVoltMax;			//0.01V
	vu16				usVoltMin;			//0.01V
	vu16             	usOutputPwr;        //输出功率
	vu16             	usInputPwr;         //输入功率
	vu16             	usNeedSleepCnt;     //需要休眠计时
	InitFinish_U     	uInit;              //初始化完成
	SysPowerType_E   	ePowerType;         //系统供电类型
	SysErrCode_N  		uErrCode;        	//错误代码
	DevState_E    		eDevState;          //设备状态
}SysInfo_T;         
#pragma pack() //取消一个字节对齐
extern  SysInfo_T    	tSysInfo; 
//		//2Tab			//4Tab
						

void vSys_TaskInit(void);
bool bSys_SetDevState(DevState_E step, bool bz);
void vSys_TickTimer(void);

#if(!boardUSE_OS)
void vSys_Task(void *pvParameters);
#endif

#if(boardLOW_POWER)
bool bSys_LowPowerExist(void);
void vSys_PowerTypeSelect(void);
#endif  //boardLOW_POWER

#endif
