/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_rec_data_proc.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器接收数据解析与状态装载实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Dcac/md_dcac_rec_data_proc.h"

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#include "Print/print_prot_frame.h"
#include "Print/print_task.h"
#include "function.h"
#include "check.h"
#include "app_info.h"
#include <string.h>

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#include "MD_Dcac/md_dcac_queue_task_update.h"
#endif  /* boardUPDATE */

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 处理接收到的 DCAC Modbus 数据
 * 说明(备注)  : 无
 * 传入参数    : p_proto_rx: 接收协议对象指针, p_proto_tx: 发送匹配对象指针
 * 输出参数    : 无
 * 返回值      : int8_t: 1-成功, 小于0-错误码
 ************************************************************************************************************************/
int8_t c_dcac_rec_proc_data(ModbusProtoRx_t *p_proto_rx, ModbusProtoTx_t *p_proto_tx)
{
    if (p_proto_rx == NULL || p_proto_tx == NULL)
        return -1;

    if (uPrint.tFlag.bDcacRecTask)
    {
        sMyPrint("bDcacRecTask:接收地址%d:", p_proto_tx->usRegAddr);
        for (int i = 0; i < p_proto_rx->ucValidLen; i++)
            sMyPrint("%x ", p_proto_rx->ucpValidData[i]);
        sMyPrint("\r\n");
    }

    /* 协议中间层统一校验回包一致性与迟到包 */
    s8 c_check = cModbus_CheckReply(p_proto_tx, p_proto_rx);
    if (c_check <= 0)
    {
        if (uPrint.tFlag.bDcacRecTask || uPrint.tFlag.bImportant)
            log_w("bDcacRecTask:回包校验未通过(代码%d),当前等待寄存器%d", c_check, p_proto_tx->usRegAddr);
        return c_check;
    }

    switch (p_proto_tx->usRegAddr)
    {
        case dcacREG_ADDR_GET_PARAM1:
        {
            DCAC_Param1_t tParam1;
            if (p_proto_rx->ucCharLen != sizeof(tParam1))
                return -4;

            bFunc_SwapU16Array((uint8_t *)&tParam1, p_proto_rx->ucpValidData, p_proto_rx->ucCharLen / 2);

            tDcacRx.usOutVolt = tParam1.usOutVolt;

			//G3604 0.1A     G2404 0.01A
			if(strstr(boardSOFTWARE_VERSION, "G3604") != NULL)
				tDcacRx.usOutCurr = LIMIT_MIN(tParam1.sOutCurr, 0);
			else
            	tDcacRx.usOutCurr = LIMIT_MIN(tParam1.sOutCurr / 10, 0);
			
            tDcacRx.usOutPwr  = (tParam1.usOutPwr > 5) ? tParam1.usOutPwr : 0;
            tDcacRx.usOutFreq = tParam1.usOutFreq / 10;
            tDcacRx.uState.usState = tParam1.usState;

			s16 fan_temp = 25;
			// if(tParam1.usFan > 10 && tParam1.usFan <25)
			// 	fan_temp = 40;
			// else if(tParam1.usFan > 25 && tParam1.usFan < 50)
			// 	fan_temp = 43;
			// else if(tParam1.usFan > 50 && tParam1.usFan < 75)
			// 	fan_temp = 50;
			// else if(tParam1.usFan > 75)
			// 	fan_temp = 55;
			
			s16 temp = MAX3(tParam1.sTemp1, tParam1.sTemp2, tParam1.sTemp3);
			temp = temp / 10;
			sMpptMaxTemp = tDcacRx.sMaxTemp = MAX2(temp, fan_temp);
			
			temp = MIN3(tParam1.sTemp1, tParam1.sTemp2, tParam1.sTemp3);
			tDcacRx.sMinTemp = temp / 10;
		}
		break;

        case dcacREG_ADDR_GET_PARAM2:
        {
            DCAC_Param2_t tParam2;
            if (p_proto_rx->ucCharLen != sizeof(tParam2))
                return -5;

            bFunc_SwapU16Array((uint8_t *)&tParam2, p_proto_rx->ucpValidData, p_proto_rx->ucCharLen / 2);
			//更新数据
			// if(tMppt.eDevState > DS_BOOTING && tDcac.eChgState == DS_SHUT_DOWN)
				/* bit0:待机状态标志,非故障,屏蔽 */
				tDcacRx.uErrCode.usCode[0] = tParam2.uDcErrCode & (~0x0001);
			// else
				// tDcacRx.uErrCode.usCode[0] = tParam2.uDcErrCode;

            tDcacRx.uErrCode.usCode[1] = tParam2.uAcErrCode;
            /* bit2:输入欠压保护(非故障), bit8:输入缓启动中(非故障), 屏蔽过滤 */
            tDcacRx.uErrCode.usCode[2] = tParam2.uInErrCode & (~0x0140);
            /* bit0:系统运行状态标志 */
            tDcacRx.uErrCode.usCode[3] = tParam2.usSysErr & 0x01;
        }
		break;
		
        case dcacREG_ADDR_GET_PARAM3:
        {
            DCAC_Param3_t tParam3;
            if (p_proto_rx->ucCharLen != sizeof(tParam3))
                return -6;

            bFunc_SwapU16Array((uint8_t *)&tParam3, p_proto_rx->ucpValidData, p_proto_rx->ucCharLen / 2);

            tDcacRx.usInVolt    = tParam3.usAcInVolt;
            tDcacRx.usInPwr     = LIMIT_MIN(tParam3.sAcInPwr, 0);
            tDcacRx.usInChgPwr  = LIMIT_MIN(tParam3.sAcChgPwr, 0);
            tDcacRx.usChgPwr    = LIMIT_MIN(tParam3.sBatInPwr, 0);

            if (tDcacRx.usInVolt < tAppMemParam.tDCAC.usMinInVolt)
                tParam3.sAcInCurr = 0;

			//G3604 0.1A     G2404 0.01A
			if(strstr(boardSOFTWARE_VERSION, "G3604") != NULL)
				tDcacRx.usInCurr = LIMIT_MIN(tParam3.sAcInCurr, 0);
			else
				tDcacRx.usInCurr = LIMIT_MIN(tParam3.sAcInCurr / 10, 0);
		}
		break;

        case dcacREG_ADDR_SET_TOTAL_CHG_PWR:
        case dcacREG_ADDR_SET_AC_CHG_PWR:
        case dcacREG_ADDR_DISCHG_SW:
        {
        }
        break;

        default:
            return -99;
    }

    return 1;
}


