/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB / 快充供电管理任务头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef USB_TASK_H_
#define USB_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardUSB_EN)
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Types*********************************************************************//
/* USB 队列任务 ID */
typedef enum
{
	UTI_NULL = 0,		/* 空任务函数 */
	UTI_INIT,			/* 初始化任务 */
	UTI_CLOSING,		/* 关闭中任务 */
	UTI_SHUT_DOWN,		/* 关闭完成任务 */
	UTI_ERR,			/* 错误任务 */
	UTI_BOOTING,		/* 载入中任务 */
	UTI_WORK,			/* 工作中任务 */
}UsbTaskId_E;

/* USB 错误代码枚举 */
typedef enum 
{  
	UEC_CLEAR_ALL = 0,
	UEC_POWER_ERR,
	UEC_OT,
	UEC_OL,
	UEC_BAT_VOLT_LOW,
	UEC_IC1_LOST,
	UEC_IC2_LOST,
	UEC_COLSE_FAULT,	/* 关闭失败 */
	UEC_BOOT_FAULT,		/* 开启失败 */
	UEC_QC_POWER_ERR,
}UsbErrCode_E;

/* USB 错误状态位域 */
typedef union
{
	struct
	{
		uint16_t		bPowerErr  : 1;		/* 电源错误 */
		uint16_t		bOT        : 1;		/* 过温 */
		uint16_t		bOL        : 1;		/* 过载 */
		uint16_t		bBatUV     : 1;		/* 电池欠压 */
		uint16_t		bIc1Lost   : 1;		/* IC1丢失 */
		uint16_t		bIc2Lost   : 1;		/* IC2丢失 */
		uint16_t		bCloseFault: 1;		/* 关断故障 */
		uint16_t		bBootFault : 1;		/* 启动故障 */
		uint16_t		bQcPowerErr: 1;		/* QC电源错误 */
	}tCode;
	uint16_t			ucErrCode;
}UsbErrCode_U;

/* USB 任务对象结构体 */
typedef struct
{
	DevState_E			eDevState;			/* 设备状态 */
	UsbErrCode_U		uErrCode;			/* 错误代码 */
	vu16				usAutoOffTime;		/* 自动关机时间 (S) */
	vu16				usAutoOffCnt;		/* 自动关机计时 (S) */
	vu16				usInVolt;			/* 0.1V */
	vu16				usInCurr;			/* 0.1A */
	vu16				usOutPwr;			/* W */
	vs16				sMaxTemp;			/* 摄氏度 */
}Usb_T;

/* USB 记忆参数结构体 */
#pragma pack(1)
typedef struct
{
	vu16				usAutoOffTime;		/* 自动关闭时间 (0为关闭此功能) */
	vu16				usMaxInVolt;		/* 最大输入电压 (0.1V) */
	vu16				usMinInVolt;		/* 最小输入电压 (0.1V) */
	vu16				usMinOpenVolt;		/* 最小开启电压 (0.1V) */
	s8					sMaxTemp;			/* 允许的最大温度 (摄氏度) */
}UsbMemParam_T;
#pragma pack()

//****************************************************Globals*******************************************************************//
extern Task_T       *tpUsbTask;
extern TaskHandle_t tUsbTaskHandler;
extern vu16         usQcPwr;
extern vu16         usWcPwr;
extern Usb_T        tUsb;

//****************************************************Extern********************************************************************//
s8   cUsb_TaskInit(void);
void vUsb_Init(void);
s8   cUsb_Switch(SwitchType_E e_tri_type, bool b_fore_en);
void bUsb_SetDevState(DevState_E e_stat);
void bUsb_SetErrCode(UsbErrCode_E e_code, bool b_set);
void vUsb_RefreshOffTime(void);
void vUsb_TickTimer(void);
bool bUsb_MemParamInit(UsbMemParam_T *p_usb_mem);
void vUsb_MemParamSet(uint8_t uc_item, bool b_add);
s8   cUsb_CheckInVolt(void);
s8   cUsb_CheckBatVolt(void);
s8   cUsb_CheckQcInVolt(void);

#if (!boardUSE_OS)
void vUsb_Task(void *p_v_parameters);
#endif  /* !boardUSE_OS */

#if (boardLOW_POWER)
void vUsb_EnterLowPower(void);
void vUsb_ExitLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardUSB_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* USB_TASK_H_ */
