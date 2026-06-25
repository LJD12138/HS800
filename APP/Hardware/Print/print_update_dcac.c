/*******************************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\11-G24\1.software\G2404-3\APP\Hardware\Print
 * File    : print_update_dcac.c
 * Date    : 2026-05-18 18:27:19
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "Print/print_queue_task_update.h"
#include "function.h"
#include "check.h"

#if(boardUPDATE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_queue_task_update.h"


//****************************************************Macros*******************************************************************//
#define        printUPDATE_DCAC_FILE_HEAD_MAGIC_OFFSET     0
#define        printUPDATE_DCAC_FILE_HEAD_MAGIC_SWAP       0x4D475350UL
#define        printUPDATE_DCAC_FILE_HEAD_CRC_OFFSET       4
#define        printUPDATE_DCAC_FILE_HEAD_IC_OFFSET        8
#define        printUPDATE_DCAC_FILE_HEAD_SIZE_OFFSET      48


//****************************************************Parameter Initialization************************************************//



//****************************************************Function Declaration****************************************************//
static bool b_print_dcac_forward_to_dcac(const u8* data, u16 len);
static bool b_print_dcac_parse_file_head(const u8* data, u16 len);
static bool b_print_dcac_check_data_packet(BaikuProtoRx_t* proto, u32* ulp_next_crc_state);


/***********************************************************************************************************************
-----函数功能    DCAC进入数据透传前的准备流程
-----说明(备注)  处理上位机传输协议握手(C4/C2/C3)，握手完成后再进入文件头阶段
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      1:准备完成  0:等待中  负值:准备失败
************************************************************************************************************************/
s8 c_print_dcac_prepare_update(Task_T* tp_task)
{
    s8 c_ret = 0;

    if(tp_task == NULL)
    {
        bUpdate_SetErrCode(UEF_P_TASK_NULL);
        return -1;
    }

    if(us_char_send_dev_len)
    {
        c_ret = cBaiku_ProtoCheck(tpPrintProtoRx);
        if(c_ret > 0)
        {
            switch(tpPrintProtoRx->ucCmd)
            {
                case baikuCMD_SET_PROTO:    /* C2 主机设置升级协议 */
                {
                    if(uc_print_ready_update_step != 1)
                        return 0;

                    if(tpPrintProtoRx->ucValidLen != 3 || tpPrintProtoRx->ucpValidData == NULL)
                    {
                        bUpdate_SetErrCode(UEF_P_C2_DATA_ERR);
                        return -4;
                    }

                    memcpy((u8*)&tUpdate.usTotalFrmValue, &tpPrintProtoRx->ucpValidData[1], 2);
 
                    /* 选择Baiku协议并回复确认 */
                    cUpdate_ProtoSelect(UO_DCAC, PT_BAIKU);
                    if(c_relayC3_reply_set_proto(tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen) <= 0)
                    {
                        bUpdate_SetErrCode(UEF_P_C3_REPLY_FAIL);
                        return -5;
                    }

                    uc_print_ready_update_step++;
                    tp_task->usStepWaitCnt = 0;
                    tp_task->usStepRepeatCnt = 0;
                }
                break;

                case baikuCMD_REPLY_DATA:   /* C5 主机下发文件头数据 */
                {
                    if(uc_print_ready_update_step != 3)
                        return 0;

                    /* 数据内容为空，请求重发 */
                    if(tpPrintProtoRx->ucValidLen != 56 || tpPrintProtoRx->ucpValidData == NULL)
                    {
                        bUpdate_SetErrCode(UEF_P_C5_DATA_ERR);
                        return -10;
                    }

                    /* 解析DCAC升级文件头 */
                    if(b_print_dcac_parse_file_head(tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen) == false)
                    {
                        bUpdate_SetErrCode(UEF_P_HEAD_PARSE_FAIL);
                        return -11;
                    }

                    // 转发到DCAC任务
                    if(b_print_dcac_forward_to_dcac(tpPrintProtoRx->ucpValidData,
                                                    tpPrintProtoRx->ucValidLen) == false)
                    {
                        bUpdate_SetErrCode(UEF_P_FWD_DCAC_FAIL);
                        return -12;
                    }

                    tp_task->usStepWaitCnt = 0;
                    tp_task->usStepRepeatCnt = 0;
                    return 1;   /* 准备完成，进入下一个阶段 */
                }

                case baikuCMD_REPLY_CANEL:  /* C8 主机取消升级 */
                {
                    //已经成功了
                    if(tUpdate.eHostResult == UTR_OK
                       || tUpdate.eHostResult == UTR_LATEST)
                        return 1;

                    bUpdate_SetErrCode(UEF_P_CANCEL_REQ);
                    tUpdate.eHostResult = UTR_CANCEL;

                    #if(boardUSE_OS)
                    /* 通过任务通知唤醒DCAC任务 */
                    if(tDcacTaskHandler != NULL)
                        xTaskNotifyGive(tDcacTaskHandler);
                    #endif
                    return 1;
                }

                default:
                    break;
            }
        }
    }

    switch(uc_print_ready_update_step)
    {
        case 0://发送C4握手命令
        case 2://再次发送C4获取文件头
        {
            //第一次不需要延时
            if(uc_print_ready_update_step == 0)
                tp_task->usStepWaitCnt = 200 / printTASK_UPDATE_CYCLE_TIME;

            tp_task->usStepWaitCnt++;
            if(tp_task->usStepWaitCnt < (200 / printTASK_UPDATE_CYCLE_TIME))
                break;

            tp_task->usStepWaitCnt = 0;
            if(c_relayC4_req_start_send() <= 0)
                break;

            uc_print_ready_update_step++;
        }

        case 1://等待C2回复
        case 3://等待C5回复
        {
            tp_task->usStepWaitCnt++;
            if(tp_task->usStepWaitCnt >= (1000 / printTASK_UPDATE_CYCLE_TIME))
            {
                tp_task->usStepWaitCnt = 0;

                tp_task->usStepRepeatCnt++;
                if(tp_task->usStepRepeatCnt > 3)
                {
                    tp_task->usStepRepeatCnt = 0;
                    bUpdate_SetErrCode(UEF_P_C4_TIMEOUT);
                    return -13;
                }

                uc_print_ready_update_step--;
            }
        }
        break;

        default:
            break;
    }

    return 0;
}

