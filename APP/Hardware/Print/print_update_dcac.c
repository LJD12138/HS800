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
#include "MD_Dcac/md_dcac_prot_frame.h"


//****************************************************Macros*******************************************************************//
#define        printUPDATE_DCAC_FILE_HEAD_MAGIC_OFFSET     0
#define        printUPDATE_DCAC_FILE_HEAD_MAGIC_SWAP       0x4D475350UL
#define        printUPDATE_DCAC_FILE_HEAD_CRC_OFFSET       4
#define        printUPDATE_DCAC_FILE_HEAD_IC_OFFSET        8
#define        printUPDATE_DCAC_FILE_HEAD_SIZE_OFFSET      48


//****************************************************Parameter Initialization************************************************//



//****************************************************Function Declaration****************************************************//
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
                    if(eDcacPrepStage != DPS_WAIT_PRINT_UPDATE_REQ)
                        return 0;

                    if(tpPrintProtoRx->ucValidLen != 3 || tpPrintProtoRx->ucpValidData == NULL)
                    {
                        bUpdate_SetErrCode(UEF_P_C2_DATA_ERR);
                        return -4;
                    }

                    /* 校验主机请求的协议类型，当前Print通道仅支持Baiku协议 */
                    if((ProtoType_E)tpPrintProtoRx->ucpValidData[0] != PT_BAIKU)
                    {
                        bUpdate_SetErrCode(UEF_P_C2_DATA_ERR);
                        return -4;
                    }

                    memcpy((u8*)&tUpdate.usTotalFrmValue, &tpPrintProtoRx->ucpValidData[1], 2);

                    /* 选择Baiku协议并回复确认（按当前升级对象选择协议） */
                    cUpdate_ProtoSelect(tUpdate.eObj, PT_BAIKU);
                    if(c_print_cs_C3_reply_set_proto(tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen) <= 0)
                    {
                        bUpdate_SetErrCode(UEF_P_C2_REPLY_FAIL);
                        return -5;
                    }

                    bDcac_SetPrepStage(tpDcacTask,DPS_PRINT_SEND_C4);
                }
                break;

                case baikuCMD_REPLY_DATA:   /* C5 主机下发文件头数据 */
                {
                    if(eDcacPrepStage != DPS_PRINT_WAIT_REPLY_C5)
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

                    /* 检查DCAC缓存空间是否足够（先检查，避免已下发却无法缓存导致无法重发） */
                    if(tpPrintProtoRx->ucValidLen > lwrb_get_free(&tpDcacTask->tReplyBuff))
                    {
                        bUpdate_SetErrCode(UEF_P_FWD_DCAC_FAIL);
                        return -12;
                    }

                    /* 写入数据到DCAC缓存，用于A2超时后重新发送 */
                    if(b_dcac_update_buf_write(tpDcacTask, tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen) == false)
                    {
                        bUpdate_SetErrCode(UEF_P_FWD_DCAC_FAIL);
                        return -13;
                    }

                    // 直接下发给DCAC模块
                    //发送文件头,如果失败延时200ms重试3次
                    bool b_send_ok = false;
                    for(int i = 0; i < 3; i++)
                    {
                        if(b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj), MEGMEET_CMD_FILE_HEAD, tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen))
                        {
                            b_send_ok = true;
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(200));
                    }

                    if(b_send_ok == false)
                    {
                        bUpdate_SetErrCode(UEF_P_FWD_DCAC_FAIL);
                        return -13;
                    }

                    /* 文件头已下发，切换至等待A2回复阶段 */
                    bDcac_SetPrepStage(tpDcacTask, DPS_WAIT_A2);

                    return 1;   /* 准备完成，进入下一个阶段 */
                }

                case baikuCMD_REPLY_CANEL:  /* C8 主机取消升级 */
                {
                    //已经成功了
                    if(tUpdate.eHostResult == UTR_OK
                       || tUpdate.eHostResult == UTR_LATEST)
                        return 1;

                    bUpdate_SetErrCode(UEF_P_CANCEL_REQ);
                    bUpdate_SetResult(URT_HOST, UTR_CANCEL);

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

    return 0;
}

