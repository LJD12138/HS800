/***********************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\11-G24\1.software\G2404-3\APP\Hardware\Print
 * File    : print_queue_task_update.h
 * Date    : 2026-05-18 18:21:25
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
************************************************************************************************************************/
#ifndef PRINT_QUEUE_TASK_UPDATE_H
#define PRINT_QUEUE_TASK_UPDATE_H


#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================includes====================================*/
#include "board_config.h"

#if(boardUPDATE)
#include "Print/print_queue_task.h"
#include "Baiku/baiku_proto.h"

/* ==========================================macros======================================*/
#define        printTASK_UPDATE_CYCLE_TIME                 100

/* ==========================================globals=====================================*/
extern u16 us_char_send_dev_len;
extern u16 us_char_send_print_len;

/* ==========================================types=======================================*/

/**
 * @brief Print升级队列任务步骤枚举
 */
typedef enum
{
    PRINT_UPDATE_STEP_INIT = 0x00,          /*!< 步骤0：初始化 */
    PRINT_UPDATE_STEP_WAIT_SLAVE_READY,     /*!< 步骤1：等待从机初始化完成 */
    PRINT_UPDATE_STEP_PREPARE_UPDATE,       /*!< 步骤2：准备Print进入升级 */
    PRINT_UPDATE_STEP_BMS_UPDATE,          /*!< 步骤3：执行BMS升级数据转发 */
    PRINT_UPDATE_STEP_DCAC_UPDATE,          /*!< 步骤4：执行DCAC升级主流程 */
    PRINT_UPDATE_STEP_ERROR,                /*!< 步骤5：升级错误 */
    PRINT_UPDATE_STEP_FINISH_CLEANUP,       /*!< 步骤6：Print已经升级完成,收尾 */
    PRINT_UPDATE_STEP_END,         /*!< 步骤7：升级完成，等待重新启动 */
} PrintUpdateStep_E;

/* ==========================================extern======================================*/
#if(boardBMS_EN)
s8 c_print_bms_prepare_update(Task_T* tp_task);
s8 c_print_bms_update_firmware_transfer(Task_T *tp_task);
#endif

#if(boardDCAC_EN)
s8 c_print_dcac_prepare_update(Task_T* tp_task);
s8 c_print_dcac_update_firmware_transfer(Task_T *tp_task);
s8 cPrint_GetUpdateStage(void);
#endif

#endif  //boardUPDATE

#ifdef __cplusplus
}
#endif

#endif  //PRINT_QUEUE_TASK_UPDATE_H
