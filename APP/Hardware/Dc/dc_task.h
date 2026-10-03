/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Dc
 * File    : dc_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : DC 12V 供电控制与保护处理任务头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef DC_TASK_H_
#define DC_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDC_EN)
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Types*********************************************************************//
/* DC 队列任务 ID (前缀 DCTI_, 与 DCAC 的 DTI_ 区分避免枚举成员重定义) */
typedef enum
{
	DCTI_NULL = 0,		/* 空任务函数 */
	DCTI_INIT,			/* 初始化任务 */
	DCTI_CLOSING,		/* 关闭中任务 */
	DCTI_SHUT_DOWN,		/* 关闭完成任务 */
	DCTI_ERR,			/* 错误任务 */
	DCTI_BOOTING,		/* 载入中任务 */
	DCTI_WORK,			/* 工作中任务 */
}DcTaskId_E;

/* DC 错误代码枚举 */
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

/* DC 错误状态位域 */
typedef union
{
	struct
	{
		uint8_t			bPowerErr : 1;		/* 电源错误 */
		uint8_t			bOT       : 1;		/* 过温 */
		uint8_t			bOL       : 1;		/* 过载 */
		uint8_t			bCloseFail: 1;		/* 关闭失败 */
		uint8_t			bOutLow   : 1;		/* 输出低 */
		uint8_t			bOutHigh  : 1;		/* 输出高 */
		uint8_t			bNtcLost  : 1;		/* NTC丢失 */
	}tCode;
	uint8_t				ucErrCode;
}DcErrCode_U;

/* DC 运行控制对象 */
typedef struct
{
	DevState_E			eDevState;			/* 设备状态 */
	DcErrCode_U			uErrCode;			/* DC任务错误状态 */
	vu16				usInVolt;			/* 0.1V */
	vu16				usInCurr;			/* 0.1A */
	vu16				usOutVolt;			/* 0.1V */
	vu16				usOutCurr;			/* 0.1A */
	vu16				usOutPwr;			/* W */
	vu16				usAutoOffTime;		/* 自动关机时间 (S) */
	vu16				usAutoOffCnt;		/* 自动关机计时 (S) */
	s16					sMaxTemp;			/* 摄氏度 */
}Dc_T;

/* DC 记忆参数 */
#pragma pack(1)
typedef struct
{
	vu16				usAutoOffTime;		/* 自动关闭时间 (0为关闭此功能) */
	vu16				usMaxOutVolt;		/* 最大输出电压 (0.1V) */
	vu16				usMinOutVolt;		/* 最小输出电压 (0.1V) */
	vu16				usOverLoadPwr;		/* 过载功率 (W) */
	vu16				usMinOpenVolt;		/* 最小开启电压 (0.1V) */
	s8					sMaxTemp;			/* 允许的最大温度 (摄氏度) */
}DcMemParam_T;
#pragma pack()

//****************************************************Extern********************************************************************//
extern Dc_T         tDc;
extern Task_T       *tpDcTask;
extern TaskHandle_t tDcTaskHandler;

s8   cDc_TaskInit(void);
s8   cDc_Switch(SwitchType_E e_tri_type, bool b_fore_en);
void vDc_TickTimer(void);
void vDc_RefreshOffTime(void);
bool bDc_MemParamInit(DcMemParam_T *p_dc_mem);
void vDc_MemParamSet(uint8_t uc_item, bool b_add);

/* 供队列任务调用的状态机/保护接口(均在 DcTask 上下文内执行) */
void vDc_ParamInit(void);
void vDc_SetWorkState(DevState_E e_stat);
void vDc_SetErrCode(DcErrCode_E e_code, bool b_set);
void vDc_ProtectProcess(void);
s8   cDc_InfoInit(void);
s8   cDc_CheckOutVolt(void);

/* ADC 越限哨兵: AdcTask 滤波更新后调用，峰值过流越限时投递保护扫描任务 */
void vDc_AdcWatchdog(void);

#if (!boardUSE_OS)
void vDc_Task(void *p_v_parameters);
#endif  /* !boardUSE_OS */

#if (boardLOW_POWER)
void vDc_EnterLowPower(void);
void vDc_ExitLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardDC_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* DC_TASK_H_ */
