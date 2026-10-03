/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task_run_log.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 运行日志下发队列子任务(解锁/获取/读槽/重置)实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_queue_task.h"

#if (boardBMS_EN && boardRUN_LOG_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//
#define			bmsTASK_RUN_LOG_CYCLE_TIME				50

//****************************************************Parameter Initialization**************************************************//
static bool s_b_bms_run_log_busy = false;

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 标记 BMS 运行日志流式传输完成
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBms_RunLogFinished(void)
{
    s_b_bms_run_log_busy = false;
    #if (boardUSE_OS)
    xTaskNotifyGive(tBmsTaskHandler);
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 查询 BMS 运行日志流式传输是否进行中
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-传输中, false-空闲
 ************************************************************************************************************************/
bool bBms_IsRunLogBusy(void)
{
    return s_b_bms_run_log_busy;
}

/***********************************************************************************************************************
 * 函数功能    : BMS 运行日志命令下发任务(解锁0x8A/获取0xB4/读槽0xB6/重置0xB8)
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_run_log(Task_T *p_task)
{
    uint16_t us_param = p_task->usInParam;
    uint16_t us_timeout_ms = 5000; /* 默认 5S 超时 */

    /* 0xB8 重置需要擦除 EEPROM (3~4S), 超时放宽至 10S */
    if (p_task->ucID == BTI_RESET_RUN_LOG)
        us_timeout_ms = 10000;
    /* 0xB4 获取多条日志传输耗时较长 (最多1113条*15ms≈17S), 单帧超时保护放宽至 30S */
    else if (p_task->ucID == BTI_GET_RUN_LOG)
        us_timeout_ms = 30000;

    switch (p_task->ucStep)
    {
        case 0:
        {
            s8 c_ret = 0;
            if (p_task->ucID == BTI_LOG_UNLOCK)
                c_ret = c_bms_cs_log_unlock((uint8_t)us_param);
            else if (p_task->ucID == BTI_GET_RUN_LOG)
            {
                s_b_bms_run_log_busy = true;
                c_ret = c_bms_cs_get_run_log(us_param);
            }
            else if (p_task->ucID == BTI_READ_RUN_LOG_SLOT)
                c_ret = c_bms_cs_read_run_log_slot(us_param);
            else if (p_task->ucID == BTI_RESET_RUN_LOG)
                c_ret = c_bms_cs_reset_run_log((uint8_t)us_param);

            if (c_ret > 0)
                cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            if (p_task->ucID == BTI_GET_RUN_LOG)
            {
                /* 等待接收结束帧(0xB5 len=0 会调用 vBms_RunLogFinished 清除标志) */
                if (s_b_bms_run_log_busy == false)
                    cQueue_GotoStep(p_task, STEP_END);
                /* 传输流进行中，继续保持在步骤1等待，避免切回主任务发送0x07轮询产生总线冲突 */
            }
            else
                /* 单帧指令(解锁/读槽/重置)已完成发送与应答接收 */
                cQueue_GotoStep(p_task, STEP_END);
        }
        break;

        default:
        {
            s_b_bms_run_log_busy = false;
            cQueue_GotoStep(p_task, STEP_END);
        }
        break;
    }

    if (bQueue_IsTaskTimeoutMs(p_task, us_timeout_ms))
    {
        if (uPrint.tFlag.bBmsTask)
            log_w("bBmsTask: 日志任务等待超时, ID=%d, 步骤=%d", p_task->ucID, p_task->ucStep);
        s_b_bms_run_log_busy = false;
        cQueue_GotoStep(p_task, STEP_END);
    }

    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, bmsTASK_RUN_LOG_CYCLE_TIME);   /* 周期节拍；新任务投递立即唤醒抢占 */
    #endif  /* boardUSE_OS */
}

#endif  /* boardBMS_EN && boardRUN_LOG_EN */
