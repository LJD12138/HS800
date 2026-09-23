/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_rec_data_proc.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 接收数据解析与状态装载实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_rec_data_proc.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Print/print_task.h"
#include "Baiku/baiku_proto.h"

#if (boardUPDATE)
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
#include "Print/print_prot_frame.h"
#endif  /* boardUPDATE */

//****************************************************Macros********************************************************************//

/* BMS 0x08 回复参数通信协议帧 (紧凑布局, 严格与下位机通信物理报文对齐) */
#pragma pack(1)
typedef struct
{
	uint16_t			usSOC;				/* 总的SOC      1% */
	int16_t				sTotalCurr;			/* 总的电流     0.01A */
	uint16_t			usChgFullTime;		/* 总的充满时间 1min */
	uint16_t			usDisChgEmptyTime;	/* 总的放空时间 1min */
	uint16_t			usPermMaxChgPwr;	/* 许可的最大充电功率 W */
	uint16_t			usPermMaxDisChgPwr;	/* 许可的最大放电功率 W */
	uint8_t				ucOnlineNum;		/* 在线设备数   最大6台 */
	uint8_t				ucMasterNum;		/* 选中的数量 */
	uint16_t			usState;			/* 主机系统状态 */
	struct
	{
		uint16_t		usSOC;				/* 当前SOC      1% */
		uint16_t		usVolt;				/* 当前电压     0.01V */
		int16_t			sCurr;				/* 当前电流     0.01A */
		uint16_t		usCalcCapAH;		/* 估算容量     0.1AH */
		uint16_t		usCycleCnt;			/* 循环次数 */
		int16_t			sMaxTemp;			/* 主机最高温度 1℃ */
		int16_t			sMinTemp;			/* 主机最低温度 1℃ */
		int16_t			sBoardTempMax;		/* 板载最高温   1℃ */
		uint32_t		ulErrCode;			/* 错误代码 */
	}					atDevInfo[bmsDEV_NUM];
}BmsProto08Param_T;
#pragma pack()

//****************************************************Function Declaration******************************************************//
static s8 c_bms_relay08_param(BaikuProtoRx_t *p_proto);
#if (boardUPDATE)
static s8 c_bms_handle_update_c9(BaikuProtoRx_t *p_proto);
#endif  /* boardUPDATE */


/***********************************************************************************************************************
 * 函数功能    : 处理接收到的 BMS 协议数据
 * 说明(备注)  : 无
 * 传入参数    : p_proto: 接收协议对象指针
 * 输出参数    : 无
 * 返回值      : s8: 1-成功, 负数-失败
 ************************************************************************************************************************/
