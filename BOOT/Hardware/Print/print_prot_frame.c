#include "Print/print_prot_frame.h"

#if(boardPRINT_IFACE)
#include "Print/print_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"

#include "..\..\BOOT\Application\flash_allot_table.h"

#include "boot_info.h"

#if(boardCONSOLE_EN)
#include "MD_Console/md_console_task.h"
#endif  //boardCONSOLE_EN

#if(boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_heat_manage_task.h"
#endif  //boardHEAT_MANAGE_EN

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

#define     	printDEV_ADRR							printCONSOLE_MASTER_ADDR
#define    		printWAIT_NOTIFY_OUTTIME				1000     //任务通知超时时间 MS
#define       	printTX_FRAME_SIZE                     	256
#define       	printRX_FRAME_SIZE                     	256
														
//****************************************************参数初始化**************************************************//
__ALIGNED(4) BaikuProtoTx_t *tpPrintProtoTx = NULL;	//发送协议
__ALIGNED(4) BaikuProtoRx_t *tpPrintProtoRx = NULL;

vu8 uc_next_cmd = 0;

//****************************************************函数声明****************************************************//
static s8 c_print_data_trans(u8 cmd, u8* data, u8 len);

/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bPrint_SendProtInit(void)
{
	s8 c_result = cBaiku_ProtoSendInit(&tpPrintProtoTx,//协议指针
								printTX_FRAME_SIZE, 	//协议缓存器大小
								printDEV_ADRR);			//协议设备ID
	if(c_result <= 0)
		return false;
	
	return true;
}

bool bPrint_RecProtInit(void)
{
	s8 c_result = cBaiku_ProtoRecInit(&tpPrintProtoRx, 	//协议指针
								printRX_FRAME_SIZE,		//协议缓存器大小
								printDEV_ADRR,			//协议设备ID
								boardREPET_TIMER_CYCLE_TMIE);//计数器采样时间
	if(c_result <= 0)
		return false;
	
	return true;
}




/***********************************************************************************************************************
-----函数功能    持续回复
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_cycle_relay_data(void)
{
	static vu16 us_delay_cnt = 0;
	
	if(uc_next_cmd != 0x0D)
	{
		us_delay_cnt = 0;
		return 0;
	}
	
	us_delay_cnt++;
	if(us_delay_cnt >= (100/printTASK_CYCLE_TIME))
	{
		us_delay_cnt = 0;
//		c_relay0E_mppt_param();
	}
	return true;
}

/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay84_set_proto(BaikuProtoRx_t* proto)
{
	u8 obj = proto->ucpValidData[0];
	
	if(proto->ucValidLen != 3 || obj >= 2)
		return -10;

	if(cUpdate_ChSelect(CT_PRINT, (ProtoType_E)tpPrintProtoRx->ucpValidData[0]) <= 0)
		return -11;

	memcpy((u16*)&tUpdate.usTotalFrmValue, &tpPrintProtoRx->ucpValidData[1], 2);
	return c_print_data_trans(baikuCMD_REPLY_SET_PROTO, NULL, 0);
}

/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay84_set_print_state(BaikuProtoRx_t* proto)
{
	u8 temp = 0;
	u8 len = sizeof(uPrint) + 1;
	u8 obj = proto->ucpValidData[0];
	
	if(proto->ucValidLen != len || obj != 0 || proto->ucpValidData == NULL)
	{
		temp = 0xFF;
		c_print_data_trans(baikuCMD_REPLY_SET_PRINT_STATE, &temp, 1);
		return -10;
	}

	memcpy((u8*)&uPrint.ulFlag, &proto->ucpValidData[1], len -1);
	temp = 0x00;
	return c_print_data_trans(baikuCMD_REPLY_SET_PRINT_STATE, &temp, 1);
}

/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay86_get_print_state(BaikuProtoRx_t* proto)
{
	uint8_t data[5] = {0};
    uint8_t len = 0;
	u8 obj = proto->ucpValidData[0];
	
	if(proto->ucValidLen != 1 || obj != 0 ||  proto->ucpValidData == NULL)
		return -10;
	
	len = 1;
	data[0] = obj;
	
    len += sizeof(uPrint); 
    if(len > sizeof(data)) return -11;//data长度不足    
    memcpy(&data[1], (u8*)&uPrint, sizeof(uPrint));

    return c_print_data_trans(baikuCMD_REPLY_PRINT_STATE, data, sizeof(data));
}


/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay88_sys_set(BaikuProtoRx_t* proto)
{
	u8 temp = 0;
	tSysSetParam tparam;
			
	if(proto->ucValidLen != sizeof(tparam))
	{
		temp = 0xFF;
		c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1);
		return -10;
	}
	
	memcpy(&tparam, proto->ucpValidData, proto->ucValidLen);
	//BMS
	if(tparam.obj == UO_DEFAULT ||
		tparam.obj == UO_BMS)
	{
		if(tparam.cmd == mainUPDATE_FLAG)
		{
			temp = 0x00;
			c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1);
			
			#if(boardPRINT_IFACE)
			cUpdate_ChSelect(CT_PRINT, PT_XMODEM);
			#endif
		}
		else if(tparam.cmd == mainINIT_BOOT_PARAM_FLAG)
		{
			temp = 0x00;
			c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1);
			
			SysTaskId_E e_task_id = eBoot_InfoInit(true);
			cQueue_AddQueueTask(tpSysTask, e_task_id, 0, false);
		}
	}
	
	return 1;
}

/***********************************************************************************************************************
-----函数功能	数据传输
-----说明(备注) 
-----传入参数	cmd:指令
				data:指向数据指针
				len:数据的长度
-----输出参数	none
-----返回值		-1:写入的Len超出最大长度
				-2:等会回复超时
				-3:数据发送错误
				0:无操作
				1:操作成功
************************************************************************************************************************/
static s8 c_print_data_trans(u8 cmd, u8* data, u8 len)
{
	s8 result = 0;
	
	if(tpPrintProtoTx == NULL)
		return 0;
	
	#if(boardPRINT_IFACE)
	result = cBaiku_ProtoCreate(tpPrintProtoTx, cmd, data, len);
	if(result > 0)
	{
		lwrb_write(&tPrintTxBuff, tpPrintProtoTx->ucaFrameData, tpPrintProtoTx->ucFrameLen);
			return bPrint_SendDataToUsart();
	}
	#endif
	return result;
}

#endif  //boardPRINT_IFACE
