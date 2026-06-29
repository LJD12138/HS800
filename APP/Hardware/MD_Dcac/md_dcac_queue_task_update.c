/*****************************************************************************************************************
*                                                                                                                *
*                                         DCAC upgrade queue task                                               *
*                                         DCAC升级队列任务（主机端）                                            *
*                                                                                                                *
* 功能说明：                                                                                                      *
*   本文件实现DCAC（逆变器）模块的在线升级队列任务，基于Megmeet协议与DCAC从机进行交互。                          *
*   升级流程包括：请求升级(F0/F1) -> 跳转BOOT(F6/F7) -> 再次握手(F0/F1) -> 设置波特率(F2/F3) ->                 *
*                下发文件头(A1/A2) -> 下发固件数据(A3/A4) -> 查询结果(A5/A6)。                                  *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task_update.h"
#include <stdbool.h>

#if(boardDCAC_EN && boardUPDATE)
#include "MD_Dcac/md_dcac_queue_task.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "Megmeet/megmeet_proto.h"
#include "Baiku/baiku_proto.h"
#include "check.h"

/* ========================================== 宏定义 ========================================== */
#define        dcacTASK_UPDATE_CYCLE_TIME                 500     /*!< DCAC升级任务周期，单位：ms */

#define        dcacUPDATE_HS_TIMEOUT_MS                   1000    /*!< 握手阶段单次等待超时时间，单位：ms */
#define        dcacUPDATE_F7_WAIT_MS                      1000    /*!< 等待F7(跳转BOOT回复)超时时间，单位：ms */
#define        dcacUPDATE_BOOT_JUMP_DELAY_MS              500     /*!< BOOT跳转后等待稳定延时，单位：ms */

#define        dcacUPDATE_MAX_RETRY_COUNT                 3       /*!< 最大重试次数 */

/* ========================================== 变量声明 ========================================== */
DcacPrepStage_E eDcacPrepStage;
DcacFwTransStage_E eDcacFwTransStage;


/* ========================================== 静态函数声明 ========================================== */
static bool b_dcac_check_task_valid(Task_T *tp_task);
static s8 c_dcac_prepare_update(Task_T *tp_task, u16 us_reply_len, u16 us_cycle_time);
static s8 c_dcac_update_firmware_transfer(Task_T *tp_task, u16 us_reply_len, u16 us_cycle_time);
static bool b_dcac_check_retry_limit(Task_T *tp_task, UpdateErrCode_E e_err_code);
static bool b_dcac_check_wait_timeout(Task_T *tp_task, u16 us_timeout_ms, u16 us_cycle_time);


