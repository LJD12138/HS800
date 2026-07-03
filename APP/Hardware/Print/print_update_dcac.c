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
#include "Sys/sys_queue_task_update.h"
#include "function.h"
#include "check.h"
#include <stdbool.h>

#if(boardUPDATE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_queue_task_update.h"
#include "MD_Dcac/md_dcac_prot_frame.h"


//****************************************************Macros*******************************************************************//
#define        printUPDATE_DCAC_FILE_HEAD_MAGIC_OFFSET     0
#define        printUPDATE_DCAC_FILE_HEAD_CRC_OFFSET       4
#define        printUPDATE_DCAC_FILE_HEAD_IC_OFFSET        8
#define        printUPDATE_DCAC_FILE_HEAD_SIZE_OFFSET      48


//****************************************************Parameter Initialization************************************************//



//****************************************************Function Declaration****************************************************//
static bool b_print_dcac_parse_file_head(const u8* data, u16 len);
static bool b_print_dcac_check_data_packet(BaikuProtoRx_t* proto, u32* ulp_next_crc_state);
static bool b_dcac_c8_proc_rec_data(Task_T *tp_task);
static s8  c_print_dcac_handle_set_proto(void);
static s8  c_print_dcac_handle_file_head(void);
static s8  c_print_dcac_handle_host_fw_data(Task_T* tp_task);
static s8  c_print_dcac_handle_finish(Task_T* tp_task);


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
                /* C2 主机设置升级协议 */
                case baikuCMD_SET_PROTO:    
                {
                    return c_print_dcac_handle_set_proto();
                }
                
                /* C5 主机下发文件头数据 */
                case baikuCMD_REPLY_DATA:   
                {
                    return c_print_dcac_handle_file_head();
                }
                
                /* C8 主机取消升级 */
                case baikuCMD_REPLY_CANEL:  
                {
                    //已经成功了
                    if(tUpdate.eHostResult == UTR_OK
                       || tUpdate.eHostResult == UTR_LATEST)
                        return 1;

                    b_dcac_c8_proc_rec_data(tp_task);
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
-----函数功能    固件传输阶段
-----说明(备注)  管理DCAC固件升级各阶段状态，处理与主机及DCAC模块的数据交互
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      正值:传输完成 0:继续  负值:处理失败
************************************************************************************************************************/
s8 c_print_dcac_update_firmware_transfer(Task_T *tp_task)
{
    s8 c_ret = 0;
    
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

    if(tUpdate.eSlaveResult != UTR_OK 
        && tUpdate.eSlaveResult != UTR_LATEST
        && tUpdate.eSlaveResult != UTR_RUNNING)
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
            /* 根据主机命令码分发处理 */
            switch(tpPrintProtoRx->ucCmd)
            {
                /* C5 主机下发固件数据 */
                case baikuCMD_REPLY_DATA:   
                {
                    return c_print_dcac_handle_host_fw_data(tp_task);
                }
                break;

                /* C7 主机发送完成帧 */
                case baikuCMD_REPLY_FINISH:   
                {
                    return c_print_dcac_handle_finish(tp_task);
                }

                /* C8 主机取消升级 */
                case baikuCMD_REPLY_CANEL:   
                {
                    b_dcac_c8_proc_rec_data(tp_task);
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

    /* 校验文件标识 */
    ul_magic = ulFunc_GetLe32(&data[printUPDATE_DCAC_FILE_HEAD_MAGIC_OFFSET]);
    if(ul_magic != MEGMEET_FILE_MAGIC)
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
    tUpdate.usRecFrameCnt = 0;

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

    len = proto->ucValidLen;
    if(len > MEGMEET_FRM_PKG_SIZE)
        return false;

    /* SN从2开始(SN=0为文件头,SN=1保留),减2得到0基包序号 */
    if(proto->ucSN < 2)
        return false;
    tUpdate.usRecFrameCnt = proto->ucSN - 2;

    /* 固件数据边界校验与CRC32完整性校验
     * 默认启用,调试时可通过定义 boardUPDATE_DCAC_CRC_EN=0 关闭 */
#if(!defined(boardUPDATE_DCAC_CRC_EN) || boardUPDATE_DCAC_CRC_EN)
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
#else
    *ulp_next_crc_state = tUpdate.ulFwCalcCrc32;  /* CRC关闭时保持当前状态不变 */
#endif  //boardUPDATE_DCAC_CRC_EN
    return true;
}

/***********************************************************************************************************************
-----函数功能    处理主机设置升级协议(C2)
-----说明(备注)  校验并保存主机协议设置，回复C3确认并切换准备阶段
-----传入参数    none
-----输出参数    none
-----返回值      0:继续  负值:处理失败
************************************************************************************************************************/
static s8 c_print_dcac_handle_set_proto(void)
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
    tUpdate.usTotalFrmValue -= 2;
    
    /* 选择Baiku协议并回复确认（按当前升级对象选择协议） */
    cUpdate_ProtoSelect(tUpdate.eObj, PT_BAIKU);
    if(c_print_cs_C3_reply_set_proto(tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen) <= 0)
    {
        bUpdate_SetErrCode(UEF_P_C2_REPLY_FAIL);
        return -5;
    }

    vUpdate_ResetRecTimeout(true);
    bDcac_SetPrepStage(tpDcacTask, DPS_PRINT_SEND_C4);

    return 0;
}

/***********************************************************************************************************************
-----函数功能    处理主机下发的文件头数据(C5)
-----说明(备注)  解析并缓存文件头，转发给DCAC模块，切换至等待A2阶段
-----传入参数    none
-----输出参数    none
-----返回值      1:准备完成  0:继续  负值:处理失败
************************************************************************************************************************/
static s8 c_print_dcac_handle_file_head(void)
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

    vUpdate_ResetRecTimeout(true);
    /* 文件头已下发，切换至等待A2回复阶段 */
    bDcac_SetPrepStage(tpDcacTask, DPS_WAIT_A2);

    return 1;   /* 准备完成，进入下一个阶段 */
}

/***********************************************************************************************************************
-----函数功能    处理主机下发的固件数据(C5)
-----说明(备注)  将主机下发的固件数据包转发给DCAC模块，并更新相关状态
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      0:继续  负值:处理失败
************************************************************************************************************************/
static s8 c_print_dcac_handle_host_fw_data(Task_T *tp_task)
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

    /* 组装 A3 负载: [包序号-L(1) + 包序号-H(1) + 固件数据(N)] */
    u8 uca_a3_payload[MEGMEET_FRM_PKG_SIZE + 2];
    u16 us_a3_len = tpPrintProtoRx->ucValidLen + 2;
    u16 us_seq = tUpdate.usRecFrameCnt;
    uca_a3_payload[0] = (u8)(us_seq & 0x00FF);          /* 包序号 - L */
    uca_a3_payload[1] = (u8)((us_seq >> 8) & 0x00FF);   /* 包序号 - H */
    memcpy(&uca_a3_payload[2], tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen);

    // 直接下发给DCAC模块
    //发送固件数据,如果失败延时200ms重试3次
    bool b_send_ok = false;
    for(int i = 0; i < 3; i++)
    {
        if(b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj), MEGMEET_CMD_FIRMWARE_DATA, uca_a3_payload, us_a3_len))
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

    /* 写入数据到DCAC缓存(包含2字节包序号)，用于A4超时后重新发送 */
    if(b_dcac_update_buf_write(tpDcacTask, uca_a3_payload, us_a3_len) == false)
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
    
    vUpdate_ResetRecTimeout(true);
    bDcac_SetFwTransStage(tp_task, DFTS_WAIT_SLAVE_REPLY);

    return 0;
}

