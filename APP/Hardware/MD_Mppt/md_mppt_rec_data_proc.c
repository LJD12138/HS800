#include "MD_Mppt/md_mppt_rec_data_proc.h"

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_task.h"
#include "Print/print_task.h"

#include "function.h"
#include "app_info.h"

//****************************************************参数初始化**************************************************//   


/***********************************************************************************************************************
-----函数功能    处理接收到的数据
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
s8 c_mppt_rec_proc_data(ModbusProtoRx_t* proto_rx, ModbusProtoTx_t* proto_tx)
{
	static vu16  last_err_state=0; 
	static vu16  us_overload_cnt=0; 
	static vu16  us_over_curr_cnt=0; 
	static vu16  us_check_volt_cnt=0;
	static vu16  us_open_ac_input_cnt=0;
	static vu16  us_total_curr = 0;
    static vu16  us_dyn_delay = 0;
	static vu16  us_temp = 0;
	static u8    uc_pwr_level = 0;  // 当前功率档位 (0-3)
	
	if(uPrint.tFlag.bMpptRecTask)
	{
		sMyPrint("\r\n bMpptRecTask:接收地址%d:", proto_tx->usRegAddr);
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
		case mpptREG_ADDR_SET_PV_CHG_PWR:
		{
			if(proto_rx->ucValidLen != 2 || proto_rx->ucpValidData == NULL)
				return -10;

			if(tpMpptTask->tReplyBuff.buff == NULL)
				return -11;
			
			#pragma pack (1)   //强制进行1字节对齐
			struct
			{
//				u8 uc_obj;
				u16 us_in_pwr;
			}t_mppt_chg;
			#pragma pack() //取消一个字节对齐
			
			t_mppt_chg.us_in_pwr = (proto_rx->ucpValidData[0] << 8) | proto_rx->ucpValidData[1];

			lwrb_reset(&tpMpptTask->tReplyBuff);
			lwrb_write(&tpMpptTask->tReplyBuff, (u8*)&t_mppt_chg, sizeof(t_mppt_chg));
		}
		break;
		
		case mpptREG_ADDR_GET_PARAM1 :
		{
			MpptParam_T t_param;
			
			if(proto_rx->ucCharLen != sizeof(t_param))
				return -20;
			
			if(proto_rx->ucpValidData == NULL)
				return -21;
			
			//装载参数
			bFunc_SwapU16Array((u8*)&t_param, proto_rx->ucpValidData, proto_rx->ucCharLen/2);
			
			if(t_param.usInState != 0)
				bMppt_SetDevState(DS_WORK);
			else
				bMppt_SetDevState(DS_SHUT_DOWN);
			
			tMpptRx.uInType = (MpptInType_U)t_param.usInState;
			tMpptRx.uErrCode.usCode = t_param.usErrCode;
			
			tMpptRx.usInVolt = t_param.usInVolt;
			tMpptRx.usInCurr = t_param.usInCurr * 10;
			tMpptRx.usInPwr = t_param.usInPwr * 10;

			// 功率到温度的映射，带滞回功能防止频繁切换
			// 最高800W，间隔200W一个档位，共4档：41, 45, 49, 55
			// 滞回阈值：50W（上升沿和下降沿各50W）
			s16 s_pwr_temp = 41;  // 默认温度
			
			if(tMppt.eDevState == DS_WORK)
			{
				u32 us_pwr = tMpptRx.usInPwr / 10;
				u8 uc_new_level = uc_pwr_level;
				
				// 根据当前档位和滞回逻辑判断新档位
				switch(uc_pwr_level)
				{
					case 0:  // 当前档位0 (41°C)，阈值: 0-200W
						if(us_pwr >= 250)  // 上升沿阈值250W
							uc_new_level = 1;
						break;
					case 1:  // 当前档位1 (45°C)，阈值: 200-400W
						if(us_pwr >= 450)  // 上升沿阈值450W
							uc_new_level = 2;
						else if(us_pwr < 150)  // 下降沿阈值150W
							uc_new_level = 0;
						break;
					case 2:  // 当前档位2 (49°C)，阈值: 400-600W
						if(us_pwr >= 650)  // 上升沿阈值650W
							uc_new_level = 3;
						else if(us_pwr < 350)  // 下降沿阈值350W
							uc_new_level = 1;
						break;
					case 3:  // 当前档位3 (55°C)，阈值: 600-800W
						if(us_pwr < 550)  // 下降沿阈值550W
							uc_new_level = 2;
						break;
					default:
						uc_new_level = 0;
						break;
				}
				
				// 更新档位
				uc_pwr_level = uc_new_level;
				
				// 根据档位映射温度
				switch(uc_pwr_level)
				{
					case 0: s_pwr_temp = 41; break;
					case 1: s_pwr_temp = 45; break;
					case 2: s_pwr_temp = 49; break;
					case 3: s_pwr_temp = 55; break;
					default: s_pwr_temp = 41; break;
				}
				
				// 取功率映射温度和sMpptMaxTemp中较大值
				tMpptRx.sMaxTemp = (s_pwr_temp > sMpptMaxTemp) ? s_pwr_temp : sMpptMaxTemp;
			}
			else
			{
				tMpptRx.sMaxTemp = 25;
				uc_pwr_level = 0;  // 非工作状态重置档位
			}

			tMppt.sMaxTemp = tMpptRx.sMaxTemp;
		}
		break;
		
		default:
			return -99;
	}
    return 1;
}
#endif  //boardMPPT_EN