/***********************************************************************************************************************
-----函数功能    DCAC升级主流程控制
-----说明(备注)  管理DCAC固件升级各阶段状态，处理与主机及DCAC模块的数据交互
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      正值:升级完成 0:继续  负值:处理失败
************************************************************************************************************************/
s8 c_print_update_dcac(Task_T *tp_task)
{
    s8 c_ret = 0;
    UpdateTaskResult_E e_slave_result = tUpdate.eSlaveResult;
    bool b_can_query_now = false;

    /* DCAC任务未初始化或缓存未分配 */
    if(tpDcacTask == NULL)
    {
        bUpdate_SetErrCode(UEF_P_TASK_NULL);
        return -1;
    }

    if(tpDcacTask->tReplyBuff.buff == NULL)
    {
        bUpdate_SetErrCode(UEF_P_BUFF_NULL);
        return -1;
    }

    if(e_slave_result != UTR_OK 
        && e_slave_result != UTR_LATEST
        && e_slave_result != UTR_RUNNING)
    {
        bUpdate_SetErrCode(UEF_P_SLAVE_RESULT_ERR);
        return -1;
    }

    /* 处理上位机的数据 */
    if(us_char_send_dev_len)
    {
        /* 读取数据并解析主机命令帧 */
        c_ret = cBaiku_ProtoCheck(tpPrintProtoRx);
        if(c_ret > 0)
        {
            vUpdate_ResetTimeout();

            /* 根据主机命令码分发处理 */
            switch(tpPrintProtoRx->ucCmd)
            {
                case baikuCMD_REPLY_DATA:   /* C5 主机下发固件数据 */
                {
                    if(tUpdate.ucStage != DUS_WAIT_HOST_REPLY)
                        return 0;

                    /* 数据内容为空，请求重发 */
                    if(tpPrintProtoRx->ucpValidData == NULL || tpPrintProtoRx->ucValidLen == 0)
                        return -10;

                    /* 校验数据包合法性 */
                    u32 ul_next_crc_state = 0;
                    if(b_print_dcac_check_data_packet(tpPrintProtoRx, &ul_next_crc_state) == false)
                        return 0;//等待超时继续发送

                    /* 转发数据到DCAC模块 */
                    if(b_print_dcac_forward_to_dcac(tpPrintProtoRx->ucpValidData,
                                                    tpPrintProtoRx->ucValidLen) == false)
                        return -11;

                    /* 当前包先挂起，待 A4 成功后再提交 */
                    tUpdate.ulFwPendCrc32 = ul_next_crc_state;
                    tUpdate.usPendPacketLen = tpPrintProtoRx->ucValidLen;
                    vUpdate_SetStage(tp_task, DUS_SLAVE_SEND_DATA);
                }
                break;

                case baikuCMD_REPLY_FINISH: /* C7 主机发送完成帧 */
                {
                    b_can_query_now = (tUpdate.ucStage != DUS_SLAVE_SEND_DATA &&
                                       tUpdate.ucStage != DUS_WAIT_SLAVE_REPLY &&
                                       tUpdate.ucStage != DUS_WAIT_SLAVE_RESULT_REPLY);

                    if(b_can_query_now)
                        vUpdate_SetStage(tp_task, DUS_GET_SLAVE_RESULT);

                    #if(boardUSE_OS)
                    /* 通过任务通知唤醒DCAC任务 */
                    if(tDcacTaskHandler != NULL)
                        xTaskNotifyGive(tDcacTaskHandler);
                    #endif
                    return 1;
                }

                case baikuCMD_REPLY_CANEL:  /* C8 主机取消升级 */
                {
                    b_can_query_now = (tUpdate.ucStage != DUS_SLAVE_SEND_DATA &&
                                       tUpdate.ucStage != DUS_WAIT_SLAVE_REPLY &&
                                       tUpdate.ucStage != DUS_WAIT_SLAVE_RESULT_REPLY);

                    bUpdate_SetErrCode(UEF_P_CANCEL_REQ);

                    if(b_can_query_now)
                        vUpdate_SetStage(tp_task, DUS_GET_SLAVE_RESULT);

                    #if(boardUSE_OS)
                    /* 通过任务通知唤醒DCAC任务 */
                    if(tDcacTaskHandler != NULL)
                        xTaskNotifyGive(tDcacTaskHandler);
                    #endif
                    return -1;
                }

                default:
                    break;
            }
        }
    }

    /* DCAC升级状态机 */
    switch(tUpdate.ucStage)
    {
        case DUS_HOST_REQ_DATA: /* 主机请求数据 */
        {
            if(c_relayC6_req_cont_send() <= 0)
            {
                vTaskDelay(200);
                break;
            }
            vUpdate_SetStage(tp_task, DUS_WAIT_HOST_REPLY);
        }
        break;

        case DUS_WAIT_HOST_REPLY:/* 等待主机数据*/
        {
            //等待超时,用C4请求重发当前帧
            tp_task->usStepWaitCnt++;
            if(tp_task->usStepWaitCnt >= (1000 / printTASK_UPDATE_CYCLE_TIME))
            {
                tp_task->usStepWaitCnt = 0;
                tp_task->usStepRepeatCnt++;
                if(tp_task->usStepRepeatCnt > 3)
                    return -9;

                c_relayC4_req_resend_curr();
            }
        }
        break;

        default:
            break;
    }

    return 0;
}



