#include "MD_Dcac/md_dcac_rec_data_proc.h"
#include <string.h>

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#include "Print/print_prot_frame.h"
#include "Print/print_task.h"


#include "function.h"
#include "check.h"

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#include "MD_Dcac/md_dcac_queue_task_update.h"
#endif  //boardUPDATE

//****************************************************函数声明****************************************************//

  
/***********************************************************************************************************************
-----函数功能    处理接收到的数据
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
s8 c_dcac_rec_proc_data(ModbusProtoRx_t* proto_rx, ModbusProtoTx_t* proto_tx)
{
	static vu16  last_err_state=0;
	static vu16  us_overload_cnt=0;
	static vu16  us_over_curr_cnt=0;
	static vu16  us_check_volt_cnt=0;
	static vu16  us_open_ac_input_cnt=0;
	static vu16  us_total_curr = 0;
    static vu16  us_dyn_delay = 0;
	static vu16  us_temp = 0;
	u16 us_reg_data = 0;
	
	if(proto_rx == NULL || proto_tx == NULL)
		return 0;
	
	if(uPrint.tFlag.bDcacRecTask)
	{
		sMyPrint("bDcacRecTask:接收地址%d:", proto_tx->usRegAddr);
		for(int i = 0; i < proto_rx->ucValidLen; i++)
			sMyPrint("%x ",proto_rx->ucpValidData[i]);
		sMyPrint("\r\n");
	}
	
	if(proto_rx->ucCmd == modbusREAD_MULTI_REG ||
		proto_rx->ucCmd == modbusREAD_MULTI_BIT)
	{
		if(proto_rx->ucCharLen != proto_tx->ucCharLen)
		{
			if(uPrint.tFlag.bDcacRecTask || uPrint.tFlag.bImportant)
				log_w("bDcacRecTask:迟到回复(期望长度%d,收到%d),当前等待寄存器%d",
					proto_tx->ucCharLen, proto_rx->ucCharLen, proto_tx->usRegAddr);
			return -1;
		}
		if(proto_rx->ucValidLen != proto_rx->ucCharLen || proto_rx->ucpValidData == NULL)
			return -7;
	}
	else if(proto_rx->ucCmd == modbusWRITE_MULTI_REG)
	{
		if(proto_rx->usRegAddr != proto_tx->usRegAddr ||
			proto_rx->usRegSize != proto_tx->usRegSize)
			return -2;
	}
	else if(proto_rx->ucCmd == modbusWRITE_SINGLE_REG ||
		proto_rx->ucCmd == modbusWRITE_SINGLE_BIT)
	{
		if(proto_rx->usRegAddr != proto_tx->usRegAddr)
			return -3;
		if(proto_rx->ucValidLen != 2 || proto_rx->ucpValidData == NULL)
			return -8;
		bFunc_SwapU16Array((u8*)&us_reg_data, proto_rx->ucpValidData, 1);
		if(us_reg_data != proto_tx->usRegData)
			return -9;
	}

	switch(proto_tx->usRegAddr)
	{
		case dcacREG_ADDR_GET_PARAM1 :
		{
			DCAC_Param1_t tParam1;
			
			if(proto_rx->ucCharLen != sizeof(tParam1))
				return -4;
			
			//装载参数
			bFunc_SwapU16Array((u8*)&tParam1, proto_rx->ucpValidData, proto_rx->ucCharLen / 2);
			//更新数据
			tDcacRx.usOutVolt = tParam1.usOutVolt;

			//G3604 0.1A     G2404 0.01A
			if(strstr(boardSOFTWARE_VERSION, "G3604") != NULL)
				tDcacRx.usOutCurr = LIMIT_MIN(tParam1.sOutCurr, 0);
			else
				tDcacRx.usOutCurr = LIMIT_MIN(tParam1.sOutCurr / 10, 0);
			
			if(tParam1.usOutPwr > 2)
				tDcacRx.usOutPwr = tParam1.usOutPwr;
			else
				tDcacRx.usOutPwr = 0;
				
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
		
		case dcacREG_ADDR_GET_PARAM2 :
		{
			DCAC_Param2_t tParam2;
			
			if(proto_rx->ucCharLen != sizeof(tParam2))
				return -5;

			//装载参数
			bFunc_SwapU16Array((u8*)&tParam2, proto_rx->ucpValidData, proto_rx->ucCharLen/2);
			//更新数据
			// if(tMppt.eDevState > DS_BOOTING && tDcac.eChgState == DS_SHUT_DOWN)
				tDcacRx.uErrCode.usCode[0] = tParam2.uDcErrCode & (~0x0001);
			// else
				// tDcacRx.uErrCode.usCode[0] = tParam2.uDcErrCode;

			tDcacRx.uErrCode.usCode[1] = tParam2.uAcErrCode;
			tDcacRx.uErrCode.usCode[2] = tParam2.uInErrCode & (~0x140);
			tDcacRx.uErrCode.usCode[3] = tParam2.usSysErr & 0x01;
		}
		break;
		
		case dcacREG_ADDR_GET_PARAM3 :
		{
			DCAC_Param3_t tParam3;
			
			if(proto_rx->ucCharLen != sizeof(tParam3))
				return -6;

			//装载参数
			bFunc_SwapU16Array((u8*)&tParam3, proto_rx->ucpValidData, proto_rx->ucCharLen/2);
			//更新数据
			tDcacRx.usInVolt = tParam3.usAcInVolt;
			tDcacRx.usInPwr = LIMIT_MIN(tParam3.sAcInPwr, 0);
			tDcacRx.usInChgPwr = LIMIT_MIN(tParam3.sAcChgPwr, 0);
			tDcacRx.usChgPwr = LIMIT_MIN(tParam3.sBatInPwr, 0);

			//G3604 0.1A     G2404 0.01A
			if(strstr(boardSOFTWARE_VERSION, "G3604") != NULL)
				tDcacRx.usInCurr = LIMIT_MIN(tParam3.sAcInCurr, 0);
			else
				tDcacRx.usInCurr = LIMIT_MIN(tParam3.sAcInCurr / 10, 0);
		}
		break;
		
//		case dcacREG_ADDR_INIT:
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
-----函数功能    处理Megmeet协议数据
-----说明(备注)  DCAC升级期只解析从机回复并更新升级状态，baiku回传由Print升级任务统一组帧。
************************************************************************************************************************/
#if(boardUPDATE)
s8 c_dcac_rec_proc_megmeet_proto(MegmeetProtoRx_t* tp_proto_rx)
{
	UpdateFrame_t* tp_frame = NULL;
//	s8 c_result = 1;

	if(tp_proto_rx == NULL)
		return -1;

	tp_frame = &tp_proto_rx->tFrame;
	if(tp_frame->ucpFrame == NULL || tp_frame->usFrameLen < MEGMEET_FRAME_MIN_FRAME_LEN)
		return -2;

	if(tp_frame->usPayloadLen > 0 && tp_frame->ucpPayload == NULL)
		return -3;

	if(tpDcacTask->tReplyBuff.buff == NULL)
		return -4;

	if(uPrint.tFlag.bDcacRecTask)
	{
		sMyPrint("\r\n bDcacRecTask:Megmeet cmd=0x%x len=%d:", tp_frame->ucCmd, tp_frame->usPayloadLen);
		for(int i = 0; i < tp_frame->usPayloadLen; i++)
			sMyPrint("%x ", tp_frame->ucpPayload[i]);
		sMyPrint("\r\n");
	}

	// 只要收到 Megmeet 回复，就认为升级链路仍然活跃，先刷新超时计时。
	vUpdate_ResetTimeout();

	switch(tp_frame->ucCmd)
	{
		//F1 回复请求升级
		case MEGMEET_CMD_REQ_UPDATE_REPLY:
		{
			if(tp_frame->usPayloadLen != 1 || tp_frame->ucpPayload == NULL)
				return -10;

			if(eDcacPrepStage != DPS_WAIT_F1)
				return 0;

			u8 u_reply_param = tp_frame->ucpPayload[0];

			if(u_reply_param != 0x01)
			{
				bUpdate_SetErrCode(UEF_D_F1_CHECK_FAIL);
				return -3;
			}
			
			bDcac_SetPrepStage(tpDcacTask, DPS_SEND_F2);
		}
		break;

		//F3 回复切换波特率
		case MEGMEET_CMD_SET_BAUD_REPLY:
		{
			if(tp_frame->usPayloadLen != 1 || tp_frame->ucpPayload == NULL)
				return -20;

			if(eDcacPrepStage != DPS_WAIT_F3)
				return 0;

			u8 u_reply_param = tp_frame->ucpPayload[0];

			/* 0xFF表示无对应波特率 */
			if(u_reply_param == MEGMEET_BAUD_INVALID)
			{
				bUpdate_SetErrCode(UEF_D_F3_BAUD_REPLY);
				return -8;
			}

			/* 0x00表示成功切换 */
			if(u_reply_param != MEGMEET_BAUD_OK)
			{
				bUpdate_SetErrCode(UEF_D_F3_CHECK_FAIL);
				return -9;
			}

			if(bDcac_IfaceSetBaud(tUpdate.ulBaud) == false)
			{
				bUpdate_SetErrCode(UEF_D_F3_SET_BAUD);
				return -10;
			}

			/* 从机已确认波特率切换，本地串口已在接收中断中完成切换 */
			bDcac_SetDevState(DS_UPDATE_MODE);
			bDcac_SetPrepStage(tpDcacTask, DPS_WAIT_PRINT_UPDATE_REQ);
		}
		break;

		//F7 回复跳转 Boot
		case MEGMEET_CMD_JUMP_BOOT_REPLY:
		{
			if(eDcacPrepStage != DPS_WAIT_F7)
				return 0;

			if(tp_frame->usPayloadLen != 0)
			{
				bUpdate_SetErrCode(UEF_D_F7_CHECK_FAIL);
				return -60;
			}

			bDcac_SetPrepStage(tpDcacTask,DPS_BOOT_DELAY);
		}
		break;

		// A2 文件头回复
		case MEGMEET_CMD_FILE_HEAD_REPLY:
		{
			b_dcac_update_buf_reset(tpDcacTask);
				
			if(tp_frame->usPayloadLen != 1 || tp_frame->ucpPayload == NULL)
				return -30;

			if(eDcacPrepStage != DPS_WAIT_A2)
				return -31;

			//读取数据
			u8 u_reply_param = tp_frame->ucpPayload[0];

			if(u_reply_param != MEGMEET_A2_OK &&
				u_reply_param != MEGMEET_A2_VER_LATEST)
			{
				bUpdate_SetErrCode(UEF_D_A2_REPLY_ERR);
				break;
			}

			/* A2 已经是最新版本 */
			if(u_reply_param == MEGMEET_A2_VER_LATEST)
			{
				bUpdate_SetResult(URT_SLAVE, UTR_LATEST);
				cQueue_GotoStep(tpDcacTask, DUS_STEP_FINISH_CLEANUP);
				break;
			}

			bDcac_SetPrepStage(tpDcacTask, DPS_FINISH_CLEANUP);
		}
		break;

		//A4 固件数据回复
		case MEGMEET_CMD_FIRMWARE_DATA_REPLY:
		{
			b_dcac_update_buf_reset(tpDcacTask);
				
			//读取数据
			#pragma pack(1)
			struct {
				u16 usSeqNum;
				u8  ucStatus;
			} u_reply_param;
			#pragma pack()

			if(tp_frame->usPayloadLen != sizeof(u_reply_param) || tp_frame->ucpPayload == NULL)
				return -40;

			memcpy(&u_reply_param, tp_frame->ucpPayload, tp_frame->usPayloadLen);

			/* 校验包序号是否匹配 */
			if(u_reply_param.usSeqNum != tUpdate.usRecFrameCnt)
			{
				bUpdate_SetErrCode(UEF_D_A4_SEQ_MISMATCH);
				return -41;
			}

			if(u_reply_param.ucStatus != MEGMEET_A4_OK &&
			   u_reply_param.ucStatus != MEGMEET_A4_ALL_OK)
			{
				bUpdate_SetErrCode(UEF_D_A4_REPLY_ERR);
				return -42;
			}

			#if(boardUSE_OS)
			taskENTER_CRITICAL();
			#endif
			u16 us_pending_len = tUpdate.usPendPacketLen;
			u32 ul_pend_crc = tUpdate.ulFwPendCrc32;
			tUpdate.usPendPacketLen = 0;
			#if(boardUSE_OS)
			taskEXIT_CRITICAL();
			#endif

			if(us_pending_len == 0)
				return 0;
			
			if(tUpdate.usRecFrameCnt < 0xFFFF)
                tUpdate.usRecFrameCnt++;

			tUpdate.ulFwCalcCrc32 = ul_pend_crc;
			tUpdate.ulRxSize += us_pending_len;

			/* A4 已经全部完成 */
			if(u_reply_param.ucStatus == MEGMEET_A4_ALL_OK)
			{
				bDcac_SetFwTransStage(tpDcacTask, DFTS_QUERY_SLAVE_RESULT);
				break;
			}

			//Print已经结束
			if(tUpdate.eHostResult == UTR_OK || tUpdate.eHostResult == UTR_CANCEL)
			{
				bDcac_SetFwTransStage(tpDcacTask, DFTS_QUERY_SLAVE_RESULT);
				break;
			}

			/* 通知Print请求下一包数据 */
			bDcac_SetFwTransStage(tpDcacTask, DFTS_HOST_REQ_DATA);
		}
		break;

		// A6 查询结果回复
		case MEGMEET_CMD_QUERY_RESULT_REPLY:
		{
			//读取数据
                #pragma pack(1)
                struct {
                    u8  ucStatus;
                    u8  ucSlaveAddr;
                    u8  ucChipId;
                } u_reply_param;
                #pragma pack()

			if(tp_frame->usPayloadLen != sizeof(u_reply_param) || tp_frame->ucpPayload == NULL)
			return -50;

			memcpy(&u_reply_param, tp_frame->ucpPayload, tp_frame->usPayloadLen);

			/* 校验从机地址和芯片ID */
		if(u_reply_param.ucSlaveAddr != ucDcac_GetUpdateSlaveAddr(tUpdate.eObj) || 
		   u_reply_param.ucChipId != ucDcac_GetUpdateIcType(tUpdate.eObj))
		{
			bUpdate_SetErrCode(UEF_D_A6_CHECK_FAIL);
			return -51;
		}

			//回复错误
			if(u_reply_param.ucStatus > 100 &&
			   u_reply_param.ucStatus != MEGMEET_A6_VER_LATEST)
			{
				bUpdate_SetErrCode(UEF_D_A6_REPLY_ERR);
				return -52;
			}

			//回复未升级完成
			if(u_reply_param.ucStatus < 100)
			{
				bUpdate_SetResult(URT_SLAVE, UTR_RUNNING);
				bDcac_SetFwTransStage(tpDcacTask, DFTS_QUERY_SLAVE_RESULT);
				break;
			}

			/* A6 已经是最新 */
			if(u_reply_param.ucStatus == MEGMEET_A6_VER_LATEST)
				bUpdate_SetResult(URT_SLAVE, UTR_LATEST);
			else
				bUpdate_SetResult(URT_SLAVE, UTR_OK);

			/* 升级完成 */
			bDcac_SetFwTransStage(tpDcacTask, DFTS_FINISH_CLEANUP);
		}
		break;

		// 0xFF 错误回复
		case MEGMEET_CMD_ERR_REPLY:
		{
			if(tp_frame->usPayloadLen < 1)
				return -8;

			bUpdate_SetErrCode(UEF_DR_ERR_FRAME);
		}
		break;

		default:
			return -99;
	}

	return 1;
}
#endif  //boardUPDATE
#endif  //boardDCAC_EN