s8 c_bms_rec_proc_data(BaikuProtoRx_t *p_proto)
{
    s8   c_ret   = 1;
    vu16 us_temp = 0;
    
    if (uPrint.tFlag.bBmsRecTask)
        sMyPrint("bBmsRecTask:指令:0x%x, 长度:%d\r\n", p_proto->ucCmd, p_proto->ucValidLen);
    
    switch (p_proto->ucCmd)
    {
        /* 回复开关 */
        case baikuCMD_REPLY_SWITCH:               
        {
            if (p_proto->ucValidLen != 2 || p_proto->ucpValidData == NULL)
                return -10;

            if (!bQueue_WriteReply(tpBmsTask, p_proto->ucpValidData, p_proto->ucValidLen))
                return -11;
        }
        break;
        
        /* 回复参数 */
        case baikuCMD_REPLY_PARAM:                
        {
            c_ret = c_bms_relay08_param(p_proto);
            if (c_ret <= 0)
                return -20;
        }
        break;
        
        /* 回复校准结果 */
        case baikuCMD_REPLY_CALI: /* 45 */
        {
            if (p_proto->ucValidLen != 2 || p_proto->ucpValidData == NULL)
                return -40;
            
            memcpy((uint8_t *)&us_temp, p_proto->ucpValidData, p_proto->ucValidLen);

            #if (boardPRINT_IFACE)
            if (cQueue_AddQueueTask(tpPrintTask, PTI_REPLY_CALI, us_temp, false) <= 0)
                return -41;
            #endif  /* boardPRINT_IFACE */
        }
        break;
        
        /* 回复设置结果 */
        case baikuCMD_REPLY_SYS_SET: /* 89 */            
            break;
        
        /* 回复APP信息 */
        case baikuCMD_REPLY_MEM_PARAM: /* 81 */
        {
            if (p_proto->ucValidLen == 0 || p_proto->ucpValidData == NULL)
                return -50;
            
            #if (boardPRINT_IFACE)
            if (!bQueue_WriteReply(tpPrintTask, p_proto->ucpValidData, p_proto->ucValidLen))
                return -51;

            if (cQueue_AddQueueTask(tpPrintTask, PTI_REPLY_APP_INFO, p_proto->ucValidLen, false) <= 0)
                return -52;
            #endif  /* boardPRINT_IFACE */
        }
        break;
        
        #if (boardRUN_LOG_EN)
        /* 回复解锁运行日志结果 */
        case baikuCMD_REPLY_LOG_UNLOCK: /* 8B */
        {
            if (p_proto->ucValidLen != 1 || p_proto->ucpValidData == NULL)
                return -85;

            uint8_t uc_res = p_proto->ucpValidData[0];
            if (uc_res == 0)
                tBmsRx.tState.bLogLock = 0;

            #if (boardPRINT_IFACE)
            if (cQueue_AddQueueTask(tpPrintTask, PTI_REPLY_LOG_UNLOCK, uc_res, false) <= 0)
                return -86;
            #endif  /* boardPRINT_IFACE */
        }
        break;

        #if (boardPRINT_IFACE)
        /* 回复运行日志记录/结束帧 */
        case baikuCMD_REPLY_RUN_LOG: /* B5 */
        {
            if (p_proto->ucValidLen == 0 || p_proto->ucpValidData == NULL)
            {
                c_print_cs_send_run_log_end();
                vBms_RunLogFinished();
            }
            else
                c_print_cs_send_run_log_rec(p_proto->ucpValidData, p_proto->ucValidLen);
            #if (boardUSE_OS)
            xTaskNotifyGive(tPrintTaskHandler);
            #endif  /* boardUSE_OS */
        }
        break;

        /* 回复读取运行日志指定槽 */
        case baikuCMD_REPLY_RUN_LOG_SLOT: /* B7 */
        {
            if (p_proto->ucValidLen == 0 || p_proto->ucpValidData == NULL)
                return -87;

            if (tpPrintTask->tReplyBuff.buff == NULL)
                return -88;

            if (!bQueue_WriteReply(tpBmsTask, p_proto->ucpValidData, p_proto->ucValidLen))
                return -92;

            if (cQueue_AddQueueTask(tpPrintTask, PTI_REPLY_RUN_LOG_SLOT, p_proto->ucValidLen, false) <= 0)
                return -89;
        }
        break;

        /* 回复重置运行日志结果 */
        case baikuCMD_REPLY_RESET_RUN_LOG: /* B9 */
        {
            if (p_proto->ucValidLen != 1 || p_proto->ucpValidData == NULL)
                return -90;

            uint8_t uc_res = p_proto->ucpValidData[0];
            if (cQueue_AddQueueTask(tpPrintTask, PTI_REPLY_RESET_RUN_LOG, uc_res, false) <= 0)
                return -91;
        }
        break;
        #endif  /* boardPRINT_IFACE */
        #endif  /* boardRUN_LOG_EN */

        #if (boardUPDATE)
        /* 请求开始发送 */
        case baikuCMD_RRQ_START_SEND: /* C4 */               
        {
            if (tBms.eDevState == DS_UPDATE_MODE
                && tSysInfo.eDevState == DS_UPDATE_MODE 
                && tUpdate.eObj == MO_BMS
                && tUpdate.eChType == CT_PRINT
                && tUpdate.eProtoType == PT_BAIKU)
            {
                return 1;
            }
            
            if (tSysInfo.eDevState != DS_UPDATE_MODE)
            {
                if (cUpdate_ChSelect(MO_BMS, CT_PRINT) <= 0)
                    return -73;
            }
            
            if (tUpdate.eChType != CT_PRINT)
            {
                if (cUpdate_ChSelect(MO_BMS, CT_PRINT) <= 0)
                    return -71;
            }

            if (tUpdate.eProtoType != PT_BAIKU)
            {
                if (cUpdate_ProtoSelect(MO_BMS, PT_BAIKU) <= 0)
                    return -72;
            }
        }
        break;

        /* BMS正在升级 */
        case baikuCMD_BMS_UPDATE: /* C9 */
        {
            c_ret = c_bms_handle_update_c9(p_proto);
            if (c_ret <= 0)
                return c_ret;
        }
        break;
        #endif  /* boardUPDATE */
        
        default:
        {
            return -99;
        }
    }
    
    return 1; 
}

#if (boardUPDATE)
/***********************************************************************************************************************
 * 函数功能    : 处理升级模式下接收到的数据
 * 说明(备注)  : 无
 * 传入参数    : p_proto: 接收协议对象指针
 * 输出参数    : 无
 * 返回值      : s8: 1-成功, 负数-失败
 ************************************************************************************************************************/