/***********************************************************************************************************************
-----函数功能    转发数据到DCAC任务
-----说明(备注)  将接收到的升级数据写入DCAC任务回复缓存，并切换升级阶段
-----传入参数    data: 数据指针
                len: 数据长度
-----输出参数    none
-----返回值      true:转发成功  false:转发失败
************************************************************************************************************************/
static bool b_print_dcac_forward_to_dcac(const u8* data, u16 len)
{
    /* 参数及DCAC任务有效性检查 */
    if(data == NULL || len == 0 || tpDcacTask == NULL || tpDcacTask->tReplyBuff.buff == NULL)
        return false;

    /* 检查DCAC缓存空间是否足够 */
    if(len > lwrb_get_free(&tpDcacTask->tReplyBuff))
        return false;

    /* 写入数据到DCAC缓存，切换升级阶段 */
    lwrb_reset(&tpDcacTask->tReplyBuff);
    lwrb_write(&tpDcacTask->tReplyBuff, data, len);

    return true;
}

/***********************************************************************************************************************
-----函数功能    解析DCAC升级文件头
-----说明(备注)  校验文件魔数、IC类型，提取固件大小和CRC32值
-----传入参数    data: 文件头数据指针
                len: 数据长度
-----输出参数    none
-----返回值      true:解析成功  false:解析失败
************************************************************************************************************************/
static bool b_print_dcac_parse_file_head(const u8* data, u16 len)
{
    u32 ul_magic = 0;
    u8 uc_ic_type = 0;

    /* 检查数据指针和长度 */
    if(data == NULL || len != MEGMEET_FILE_HEAD_SIZE)
        return false;

    /* 校验文件魔数 */
    ul_magic = ulFunc_GetLe32(&data[printUPDATE_DCAC_FILE_HEAD_MAGIC_OFFSET]);
    if(ul_magic != MEGMEET_FILE_MAGIC && ul_magic != printUPDATE_DCAC_FILE_HEAD_MAGIC_SWAP)
        return false;

    /* 校验IC类型 */
    uc_ic_type = data[printUPDATE_DCAC_FILE_HEAD_IC_OFFSET];
    if(uc_ic_type != MEGMEET_IC_TYPE_DC && uc_ic_type != MEGMEET_IC_TYPE_AC)
        return false;

    /* 提取固件CRC32和大小，初始化接收状态 */
    tUpdate.ulFwCrc32 = ulFunc_GetLe32(&data[printUPDATE_DCAC_FILE_HEAD_CRC_OFFSET]);
    tUpdate.ulFwSize = ulFunc_GetLe32(&data[printUPDATE_DCAC_FILE_HEAD_SIZE_OFFSET]);
    tUpdate.ulRxSize = 0;
    tUpdate.ulFwCalcCrc32 = 0xFFFFFFFFUL;   /* CRC初始值 */
    tUpdate.ulFwPendCrc32 = tUpdate.ulFwCalcCrc32;
    tUpdate.usPendPacketLen = 0;
    tUpdate.usRecFrameCnt = 0;
    /* 计算总帧数，至少为1帧 */
    tUpdate.usTotalFrmValue = (u16)((tUpdate.ulFwSize + MEGMEET_FRM_PKG_SIZE - 1) / MEGMEET_FRM_PKG_SIZE);
    if(tUpdate.usTotalFrmValue == 0)
        tUpdate.usTotalFrmValue = 1;

    return (tUpdate.ulFwSize != 0);
}

