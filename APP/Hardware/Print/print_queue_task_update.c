/*****************************************************************************************************************
*                                                                                                                *
 *                                         Print upgrade queue task                                              *
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_queue_task_update.h"

#if(boardPRINT_IFACE && boardUPDATE)
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

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#endif

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_queue_task_update.h"
#endif

u16 us_char_send_dev_len = 0;
u16 us_char_send_print_len = 0;

/* ========================================== 静态函数声明 ========================================== */
static bool b_print_check_task_valid(Task_T *tp_task);


/***********************************************************************************************************************
-----函数功能    打印升级队列任务主函数
-----说明(备注)  根据升级对象分发BMS或DCAC升级流程
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_print_queue_task_update(Task_T *tp_task)
{
    s8 c_ret = 0;

    #if(boardUSE_OS)
    /* 阻塞等待任务通知或超时 */
    ulTaskNotifyTake(pdTRUE, printTASK_UPDATE_CYCLE_TIME);
    #endif

    /* 统一检查任务有效性（升级对象、缓冲区、设备状态、错误） */
    if(b_print_check_task_valid(tp_task) == false)
        cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_ERROR);

    /* 获取打印口接收缓存及任务回复缓存中的数据长度 */
    us_char_send_dev_len = lwrb_get_full(&tpPrintProtoRx->tRxBuff);
    us_char_send_print_len = lwrb_get_full(&tp_task->tReplyBuff);

    /* 根据当前步骤分发升级流程 */
    switch(tp_task->ucStep)
    {
        /* 步骤0：初始化 */
        case PRINT_UPDATE_STEP_INIT:  
        {
            us_char_send_dev_len = 0;
            us_char_send_print_len = 0;

            tPrint.eDevState = DS_SHUT_DOWN;
            lwrb_reset(&tp_task->tReplyBuff);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }

        /* 步骤1：等待从机初始化完成 */
        case PRINT_UPDATE_STEP_WAIT_SLAVE_READY:
        {
            #if(boardBMS_EN)
            if(tUpdate.eObj == MO_BMS)
            {
                if(tBms.eDevState != DS_UPDATE_MODE)
                    break;
            }
            else
            #endif  //boardBMS_EN
            #if(boardDCAC_EN)
            if(IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
            {
                if(tDcac.eDevState != DS_UPDATE_MODE)
                    break;
            }
            else
            #endif  //boardDCAC_EN
                break;

            bUpdate_SetResult(URT_HOST, UTR_RUNNING);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }

        /* 步骤2：准备Print进入升级 */
        case PRINT_UPDATE_STEP_PREPARE_UPDATE:
        {
            #if(boardBMS_EN)
            if(tUpdate.eObj == MO_BMS)
            {
                c_ret = c_print_bms_prepare_update(tp_task);
                if(c_ret < 0)
                {
                    cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_ERROR);
                    break;
                }

                if(c_ret == 0)
                    break;

                cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_BMS_UPDATE);
            }
            #endif  //boardBMS_EN

            #if(boardDCAC_EN)
            if(IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
            {
                c_ret = c_print_dcac_prepare_update(tp_task);
                if(c_ret < 0)
                {
                    cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_ERROR);
                    break;
                }

                if(c_ret == 0)
                    break;
                
                xTaskNotifyGive(tDcacTaskHandler);
                cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_DCAC_UPDATE);
            }
            #endif  //boardDCAC_EN

            tPrint.eDevState = DS_UPDATE_MODE;
        }
        break;

        /* 步骤3：执行BMS升级主流程 */
        case PRINT_UPDATE_STEP_BMS_UPDATE:  
        {
            #if(boardBMS_EN)
            c_ret = c_print_bms_update_firmware_transfer(tp_task);

            if(c_ret < 0)
                cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_ERROR); /* 进入异常 */
            else if(c_ret > 0)
                cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_FINISH_CLEANUP); /* 升级完成 */
            #endif
        }
        break;

        /* 步骤4：执行DCAC升级主流程 */
        case PRINT_UPDATE_STEP_DCAC_UPDATE:  
        {
            #if(boardDCAC_EN)
            c_ret = c_print_dcac_update_firmware_transfer(tp_task);

            if(c_ret < 0)
                cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_ERROR); /* 进入异常 */
            else if(c_ret > 0)
                cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_FINISH_CLEANUP); /* 升级完成 */
            #endif
        }
        break;

        /* 步骤5：升级错误 */
        case PRINT_UPDATE_STEP_ERROR:  
        {
            if(tUpdate.eHostResult != UTR_CANCEL)
                bUpdate_SetResult(URT_HOST, UTR_FAIL);

            cQueue_GotoStep(tp_task, PRINT_UPDATE_STEP_END);
        }
        break;

        /* 步骤6：Print已经升级完成,收尾 */
        case PRINT_UPDATE_STEP_FINISH_CLEANUP:  
        {
            bUpdate_SetResult(URT_HOST, UTR_OK);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        /* 步骤7：升级完成，退出升级任务 */
        case PRINT_UPDATE_STEP_END:  
        {
            tPrint.eDevState = DS_SHUT_DOWN;
            lwrb_reset(&tp_task->tReplyBuff);
            cBaiku_ResetRxBuff(tpPrintProtoRx);
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }
}

