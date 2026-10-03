/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_eng.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统工程模式队列任务头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef SYS_QUEUE_TASK_ENG_H_
#define SYS_QUEUE_TASK_ENG_H_

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardENG_MODE_EN)
#include "queue_task.h"

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//
//工程步骤
typedef enum
{
	EMS_INIT = 0,		//初始化
	EMS_SYS,			//系统测试
	EMS_LCD,			//屏幕测试
	EMS_BAT,			//电池测试
	EMS_DCAC,			//逆变测试
	EMS_MPPT,			//光伏测试
	EMS_USB,			//USB测试
	EMS_DC,				//DC测试
	EMS_ADC,			//ADC测试
	#if (boardLIGHT_EN)
	EMS_LIGHT,			//照明测试
	#endif  //boardLIGHT_EN
	EMS_SET,			//设置参数
	EMS_FINISH,			//完成
}EngModeStep_E;

typedef struct
{
	vu16				usEngModeCnt;		//工程模式计时
	vu8					ucEngModeItem;		//测试项目
	s8					cEngModeState;		//测试状态
}EngMode_T;

//****************************************************Globals*******************************************************************//
extern EngMode_T tEngMode;

//****************************************************Extern********************************************************************//
void vEng_RefreshEngModeTime(void);
void vEng_AdjustParam(uint8_t uc_tab, uint8_t uc_item, bool b_add);

#endif  //boardENG_MODE_EN

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  /* SYS_QUEUE_TASK_ENG_H_ */
