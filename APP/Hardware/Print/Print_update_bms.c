/*******************************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS800\APP\Hardware\Print
 * File    : Print_update_bms.c
 * Date    : 2026-06-25 16:35:50
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS升级数据处理，参考DCAC模块流程实现数据解析、转发与回复
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "Print/print_queue_task_update.h"
#include <stdbool.h>

#if(boardUPDATE)
#include "Print/print_queue_task.h"
#include "Print/print_task.h"
#include "Print/print_iface.h"
#include "Print/print_prot_frame.h"
#include "Sys/sys_queue_task_update.h"
#include "Sys/sys_task.h"
#include "Baiku/baiku_proto.h"
#include "check.h"
#include "function.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_iface.h"
#include "MD_Bms/md_bms_prot_frame.h"


//****************************************************Macros*******************************************************************//
#define        printUPDATE_BMS_FILE_HEAD_SIZE              56


//****************************************************Parameter Initialization************************************************//


//****************************************************Function Declaration****************************************************//
static bool b_print_c5_proc_rec_data(u8 ucSN);
static bool b_print_c8_proc_rec_data(void);

/***********************************************************************************************************************
-----函数功能    BMS进入数据透传前的准备流程
-----说明(备注)  处理上位机传输协议握手(C4/C2/C3)，握手完成后再进入文件头阶段
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      1:准备完成  0:等待中  负值:准备失败
************************************************************************************************************************/
s8 c_print_bms_prepare_update(Task_T* tp_task)
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

                    /* 选择Baiku协议 */
                    if(cUpdate_ProtoSelect(UO_BMS, PT_BAIKU) < 0)
                    {
                        bUpdate_SetErrCode(UEF_P_C2_DATA_ERR);
                        return -4;
                    }

                    /* 转发C2到BMS模块 */
                    if(c_bms_cs_C2_set_update_proto(tpPrintProtoRx->ucpValidData, tpPrintProtoRx->ucValidLen) == false)
                    {
                        bUpdate_SetErrCode(UEF_P_FWD_BMS_FAIL);
                        return -5;
                    }

                    vUpdate_ResetRecTimeout(true);
                }
                break;

                case baikuCMD_REPLY_DATA:   /* C5 主机下发文件头数据 */
                {
                    if(b_print_c5_proc_rec_data(tpPrintProtoRx->ucSN) == false)
                        return -10;
                    return 1;   /* 准备完成，进入下一个阶段 */
                }

                case baikuCMD_REPLY_CANEL:  /* C8 主机取消升级 */
                {
                    /* 已经成功则忽略取消指令 */
                    if(tUpdate.eHostResult == UTR_OK
                       || tUpdate.eHostResult == UTR_LATEST)
                        return 1;

                    b_print_c8_proc_rec_data();
                    return -1;  /* 返回负值表示取消，与正常完成区分 */
                }

                default:
                    break;
            }
        }
    }

    return 0;
}


/***********************************************************************************************************************
-----函数功能    BMS升级主流程控制
-----说明(备注)  管理BMS固件升级各阶段状态，处理与主机及BMS模块的数据交互
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      正值:升级完成 0:继续  负值:处理失败
************************************************************************************************************************/
s8 c_print_bms_update_firmware_transfer(Task_T *tp_task)
{
    s8 c_ret = 0;

    /* BMS任务未初始化或缓存未分配 */
    if(tpBmsTask == NULL)
    {
        bUpdate_SetErrCode(UEF_P_TASK_NULL);
        return -1;
    }

    if(tpBmsTask->tReplyBuff.buff == NULL)
    {
        bUpdate_SetErrCode(UEF_P_BUFF_NULL);
        return -2;
    }

    if(tUpdate.eSlaveResult != UTR_OK
        && tUpdate.eSlaveResult != UTR_LATEST
        && tUpdate.eSlaveResult != UTR_RUNNING)
    {
        bUpdate_SetErrCode(UEF_P_SLAVE_RESULT_ERR);
        return -3;
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
                case baikuCMD_REPLY_DATA:   /* C5 主机下发固件数据 */
                {
                    if(b_print_c5_proc_rec_data(tpPrintProtoRx->ucSN) == false)
                        return -10;
                }
                break;

                case baikuCMD_REPLY_FINISH: /* C7 主机发送完成帧 */
                {
                    /* 完成前校验：总帧数必须有效且已收帧数等于总帧数 */
                    if(tUpdate.usTotalFrmValue == 0 ||
                       tUpdate.usRecFrameCnt != tUpdate.usTotalFrmValue)
                    {
                        bUpdate_SetErrCode(UEF_P_FINISH_MISMATCH);
                        return -1;
                    }

                    c_bms_cs_C7_update_finish();
                    return 1;
                }

                case baikuCMD_REPLY_CANEL:  /* C8 主机取消升级 */
                {
                    b_print_c8_proc_rec_data();
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
-----函数功能    处理C5主机下发的固件数据
-----说明(备注)  从BMS任务回复缓存中读取数据，转发到BMS模块处理
-----返回值     bool
-----作者       LJD
-----日期       2026-06-26
************************************************************************************************************************/
static bool b_print_c5_proc_rec_data(u8 ucSN)
{
    /* 数据内容为空或长度不符，请求重发 */
    if(tpPrintProtoRx->ucValidLen == 0 || tpPrintProtoRx->ucpValidData == NULL)
    {
        bUpdate_SetErrCode(UEF_P_C5_DATA_ERR);
        return false;
    }
    tUpdate.usRecFrameCnt = ucSN;

    /* 转发文件头到BMS任务，携带上位机下发的SN */
    if(c_bms_cs_C5_send_file(tpPrintProtoRx->ucpValidData,
                                    tpPrintProtoRx->ucValidLen,
                                    ucSN) == false)
    {
        bUpdate_SetErrCode(UEF_P_FWD_BMS_FAIL);
        return false;
    }
    vUpdate_ResetRecTimeout(true);
	
	 /* 记录本包长度，待BMS回复C6确认后再提交计数 */
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	tUpdate.usPendPacketLen = tpPrintProtoRx->ucValidLen;
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
	
    return true;
}

/***********************************************************************************************************************
-----函数功能    处理C8主机取消升级
-----说明(备注)  发送C8取消升级命令到BMS模块
-----传入参数    none
-----输出参数    none
-----返回值     bool false 异常
-----作者       LJD
-----日期       2026-06-26
************************************************************************************************************************/
static bool b_print_c8_proc_rec_data(void)
{
    /* 取消升级 */
    if(tUpdate.eHostResult != UTR_FAIL)
        bUpdate_SetResult(URT_HOST, UTR_CANCEL);
    
    c_bms_cs_C8_trans_cancel();
    #if(boardUSE_OS)
    /* 通过任务通知唤醒BMS任务 */
    if(tBmsTaskHandler != NULL)
        xTaskNotifyGive(tBmsTaskHandler);
    #endif

    return true;
}

#endif  //boardBMS_EN
#endif  //boardUPDATE
