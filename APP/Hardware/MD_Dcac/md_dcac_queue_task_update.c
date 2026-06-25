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

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_queue_task.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Megmeet/megmeet_proto.h"
#include "Baiku/baiku_proto.h"
#include "check.h"

/* ========================================== 宏定义 ========================================== */
#define        dcacTASK_UPDATE_CYCLE_TIME                 500     /*!< DCAC升级任务周期，单位：ms */

#define        dcacUPDATE_HS_TIMEOUT_MS                   1000    /*!< 握手阶段单次等待超时时间，单位：ms */
#define        dcacUPDATE_F7_WAIT_MS                      1000    /*!< 等待F7(跳转BOOT回复)超时时间，单位：ms */
#define        dcacUPDATE_BOOT_JUMP_DELAY_MS              500     /*!< BOOT跳转后等待稳定延时，单位：ms */

#define        dcacUPDATE_MAX_RETRY_COUNT                 3       /*!< 最大重试次数 */


/* ========================================== 静态函数声明 ========================================== */
static bool b_dcac_check_task_valid(Task_T *tp_task);
static s8 c_dcac_update_data_exchange(Task_T *tp_task, u16 us_reply_len);
static bool b_dcac_check_retry_limit(Task_T *tp_task, UpdateErrCode_E e_err_code);
static bool b_dcac_check_wait_timeout(Task_T *tp_task, u16 us_timeout_ms);
static bool b_dcac_check_wait_timeout_next(Task_T *tp_task, u16 us_timeout_ms);



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
    //true: 已收到任务通知，进入周期性执行流程；
    //false: 没有收到任务通知，可能是RTOS调度异常，直接返回不执行
    bool b_task_notify_flag = 0; /* 任务通知标志，确保在RTOS环境下正确等待周期 */
    s8 c_ret = 0;
    
    #if(boardUSE_OS)
    b_task_notify_flag = (ulTaskNotifyTake(pdTRUE, dcacTASK_UPDATE_CYCLE_TIME) > 0)? true : false;
    #endif
    
    /* 统一检查任务有效性（升级对象、缓冲区、设备状态、错误） */
    if(b_dcac_check_task_valid(tp_task) == false)
        return;

    //获取任务缓存器的返回值长度，判断是否有数据需要处理
    u16 us_reply_len = lwrb_get_full(&tp_task->tReplyBuff);

    switch(tp_task->ucStep)
    {
        /*---------------- 步骤0：初始化升级环境 ----------------*/
        case DCAC_UPDATE_STEP_INIT:
        {
            if((tDcacMegmeetProtoTx == NULL || tpDcacMegmeetProtoRx == NULL) && bDcac_MegmeetProtInit() == false)
            {
                if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_e("bDcacTask:Megmeet协议对象初始化失败");

                bUpdate_SetErrCode(UEF_D_SEND_F0_FAIL);
                break;
            }

            bDcac_SetDevState(DS_SHUT_DOWN);
            tUpdate.eSlaveResult = UTR_RUNNING;
            vUpdate_SetStage(NULL, DUS_IDLE); /* 复位升级阶段 */
            lwrb_reset(&tp_task->tReplyBuff);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }

        /*---------------- 步骤1：发送F0请求升级 ----------------*/
        case DCAC_UPDATE_STEP_SEND_F0:
        {
            if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_F0_FAIL))
                break;

            if(b_dcac_send_f0() == false)
                break;

            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        /*---------------- 步骤2：等待F1回复 ----------------*/
        case DCAC_UPDATE_STEP_WAIT_F1:
        {
            /* 收到F1确认，直接跳转到步骤6（设置波特率） */
            if(us_reply_len)
            {
                if(us_reply_len != 1)
                {
                    bUpdate_SetErrCode(UEF_D_F1_CHECK_FAIL);
                    break;
                }

                //读取数据
                u8 u_reply_param;
			    lwrb_read(&tp_task->tReplyBuff, (u8*)&u_reply_param, us_reply_len);

                if(u_reply_param != 0x01)
                {
                    bUpdate_SetErrCode(UEF_D_F1_CHECK_FAIL);
                    break;
                }
    
                cQueue_GotoStep(tp_task, DCAC_UPDATE_STEP_SEND_F2);
                break;
            }

            //超时下一步
            if(b_dcac_check_wait_timeout_next(tp_task, dcacUPDATE_HS_TIMEOUT_MS) == false)
                break;
        }

        /*---------------- 步骤3：发送F6请求跳转BOOT ----------------*/
        case DCAC_UPDATE_STEP_SEND_F6:
        {
            if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_F6_FAIL))
                break;

            if(b_dcac_send_f6() == false)
                break;

            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        /*---------------- 步骤4：等待F7回复 ----------------*/
        case DCAC_UPDATE_STEP_WAIT_F7:
        {
            //等待超时
            if(b_dcac_check_wait_timeout(tp_task, dcacUPDATE_F7_WAIT_MS))
            {
                cQueue_GotoStep(tp_task, STEP_FORWARD); /* 返回上一步继续发送 */
                break;
            }
            
            //处理接受数据
            if(b_task_notify_flag)
            {
                if(us_reply_len != 0)
                {
                    bUpdate_SetErrCode(UEF_D_F7_CHECK_FAIL);
                    break;
                }
                cQueue_GotoStep(tp_task, STEP_NEXT);
            }
            else
                break;
        }

        /*---------------- 步骤5：BOOT跳转后延时等待 ----------------*/
        case DCAC_UPDATE_STEP_BOOT_DELAY:
        {
            tp_task->usStepWaitCnt++;
            if(tp_task->usStepWaitCnt > (dcacUPDATE_BOOT_JUMP_DELAY_MS / dcacTASK_UPDATE_CYCLE_TIME))
                cQueue_GotoStep(tp_task, DCAC_UPDATE_STEP_SEND_F0); /* 延时结束，再次发送F0握手 */   
        }
        break;

        /*---------------- 步骤6：发送F2设置波特率 ----------------*/
        case DCAC_UPDATE_STEP_SEND_F2:
        {
            if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_F2_FAIL))
                break;

            /* 波特率已是目标值，直接进入上位机协议握手阶段 */
            if(tUpdate.ulBaud == dcacUSART_BAUD)
            {
                bDcac_SetDevState(DS_UPDATE_MODE);
                cQueue_GotoStep(tp_task, DCAC_UPDATE_STEP_SEND_A1);
                break;
            }

            if(b_dcac_send_f2(tUpdate.ulBaud) == false)
                break;

            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        /*---------------- 步骤7：等待F3回复 ------------------------*/
        case DCAC_UPDATE_STEP_WAIT_F3:
        {
            //等待超时
            if(b_dcac_check_wait_timeout(tp_task, dcacUPDATE_HS_TIMEOUT_MS))
            {
                cQueue_GotoStep(tp_task, STEP_FORWARD); /* 返回上一步继续发送 */
                break;
            }

            //处理接受数据
            if(us_reply_len)
            {
                if(us_reply_len != 1)
                {
                    bUpdate_SetErrCode(UEF_D_F3_CHECK_FAIL);
                    break;
                }

                //读取数据
                u8 u_reply_param;
                lwrb_read(&tp_task->tReplyBuff, (u8*)&u_reply_param, us_reply_len);

                /* 0xFF表示无对应波特率 */
                if(u_reply_param == MEGMEET_BAUD_INVALID)
                {
                    bUpdate_SetErrCode(UEF_D_F3_BAUD_REPLY);
                    break;
                }

                /* 0x00表示成功切换 */
                if(u_reply_param != MEGMEET_BAUD_OK)
                {
                    bUpdate_SetErrCode(UEF_D_F3_CHECK_FAIL);
                    break;
                }

                if(bDcac_IfaceSetBaud(tUpdate.ulBaud) == false)
                {
                    bUpdate_SetErrCode(UEF_D_F3_SET_BAUD);
                    break;
                }

                /* 从机已确认波特率切换，本地串口已在接收中断中完成切换 */
                bDcac_SetDevState(DS_UPDATE_MODE);
                cQueue_GotoStep(tp_task, STEP_NEXT);
            }
            else
                break;
        }

        /*---------------- 步骤8：发送A1文件头 ----------------*/
        case DCAC_UPDATE_STEP_SEND_A1:
        {
            if(tPrint.eDevState != DS_UPDATE_MODE)
                break;

            /* 重试超过3次，判定为失败 */
            if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_A1_FAIL))
                break;

            //校验数据
            u8 uca_buff[MEGMEET_FILE_HEAD_SIZE] = {0};
            if(us_reply_len != MEGMEET_FILE_HEAD_SIZE)
            {
                bUpdate_SetErrCode(UEF_D_HEAD_LEN_ERR);
                break;
            }

            //发送文件头
            lwrb_peek(&tp_task->tReplyBuff, 0, uca_buff, us_reply_len);
            if(b_dcac_send_megmeet_frame(0, MEGMEET_CMD_FILE_HEAD, uca_buff, us_reply_len) == false)
            {
                vTaskDelay(200);
                break;
            }
            
            cQueue_GotoStep(tp_task, STEP_NEXT); /* 发送成功，进入步骤9等待A2 */
            break;
        }

        /*---------------- 步骤9：等待A2文件头回复 ----------------*/
        case DCAC_UPDATE_STEP_WAIT_A2:
        {
            if(b_dcac_check_wait_timeout(tp_task, dcacUPDATE_HS_TIMEOUT_MS))
            {
                cQueue_GotoStep(tp_task, STEP_FORWARD); /* 返回上一步继续发送 */
                break;
            }

            //处理接受数据
            if(us_reply_len)
            {
                if(us_reply_len != 1)
                {
                    bUpdate_SetErrCode(UEF_D_A2_CHECK_FAIL);
                    break;
                }

                //读取数据
                u8 u_reply_param;
                lwrb_read(&tp_task->tReplyBuff, (u8*)&u_reply_param, us_reply_len);

                if(u_reply_param != MEGMEET_A2_OK &&
                   u_reply_param != MEGMEET_A2_VER_LATEST)
                {
                    bUpdate_SetErrCode(UEF_D_A2_REPLY_ERR);
                    break;
                }

                lwrb_reset(&tp_task->tReplyBuff);

                /* A2 已经是最新版本 */
                if(u_reply_param == MEGMEET_A2_VER_LATEST)
                {
                    tUpdate.eSlaveResult = UTR_LATEST;
                    cQueue_GotoStep(tp_task, DCAC_UPDATE_STEP_FINISH_CLEANUP);
                    break;
                }

                vUpdate_SetStage(tp_task, DUS_SLAVE_READY_OK);
                cQueue_GotoStep(tp_task, STEP_NEXT);
            }
            else
                break;
        }

        /*---------------- 步骤10：根据升级阶段执行数据交互（A3/A4/A5/A6） ----------------*/
        case DCAC_UPDATE_STEP_DATA_EXCHANGE:
        {
            c_ret = c_dcac_update_data_exchange(tp_task, us_reply_len);

            if(c_ret < 0)
                cQueue_GotoStep(tp_task, STEP_NEXT); /* 进入异常 */
            else if(c_ret > 0)
                cQueue_GotoStep(tp_task, DCAC_UPDATE_STEP_FINISH_CLEANUP); /* 升级完成 */
        }
        break;

        /*---------------- 步骤11：升级错误,收尾 ----------------*/
        case DCAC_UPDATE_STEP_ERROR_CLEANUP:
        {
            if(tUpdate.eErrCode == UEF_NONE)
                bUpdate_SetErrCode(UEF_D_RESEND_FAIL);

			tUpdate.eSlaveResult = UTR_FAIL;
            cQueue_GotoStep(tp_task, DCAC_UPDATE_STEP_END);
            break;
        }

        /*---------------- 步骤12：升级完成，收尾 ----------------*/
        case DCAC_UPDATE_STEP_FINISH_CLEANUP:
        {
            vDcac_IfaceInit(); /* 升级完成后重置接口状态，准备进入正常工作模式 */
            cQueue_GotoStep(tp_task, STEP_NEXT);
            break;
        }

        /*---------------- 步骤13：结束 ---------------------------*/
        case DCAC_UPDATE_STEP_END:
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
    /* 参数合法性检查：升级对象无效则结束任务 */
    if(tUpdate.eObj != UO_DCAC 
        || tp_task == NULL)
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
    if(cDcac_GetUpdateStage() != UPDATE_QUEUE_STAGE_ERR &&
       cDcac_GetUpdateStage() != UPDATE_QUEUE_STAGE_WAIT_RESTART &&
       tUpdate.eErrCode != UEF_NONE)
    {
        cQueue_GotoStep(tp_task, DCAC_UPDATE_STEP_ERROR_CLEANUP); /* 进入错误收尾阶段 */
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
-----输出参数    none
-----返回值      true: 已超时  false: 未超时
******************************************************************************************************************/
static bool b_dcac_check_wait_timeout(Task_T *tp_task, u16 us_timeout_ms)
{
    tp_task->usStepWaitCnt++;
    if(tp_task->usStepWaitCnt >= (us_timeout_ms / dcacTASK_UPDATE_CYCLE_TIME))
    {
        return true;
    }
    return false;
}

/*****************************************************************************************************************
-----函数功能    检查等待是否超时（跳转到STEP_NEXT）
-----说明(备注)  统一的等待超时检查逻辑，增加等待计数器并检查是否超时，超时后跳转到STEP_NEXT
-----传入参数    tp_task      : 指向任务结构体的指针
                us_timeout_ms: 超时时间（单位：ms）
-----输出参数    none
-----返回值      true: 已超时  false: 未超时
******************************************************************************************************************/
static bool b_dcac_check_wait_timeout_next(Task_T *tp_task, u16 us_timeout_ms)
{
    tp_task->usStepWaitCnt++;
    if(tp_task->usStepWaitCnt >= (us_timeout_ms / dcacTASK_UPDATE_CYCLE_TIME))
    {
        cQueue_GotoStep(tp_task, STEP_NEXT); /* 超时，跳转到下一步 */
        return true;
    }
    return false;
}


/*****************************************************************************************************************
-----函数功能    DCAC升级数据交互处理（步骤10）
-----说明(备注)  根据升级阶段执行A3/A4/A5/A6数据交互，
                包括请求数据、发送固件包、等待回复、查询结果。
-----传入参数    tp_task      : 指向任务结构体的指针
                us_reply_len : 回复数据长度
-----输出参数    none
-----返回值      正值:升级完成 0:继续  负值:处理失败
******************************************************************************************************************/
static s8 c_dcac_update_data_exchange(Task_T *tp_task, u16 us_reply_len)
{
    switch(tUpdate.ucStage)
    {
        case DUS_SLAVE_READY_OK: /* 进入请求数据阶段，通知Print准备数据 */
        {
            vUpdate_SetStage(tp_task, DUS_HOST_REQ_DATA);
            break;
        }

        case DUS_SLAVE_SEND_DATA: /* 发送A3固件包数据 */
        {
            //校验数据
            u8 uca_buff[MEGMEET_FRM_PKG_SIZE] = {0};
            if(us_reply_len == 0 || us_reply_len > MEGMEET_FRM_PKG_SIZE)
            {
                vUpdate_SetStage(tp_task, DUS_HOST_REQ_DATA); /* 数据长度异常，返回上一步重新请求 */
                lwrb_reset(&tp_task->tReplyBuff); /* 清空异常数据 */
                break;
            }

            //处理数据
            lwrb_peek(&tp_task->tReplyBuff, 0, uca_buff, us_reply_len);
            if(b_dcac_send_megmeet_frame(0, MEGMEET_CMD_FIRMWARE_DATA, uca_buff, us_reply_len) == false)
            {
                if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_A3_FAIL))
                    return -1;

                vTaskDelay(200);
                break;
            }
            vUpdate_SetStage(tp_task, DUS_WAIT_SLAVE_REPLY);
            break;
        }

        case DUS_WAIT_SLAVE_REPLY: /* 等待A4固件包数据回复 */
        {
            tp_task->usStepWaitCnt++;
            if(tp_task->usStepWaitCnt >= (dcacUPDATE_HS_TIMEOUT_MS / dcacTASK_UPDATE_CYCLE_TIME))
            {
                tp_task->usStepWaitCnt = 0;
                if(b_dcac_check_retry_limit(tp_task, UEF_D_RESEND_FAIL))
                    return -2;

                vUpdate_SetStage(tp_task, DUS_SLAVE_SEND_DATA); //返回上一步继续
                break;
            }

            //处理接受数据
            if(us_reply_len)
            {
                if(us_reply_len != 3)
                {
                    bUpdate_SetErrCode(UEF_D_A4_CHECK_FAIL);
                    return -3;
                }

                //读取数据
                #pragma pack(1)
                struct {
                    u16 usSeqNum;
                    u8  ucStatus;
                } u_reply_param;
                #pragma pack()

                lwrb_read(&tp_task->tReplyBuff, (u8*)&u_reply_param, us_reply_len);

                /* 校验包序号是否匹配 */
                if(u_reply_param.usSeqNum != tUpdate.usRecFrameCnt)
                {
                    bUpdate_SetErrCode(UEF_D_A4_SEQ_MISMATCH);
                    return -4;
                }

                if(u_reply_param.ucStatus != MEGMEET_A4_OK &&
                   u_reply_param.ucStatus != MEGMEET_A4_ALL_OK)
                {
                    bUpdate_SetErrCode(UEF_D_A4_REPLY_ERR);
                    return -5;
                }

                lwrb_reset(&tp_task->tReplyBuff);

                if(tUpdate.usRecFrameCnt < 0xFFFF)
                    tUpdate.usRecFrameCnt++;

                if(tUpdate.usPendPacketLen == 0)
                    return 0;

                tUpdate.ulFwCalcCrc32 = tUpdate.ulFwPendCrc32;
                tUpdate.ulRxSize += tUpdate.usPendPacketLen;
                tUpdate.ulFwPendCrc32 = tUpdate.ulFwCalcCrc32;
                tUpdate.usPendPacketLen = 0;

                /* A4 已经全部完成 */
                if(u_reply_param.ucStatus == MEGMEET_A4_ALL_OK)
                {
                    vUpdate_SetStage(tp_task, DUS_GET_SLAVE_RESULT);
                    break;
                }

                //Print已经结束
                if(tUpdate.eHostResult == UTR_OK || tUpdate.eHostResult == UTR_CANCEL)
                {
                    vUpdate_SetStage(tp_task, DUS_GET_SLAVE_RESULT);
                    break;
                }

                /* 通知Print请求下一包数据 */
                vUpdate_SetStage(tp_task, DUS_HOST_REQ_DATA);
            }
        }
        break;

        case DUS_GET_SLAVE_RESULT: /* 发送查询结果请求A5 */
        {
            if(b_dcac_send_megmeet_frame(0, MEGMEET_CMD_QUERY_RESULT, NULL, 0) == false)
            {
                if(b_dcac_check_retry_limit(tp_task, UEF_D_SEND_A5_FAIL))
                    return -6;

                vTaskDelay(200);
                break;
            }

            tp_task->usStepWaitCnt = 0;
            vUpdate_SetStage(tp_task, DUS_WAIT_SLAVE_RESULT_REPLY);
        }
        break;

        case DUS_WAIT_SLAVE_RESULT_REPLY: /* 等待A6查询结果回复 */
        {
            tp_task->usStepWaitCnt++;
            if(tp_task->usStepWaitCnt >= (dcacUPDATE_HS_TIMEOUT_MS / dcacTASK_UPDATE_CYCLE_TIME))
            {
                tp_task->usStepWaitCnt = 0;
                if(b_dcac_check_retry_limit(tp_task, UEF_D_A6_REPLY_ERR))
                    return -7;

                vUpdate_SetStage(tp_task, DUS_GET_SLAVE_RESULT); //返回上一步继续
                break;
            }

            //处理接受数据
            if(us_reply_len)
            {
                if(us_reply_len != 3)
                {
                    bUpdate_SetErrCode(UEF_D_A6_CHECK_FAIL);
                    return -8;
                }

                //读取数据
                #pragma pack(1)
                struct {
                    u8  ucStatus;
                    u8  ucSlaveAddr;
                    u8  ucChipId;
                } u_reply_param;
                #pragma pack()

                lwrb_read(&tp_task->tReplyBuff, (u8*)&u_reply_param, us_reply_len);
                lwrb_reset(&tp_task->tReplyBuff);

                /* 校验从机地址和芯片ID */
                if(u_reply_param.ucSlaveAddr != 0 || 
                   u_reply_param.ucChipId != MEGMEET_IC_TYPE_DC)
                {
                    bUpdate_SetErrCode(UEF_D_A6_CHECK_FAIL);
                    return -9;
                }

                //回复错误
                if(u_reply_param.ucStatus > 100 &&
                   u_reply_param.ucStatus != MEGMEET_A6_VER_LATEST)
                {
                    bUpdate_SetErrCode(UEF_D_A6_REPLY_ERR);
                    return -10;
                }

                //回复未升级完成
                if(u_reply_param.ucStatus < 100)
                {
                    tUpdate.eSlaveResult = UTR_RUNNING;
                    vUpdate_SetStage(tp_task, DUS_GET_SLAVE_RESULT);
                    break;
                }

                /* A6 已经是最新 */
                if(u_reply_param.ucStatus == MEGMEET_A6_VER_LATEST)
                {
                    tUpdate.eSlaveResult = UTR_LATEST;
                    tUpdate.usRecFrameCnt = 100;
                }
                else
                {
                    tUpdate.eSlaveResult = UTR_OK;
                    tUpdate.usRecFrameCnt = u_reply_param.ucStatus;
                }

                /* 升级完成 */
                tUpdate.usTotalFrmValue = 100;
                vUpdate_SetStage(tp_task, DUS_HOST_REQ_DATA);
                return 1;
            }
        }
        break;

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
	   tUpdate.eObj != UO_DCAC ||
	   tDcac.eDevState != DS_UPDATE_MODE)
		return -1;

	if(tpDcacTask == NULL)
		return -2;

	if(tpDcacTask->ucID != DTI_UPDATE)
		return -3;

	if(tpDcacTask->ucStep == DCAC_UPDATE_STEP_ERROR_CLEANUP)
		return UPDATE_QUEUE_STAGE_ERR;

	if(tpDcacTask->ucStep == DCAC_UPDATE_STEP_END)
		return UPDATE_QUEUE_STAGE_WAIT_RESTART;

	if(tpDcacTask->ucStep > DCAC_UPDATE_STEP_FINISH_CLEANUP)
		return UPDATE_QUEUE_STAGE_FINISH;

	return UPDATE_QUEUE_STAGE_RUNNING;
}

#endif  //boardDCAC_EN