/***********************************************************************************************************************
-----函数功能    处理主机发送完成帧(C7)
-----说明(备注)  根据当前状态切换阶段并唤醒DCAC任务
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      1:传输完成
************************************************************************************************************************/
static s8 c_print_dcac_handle_finish(Task_T *tp_task)
{
    //从机未完成
    if(tUpdate.eSlaveResult != UTR_OK && tUpdate.eSlaveResult != UTR_LATEST)
        bDcac_SetFwTransStage(tp_task, DFTS_QUERY_SLAVE_RESULT);

    #if(boardUSE_OS)
    /* 通过任务通知唤醒DCAC任务 */
    if(tDcacTaskHandler != NULL)
        xTaskNotifyGive(tDcacTaskHandler);
    #endif
    return 1;
}

/***********************************************************************************************************************
-----函数功能    处理C8主机取消升级
-----说明(备注)  取消升级并通知DCAC模块查询结果
-----传入参数    none
-----输出参数    none
-----返回值     bool false 异常
-----作者       LJD
-----日期       2026-06-26
************************************************************************************************************************/
static bool b_dcac_c8_proc_rec_data(Task_T *tp_task)
{
    //从机未完成
    if(tUpdate.eSlaveResult != UTR_OK && tUpdate.eSlaveResult != UTR_LATEST)
        bDcac_SetFwTransStage(tp_task, DFTS_QUERY_SLAVE_RESULT);

    if(tUpdate.eHostResult != UTR_FAIL)
        bUpdate_SetResult(URT_HOST, UTR_CANCEL);

    #if(boardUSE_OS)
    /* 通过任务通知唤醒DCAC任务 */
    if(tDcacTaskHandler != NULL)
        xTaskNotifyGive(tDcacTaskHandler);
    #endif
    return true;
}

#endif  //boardUPDATE