/*****************************************************************************************************************
-----函数功能    检查打印升级任务的有效性
-----说明(备注)  统一的任务有效性检查逻辑，包括升级对象、缓冲区、设备状态和错误检查
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      true: 任务有效  false: 任务无效（已设置错误码或跳转步骤）
******************************************************************************************************************/
static bool b_print_check_task_valid(Task_T *tp_task)
{
    /* 参数合法性检查：任务指针为空则直接返回 */
    if(tp_task == NULL)
        return false;

    /* 升级对象无效则结束任务 */
    if(tUpdate.eObj >= MO_INVAILD)
    {
        bUpdate_SetErrCode(UEF_PQ_INVALID_OBJ);
        return false;
    }

    /* 检查回复缓冲区是否有效 */
    if(tp_task->tReplyBuff.buff == NULL)
    {
        bUpdate_SetErrCode(UEF_PQ_BUFF_NULL);
        return false;
    }

    /* 检查设备是否处于升级模式，且任务队列无新的任务*/
    if(tSysInfo.eDevState != DS_UPDATE_MODE || lwrb_get_full(&tp_task->tQueueBuff))
        return false;

    /* 检查是否存在报错，若有错误则进入错误处理流程 */
    if(tUpdate.eErrCode != UEF_NONE && tp_task->ucStep < PRINT_UPDATE_STEP_ERROR)
        return false;

    return true;
}

#if(boardDCAC_EN)



/***********************************************************************************************************************
-----函数功能    获取升级阶段
-----说明(备注)  最终升级结果以tUpdate.eHostResult、tUpdate.eSlaveResult为准
-----传入参数    none
-----输出参数    none
-----返回值      2:升级完成，等待重新启动  1:升级完成  0:升级中
                 -1:设备不在升级模式  -2:任务指针为空  -3:任务ID不匹配
************************************************************************************************************************/
s8 cPrint_GetUpdateStage(void)
{
	if(tSysInfo.eDevState != DS_UPDATE_MODE ||
	   tUpdate.eChType != CT_PRINT)
		return -1;

	if(tpPrintTask == NULL)
		return -2;

	if(tpPrintTask->ucID != PTI_UPDATE)
		return -3;

	if(tpPrintTask->ucStep == PRINT_UPDATE_STEP_ERROR)
		return UPDATE_QUEUE_STAGE_ERR;

	if(tpPrintTask->ucStep == PRINT_UPDATE_STEP_END)
		return UPDATE_QUEUE_STAGE_WAIT_RESTART;

	if(tpPrintTask->ucStep > PRINT_UPDATE_STEP_FINISH_CLEANUP)
		return UPDATE_QUEUE_STAGE_FINISH;

	return UPDATE_QUEUE_STAGE_RUNNING;
}

#endif
#endif