/*****************************************************************************************************************
-----函数功能    DCAC升级队列任务主函数
-----说明(备注)  按步骤执行DCAC在线升级流程，每步周期为dcacTASK_UPDATE_CYCLE_TIME(500ms)。
                若当前设备状态非升级模式或升级对象非DCAC，则直接结束任务。
-----传入参数    tp_task: 指向任务结构体的指针
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_update(Task_T *tp_task)
{
    s8 c_ret = 0;
    u16 us_cycle_time = dcacTASK_UPDATE_CYCLE_TIME;
    
    /* 动态调整升级固件传输阶段的轮询周期，以提升传输速率 */
    if(tp_task->ucStep == DUS_STEP_FW_TRANS)
    {
        us_cycle_time = 100;
    }
    
    #if(boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, us_cycle_time);
    #endif
    
    /* 统一检查任务有效性（升级对象、缓冲区、设备状态、错误） */
    if(b_dcac_check_task_valid(tp_task) == false)
        return;

    //获取任务缓存器的返回值长度，判断是否有数据需要处理
    u16 us_reply_len = lwrb_get_full(&tp_task->tReplyBuff);

    switch(tp_task->ucStep)
    {
        /*---------------- 步骤0：初始化升级环境 ----------------*/
        case DUS_STEP_INIT:
        {
            if((tDcacMegmeetProtoTx == NULL || tpDcacMegmeetProtoRx == NULL) && bDcac_MegmeetProtInit() == false)
            {
                if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_e("bDcacTask:Megmeet协议对象初始化失败");

                bUpdate_SetErrCode(UEF_D_SEND_F0_FAIL);
                break;
            }

            bDcac_SetDevState(DS_SHUT_DOWN);
            bUpdate_SetResult(URT_SLAVE, UTR_RUNNING);
            bDcac_SetPrepStage(tp_task, DPS_IDLE); /* 复位升级准备阶段 */
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }

        /*---------------- 步骤1：升级准备阶段（F0/F1/F6/F7/延时/F2/F3） ----------------*/
        case DUS_STEP_PREPARE:
        {
            c_ret = c_dcac_prepare_update(tp_task, us_reply_len, us_cycle_time);
            if(c_ret < 0)
            {
                cQueue_GotoStep(tp_task, DUS_STEP_ERROR_CLEANUP);
                break;
            }
            else if(c_ret == 0)
                break;
            /* c_ret > 0: 准备完成，进入数据交互阶段 */
            bDcac_SetFwTransStage(tp_task, DFTS_IDLE); /* 复位升级阶段 */
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }

        /*---------------- 步骤4：根据升级阶段执行数据交互（A3/A4/A5/A6） ----------------*/
        case DUS_STEP_FW_TRANS:
        {
            c_ret = c_dcac_update_firmware_transfer(tp_task, us_reply_len, us_cycle_time);

            if(c_ret < 0)
            {
                cQueue_GotoStep(tp_task, DUS_STEP_ERROR_CLEANUP); /* 进入异常 */
                break;
            }
            else if(c_ret == 0)
                break;

            cQueue_GotoStep(tp_task, DUS_STEP_FINISH_CLEANUP); /* 升级完成 */
        }
        break;

        /*---------------- 步骤5：升级错误,收尾 ----------------*/
        case DUS_STEP_ERROR_CLEANUP:
        {
            if(tUpdate.eErrCode == UEF_NONE)
                bUpdate_SetErrCode(UEF_D_RESEND_FAIL);
            bUpdate_SetResult(URT_SLAVE, UTR_FAIL);

            /* 若上位机还在等待，主动发送取消指令防止上位机死等 */
            if(tUpdate.eHostResult == UTR_RUNNING)
            {
                c_print_cs_C8_trans_cancel();
            }

            /* 异常退出时恢复默认波特率与接口状态，避免遗留高波特率配置 */
            bDcac_IfaceSetBaud(dcacUSART_BAUD);
            vDcac_IfaceInit();

            cQueue_GotoStep(tp_task, DUS_STEP_END);
            break;
        }

        /*---------------- 步骤6：升级完成，收尾 ----------------*/
        case DUS_STEP_FINISH_CLEANUP:
        {
            vDcac_IfaceInit(); /* 升级完成后重置接口状态，准备进入正常工作模式 */
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;
        }

        /*---------------- 步骤7：结束 ---------------------------*/
        case DUS_STEP_END:
        {
            bDcac_SetDevState(DS_SHUT_DOWN);
            lwrb_reset(&tp_task->tReplyBuff);
            cModbus_ResetTx(tpDcacProtoTx, tpDcacProtoTx->usFrameDataSize);
            cModbus_ResetRxBuff(tpDcacProtoRx);
            cQueue_GotoStep(tp_task, STEP_END);//退出当前任务,等待重新载入
        }
        break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }
}