s8 c_bms_rec_proc_data_for_update(BaikuProtoRx_t *p_proto)
{
    s8 c_ret = 1;

    switch (p_proto->ucCmd)
    {
        case baikuCMD_REPLY_SET_PROTO: /* C3 */
            if (p_proto->ucValidLen != 3 || p_proto->ucpValidData == NULL)
                return -80;

            if ((ProtoType_E)p_proto->ucpValidData[0] >= PT_INVAILD ||
                (ProtoType_E)p_proto->ucpValidData[0] != tUpdate.eProtoType)
            {
                return -81;
            }

            vUpdate_ResetRecTimeout(true);

            memcpy((uint8_t *)&tUpdate.usTotalFrmValue, &p_proto->ucpValidData[1], 2);

            c_print_cs_C3_reply_set_proto(p_proto->ucpValidData, p_proto->ucValidLen);
            break;

        /* 请求开始发送 */
        case baikuCMD_RRQ_START_SEND: /* C4 */               
            c_print_cs_C4_req_start_send();

            if (tBms.eDevState == DS_UPDATE_MODE
                && tSysInfo.eDevState == DS_UPDATE_MODE 
                && tUpdate.eObj == MO_BMS
                && tUpdate.eChType == CT_PRINT
                && tUpdate.eProtoType == PT_BAIKU)
            {
                return 1;
            }
            
            if (tUpdate.eChType != CT_PRINT)
            {
                if (cUpdate_ChSelect(MO_BMS, CT_PRINT) <= 0)
                    return -71;
            }

            if (tUpdate.eProtoType != PT_BAIKU)
            {
                if (cUpdate_ProtoSelect(MO_BMS, PT_BAIKU) <= 0)
                    return -72;
            }

            if (tBms.eDevState != DS_UPDATE_MODE)
                cQueue_AddQueueTask(tpBmsTask, BTI_UPDATE, 0, false);
            break;

        /* 继续发送 */
        case baikuCMD_RRQ_CONT_SEND: /* C6 */
        {
            uint16_t us_pending_len = tUpdate.usPendPacketLen;
            tUpdate.usPendPacketLen = 0;

            if (us_pending_len == 0)
                return 0;

            vUpdate_ResetRecTimeout(true);
            vUpdate_ResetTimeout();

            tUpdate.ulRxSize += us_pending_len;
            
            c_print_cs_C6_req_cont_send();
        }
        break;

        /* 取消发送 */
        case baikuCMD_REPLY_CANEL: /* C8 */
            c_print_cs_C8_trans_cancel();
            bUpdate_SetResult(URT_SLAVE, UTR_CANCEL);
            break;

        /* BMS正在升级 */
        case baikuCMD_BMS_UPDATE: /* C9 */
            c_ret = c_bms_handle_update_c9(p_proto);
            if (c_ret <= 0)
                return c_ret;
            break;

        default:
            return -99;
    }
    return 1;
}
#endif  /* boardUPDATE */

/***********************************************************************************************************************
 * 函数功能    : 解析并装载 BMS 0x08 参数回复
 * 说明(备注)  : 无
 * 传入参数    : p_proto: 接收协议对象指针
 * 输出参数    : 无
 * 返回值      : s8: 1-成功, 负数-失败
 ************************************************************************************************************************/
