/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_queue_task_update.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print模块升级队列任务主流程及从机升级状态调度
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_queue_task_update.h"

#if (boardPRINT_IFACE && boardUPDATE)
#include "Print/print_queue_task.h"
#include "Print/print_task.h"
#include "Print/print_iface.h"
#include "Print/print_prot_frame.h"
#include "Sys/sys_queue_task_update.h"
#include "Sys/sys_task.h"
#include "Baiku/baiku_proto.h"
#include "Megmeet/megmeet_proto.h"
#include "check.h"
#include "function.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#endif  /* boardBMS_EN */

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_queue_task_update.h"
#endif  /* boardDCAC_EN */

//****************************************************Parameter Initialization**************************************************//
u16 us_char_send_dev_len = 0;

//****************************************************Function Declaration******************************************************//
static bool b_print_check_task_valid(Task_T *p_task);

/***********************************************************************************************************************
 * 函数功能    : Print升级队列任务主函数
 * 说明(备注)  : 根据升级对象分发BMS或DCAC升级流程
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_print_queue_task_update(Task_T *p_task)
{
    s8 c_ret = 0;

    #if (boardUSE_OS)
    /* 阻塞等待任务通知或超时 */
    ulTaskNotifyTake(pdTRUE, printTASK_UPDATE_CYCLE_TIME);
    #endif  /* boardUSE_OS */

    /* 统一检查任务有效性（升级对象、缓冲区、设备状态、错误） */
    if (b_print_check_task_valid(p_task) == false)
        cQueue_GotoStep(p_task, PUS_ERROR);

    /* 获取打印口接收缓存中的数据长度 */
    us_char_send_dev_len = lwrb_get_full(&tpPrintProtoRx->tRxBuff);

    /* 根据当前步骤分发升级流程 */
    switch (p_task->ucStep)
    {
        /* 步骤0：初始化 */
        case PUS_INIT:  
        {
            us_char_send_dev_len = 0;

            tPrint.eDevState = DS_SHUT_DOWN;
            lwrb_reset(&p_task->tReplyBuff);
            cQueue_GotoStep(p_task, STEP_NEXT);
        }
		break;

        /* 步骤1：等待从机初始化完成 */
        case PUS_WAIT_SLAVE_READY:
        {
            #if (boardBMS_EN)
            if (tUpdate.eObj == MO_BMS)
            {
                if (tBms.eDevState != DS_UPDATE_MODE)
                    break;
            }
            else
            #endif  /* boardBMS_EN */
            #if (boardDCAC_EN)
            if (IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
            {
                if (tDcac.eDevState != DS_UPDATE_MODE)
                    break;
            }
            else
            #endif  /* boardDCAC_EN */
                break;

            bUpdate_SetResult(URT_HOST, UTR_RUNNING);
            cQueue_GotoStep(p_task, STEP_NEXT);
        }
		break;

        /* 步骤2：准备Print进入升级 */
        case PUS_PREPARE_UPDATE:
        {
            #if (boardBMS_EN)
            if (tUpdate.eObj == MO_BMS)
            {
                c_ret = c_print_bms_prepare_update(p_task);
                if (c_ret < 0)
                {
                    cQueue_GotoStep(p_task, PUS_ERROR);
                    break;
                }

                if (c_ret == 0)
                    break;

                cQueue_GotoStep(p_task, PUS_BMS_UPDATE);
            }
            #endif  /* boardBMS_EN */

            #if (boardDCAC_EN)
            if (IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
            {
                c_ret = c_print_dcac_prepare_update(p_task);
                if (c_ret < 0)
                {
                    cQueue_GotoStep(p_task, PUS_ERROR);
                    break;
                }

                if (c_ret == 0)
                    break;
                
                xTaskNotifyGive(tDcacTaskHandler);
                cQueue_GotoStep(p_task, PUS_DCAC_UPDATE);
            }
            #endif  /* boardDCAC_EN */

            tPrint.eDevState = DS_UPDATE_MODE;
        }
        break;

        /* 步骤3：执行BMS升级主流程 */
        case PUS_BMS_UPDATE:  
        {
            #if (boardBMS_EN)
            c_ret = c_print_bms_update_firmware_transfer(p_task);

            if (c_ret < 0)
                cQueue_GotoStep(p_task, PUS_ERROR); /* 进入异常 */
            else if (c_ret > 0)
                cQueue_GotoStep(p_task, PUS_FINISH_CLEANUP); /* 升级完成 */
            #endif  /* boardBMS_EN */
        }
        break;

        /* 步骤4：执行DCAC升级主流程 */
        case PUS_DCAC_UPDATE:  
        {
            #if (boardDCAC_EN)
            c_ret = c_print_dcac_update_firmware_transfer(p_task);

            if (c_ret < 0)
                cQueue_GotoStep(p_task, PUS_ERROR); /* 进入异常 */
            else if (c_ret > 0)
                cQueue_GotoStep(p_task, PUS_FINISH_CLEANUP); /* 升级完成 */
            #endif  /* boardDCAC_EN */
        }
        break;

        /* 步骤5：升级错误 */
        case PUS_ERROR:  
        {
            if (tUpdate.eHostResult != UTR_CANCEL)
                bUpdate_SetResult(URT_HOST, UTR_FAIL);

            cQueue_GotoStep(p_task, PUS_END);
        }
        break;

        /* 步骤6：Print已经升级完成,收尾 */
        case PUS_FINISH_CLEANUP:  
        {
            bUpdate_SetResult(URT_HOST, UTR_OK);
            cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        /* 步骤7：升级完成，退出升级任务 */
        case PUS_END:  
        {
            tPrint.eDevState = DS_SHUT_DOWN;
            lwrb_reset(&p_task->tReplyBuff);
            cBaiku_ResetRxBuff(tpPrintProtoRx);
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
 * 函数功能    : 检查打印升级任务的有效性
 * 说明(备注)  : 统一检查升级对象、缓冲区、设备状态及错误码
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : true: 任务有效, false: 任务无效
 ************************************************************************************************************************/
static bool b_print_check_task_valid(Task_T *p_task)
{
    /* 参数合法性检查：任务指针为空则直接返回 */
    if (p_task == NULL)
        return false;

    /* 升级对象无效则结束任务 */
    if (tUpdate.eObj >= MO_INVAILD)
    {
        bUpdate_SetErrCode(UEF_PQ_INVALID_OBJ);
        return false;
    }

    /* 检查回复缓冲区是否有效 */
    if (p_task->tReplyBuff.buff == NULL)
    {
        bUpdate_SetErrCode(UEF_PQ_BUFF_NULL);
        return false;
    }

    /* 检查设备是否处于升级模式，且任务队列无新的任务*/
    if (tSysInfo.eDevState != DS_UPDATE_MODE || lwrb_get_full(&p_task->tQueueBuff))
        return false;

    /* 检查是否存在报错，若有错误则进入错误处理流程 */
    if (tUpdate.eErrCode != UEF_NONE && p_task->ucStep < PUS_ERROR)
        return false;

    return true;
}

#if (boardDCAC_EN)
/***********************************************************************************************************************
 * 函数功能    : 获取升级阶段
 * 说明(备注)  : 最终升级结果以tUpdate.eHostResult、tUpdate.eSlaveResult为准
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 升级队列阶段状态码或错误码
 ************************************************************************************************************************/
s8 cPrint_GetUpdateStage(void)
{
	if (tSysInfo.eDevState != DS_UPDATE_MODE ||
	    tUpdate.eChType != CT_PRINT)
		return -1;

	if (tpPrintTask == NULL)
		return -2;

	if (tpPrintTask->ucID != PTI_UPDATE)
		return -3;

	if (tpPrintTask->ucStep == PUS_ERROR)
		return UPDATE_QUEUE_STAGE_ERR;

	if (tpPrintTask->ucStep == PUS_END)
		return UPDATE_QUEUE_STAGE_WAIT_RESTART;

	if (tpPrintTask->ucStep > PUS_FINISH_CLEANUP)
		return UPDATE_QUEUE_STAGE_FINISH;

	return UPDATE_QUEUE_STAGE_RUNNING;
}
#endif  /* boardDCAC_EN */

#endif  /* boardPRINT_IFACE && boardUPDATE */