/*****************************************************************************************************************
-----函数功能    检查DCAC升级任务的有效性
-----说明(备注)  统一的任务有效性检查逻辑，包括升级对象、缓冲区、设备状态和错误检查
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      true: 任务有效  false: 任务无效（已设置错误码或跳转步骤）
******************************************************************************************************************/
static bool b_dcac_check_task_valid(Task_T *tp_task)
{
    /* 参数合法性检查：任务指针为空则直接返回 */
    if(tp_task == NULL)
        return false;

    /* 升级对象无效则结束任务 */
    if(!IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
    {
        bUpdate_SetErrCode(UEF_D_INVALID_OBJ);
        cQueue_GotoStep(tp_task, STEP_END);
        return false;
    }

    /* 检查回复缓冲区是否有效 */
    if(tp_task->tReplyBuff.buff == NULL)
    {
        bUpdate_SetErrCode(UEF_D_BUFF_NULL);
        return false;
    }

    /* 检查设备是否处于升级模式，且任务队列无残留数据 */
    if(tSysInfo.eDevState != DS_UPDATE_MODE || lwrb_get_full(&tp_task->tQueueBuff))
    {
        cQueue_GotoStep(tp_task, STEP_END);
        return false;
    }

    /* 检查是否存在报错，若有错误则进入错误处理流程 */
    if(tUpdate.eErrCode != UEF_NONE)
    {
        cQueue_GotoStep(tp_task, DUS_STEP_ERROR_CLEANUP); /* 进入错误收尾阶段 */
        return false;
    }

    return true;
}


/*****************************************************************************************************************
-----函数功能    检查重试次数是否超限
-----说明(备注)  统一的重试次数检查逻辑，增加计数器并检查是否超限，超限则设置错误码
-----传入参数    tp_task    : 指向任务结构体的指针
                e_err_code : 超限时要设置的错误码
-----输出参数    none
-----返回值      true: 已超限  false: 未超限
******************************************************************************************************************/
static bool b_dcac_check_retry_limit(Task_T *tp_task, UpdateErrCode_E e_err_code)
{
    tp_task->usStepRepeatCnt++;
    if(tp_task->usStepRepeatCnt > dcacUPDATE_MAX_RETRY_COUNT)
    {
        bUpdate_SetErrCode(e_err_code);
        return true;
    }
    return false;
}

/*****************************************************************************************************************
-----函数功能    检查等待是否超时（跳转到STEP_NEXT）
-----说明(备注)  统一的等待超时检查逻辑，增加等待计数器并检查是否超时，超时后跳转到STEP_NEXT
-----传入参数    tp_task      : 指向任务结构体的指针
                us_timeout_ms: 超时时间（单位：ms）
                us_cycle_time: 任务周期时间（单位：ms）
-----输出参数    none
-----返回值      true: 已超时  false: 未超时
******************************************************************************************************************/
static bool b_dcac_check_wait_timeout(Task_T *tp_task, u16 us_timeout_ms, u16 us_cycle_time)
{
    tp_task->usStepWaitCnt++;
    if(tp_task->usStepWaitCnt >= (us_timeout_ms / us_cycle_time))
    {
        return true;
    }
    return false;
}

/*****************************************************************************************************************
-----函数功能    DCAC升级准备阶段处理（步骤1~7）
-----说明(备注)  处理升级准备阶段的F0/F1/F6/F7/BOOT延时/F2/F3交互流程，
                包括请求升级、跳转BOOT、再次握手、设置波特率。
-----传入参数    tp_task             : 指向任务结构体的指针
                us_reply_len        : 回复数据长度
                us_cycle_time       : 任务调度周期
-----输出参数    none
-----返回值      正值:准备完成，进入数据交互阶段  0:继续  负值:处理失败
******************************************************************************************************************/
static s8 c_dcac_prepare_update(Task_T *tp_task, u16 us_reply_len, u16 us_cycle_time)
{
    switch(eDcacPrepStage)
    {
        //复位初始化
        case DPS_IDLE:
        {
            lwrb_reset(&tp_task->tReplyBuff);
            bDcac_SetPrepStage(tp_task, DPS_SEND_F0);
        }
        break;

        /*---------------- 步骤1：发送F0请求升级 ----------------*/
        case DPS_SEND_F0:
        {
            if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_F0_FAIL))
                return -1;

            if(b_dcac_send_f0() == false)
                break;

            bDcac_SetPrepStage(tp_task,DPS_WAIT_F1);
        }
        break;

        /*---------------- 步骤2：等待F1回复 ----------------*/
        case DPS_WAIT_F1:
        {
            //超时进入F6跳转BOOT
            if(b_dcac_check_wait_timeout(tp_task, dcacUPDATE_HS_TIMEOUT_MS, us_cycle_time) == false)
                break;

            bDcac_SetPrepStage(tp_task, DPS_SEND_F6);
        }
        /* fall through */

        /*---------------- 步骤3：发送F6请求跳转BOOT ----------------*/
        case DPS_SEND_F6:
        {
            if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_F6_FAIL))
                return -4;

            if(b_dcac_send_f6() == false)
                break;

            bDcac_SetPrepStage(tp_task, DPS_WAIT_F7);
        }
        break;

        /*---------------- 步骤4：等待F7回复 ----------------*/
        case DPS_WAIT_F7:
        {
            if(b_dcac_check_wait_timeout(tp_task, dcacUPDATE_F7_WAIT_MS, us_cycle_time) == true)
                bDcac_SetPrepStage(tp_task,DPS_SEND_F6); /* 返回上一步继续发送 */
        }
        break;

        /*---------------- 步骤5：BOOT跳转后延时等待 ----------------*/
        case DPS_BOOT_DELAY:
        {
            if(b_dcac_check_wait_timeout(tp_task, dcacUPDATE_BOOT_JUMP_DELAY_MS, us_cycle_time) == true)
                bDcac_SetPrepStage(tp_task,DPS_SEND_F0); /* 延时结束，再次发送F0握手 */
        }
        break;

        /*---------------- 步骤6：发送F2设置波特率 ----------------*/
        case DPS_SEND_F2:
        {
            if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_F2_FAIL))
                return -6;

            /* 波特率已是目标值 */
            if(tUpdate.ulBaud == dcacUSART_BAUD)
            {
                bDcac_SetDevState(DS_UPDATE_MODE);
                bDcac_SetPrepStage(tp_task, DPS_WAIT_PRINT_UPDATE_REQ);
                break;
            }

            if(b_dcac_send_f2(tUpdate.ulBaud) == false)
                break;

            bDcac_SetPrepStage(tp_task,DPS_WAIT_F3);
        }
        break;

        /*---------------- 步骤7：等待F3回复 ------------------------*/
        case DPS_WAIT_F3:
        {
            //等待超时
            if(b_dcac_check_wait_timeout(tp_task, dcacUPDATE_HS_TIMEOUT_MS, us_cycle_time))
            {
                bDcac_SetPrepStage(tp_task,DPS_SEND_F2); /* 返回上一步继续发送 */
                break;
            }
        }
        break;

        /*---------------- 步骤8：等待Print请求升级,等待5S ----------------*/
        case DPS_WAIT_PRINT_UPDATE_REQ:
        {
            if(b_dcac_check_wait_timeout(tp_task, 5000, us_cycle_time) == true)
                bDcac_SetPrepStage(tp_task,DPS_PRINT_SEND_C4);
        }
        break;

        /*---------------- 步骤9：发送C4获取头文件 ----------------*/
        case DPS_PRINT_SEND_C4:
        {
            if(c_print_cs_C4_req_start_send() <= 0)
                break;

            bDcac_SetPrepStage(tp_task,DPS_PRINT_WAIT_REPLY_C5);
        }
        break;

        /*---------------- 步骤10：等待回复C5 ----------------*/
        case DPS_PRINT_WAIT_REPLY_C5:
        {
            if(b_dcac_check_wait_timeout(tp_task, 1000, us_cycle_time) == true)
            {
                tp_task->usStepWaitCnt = 0;
                if(b_dcac_check_retry_limit(tp_task, UEF_P_C4_TIMEOUT))
                    return -13;
            }
        }
        break;

        /*---------------- 步骤11：等待A2文件头回复 ----------------*/
        case DPS_WAIT_A2:
        {
            if(b_dcac_check_wait_timeout(tp_task, 1000, us_cycle_time) == true)
            {
                tp_task->usStepWaitCnt = 0;
                if(b_dcac_check_retry_limit(tp_task, UEF_D_A2_TIMEOUT))
                {
                    lwrb_reset(&tp_task->tReplyBuff);
                    return -14;
                }

                //重新发送
                //校验数据
                u8 uca_buff[MEGMEET_FILE_HEAD_SIZE] = {0};
                if(us_reply_len != MEGMEET_FILE_HEAD_SIZE)
                {
                    bUpdate_SetErrCode(UEF_D_HEAD_LEN_ERR);
                    break;
                }

                //发送文件头
                if(b_dcac_update_buf_peek(tp_task, uca_buff, us_reply_len) == false)
                {
                    bUpdate_SetErrCode(UEF_D_HEAD_LEN_ERR);
                    break;
                }
                if(b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj), MEGMEET_CMD_FILE_HEAD, uca_buff, us_reply_len) == false)
                {
                    vTaskDelay(200);
                    break;
                }
            }
        }
        break;

        case DPS_FINISH_CLEANUP:
        {
            return 1;
        }

        default:
            break;
    }

    return 0;
}


