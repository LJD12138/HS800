#ifndef USB_TASK_H_
#define USB_TASK_H_

#include "board_config.h"

#if(boardUSB_EN)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

extern Task_T *tpUsbTask;
extern TaskHandle_t tUsbTaskHandler;

//*********************************任务ID***********************************
typedef enum
{
	UTI_NULL = 0,       //空任务函数
    UTI_INIT,           //初始化电池包
	UTI_CLOSING,		//关闭中
	UTI_SHUT_DOWN,		//关闭完成
	UTI_ERR,			//错误
	UTI_BOOTING,		//载入中
	UTI_WORK,			//工作中
}UsbTaskId_E;

//*****************************错误状态*************************************
typedef enum 
{  
	UEC_CLEAR_ALL = 0,
    UEC_POWER_ERR,
    UEC_OT,
	UEC_OL,
	UEC_BAT_VOLT_LOW,
	UEC_IC1_LOST,
	UEC_IC2_LOST,
	UEC_COLSE_FAULT,	//关闭失败
	UEC_BOOT_FAULT,		//开启失败
}UsbErrCode_E;

typedef union
{
	struct
	{
		u8 				bPowerErr:1;		//电源错误
		u8 				bOT:1;				//过温
		u8 				bOL:1;				//过载
		u8 				bBatUV:1;			//电池电压低
		u8 				bIc1Lost:1;			//丢失
		u8 				bIc2Lost:1;			//丢失
		u16 			bCloseFault :1;
		u16 			bBootFault :1;
	}tCode;
	u8 ucErrCode;
}UsbErrCode_U;

//*********************************任务对象**********************************
#pragma pack(1)
typedef struct
{
	DevState_E 			eDevState;     		//设备状态
	UsbErrCode_U  		uErrCode;      		//错误代码
	vu16           		usAutoOffTime; 		//自动关机时间
	vu16           		usAutoOffCnt;  		//自动关机计时
	vu16				usInVolt;     		//0.1V
	vu16    			usInCurr;			//0.1A
	vu16    			usOutPwr;     		//W
	vu16    			usWcPwr;     		//W
	vu16    			usPdPwr;     		//W
	vs16             	sMaxTemp;			//1摄氏度
}Usb_T;   
#pragma pack()
extern Usb_T			tUsb; 

//*********************************记忆参数**********************************
#pragma pack(1)//强制一个字节对齐
typedef struct
{
	vu16             	usAutoOffTime;   	//自动关闭时间  0为关闭此功能
	vu16             	usMaxInVolt;		//最大输入电压
	vu16             	usMinInVolt;		//最小输入电压
	vu16             	usMinOpenVolt;      //最小开启电压
	s8               	sMaxTemp;      		//允许的最大温度
}UsbMemParam_T;
#pragma pack() //取消一个字节对齐


bool bUsb_TaskInit(void);
void vUsb_Init(void);
s8 cUsb_Switch(SwitchType_E Tri_Type, bool fore_en);
void bUsb_SetDevState(DevState_E stat);
void bUsb_SetErrCode(UsbErrCode_E code, bool set);
void vUsb_RefreshOffTime(void);
void vUsb_TickTimer(void);
bool bUsb_MemParamInit(UsbMemParam_T* p_usb_mem);
void vUsb_MemParamSet(u8 item, bool add);
s8 cUsb_CheckInVolt(void);
s8 cUsb_CheckBatVolt(void);

#if(!boardUSE_OS)
void vUsb_Task(void *pvParameters);
#endif

#if(boardLOW_POWER)
void vUsb_EnterLowPower(void);
void vUsb_ExitLowPower(void);
#endif

#endif  //boardUSB_EN

#endif  //USB_TASK_H_
