/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_queue_task_update.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print模块升级队列任务步骤枚举与从机固件升级接口声明
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef __PRINT_QUEUE_TASK_UPDATE_H
#define __PRINT_QUEUE_TASK_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardUPDATE)
#include "Print/print_queue_task.h"
#include "Baiku/baiku_proto.h"

//****************************************************Macros********************************************************************//
#define     	printTASK_UPDATE_CYCLE_TIME     		100

//****************************************************Globals*******************************************************************//
extern u16 us_char_send_dev_len;

//****************************************************Types*********************************************************************//
/* Print升级队列任务步骤枚举 */
typedef enum
{
	PUS_INIT = 0x00,      //步骤0：初始化
	PUS_WAIT_SLAVE_READY, //步骤1：等待从机初始化完成
	PUS_PREPARE_UPDATE,   //步骤2：准备Print进入升级
	PUS_BMS_UPDATE,       //步骤3：执行BMS升级数据转发
	PUS_DCAC_UPDATE,      //步骤4：执行DCAC升级主流程
	PUS_ERROR,            //步骤5：升级错误
	PUS_FINISH_CLEANUP,   //步骤6：Print已经升级完成,收尾
	PUS_END,              //步骤7：升级完成，等待重新启动
}PrintUpdateStep_E;

//****************************************************Extern********************************************************************//
#if (boardBMS_EN)
s8 c_print_bms_prepare_update(Task_T* p_task);
s8 c_print_bms_update_firmware_transfer(Task_T *p_task);
#endif  /* boardBMS_EN */

#if (boardDCAC_EN)
s8 c_print_dcac_prepare_update(Task_T* p_task);
s8 c_print_dcac_update_firmware_transfer(Task_T *p_task);
s8 cPrint_GetUpdateStage(void);
#endif  /* boardDCAC_EN */

#endif  /* boardUPDATE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __PRINT_QUEUE_TASK_UPDATE_H */