/*****************************************************************************************************************
-----函数功能    DCAC固件传输处理（步骤4）
-----说明(备注)  根据升级阶段执行A3/A4/A5/A6数据交互，
                包括请求数据、发送固件包、等待回复、查询结果。
-----传入参数    tp_task      : 指向任务结构体的指针
                us_reply_len : 回复数据长度
                us_cycle_time: 任务调度周期
-----输出参数    none
-----返回值      正值:升级完成 0:继续  负值:处理失败
******************************************************************************************************************/
static s8 c_dcac_update_firmware_transfer(Task_T *tp_task, u16 us_reply_len, u16 us_cycle_time)
{
    switch(eDcacFwTransStage)
    {
        case DFTS_IDLE:
        {
            bDcac_SetFwTransStage(tp_task, DFTS_SLAVE_READY_OK);
        }
        break;

        case DFTS_SLAVE_READY_OK: /* 进入请求数据阶段，通知Print准备数据 */
        {
            bDcac_SetFwTransStage(tp_task, DFTS_HOST_REQ_DATA);
        }
        break;

        case DFTS_HOST_REQ_DATA: /* C6主机请求数据 */
        {
            if(c_print_cs_C6_req_cont_send() <= 0)
            {
                vTaskDelay(200);
                break;
            }
            bDcac_SetFwTransStage(tp_task, DFTS_WAIT_HOST_REPLY);
        }
        break;

        case DFTS_WAIT_HOST_REPLY:/* C5 等待主机数据*/
        {
            if(b_dcac_check_wait_timeout(tp_task, 1000, us_cycle_time) == true)
            {
                tp_task->usStepWaitCnt = 0;
                if(b_dcac_check_retry_limit(tp_task, UEF_D_C5_TIMEOUT))
                    return -9;

                //没有回复,继续请求
                c_print_cs_C6_req_cont_send();
            }
        }
        break;

        case DFTS_SEND_FW_DATA: /* 发送A3固件包数据 */
        {
            //校验数据
            u8 uca_buff[MEGMEET_FRM_PKG_SIZE] = {0};
            if(us_reply_len == 0 || us_reply_len > MEGMEET_FRM_PKG_SIZE)
            {
                if(b_dcac_check_retry_limit(tp_task, UEF_D_DATA_LEN_ERR))
                    return -11;
                bDcac_SetFwTransStage(tp_task, DFTS_HOST_REQ_DATA); /* 数据长度异常，返回上一步重新请求 */
                b_dcac_update_buf_reset(tp_task); /* 清空异常数据 */
                break;
            }

            //处理数据
            if(b_dcac_update_buf_peek(tp_task, uca_buff, us_reply_len) == false)
            {
                bDcac_SetFwTransStage(tp_task, DFTS_HOST_REQ_DATA); /* 缓存异常，重新请求 */
                break;
            }
            if(b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj), MEGMEET_CMD_FIRMWARE_DATA, uca_buff, us_reply_len) == false)
            {
                if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_A3_FAIL))
                    return -1;

                vTaskDelay(200);
                break;
            }
            bDcac_SetFwTransStage(tp_task, DFTS_WAIT_SLAVE_REPLY);
            break;
        }

        case DFTS_WAIT_SLAVE_REPLY: /* 等待A4固件包数据回复 */
        {
            if(b_dcac_check_wait_timeout(tp_task, dcacUPDATE_HS_TIMEOUT_MS, us_cycle_time) == true)
            {
                tp_task->usStepWaitCnt = 0;
                if(b_dcac_check_retry_limit(tp_task, UEF_D_RESEND_FAIL))
                    return -2;

                bDcac_SetFwTransStage(tp_task, DFTS_SEND_FW_DATA); //返回上一步继续
                break;
            }
        }
        break;

        case DFTS_QUERY_SLAVE_RESULT: /* 发送查询结果请求A5 */
        {
            if(b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj), MEGMEET_CMD_QUERY_RESULT, NULL, 0) == false)
            {
                if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_A5_FAIL))
                    return -6;

                vTaskDelay(200);
                break;
            }

            bDcac_SetFwTransStage(tp_task, DFTS_WAIT_SLAVE_RESULT_REPLY);
        }
        break;

        case DFTS_WAIT_SLAVE_RESULT_REPLY: /* 等待A6查询结果回复 */
        {
            tp_task->usStepWaitCnt++;
            if(tp_task->usStepWaitCnt >= (dcacUPDATE_HS_TIMEOUT_MS / us_cycle_time))
            {
                tp_task->usStepWaitCnt = 0;
                if(b_dcac_check_retry_limit(tp_task, UEF_D_A6_REPLY_ERR))
                    return -7;

                bDcac_SetFwTransStage(tp_task, DFTS_QUERY_SLAVE_RESULT); //返回上一步继续
                break;
            }
        }
        break;

        case DFTS_FINISH_CLEANUP: /* 升级完成，收尾 */
        {
            return 1;
        }

        default:
            break;
    }

    return 0;
}

