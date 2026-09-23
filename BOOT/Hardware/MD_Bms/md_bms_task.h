/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
 * File    : md_bms_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 电池管理任务、开关控制与记忆参数管理头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_BMS_TASK_H_
#define MD_BMS_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardBMS_EN)
#include "queue_task.h"
#include "MD_Bms/md_bms_rec_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			bmsTASK_CYCLE_TIME						100

//****************************************************Types*********************************************************************//

/* 任务ID */
typedef enum
{
	BTI_NULL = 0,			/* 空任务函数 */
	BTI_INIT,				/* 初始化电池包 */
	BTI_MAIN,				/* 循环获取电池包的数据 */
	BTI_CTRL_BMS_SW,		/* 控制BMS开启 */
	BTI_ERR_PROCESS,		/* 错误处理 */
	BTI_REQ_SET_CMD,		/* 设置指令 */
	BTI_GET_INFO,			/* 获取BMS信息 */
	BTI_CALI,				/* 校准 */
	BTI_UPDATE,				/* 升级 */
    #if (boardRUN_LOG_EN)
	BTI_LOG_UNLOCK,			/* 解锁运行日志 */
	BTI_GET_RUN_LOG,		/* 获取运行日志 10 */
	BTI_READ_RUN_LOG_SLOT,	/* 读取运行日志指定槽 */
	BTI_RESET_RUN_LOG,		/* 重置运行日志 */
    #endif  /* boardRUN_LOG_EN */
}BmsTaskId_E;

/* 工作状态 */
typedef enum
{
	BWS_NULL = 0,		/* 关闭 */
	BWS_DISCHG,			/* 放电 */
	BWS_CHG,			/* 充电 */
}BmsWorkState_E;

/* 错误状态枚举 */
typedef enum
{
	BEC_CLEAR_ALL = 0,		/* 清所有错误 */
	BEC_BMS_ERR = 1,		/* 模块上报报错 */
	BEC_SYS_DEV_LOST = 34,	/* 模块丢失 */
	BEC_SYS_CHG_OT,			/* 充电过温 */
	BEC_SYS_DISCHG_OT,		/* 放电过温 */
	BEC_SYS_CHG_UT,			/* 充电低温 */
	BEC_SYS_DISCHG_UT,		/* 放电低温 */
	BEC_SYS_LOW_VOLT,		/* 欠压 */
}BmsErrCode_E;

/* 错误状态联合体 */
typedef union
{
	struct
	{
		/* BMS上报错误(开始) */
		ErrCode_U		uBmsCode;			/* 错误代码 */
		/* 电池包上报错误(结束) */
		
		/* 系统判断的错误 */
		vu32			bSysDevLost:1;		/* 模块丢失 */
		vu32			bSysChgOT:1;		/* 充电过温 */
		vu32			bSysDisChgOT:1;		/* 放电过温 */
		vu32			bSysChgUT:1;		/* 充电低温 */
		vu32			bSysDisChgUT:1;		/* 放电低温 */
		vu32			bSysLV:1;			/* 欠压 */
	}tCode;
	vu64				ullCode;
}BmsErrCode_N;

/* 许可联合体 */
typedef union
{
	struct 
	{
		u8				bChgPerm:1;			/* 充电许可 */
		u8				bDisChgPerm:1;		/* 放电许可 */
		u8				bForceClose:1;		/* 强制关闭 */
		u8				temp:5;
	}tPerm;
	u8					ucPerm;
}BmsPerm_U;

/* 许可对象枚举 */
typedef enum
{
	BPO_CHG = 0,		/* 充电 */
	BPO_DISCHG,			/* 放电 */
	BPO_ALL,			/* 全部 */
}BmsPermObject_E;

/* 任务对象 */
typedef struct
{
	DevState_E			eDevState;			/* 设备状态 */
	BmsErrCode_N		uErrCode;			/* tBms错误状态 */
	BmsWorkState_E		eWorkState;			/* tBms工作状态 */
	BmsPerm_U			uPerm;				/* 许可 */
	vu16				usAutoOffCnt;		/* 关闭电池包的时间,0为不开启 */
	vu16				usAutoOffTime;		/* 时间戳大于这个值就关闭电池包 */
	s16					sMaxTemp;			/* 1摄氏度 */
	s16					sMinTemp;			/* 1摄氏度 */
}Bms_T;

/* 记忆参数 */
#pragma pack(1)
typedef struct
{
	s8					cChgMaxTemp;		/* 充电允许的最大温度 */
	s8					cDisChgMaxTemp;		/* 放电允许的最大温度 */
	s8					cChgMinTemp;		/* 充电允许的最小温度 */
	s8					cDisChgMinTemp;		/* 放电允许的最小温度 */
	vu16				usMaxVolt;			/* 最大输入电压 */
	vu16				usMinVolt;			/* 最小输入电压 */
	vu16				usChgVolt;			/* 充电电压 */
}BmsMemParam_T;
#pragma pack()

//****************************************************Globals*******************************************************************//
extern Task_T           *tpBmsTask;
extern Bms_T            tBms;

#if (boardUSE_OS)
extern TaskHandle_t     tBmsTaskHandler;
#endif  /* boardUSE_OS */

//****************************************************Extern********************************************************************//
s8 cBms_TaskInit(void);
bool bBms_TaskParamInit(void);

#if (!boardUSE_OS)
void vBms_Task(void *p_param);
#endif  /* !boardUSE_OS */

#endif  /* boardBMS_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_BMS_TASK_H_ */