/***********************************************************************************************************************
-----函数功能    固件传输阶段
-----说明(备注)  管理DCAC固件升级各阶段状态，处理与主机及DCAC模块的数据交互
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      正值:传输完成 0:继续  负值:处理失败
************************************************************************************************************************/
s8 c_print_dcac_update_firmware_transfer(Task_T *tp_task)
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
                    if(eDcacFwTransStage != DFTS_WAIT_HOST_REPLY)
                        return 0;

                    /* 数据内容为空，请求重发 */
                    if(tpPrintProtoRx->ucpValidData == NULL || tpPrintProtoRx->ucValidLen == 0)
                        return -10;

                    /* 校验数据包合法性 */
                    u32 ul_next_crc_state = 0;
                    if(b_print_dcac_check_data_packet(tpPrintProtoRx, &ul_next_crc_state) == false)
                        return 0;//等待超时继续发送

                    // 直接下发给DCAC模块
                    //发送固件数据,如果失败延时200ms重试3次
                    bool b_send_ok = false;
                    for(int i = 0; i < 3; i++)
                    {
                        if(b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj), MEGMEET_CMD_FIRMWARE_DATA, tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen))
                        {
                            b_send_ok = true;
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(200));
                    }

                    if(b_send_ok == false)
                    {
                        bUpdate_SetErrCode(UEF_P_FWD_DCAC_FAIL);
                        return -13;
                    }

                    /* 写入数据到DCAC缓存，用于A4超时后重新发送 */
                    if(b_dcac_update_buf_write(tpDcacTask, tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen) == false)
                    {
                        bUpdate_SetErrCode(UEF_P_FWD_DCAC_FAIL);
                        return -12;
                    }

                    /* 当前包先挂起，待 A4 成功后再提交 */
                    #if(boardUSE_OS)
                    taskENTER_CRITICAL();
                    #endif
                    tUpdate.ulFwPendCrc32 = ul_next_crc_state;
                    tUpdate.usPendPacketLen = tpPrintProtoRx->ucValidLen;
                    #if(boardUSE_OS)
                    taskEXIT_CRITICAL();
                    #endif
                    bDcac_SetFwTransStage(tp_task, DFTS_WAIT_SLAVE_REPLY);
                }
                break;

                case baikuCMD_REPLY_FINISH: /* C7 主机发送完成帧 */
                {
                    b_can_query_now = (eDcacFwTransStage != DFTS_SEND_FW_DATA &&
                                        eDcacFwTransStage != DFTS_WAIT_SLAVE_REPLY &&
                                        eDcacFwTransStage != DFTS_WAIT_SLAVE_RESULT_REPLY);

                    if(b_can_query_now)
                        bDcac_SetFwTransStage(tp_task, DFTS_QUERY_SLAVE_RESULT);

                    #if(boardUSE_OS)
                    /* 通过任务通知唤醒DCAC任务 */
                    if(tDcacTaskHandler != NULL)
                        xTaskNotifyGive(tDcacTaskHandler);
                    #endif
                    return 1;
                }

                case baikuCMD_REPLY_CANEL:  /* C8 主机取消升级 */
                {
                    b_can_query_now = (eDcacFwTransStage != DFTS_SEND_FW_DATA &&
                                        eDcacFwTransStage != DFTS_WAIT_SLAVE_REPLY &&
                                        eDcacFwTransStage != DFTS_WAIT_SLAVE_RESULT_REPLY);

                    bUpdate_SetErrCode(UEF_P_CANCEL_REQ);

                    if(b_can_query_now)
                        bDcac_SetFwTransStage(tp_task, DFTS_QUERY_SLAVE_RESULT);

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
    return 0;
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

    /* 校验IC类型，必须与当前升级对象对应的芯片ID一致（AC/DC可分别下发对应固件） */
    uc_ic_type = data[printUPDATE_DCAC_FILE_HEAD_IC_OFFSET];
    if(uc_ic_type != ucDcac_GetUpdateIcType(tUpdate.eObj))
        return false;

    /* 提取固件CRC32和大小，初始化接收状态 */
    tUpdate.ulFwCrc32 = ulFunc_GetLe32(&data[printUPDATE_DCAC_FILE_HEAD_CRC_OFFSET]);
    tUpdate.ulFwSize = ulFunc_GetLe32(&data[printUPDATE_DCAC_FILE_HEAD_SIZE_OFFSET]);
    tUpdate.ulRxSize = 0;
    tUpdate.ulFwCalcCrc32 = 0xFFFFFFFFUL;   /* CRC初始值 */
    tUpdate.ulFwPendCrc32 = tUpdate.ulFwCalcCrc32;
    tUpdate.usPendPacketLen = 0;

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