/***********************************************************************************************************************
-----函数功能    获取升级阶段
-----说明(备注)  最终升级结果以tUpdate.eHostResult、tUpdate.eSlaveResult为准
-----传入参数    none
-----输出参数    none
-----返回值      UPDATE_QUEUE_STAGE_FINISH:升级完成，等待重新启动  
                UPDATE_QUEUE_STAGE_WAIT_RESTART:升级完成  
                UPDATE_QUEUE_STAGE_RUNNING:升级中
                -1:设备不在升级模式  -2:任务指针为空  -3:任务ID不匹配
************************************************************************************************************************/
s8 cDcac_GetUpdateStage(void)
{
	if(tSysInfo.eDevState != DS_UPDATE_MODE ||
	   !IS_DCAC_UPDATE_OBJ(tUpdate.eObj) ||
	   tDcac.eDevState != DS_UPDATE_MODE)
		return -1;

	if(tpDcacTask == NULL)
		return -2;

	if(tpDcacTask->ucID != DTI_UPDATE)
		return -3;

	if(tpDcacTask->ucStep == DUS_STEP_ERROR_CLEANUP)
		return UPDATE_QUEUE_STAGE_ERR;

	if(tpDcacTask->ucStep == DUS_STEP_END)
		return UPDATE_QUEUE_STAGE_WAIT_RESTART;

	if(tpDcacTask->ucStep > DUS_STEP_FINISH_CLEANUP)
		return UPDATE_QUEUE_STAGE_FINISH;

	return UPDATE_QUEUE_STAGE_RUNNING;
}

/***********************************************************************************************************************
-----函数功能    设置升级准备阶段
-----说明(备注)  设置升级准备阶段（F0/F1/F6/F7/延时/F2/F3）
-----传入参数    stage: 升级准备阶段
-----返回值      true:成功  false:失败
************************************************************************************************************************/
bool bDcac_SetPrepStage(Task_T *tp_task, DcacPrepStage_E stage)
{
    tp_task->usStepWaitCnt = 0;
    tp_task->usStepRepeatCnt = 0;

    if(stage != eDcacPrepStage)
    {
        eDcacPrepStage = stage;
        return true;
    }

	return false;
}

/***********************************************************************************************************************
-----函数功能    设置DCAC固件传输阶段
-----说明(备注)  设置DCAC固件传输阶段
-----传入参数    stage: DCAC固件传输阶段
-----返回值      true:成功  false:失败
************************************************************************************************************************/
bool bDcac_SetFwTransStage(Task_T *tp_task, DcacFwTransStage_E stage)
{
    tp_task->usStepWaitCnt = 0;
    tp_task->usStepRepeatCnt = 0;

    if(stage != eDcacFwTransStage)
    {
        eDcacFwTransStage = stage;
        return true;
    }

	return false;
}

#endif  //boardDCAC_EN
