/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task_update.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 升级队列子任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_queue_task.h"

#if (boardBMS_EN && boardUPDATE)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "MD_Bms/md_bms_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"

//****************************************************Macros********************************************************************//
#define			bmsTASK_UPDATE_TIME						50

//****************************************************Function Declaration******************************************************//
static bool b_bms_check_task_valid(Task_T *p_task);

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : BMS 升级队列任务
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_update(Task_T *p_task)
{
    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, bmsTASK_UPDATE_TIME);
    #endif  /* boardUSE_OS */

    /* 升级失败,进入收尾流程 */
    if (b_bms_check_task_valid(p_task) == false)
        cQueue_GotoStep(p_task, BMS_UPDATE_STEP_ERROR_CLEANUP);

    switch (p_task->ucStep)
    {
        /* 步骤0：初始化升级环境 */
        case BMS_UPDATE_STEP_INIT:
        {
            if (p_task->tReplyBuff.buff == NULL)
            {
                if (uPrint.tFlag.bBmsTask)
                    log_w("bBmsTask:任务返回参数缓存器异常");

                bUpdate_SetErrCode(UEF_BQ_INIT_BUFF_NULL);
                cQueue_GotoStep(p_task, BMS_UPDATE_STEP_END);
                break;
            }
            bBmsUseFlag = true;
            bUpdate_SetResult(URT_SLAVE, UTR_RUNNING);
            vQueue_ResetReply(p_task);

            if (tBms.eDevState != DS_UPDATE_MODE)
                bBms_SetDevState(DS_UPDATE_MODE);

            cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        /* 步骤1：等待升级完成 */
        case BMS_UPDATE_STEP_FORWARD_DATA:
        {
            /* 检查主机升级结果，完成则进入收尾 */
            if (tUpdate.eHostResult == UTR_OK || tUpdate.eHostResult == UTR_LATEST)
            {
                cQueue_GotoStep(p_task, BMS_UPDATE_STEP_FINISH_CLEANUP);
                break;
            }

            /* 升级失败或取消 */
            if (tUpdate.eHostResult == UTR_FAIL || tUpdate.eHostResult == UTR_CANCEL)
            {
                cQueue_GotoStep(p_task, BMS_UPDATE_STEP_ERROR_CLEANUP);
                break;
            }
        }
        break;

        /* 步骤2：升级错误,收尾 */
        case BMS_UPDATE_STEP_ERROR_CLEANUP:
        {
            bUpdate_SetResult(URT_SLAVE, UTR_FAIL);
            cQueue_GotoStep(p_task, BMS_UPDATE_STEP_END);
        }
        break;

        /* 步骤3：升级完成，收尾 */
        case BMS_UPDATE_STEP_FINISH_CLEANUP:
        {
            bUpdate_SetResult(URT_SLAVE, UTR_OK);
            cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        /* 步骤4：结束 */
        case BMS_UPDATE_STEP_END:
        {
            bBms_SetDevState(DS_SHUT_DOWN);
            vQueue_ResetReply(p_task);
            cBaiku_ResetRxBuff(tpBmsProtoRx);
            cQueue_GotoStep(p_task, STEP_END);
        }
        break;

        default:
        {
            cQueue_GotoStep(p_task, STEP_END);
        }
        break;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 检查 BMS 升级任务的有效性
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : bool: true-有效, false-无效
 ************************************************************************************************************************/
static bool b_bms_check_task_valid(Task_T *p_task)
{
    /* 参数合法性检查：任务指针为空则直接返回 */
    if (p_task == NULL)
        return false;

    /* 升级对象无效则结束任务 */
    if (tUpdate.eObj != MO_BMS)
    {
        bUpdate_SetErrCode(UEF_BQ_INVALID_OBJ);
        return false;
    }

    /* 检查回复缓冲区是否有效 */
    if (p_task->tReplyBuff.buff == NULL)
    {
        bUpdate_SetErrCode(UEF_BQ_BUFF_NULL);
        return false;
    }

    /* 检查设备是否处于升级模式，且任务队列无新的任务 */
    if (tSysInfo.eDevState != DS_UPDATE_MODE || lwrb_get_full(&p_task->tQueueBuff))
        return false;

    /* 检查是否存在报错，若有错误则进入错误处理流程 */
    if (tUpdate.eErrCode != UEF_NONE && p_task->ucStep < BMS_UPDATE_STEP_ERROR_CLEANUP)
        return false;

    return true;
}

#endif  /* boardBMS_EN && boardUPDATE */
