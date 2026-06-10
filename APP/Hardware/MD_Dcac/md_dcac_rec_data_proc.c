#include "MD_Dcac/md_dcac_rec_data_proc.h"
#include "MD_Mppt/md_mppt_task.h"
#include "main.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_task.h"
#include "Print/print_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"

#include "function.h"

//****************************************************参数初始化**************************************************//   


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
		sMyPrint("\r\n bDcacRecTask:接收地址%d:", proto_tx->usRegAddr);
		for(int i = 0; i < proto_rx->ucValidLen; i++)
			sMyPrint("%x ",proto_rx->ucpValidData[i]);
		sMyPrint("\r\n");
	}
	
	if(proto_rx->ucCmd == modbusREAD_MULTI_REG ||
		proto_rx->ucCmd == modbusREAD_MULTI_BIT)
	{
		if(proto_rx->ucCharLen != proto_tx->ucCharLen)
			return -1;
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
#endif  //boardDCAC_EN
