/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application\Sys
 * File    : sys_queue_task_update.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统升级队列任务调度与通道/协议处理实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task_update.h"

#if (boardUPDATE)
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task.h"
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "Update/update_main.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  /* boardBMS_EN */

#if (boardCONSOLE_EN)
#include "MD_Console/md_console_task.h"
#include "MD_Console/md_console_rec_task.h"
#include "MD_Console/md_console_iface.h"
#include "MD_Console/md_console_prot_frame.h"
#endif  /* boardCONSOLE_EN */

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
Update_T tUpdate;

//****************************************************Function Declaration******************************************************//


/***********************************************************************************************************************
 * 函数功能    : 系统升级队列任务步骤执行
 * 说明(备注)  : 步骤0选择升级通道并初始化, 步骤1轮询对应协议解析
 * 传入参数    : tp_task: 任务控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_sys_queue_task_update(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
		case 0:
		{
			//未定义,初始化
			if (tUpdate.eChType == CT_NULL || tUpdate.eProtoType == PT_NULL)
			{
				#if (boardCONSOLE_EN)
				cUpdate_ChSelect(CT_CONSOLE, PT_BAIKU);
				#elif (boardPRINT_IFACE)
				cUpdate_ChSelect(CT_PRINT, PT_XMODEM);
				#endif  /* (boardCONSOLE_EN) */
			}
			
			if (tUpdate.eChType == CT_PRINT)
				uPrint.ulFlag = 0;
			else 
			{
				uPrint.tFlag.bSysTask = 1;
				uPrint.tFlag.bBaiKuProto = 0;
				uPrint.tFlag.bUpdate = 0;
				uPrint.tFlag.bBootInfo = 1;
			}
			bQueue_Reset(tp_task); //清空队列
			cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
		}
		break;
		
		case 1:
		{
			switch (tUpdate.eProtoType)
			{
				case PT_XMODEM:
				{
					vXmodem_Proto(&tXmodem);
				}
				break;
				
				case PT_BAIKU:
				{
					vBaiKuProto_Proto(&tBaiKuProto, tUpdate.tpProtoRx);
				}
				break;
				
				default:
				{
				}
				break;
			}

			//队列里面有任务
			if (!bQueue_IsQueueEmpty(tp_task))                 
				cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
		
        default:
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 升级任务参数初始化
 * 说明(备注)  : 清空升级管理结构体
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功
 ************************************************************************************************************************/
bool bUpdate_Init(void)
{
	memset(&tUpdate, 0, sizeof(tUpdate));
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 升级通信通道与协议绑定选择
 * 说明(备注)  : 关联对应通信外设环形缓冲区与协议处理对象
 * 传入参数    : ch_type: 通道类型(ChannelType_E); proto_type: 协议类型(ProtoType_E)
 * 输出参数    : 无
 * 返回值      : 1: 成功, <0: 错误
 ************************************************************************************************************************/
s8 cUpdate_ChSelect(ChannelType_E ch_type, ProtoType_E proto_type)
{
	s8 c_result = 1;
	
	switch (ch_type)
	{
		#if (boardCONSOLE_EN)
		case CT_CONSOLE:
		{
			if (tpConsoleProtoRx == NULL || tpConsoleProtoRx->tRxBuff.buff == NULL)
				return -2;
			
			if (tpConsoleProtoTx == NULL)
				return -3;
			
			//接收Buff
			tUpdate.pRxBuff = &tpConsoleProtoRx->tRxBuff;
			cBaiku_ResetRxBuff(tpConsoleProtoRx);
			//接收协议
			tUpdate.tpProtoRx = tpConsoleProtoRx;
			
			//发送协议
			tUpdate.tpProtoTx = tpConsoleProtoTx;
			
			if (cUpdate_ProtoSelect(proto_type) == false)
				return -1;
		}
		break;
		#endif  /* boardCONSOLE_EN */
		
		#if (boardPRINT_IFACE)
		case CT_PRINT:
		{
			if (tpPrintProtoRx == NULL || tpPrintProtoRx->tRxBuff.buff == NULL)
				return -2;
			
			//发送Buff
			tUpdate.pTxBuff = &tPrintTxBuff;
			lwrb_reset(tUpdate.pTxBuff);
			
			//接收Buff
			tUpdate.pRxBuff = &tpPrintProtoRx->tRxBuff;
			cBaiku_ResetRxBuff(tpPrintProtoRx);
			
			//接收协议
			tUpdate.tpProtoRx = tpPrintProtoRx;
			
			//发送协议
			tUpdate.tpProtoTx = tpPrintProtoTx;
			
			if (cUpdate_ProtoSelect(proto_type) == false)
				return -1;
		}
		break;
		#endif  /* boardPRINT_IFACE */
		
		#if (boardWIFI_USARTX)
		case CT_WIFI:
		{
		}
		break;
		#endif  /* boardWIFI_USARTX */
		
		default:
		{
			log_e("当前通道%d未定义,请重新选择通道", ch_type);
			ch_type = CT_NULL;
		}
		break;
	}
	
	tUpdate.eChType = ch_type;
	
	return c_result;
}

/***********************************************************************************************************************
 * 函数功能    : 选择并重置对应升级协议状态机
 * 说明(备注)  : 重置Xmodem或百酷升级协议
 * 传入参数    : type: 协议类型(ProtoType_E)
 * 输出参数    : 无
 * 返回值      : 1: 成功
 ************************************************************************************************************************/
s8 cUpdate_ProtoSelect(ProtoType_E type)
{
	switch (type)
	{
		case PT_XMODEM:
		{
			tUpdate.eProtoType = PT_XMODEM;
			bXmodem_Reset(&tXmodem);
		}
		break;
		
		case PT_BAIKU:
		{
			tUpdate.eProtoType = PT_BAIKU;
			bBaiKuProto_Reset(&tBaiKuProto, tUpdate.tpProtoRx);
		}
		break;
		
		default:
		{
		}
		break;
	}
	
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 升级任务Tick计时处理
 * 说明(备注)  : 驱动当前协议超时计时器
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vUpdate_TickTimer(void)
{
	switch (tUpdate.eProtoType)
	{
		case PT_XMODEM:
		{
			vXmodem_TickTime(&tXmodem);
			tUpdate.tpProtoRx->usLostOverTimeCnt = tXmodem.usWaitStartOutTimeCnt;
		}
		break;
		
		case PT_BAIKU:
		{
			vBaiKuProto_TickTime(&tBaiKuProto);
			tUpdate.tpProtoRx->usLostOverTimeCnt = tBaiKuProto.usWaitStartOutTimeCnt;
		}
		break;
		
		default:
		{
		}
		break;
	}
}

#endif  /* boardUPDATE */
