/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_rec_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 串口接收与环形缓冲管理任务头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_MPPT_REC_TASK_H_
#define MD_MPPT_REC_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardMPPT_EN)
#include "lwrb.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Types*********************************************************************//
#pragma pack(1)
typedef struct
{
	uint16_t			usInVolt;			/* 输入电压 0.1V */
	uint16_t			usInCurr;			/* 输入电流 0.1A */
	uint16_t			usInPwr;			/* 输入功率 1W */
	uint16_t			usInState;			/* 输入状态 1:PV输入  0:无输入 */
	uint16_t			usErrCode;			/* 故障状态 */
}MpptParam_T;
#pragma pack()

/* 输入类型枚举 */
typedef enum 
{
	MIT_NULL = 0,
	MIT_DC,				/* 适配器 */
	MIT_PV,				/* 太阳能 */
}MpptInType_U;

/* 错误状态联合体 */
typedef union
{
	struct
	{
		vu16			bInOV:1;			//输入过压
		vu16			bInUV:1;			//输入欠压
		vu16			bInUP:1;			//输入欠功率
		vu16			bOutOV:1;			//输出过压
		vu16			bReserved:1;		//预留
		vu16			bInSC:1;			//输入短路
		vu16			bInOC:1;			//输入过流
		vu16			bOutUV:1;			//输出欠压

		// vu16 bOutOC:1;		//输出过流
		// vu16 bOutSC:1;		//输出短路
		// vu16 bOL:1;			//过载
		// vu16 bOT:1;			//过温
		// vu16 bEnFault:1;	//使能故障
		vu16			:8;
	}tCode;
	vu16				usCode;
}MpptRecErrCode_U;

/* MPPT 接收运行任务对象 (4字节自然对齐) */
typedef struct
{
	MpptInType_U		uInType;			/* 输入类型 */
	MpptRecErrCode_U	uErrCode;			/* 错误状态 */
	uint16_t			usInVolt;			/* 输入电压 0.1V */
	uint16_t			usInCurr;			/* 输入电流 0.01A */
	uint16_t			usInPwr;			/* 输入功率 0.1W */
	uint16_t			usOutVolt;			/* 输出电压 0.1V */
	uint16_t			usOutCurr;			/* 输出电流 0.01A */
	uint16_t			usOutPwr;			/* 输出功率 0.1W */
	uint16_t			usMaxInPwr;			/* 最大输入功率 0.1W */
	int16_t				sMaxTemp;			/* 最大温度 (℃) */
}MpptRx_T;

//****************************************************Globals*******************************************************************//
extern MpptRx_T tMpptRx; 
extern vs16 sMpptMaxTemp;

#if (boardUSE_OS)
extern TaskHandle_t tMpptRecTaskHandle;
#endif  /* boardUSE_OS */

//****************************************************Extern********************************************************************//
s8   cMppt_RecTaskInit(void);
void vMppt_RecTickTimer(void);

#if (!boardUSE_OS)
void vMppt_RecTask(void *pvParameters);
#endif  /* !boardUSE_OS */

#endif  /* boardMPPT_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_MPPT_REC_TASK_H_ */

