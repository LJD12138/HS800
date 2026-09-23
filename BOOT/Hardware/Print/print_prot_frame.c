/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Print
 * File    : print_prot_frame.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print通信协议帧组包、解析、分发及中继处理实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_prot_frame.h"

#if (boardPRINT_IFACE)
#include "Print/print_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"

#include "flash_allot_table.h"
#include "boot_info.h"

#if (boardCONSOLE_EN)
#include "MD_Console/md_console_task.h"
#endif  /* boardCONSOLE_EN */

#if (boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_heat_manage_task.h"
#endif  /* boardHEAT_MANAGE_EN */

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			printDEV_ADRR							printCONSOLE_MASTER_ADDR
#define			printWAIT_NOTIFY_OUTTIME				1000	/* 任务通知超时时间 MS */
#define			printTX_FRAME_SIZE						256
#define			printRX_FRAME_SIZE						256

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) BaikuProtoTx_t *tpPrintProtoTx = NULL;	/* 发送协议 */
__ALIGNED(4) BaikuProtoRx_t *tpPrintProtoRx = NULL;

vu8 uc_next_cmd = 0;

//****************************************************Function Declaration******************************************************//
static s8 c_print_data_trans(u8 cmd, u8 *data, u8 len);


/***********************************************************************************************************************
 * 函数功能    : 发送通讯协议初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bPrint_SendProtInit(void)
{
	s8 c_result = cBaiku_ProtoSendInit(&tpPrintProtoTx,		/* 协议指针 */
	                                   printTX_FRAME_SIZE,	/* 协议缓存器大小 */
	                                   printDEV_ADRR);		/* 协议设备ID */
	if (c_result <= 0)
		return false;
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 接收通讯协议初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bPrint_RecProtInit(void)
{
	s8 c_result = cBaiku_ProtoRecInit(&tpPrintProtoRx,				/* 协议指针 */
	                                  printRX_FRAME_SIZE,			/* 协议缓存器大小 */
	                                  sysDEV_ADRR,					/* 协议设备ID */
	                                  boardREPET_TIMER_CYCLE_TMIE);	/* 计数器采样时间 */
	if (c_result <= 0)
		return false;
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 周期循环回复数据
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 1: 发送成功, 0: 未触发
 ************************************************************************************************************************/
s8 c_cycle_relay_data(void)
{
	static vu16 us_delay_cnt = 0;
	
	if (uc_next_cmd != 0x0D)
	{
		us_delay_cnt = 0;
		return 0;
	}
	
	us_delay_cnt++;
	if (us_delay_cnt >= (100 / printTASK_CYCLE_TIME))
		us_delay_cnt = 0;
		/* c_relay0E_mppt_param(); */
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 回复设置升级协议命令 (0x84)
 * 说明(备注)  : none
 * 传入参数    : proto: 接收协议帧指针
 * 输出参数    : none
 * 返回值      : 1: 成功, 负数: 错误码
 ************************************************************************************************************************/
s8 c_relay84_set_proto(BaikuProtoRx_t *proto)
{
	u8 obj = proto->ucpValidData[0];
	
	if (proto->ucValidLen != 3 || obj >= 2)
		return -10;

	if (cUpdate_ChSelect(CT_PRINT, (ProtoType_E)tpPrintProtoRx->ucpValidData[0]) <= 0)
		return -11;

	memcpy((u16*)&tUpdate.usTotalFrmValue, &tpPrintProtoRx->ucpValidData[1], 2);
	return c_print_data_trans(baikuCMD_REPLY_SET_PROTO, NULL, 0);
}

/***********************************************************************************************************************
 * 函数功能    : 回复设置Print状态命令 (0x84)
 * 说明(备注)  : none
 * 传入参数    : proto: 接收协议帧指针
 * 输出参数    : none
 * 返回值      : 1: 成功, 负数: 错误码
 ************************************************************************************************************************/
s8 c_relay84_set_print_state(BaikuProtoRx_t *proto)
{
	u8 temp = 0;
	u8 len = sizeof(uPrint) + 1;
	u8 obj = proto->ucpValidData[0];
	
	if (proto->ucValidLen != len || obj != 0 || proto->ucpValidData == NULL)
	{
		temp = 0xFF;
		c_print_data_trans(baikuCMD_REPLY_SET_PRINT_STATE, &temp, 1);
		return -10;
	}

	memcpy((u8*)&uPrint.ulFlag, &proto->ucpValidData[1], len - 1);
	temp = 0x00;
	return c_print_data_trans(baikuCMD_REPLY_SET_PRINT_STATE, &temp, 1);
}

/***********************************************************************************************************************
 * 函数功能    : 回复获取Print状态命令 (0x86)
 * 说明(备注)  : none
 * 传入参数    : proto: 接收协议帧指针
 * 输出参数    : none
 * 返回值      : 1: 成功, 负数: 错误码
 ************************************************************************************************************************/
s8 c_relay86_get_print_state(BaikuProtoRx_t *proto)
{
	uint8_t data[5] = {0};
	uint8_t len = 0;
	u8 obj = proto->ucpValidData[0];
	
	if (proto->ucValidLen != 1 || obj != 0 || proto->ucpValidData == NULL)
		return -10;
	
	len = 1;
	data[0] = obj;
	
	len += sizeof(uPrint); 
	if (len > sizeof(data))
		return -11;	/* data长度不足 */
	
	memcpy(&data[1], (u8*)&uPrint, sizeof(uPrint));

	return c_print_data_trans(baikuCMD_REPLY_PRINT_STATE, data, sizeof(data));
}

/***********************************************************************************************************************
 * 函数功能    : 回复系统设置指令 (0x88)
 * 说明(备注)  : none
 * 传入参数    : proto: 接收协议帧指针
 * 输出参数    : none
 * 返回值      : 1: 成功, 负数: 错误码
 ************************************************************************************************************************/
s8 c_relay88_sys_set(BaikuProtoRx_t *proto)
{
	u8 temp = 0;
	tSysSetParam tparam;
			
	if (proto->ucValidLen != sizeof(tparam))
	{
		temp = 0xFF;
		c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1);
		return -10;
	}
	
	memcpy(&tparam, proto->ucpValidData, proto->ucValidLen);
	
	/* BMS */
	if (tparam.obj == MO_DEFAULT || tparam.obj == MO_BMS)
	{
		if (tparam.cmd == mainUPDATE_FLAG)
		{
			temp = 0x00;
			c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1);
			
			#if (boardPRINT_IFACE)
			cUpdate_ChSelect(CT_PRINT, PT_XMODEM);
			#endif  /* boardPRINT_IFACE */
		}
		else if (tparam.cmd == mainINIT_BOOT_PARAM_FLAG)
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
 * 函数功能    : 打印协议数据帧构建并发送
 * 说明(备注)  : none
 * 传入参数    : cmd: 指令码, data: 指向数据指针, len: 数据的长度
 * 输出参数    : none
 * 返回值      : -1: 写入的Len超出最大长度, -2: 等待回复超时, -3: 数据发送错误, 0: 无操作, 1: 操作成功
 ************************************************************************************************************************/
static s8 c_print_data_trans(u8 cmd, u8 *data, u8 len)
{
	s8 result = 0;
	
	if (tpPrintProtoTx == NULL)
		return 0;
	
	#if (boardPRINT_IFACE)
	result = cBaiku_ProtoCreate(tpPrintProtoTx, cmd, data, len);
	if (result > 0)
	{
		lwrb_write(&tPrintTxBuff, tpPrintProtoTx->ucaFrameData, tpPrintProtoTx->ucFrameLen);
		return bPrint_SendDataToUsart();
	}
	#endif  /* boardPRINT_IFACE */
	return result;
}

/***********************************************************************************************************************
 * 函数功能    : 转发数据到打印串口
 * 说明(备注)  : 用于升级协议单字节或透传数据直接写缓冲区并触发串口发送
 * 传入参数    : data: 指向数据指针, len: 数据的长度
 * 输出参数    : none
 * 返回值      : -2: 发送等待超时, 0: 无操作, 1: 操作成功
 ************************************************************************************************************************/
s8 c_print_info_trans(u8 *data, u8 len)
{
	if (tpPrintProtoTx == NULL || data == NULL || len == 0)
		return 0;

	if (bPrint_CheckSendFinish() == false)
		return -2;

	lwrb_reset(&tPrintTxBuff);
	lwrb_write(&tPrintTxBuff, data, len);
	bPrint_SendDataToUsart();
	return 1;
}

#endif  /* boardPRINT_IFACE */
