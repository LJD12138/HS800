/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 任务队列管理与子任务分发接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_BMS_QUEUE_TASK_H_
#define MD_BMS_QUEUE_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"   

#if (boardBMS_EN)
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Globals*******************************************************************//
extern Task_T *tpBmsTask;  /* 队列任务指针 */

//****************************************************Extern********************************************************************//
bool bBms_QueueInit(void);

#if (boardRUN_LOG_EN)
void vBms_RunLogFinished(void);
bool bBms_IsRunLogBusy(void);
#endif  /* boardRUN_LOG_EN */

/* 队列子任务函数 */
void v_bms_queue_task_init(Task_T *p_task);
void v_bms_queue_task_clt_switch(Task_T *p_task);
void v_bms_queue_task_main(Task_T *p_task);
void v_bms_queue_task_cali(Task_T *p_task);
void v_bms_queue_task_err(Task_T *p_task);
void v_bms_queue_task_get_app_info(Task_T *p_task);
void v_bms_queue_task_req_set_cmd(Task_T *p_task);

#if (boardRUN_LOG_EN)
void v_bms_queue_task_run_log(Task_T *p_task);
#endif  /* boardRUN_LOG_EN */

#if (boardUPDATE)
/* BMS升级队列任务步骤枚举 */
typedef enum
{
	BMS_UPDATE_STEP_INIT = 0,	/*!< 步骤0：初始化升级环境 */
	BMS_UPDATE_STEP_FORWARD_DATA,	/*!< 步骤1：转发升级数据到BMS */
	BMS_UPDATE_STEP_ERROR_CLEANUP,	/*!< 步骤2：升级错误,收尾 */
	BMS_UPDATE_STEP_FINISH_CLEANUP,	/*!< 步骤3：升级完成，收尾 */
	BMS_UPDATE_STEP_END,	/*!< 步骤4：结束 */
}BmsUpdateStep_E;

void v_bms_queue_task_update(Task_T *p_task);
#endif  /* boardUPDATE */

#endif  /* boardBMS_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_BMS_QUEUE_TASK_H_ */