/***********************************************************************************************************************
 * 函数功能    : 处理 Megmeet 升级协议数据
 * 说明(备注)  : 无
 * 传入参数    : p_proto_rx: 协议帧接收对象
 * 输出参数    : 无
 * 返回值      : int8_t: 1-成功, 小于0-错误
 ************************************************************************************************************************/
 #if (boardUPDATE)
int8_t c_dcac_rec_proc_megmeet_proto(MegmeetProtoRx_t *p_proto_rx)
{
    UpdateFrame_t *tp_frame = NULL;

    if (p_proto_rx == NULL)
        return -1;

    tp_frame = &p_proto_rx->tFrame;
    if (tp_frame->ucpFrame == NULL || tp_frame->usFrameLen < MEGMEET_FRAME_MIN_FRAME_LEN)
        return -2;

    if (tp_frame->usPayloadLen > 0 && tp_frame->ucpPayload == NULL)
        return -3;

    if (tpDcacTask->tReplyBuff.buff == NULL)
        return -4;

    switch (tp_frame->ucCmd)
    {
        /* F1 回复请求升级 */
        case MEGMEET_CMD_REQ_UPDATE_REPLY:
        {
            if (tp_frame->usPayloadLen != 1 || tp_frame->ucpPayload == NULL)
                return -10;

            if (eDcacPrepStage != DPS_WAIT_F1)
                return 0;

            vUpdate_ResetRecTimeout(true);

            uint8_t u_reply_param = tp_frame->ucpPayload[0];
            if (u_reply_param != 0x01)
            {
                bUpdate_SetErrCode(UEF_DR_F1_CHECK_FAIL);
                return -3;
            }

            bDcac_SetPrepStage(tpDcacTask, DPS_SEND_F2);
        }
		break;

        /* F3 回复切换波特率 */
        case MEGMEET_CMD_SET_BAUD_REPLY:
        {
            if (tp_frame->usPayloadLen != 1 || tp_frame->ucpPayload == NULL)
                return -20;

            if (eDcacPrepStage != DPS_WAIT_F3)
                return 0;

            vUpdate_ResetRecTimeout(true);

            uint8_t u_reply_param = tp_frame->ucpPayload[0];
            if (u_reply_param == MEGMEET_BAUD_INVALID)
            {
                bUpdate_SetErrCode(UEF_DR_F3_BAUD_INVALID);
                return -8;
            }

            if (u_reply_param != MEGMEET_BAUD_OK)
            {
                bUpdate_SetErrCode(UEF_DR_F3_CHECK_FAIL);
                return -9;
            }

            if (bDcac_IfaceSetBaud(tUpdate.ulBaud) == false)
            {
                bUpdate_SetErrCode(UEF_DR_F3_SET_BAUD_FAIL);
                return -10;
            }

            bDcac_SetDevState(DS_UPDATE_MODE);
            bDcac_SetPrepStage(tpDcacTask, DPS_WAIT_PRINT_UPDATE_REQ);
        }
		break;		

        /* F7 回复跳转 Boot */
        case MEGMEET_CMD_JUMP_BOOT_REPLY:
        {
            if (eDcacPrepStage != DPS_WAIT_F7)
                return 0;

            if (tp_frame->usPayloadLen != 0)
            {
                bUpdate_SetErrCode(UEF_DR_F7_CHECK_FAIL);
                return -60;
            }
            vUpdate_ResetRecTimeout(true);
            bDcac_SetPrepStage(tpDcacTask, DPS_BOOT_DELAY);
        }
        break;

        /* A2 文件头回复 */
        case MEGMEET_CMD_FILE_HEAD_REPLY:
        {
            if (tp_frame->usPayloadLen != 1 || tp_frame->ucpPayload == NULL)
                return -30;

            if (eDcacPrepStage != DPS_WAIT_A2)
                return -31;

            vUpdate_ResetRecTimeout(true);

            uint8_t u_reply_param = tp_frame->ucpPayload[0];
            if (u_reply_param != MEGMEET_A2_OK && u_reply_param != MEGMEET_A2_VER_LATEST)
            {
                bUpdate_SetErrCode(UEF_DR_A2_REPLY_ERR);
                break;
            }

            b_dcac_update_buf_reset(tpDcacTask);

            if (u_reply_param == MEGMEET_A2_VER_LATEST)
            {
                bUpdate_SetResult(URT_SLAVE, UTR_LATEST);
                cQueue_GotoStep(tpDcacTask, DUS_FINISH_CLEANUP);
                break;
            }

            bDcac_SetPrepStage(tpDcacTask, DPS_FINISH_CLEANUP);
        }
		break;

        /* A4 固件数据回复 */
        case MEGMEET_CMD_FIRMWARE_DATA_REPLY:
        {
            #pragma pack(1)
            struct
            {
				uint16_t usSeqNum;
				uint8_t	ucStatus;
			}u_reply_param;
            #pragma pack()

            if (tp_frame->usPayloadLen != sizeof(u_reply_param) || tp_frame->ucpPayload == NULL)
                return -40;

            memcpy(&u_reply_param, tp_frame->ucpPayload, tp_frame->usPayloadLen);

            if (u_reply_param.usSeqNum != tUpdate.usRecFrameCnt)
            {
                bUpdate_SetErrCode(UEF_DR_A4_SEQ_MISMATCH);
                return -41;
            }

            if (u_reply_param.ucStatus != MEGMEET_A4_OK && u_reply_param.ucStatus != MEGMEET_A4_ALL_OK)
            {
                bUpdate_SetErrCode(UEF_DR_A4_REPLY_ERR);
                return -42;
            }

            vUpdate_ResetTimeout();
            vUpdate_ResetRecTimeout(true);
            b_dcac_update_buf_reset(tpDcacTask);

            mainENTER_CRITICAL();
            uint16_t us_pending_len = tUpdate.usPendPacketLen;
            uint32_t ul_pend_crc    = tUpdate.ulFwPendCrc32;
            tUpdate.usPendPacketLen = 0;
            mainEXIT_CRITICAL();

            if (us_pending_len == 0)
                return 0;

            tUpdate.ulFwCalcCrc32 = ul_pend_crc;
            tUpdate.ulRxSize += us_pending_len;

            if (u_reply_param.ucStatus == MEGMEET_A4_ALL_OK)
            {
                bUpdate_SetResult(URT_SLAVE, UTR_OK);
                bDcac_SetFwTransStage(tpDcacTask, DFTS_FINISH_CLEANUP);
                break;
            }

            if (tUpdate.eHostResult == UTR_OK || tUpdate.eHostResult == UTR_CANCEL)
            {
                bDcac_SetFwTransStage(tpDcacTask, DFTS_QUERY_SLAVE_RESULT);
                break;
            }

            bDcac_SetFwTransStage(tpDcacTask, DFTS_HOST_REQ_DATA);
        }
		break;

        /* A6 查询结果回复 */
        case MEGMEET_CMD_QUERY_RESULT_REPLY:
        {
            #pragma pack(1)
            struct
            {
				uint8_t	ucStatus;
				uint8_t	ucSlaveAddr;
				uint8_t	ucChipId;
			}u_reply_param;
            #pragma pack()

            if (tp_frame->usPayloadLen != sizeof(u_reply_param) || tp_frame->ucpPayload == NULL)
                return -50;

            vUpdate_ResetRecTimeout(true);
            memcpy(&u_reply_param, tp_frame->ucpPayload, tp_frame->usPayloadLen);

            if (u_reply_param.ucSlaveAddr != ucDcac_GetUpdateSlaveAddr(tUpdate.eObj) ||
                u_reply_param.ucChipId != ucDcac_GetUpdateIcType(tUpdate.eObj))
            {
                bUpdate_SetErrCode(UEF_DR_A6_CHECK_FAIL);
                return -51;
            }

            if (u_reply_param.ucStatus > 100 && u_reply_param.ucStatus != MEGMEET_A6_VER_LATEST)
            {
                bUpdate_SetErrCode(UEF_DR_A6_REPLY_ERR);
                return -52;
            }

            if (u_reply_param.ucStatus < 100)
            {
                bUpdate_SetErrCode(UEF_DR_A6_NOT_COMPLETE);
                return -53;
            }

            if (u_reply_param.ucStatus == MEGMEET_A6_VER_LATEST)
                bUpdate_SetResult(URT_SLAVE, UTR_LATEST);
            else
                bUpdate_SetResult(URT_SLAVE, UTR_OK);

            bDcac_SetFwTransStage(tpDcacTask, DFTS_FINISH_CLEANUP);
        }
		break;

        /* 0xFF 错误回复 */
        case MEGMEET_CMD_ERR_REPLY:
        {
            if (tp_frame->usPayloadLen < 1)
                return -8;
            bUpdate_SetErrCode(UEF_DR_ERR_FRAME);
        }
        break;

        default:
            return -99;
    }

    return 1;
}
#endif  /* boardUPDATE */

#endif  /* boardDCAC_EN */
