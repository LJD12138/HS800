#include "MD_Bms/md_bms_rec_data_proc.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Print/print_task.h"

#if(boardUPDATE)
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE


//****************************************************函数声明****************************************************//
static s8 c_relay08_param(ModbusProtoRx1_t* proto_rx);


/***********************************************************************************************************************
-----函数功能    处理接收到的数据
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
s8 c_bms_rec_proc_data(ModbusProtoRx1_t* proto_rx, ModbusProtoTx1_t* proto_tx)
{
	s8 c_ret = 1;
	vu16 us_temp = 0;
    
	if(uPrint.tFlag.bBmsRecTask)
	{
		sMyPrint("bBmsRecTask:接收地址%d:", proto_tx->usRegAddr);
		for(int i = 0; i < proto_rx->ucValidLen; i++)
			sMyPrint("%x ",proto_rx->ucpValidData[i]);
		sMyPrint("\r\n");
	}

	if(proto_rx->ucCmd == modbusREAD_MULTI_REG ||
		proto_rx->ucCmd == modbusREAD_MULTI_BIT)
	{
		if(proto_rx->ucCharLen != proto_tx->ucCharLen)
			return -1;
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
	}
	
	switch(proto_tx->usRegAddr)
    {
		//回复开关
        case bmsREG_ADDR_CTRL_SWITCH:               
        {
//			if(proto_rx->ucValidLen != 2 || proto_rx->ucpValidData == NULL)
//				return -10;

//			if(tpBmsTask->tReplyBuff.buff == NULL)
//				return -11;

			TaskInParam_U u_reply_param;

			if(proto_rx->ucpValidData[0])
			{
				u_reply_param.tTaskParam.ucObj = 0;
				u_reply_param.tTaskParam.ucParam = ST_ON;
			}
			else 
			{
				u_reply_param.tTaskParam.ucObj = 0;
				u_reply_param.tTaskParam.ucParam = ST_OFF;
			}

			lwrb_reset(&tpBmsTask->tReplyBuff);
			lwrb_write(&tpBmsTask->tReplyBuff, (u8*)&u_reply_param, sizeof(u_reply_param));
        }
        break;
		
		//回复参数
		case bmsREG_ADDR_GET_PARAM:                
        {
			c_ret = c_relay08_param(proto_rx);
			if(c_ret <= 0)
				return -20;
        }
        break;
		
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
static s8 c_relay08_param(ModbusProtoRx1_t* proto_rx)
{
	BmsParam_T t_param;
	u8 len = sizeof(t_param);
	
	if(proto_rx->ucCharLen != len || proto_rx->ucpValidData == NULL)
		return -1;
	
	memcpy((u8*)&t_param, proto_rx->ucpValidData, len);

	tBmsRx.usSOC = t_param.usSOC;
	tBmsRx.sTotalCurr = t_param.sTotalCurr;
	tBmsRx.usChgFullTime = t_param.usChgFullTime;
	tBmsRx.usDisChgEmptyTime = t_param.usDisChgEmptyTime;
	tBmsRx.tDevInfo[0].uErrCode.usCode = t_param.usErrCode & (~(0x0030));//清除充电过温和低温的错误位

	if(tBmsRx.usSOC < 100)
		tBmsRx.tState.bPermChg = 1;  //允许充电
	else 
		tBmsRx.tState.bPermChg = 0;

	if(tBmsRx.usSOC == 0)
		tBmsRx.tState.bImpermDisChg = 1;  //不允许放电
	else 
		tBmsRx.tState.bImpermDisChg = 0;

	if(t_param.usState)
		tBmsRx.tState.ucSysState = DS_WORK;
	else
		tBmsRx.tState.ucSysState = DS_SHUT_DOWN;

	//----------------------------获取充放电状态-----------------------------------------------
	if(t_param.sTotalCurr > 100)  //充电状态
		tBms.eWorkState = BWS_CHG;
	else 
		tBms.eWorkState = BWS_DISCHG;

	if(tBms.eWorkState == BWS_CHG)
		tBmsRx.tDevInfo[0].uErrCode.usCode |= t_param.usErrCode & 0x0030;
	
	//----------------------------获取故障位-------------------------------------------------
	static vu16  last_err_state=0;
	if(last_err_state != tBmsRx.tDevInfo[0].uErrCode.usCode)
	{
		last_err_state = tBmsRx.tDevInfo[0].uErrCode.usCode;
		if(tBmsRx.tDevInfo[0].uErrCode.usCode)
			bBms_SetErrCode(BEC_BMS_ERR,true);
		else 
			bBms_SetErrCode(BEC_BMS_ERR,false);
	}
	
	//----------------------------获取温度-----------------------------------------------
	tBmsRx.tDevInfo[0].sMaxTemp = t_param.sMaxTemp;
	tBmsRx.tDevInfo[0].sMinTemp = t_param.sMinTemp;
	tBms.sMaxTemp = tBmsRx.tDevInfo[0].sMaxTemp;
	tBms.sMinTemp = tBmsRx.tDevInfo[0].sMinTemp;
	
	return 1;
}

#endif  //boardBMS_EN
