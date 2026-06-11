#include "MD_Bms/md_bms_rec_data_proc.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#include "Print/print_task.h"
#include "Baiku/baiku_proto.h"

#if(boardUPDATE)
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE


//****************************************************函数声明****************************************************//
static s8 c_relay08_param(BaikuProtoRx_t* proto);


/***********************************************************************************************************************
-----函数功能    处理接收到的数据
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
s8 c_bms_rec_proc_data(BaikuProtoRx_t* proto)
{
	s8 c_ret = 1;
	vu16 us_temp = 0;
    
	if(uPrint.tFlag.bBmsRecTask)
	{
		sMyPrint("bBmsRecTask:指令:0x%x, 数据:",proto->ucCmd);
		for(int i = 0; i < proto->ucValidLen; i++)
			sMyPrint("%x ",proto->ucpValidData[i]);
		sMyPrint("\r\n");
	}
	
	switch (proto->ucCmd)
    {
		//回复开关
        case baikuCMD_REPLY_SWITCH:               
        {
			if(proto->ucValidLen != 2 || proto->ucpValidData == NULL)
				return -10;

			if(tpBmsTask->tReplyBuff.buff == NULL)
				return -11;

			lwrb_reset(&tpBmsTask->tReplyBuff);
			lwrb_write(&tpBmsTask->tReplyBuff, proto->ucpValidData, proto->ucValidLen);
        }
        break;
		
		//回复参数
		case baikuCMD_REPLY_PARAM:                
        {
			c_ret = c_relay08_param(proto);
			if(c_ret <= 0)
				return -20;
        }
        break;
		
		//回复校准结果
		case baikuCMD_REPLY_CALI://45            
        {
			if(proto->ucValidLen != 2 || proto->ucpValidData == NULL)
				return -40;
			
			memcpy((u8*)&us_temp, proto->ucpValidData, proto->ucValidLen);

			#if(boardPRINT_IFACE)
			if(cQueue_AddQueueTask(tpPrintTask, PTI_REPLY_CALI, us_temp, false) <= 0)
				return -41;
			#endif  //boardPRINT_IFACE
        }
        break;
		
		//回复设置结果
		case baikuCMD_REPLY_SYS_SET://89            
        {
            
        }
        break;
		
		//回复APP信息
		case baikuCMD_REPLY_MEM_PARAM://81
        {
            if(proto->ucValidLen == 0 || proto->ucpValidData == NULL)
				return -50;
			
			#if(boardPRINT_IFACE)
			if(tpPrintTask->tReplyBuff.buff == NULL)
				return -51;

			lwrb_reset(&tpPrintTask->tReplyBuff);
			lwrb_write(&tpPrintTask->tReplyBuff, proto->ucpValidData, proto->ucValidLen);

			if(cQueue_AddQueueTask(tpPrintTask, PTI_REPLY_APP_INFO, proto->ucValidLen, false) <= 0)
				return -52;
			#endif  //boardPRINT_IFACE
        }
        break;

		//回复协议设置
		#if(boardUPDATE)
		case baikuCMD_REPLY_SET_PROTO://C3
		{
			if(proto->ucValidLen != 3)
				return -80;

			if((ProtoType_E)proto->ucpValidData[0] != tUpdate.eProtoType)
				return -81;

			memcpy((u8*)&tUpdate.usTotalFrmValue, &proto->ucpValidData[1], 2);
		}
		break;
		
		//请求开始发送
		case baikuCMD_RRQ_START_SEND://C4               
        {
			tUpdate.usRecFrameCnt = 0;

			if(tBms.eDevState == DS_UPDATE_MODE
				&& tSysInfo.eDevState == DS_UPDATE_MODE 
				&& tUpdate.eObj == UO_BMS
				&& tUpdate.eChType == CT_PRINT
				&& tUpdate.eProtoType == PT_BAIKU)
				return 1;
			
			if(tUpdate.eChType != CT_PRINT)
				if(cUpdate_ChSelect(UO_BMS, CT_PRINT) <= 0)
					return -71;

			if(tUpdate.eProtoType != PT_BAIKU)
				if(cUpdate_ProtoSelect(UO_BMS, PT_BAIKU) <= 0)
					return -72;

			if(tBms.eDevState != DS_UPDATE_MODE)
				cQueue_AddQueueTask(tpBmsTask, BTI_UPDATE, 0, false);
        }
        break;

		//继续发送
		case baikuCMD_RRQ_CONT_SEND:  //C6
		{
			tUpdate.usRecFrameCnt++;
		}
		break;

		//取消发送
		case baikuCMD_REPLY_CANEL:  //C8
		{
			tUpdate.usRecFrameCnt = 0;
		}
		break;
		
		//BMS正在升级
		case baikuCMD_BMS_UPDATE://C9
        {
			#pragma pack(1)
			struct
			{
				vu16				usRecFrameCnt;		//记录当前接收的帧数
				vu16 				usTotalFrmValue; 	//总帧数
			}t_my_param;
			#pragma pack()


			if(tpBmsTask->ucID == BTI_REQ_SET_CMD)
				return -60;
			
			if(proto->ucValidLen != sizeof(t_my_param) || proto->ucpValidData == NULL)
				return -61;
			
			memcpy((u8*)&t_my_param, proto->ucpValidData, proto->ucValidLen);

			tUpdate.usRecFrameCnt = t_my_param.usRecFrameCnt;
			tUpdate.usTotalFrmValue = t_my_param.usTotalFrmValue;
			
            if(cUpdate_ChSelect(UO_BMS, CT_NULL) <= 0)
				return -62;
        }
        break;
		#endif  //boardUPDATE
		
		default:
			return -99;
	}
	
   return 1; 

}

/***********************************************************************************************************************
-----函数功能    回复参数  0x08
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
static s8 c_relay08_param(BaikuProtoRx_t* proto)
{
	u8 len = sizeof(tBmsRx);
	u8 cmd = proto->ucpValidData[0];
	
	if(proto->ucValidLen != (len + 1) || proto->ucpValidData == NULL)
		return -1;
	
	if(cmd != 0x00 && cmd != 0x01)
		return -2;
	
	memcpy((u8*)&tBmsRx,&proto->ucpValidData[1],len);

	static vu16  last_err_state=0;
	
	ulBmsRxErrCode = 0;
	for(int i = 0; i < bmsDEV_NUM; i++)
		ulBmsRxErrCode |= tBmsRx.tDevInfo[i].uErrCode.ulCode;
	
	//----------------------------获取故障位-------------------------------------------------
	if(last_err_state != ulBmsRxErrCode) 
	{
		last_err_state = ulBmsRxErrCode;
		if(ulBmsRxErrCode)
			bBms_SetErrCode(BEC_BMS_ERR,true);
		else 
			bBms_SetErrCode(BEC_BMS_ERR,false);
	}
	
	//----------------------------获取充放电状态-----------------------------------------------
	if(tBmsRx.sTotalCurr > 0)  //充电状态
		tBms.eWorkState = BWS_CHG;
	else 
		tBms.eWorkState = BWS_DISCHG;
	//sMyPrint("BMS电流%d  充电状态%d  温度 = %d\r\n ",tBmsRx.sTotalCurr,bBms_GetBmsChgState(),tSysInfo.sMaxTemp);
	
	//----------------------------获取温度-----------------------------------------------
	vs16 s_temp_max = tBmsRx.tDevInfo[0].sMaxTemp;
	vs16 s_temp_min = tBmsRx.tDevInfo[0].sMinTemp;
	if(tBmsRx.tDevNum.ucOnlineNum > 0)
	{
		for(int i = 1; i < tBmsRx.tDevNum.ucOnlineNum; i++)
		{
			s_temp_max = MAX2(s_temp_max, tBmsRx.tDevInfo[i].sMaxTemp);
			s_temp_min = MIN2(s_temp_min, tBmsRx.tDevInfo[i].sMinTemp);
		}
	}
	tBms.sMaxTemp = s_temp_max;
	tBms.sMinTemp = s_temp_min;
	
	return 1;
}

#endif  //boardBMS_EN