static s8 c_bms_relay08_param(BaikuProtoRx_t *p_proto)
{
    if (p_proto->ucpValidData == NULL || p_proto->ucValidLen != (sizeof(BmsProto08Param_T) + 1))
        return -1;

    uint8_t cmd = p_proto->ucpValidData[0];
    if (cmd != bmsGET_PARAM_OBJ)
        return -2;

    const BmsProto08Param_T *p_wire = (const BmsProto08Param_T *)&p_proto->ucpValidData[1];

    /* 解耦装载至 4 字节自然对齐的任务全局对象 tBmsRx */
    tBmsRx.usSOC              = p_wire->usSOC;
    tBmsRx.sTotalCurr         = p_wire->sTotalCurr;
    tBmsRx.usChgFullTime      = p_wire->usChgFullTime;
    tBmsRx.usDisChgEmptyTime  = p_wire->usDisChgEmptyTime;
    tBmsRx.usPermMaxChgPwr    = p_wire->usPermMaxChgPwr;
    tBmsRx.usPermMaxDisChgPwr = p_wire->usPermMaxDisChgPwr;
    tBmsRx.tDevNum.ucOnlineNum = p_wire->ucOnlineNum;
    tBmsRx.tDevNum.ucMasterNum = p_wire->ucMasterNum;
    memcpy((void *)&tBmsRx.tState, (const void *)&p_wire->usState, sizeof(tBmsRx.tState));

    for (uint8_t i = 0; i < bmsDEV_NUM; i++)
    {
        tBmsRx.tDevInfo[i].usSOC         = p_wire->atDevInfo[i].usSOC;
        tBmsRx.tDevInfo[i].usVolt        = p_wire->atDevInfo[i].usVolt;
        tBmsRx.tDevInfo[i].sCurr         = p_wire->atDevInfo[i].sCurr;
        tBmsRx.tDevInfo[i].usCalcCapAH   = p_wire->atDevInfo[i].usCalcCapAH;
        tBmsRx.tDevInfo[i].usCycleCnt    = p_wire->atDevInfo[i].usCycleCnt;
        tBmsRx.tDevInfo[i].sMaxTemp      = p_wire->atDevInfo[i].sMaxTemp;
        tBmsRx.tDevInfo[i].sMinTemp      = p_wire->atDevInfo[i].sMinTemp;
        tBmsRx.tDevInfo[i].sBoardTempMax = p_wire->atDevInfo[i].sBoardTempMax;
        tBmsRx.tDevInfo[i].uErrCode.ulCode = p_wire->atDevInfo[i].ulErrCode;
    }

    static vu32 s_ul_last_err_state = 0;
    
    ulBmsRxErrCode = 0;
    for (uint8_t i = 0; i < bmsDEV_NUM; i++)
        ulBmsRxErrCode |= tBmsRx.tDevInfo[i].uErrCode.ulCode;
    
    /*----------------------------获取故障位-------------------------------------------------*/
    if (s_ul_last_err_state != ulBmsRxErrCode) 
    {
        s_ul_last_err_state = ulBmsRxErrCode;
        if (ulBmsRxErrCode)
            bBms_SetErrCode(BEC_BMS_ERR, true);
        else 
            bBms_SetErrCode(BEC_BMS_ERR, false);
    }
    
    /*----------------------------获取充放电状态-----------------------------------------------*/
    if (tBmsRx.sTotalCurr > 0)
        tBms.eWorkState = BWS_CHG;
    else 
        tBms.eWorkState = BWS_DISCHG;
    
    /*----------------------------获取温度-----------------------------------------------*/
    vs16 s_temp_max = tBmsRx.tDevInfo[0].sMaxTemp;
    vs16 s_temp_min = tBmsRx.tDevInfo[0].sMinTemp;
    if (tBmsRx.tDevNum.ucOnlineNum > 0)
    {
        uint8_t uc_dev_cnt = tBmsRx.tDevNum.ucOnlineNum;
        if (uc_dev_cnt > bmsDEV_NUM)
            uc_dev_cnt = bmsDEV_NUM;  /* 上界保护,防止越界访问 */
        for (uint8_t i = 1; i < uc_dev_cnt; i++)
        {
            s_temp_max = MAX2(s_temp_max, tBmsRx.tDevInfo[i].sMaxTemp);
            s_temp_min = MIN2(s_temp_min, tBmsRx.tDevInfo[i].sMinTemp);
        }
    }
    tBms.sMaxTemp = s_temp_max;
    tBms.sMinTemp = s_temp_min;
    
    return 1;
}

#if (boardUPDATE)
/***********************************************************************************************************************
 * 函数功能    : 处理 BMS 升级状态上报 (C9)
 * 说明(备注)  : 无
 * 传入参数    : p_proto: 拜库协议接收结构体指针
 * 输出参数    : 无
 * 返回值      : s8: 1-处理成功, 负数-处理失败
 ************************************************************************************************************************/
static s8 c_bms_handle_update_c9(BaikuProtoRx_t *p_proto)
{
    #pragma pack(1)
    struct
    {
		vu16			usRecFrameCnt;		/* 记录当前接收的帧数 */
		vu16			usTotalFrmValue;	/* 总帧数 */
	}t_my_param;
    #pragma pack()

    if (tpBmsTask == NULL || tpBmsTask->ucID == BTI_REQ_SET_CMD)
        return -60;

    if (p_proto->ucValidLen != sizeof(t_my_param) || p_proto->ucpValidData == NULL)
        return -61;

    memcpy((uint8_t *)&t_my_param, p_proto->ucpValidData, p_proto->ucValidLen);

    /* 校验总帧数有效且已收帧数不超过总帧数 */
    if (t_my_param.usTotalFrmValue == 0 ||
        t_my_param.usRecFrameCnt > t_my_param.usTotalFrmValue)
    {
        return -62;
    }

    tUpdate.usRecFrameCnt = t_my_param.usRecFrameCnt;
    tUpdate.usTotalFrmValue = t_my_param.usTotalFrmValue;

    /* 升级完成 */
    if (tUpdate.usRecFrameCnt >= tUpdate.usTotalFrmValue)
        bUpdate_SetResult(URT_SLAVE, UTR_OK);
    else
        bUpdate_SetResult(URT_SLAVE, UTR_RUNNING);

    return 1;
}
#endif  /* boardUPDATE */

#endif  /* boardBMS_EN */