/***********************************************************************************************************************
-----函数功能    校验DCAC数据包
-----说明(备注)  校验序列号、包长度、数据边界及CRC32连续性
-----传入参数    proto: 拜库协议接收结构体指针
                ulp_next_crc_state: 下一CRC状态输出指针
-----输出参数    ulp_next_crc_state: 更新后的CRC状态
-----返回值      true:校验通过  false:校验失败
************************************************************************************************************************/
static bool b_print_dcac_check_data_packet(BaikuProtoRx_t* proto, u32* ulp_next_crc_state)
{
    u32 ul_remaining = 0;
    u32 ul_next_crc_state = 0;
    u16 len = 0;

    /* 参数有效性检查 */
    if(proto == NULL || proto->ucpValidData == NULL || proto->ucValidLen == 0 || ulp_next_crc_state == NULL)
        return false;

    /* 校验帧序列号 */
    if(proto->ucSN != ((tUpdate.usRecFrameCnt + 1) & 0x00ff))
        return false;

    len = proto->ucValidLen;
    if(len > MEGMEET_FRM_PKG_SIZE)
        return false;

    /* 校验数据长度与固件大小的边界关系 */
    if(tUpdate.ulFwSize != 0)
    {
        /* 已接收大小不能超过固件总大小 */
        if(tUpdate.ulRxSize >= tUpdate.ulFwSize)
            return false;

        ul_remaining = tUpdate.ulFwSize - tUpdate.ulRxSize;
        if(len > ul_remaining)
            return false;

        /* 非最后一包时，长度必须为标准包大小或192字节 */
        if(ul_remaining > MEGMEET_FRM_PKG_SIZE && len != MEGMEET_FRM_PKG_SIZE && len != 192)
            return false;

        /* 最后一包长度必须等于剩余字节数 */
        if(ul_remaining <= MEGMEET_FRM_PKG_SIZE && len != ul_remaining)
            return false;
    }

    /* 更新CRC32并校验最终CRC */
    ul_next_crc_state = ulCheck_Crc32Update(tUpdate.ulFwCalcCrc32, proto->ucpValidData, len);
    if(tUpdate.ulFwSize != 0 && (tUpdate.ulRxSize + len) == tUpdate.ulFwSize)
    {
        /* 最终CRC需取反后与文件头CRC比对 */
        if((ul_next_crc_state ^ 0xFFFFFFFFUL) != tUpdate.ulFwCrc32)
            return false;
    }

    *ulp_next_crc_state = ul_next_crc_state;
    return true;
}


#endif  //boardUPDATE
