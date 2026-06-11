/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task_update.h"

#if(boardUPDATE)
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task.h"
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "Update/update_main.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif //boardBMS_EN

#if(boardCONSOLE_EN)
#include "MD_Console/md_console_task.h"
#include "MD_Console/md_console_rec_task.h"
#include "MD_Console/md_console_iface.h"
#include "MD_Console/md_console_prot_frame.h"
#endif

//****************************************************参数初始化**************************************************//
Update_T  tUpdate;


/***********************************************************************************************************************
-----函数功能    工作
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_update(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
		
		case 0:
		{
			//未定义,初始化
			if(tUpdate.eChType == CT_NULL || tUpdate.eProtoType == PT_NULL)
			{
				#if(boardCONSOLE_EN)
				cUpdate_ChSelect(CT_CONSOLE, PT_BAIKU);
				#elif(boardPRINT_IFACE)
				cUpdate_ChSelect(CT_PRINT, PT_XMODEM);
				#endif
			}
			
			if(tUpdate.eChType == CT_PRINT)
			{
				uPrint.ulFlag = 0;
			}
			else 
			{
				uPrint.tFlag.bSysTask = 1;
				uPrint.tFlag.bBaiKuProto = 0;
				uPrint.tFlag.bUpdate = 0;
				uPrint.tFlag.bBootInfo = 1;
			}
			lwrb_reset(&tp_task->tQueueBuff);//清空队列
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
		}break;
		
		case 1:
		{
			
			#if(boardBMS_EN)
			vBms_Task(NULL);
			#endif //boardBMS_EN
		
			#if(boardCONSOLE_USARTX)
			vConsole_Task(NULL);
			vConsole_RecTask(NULL);
			#endif //boardCONSOLE_EN
			
			switch(tUpdate.eProtoType)
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
					break;
			}

			
			//队列里面有任务
			if(lwrb_get_full(&tp_task->tQueueBuff))                 
				cQueue_GotoStep(tp_task, STEP_END);  //结束
		}break;
		
		
        default:
				cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
}

/*****************************************************************************************************************
-----函数功能    电池包任务参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool bUpdate_Init(void)
{
	memset(&tUpdate, 0, sizeof(tUpdate));
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    升级通道选择
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
s8 cUpdate_ChSelect(ChannelType_E ch_type, ProtoType_E proto_type)
{
	s8 c_result = 1;
	
	switch(ch_type)
	{
		#if(boardCONSOLE_EN)
		case CT_CONSOLE:
		{
			if(tpConsoleProtoRx == NULL || tpConsoleProtoRx->tRxBuff.buff == NULL)
				return -2;
			
			if(tpConsoleProtoTx == NULL)
				return -3;
			
			//接受Buff
			tUpdate.pRxBuff = &tpConsoleProtoRx->tRxBuff;
			cBaiku_ResetRxBuff(tpConsoleProtoRx);
			//接受协议
			tUpdate.tpProtoRx = tpConsoleProtoRx;
			
			//发送协议
			tUpdate.tpProtoTx = tpConsoleProtoTx;
			
			if(cUpdate_ProtoSelect(proto_type) == false)
				return -1;
		}
		break;
		#endif
		
		#if(boardPRINT_IFACE)
		case CT_PRINT:
		{
			if(tpPrintProtoRx == NULL || tpPrintProtoRx->tRxBuff.buff == NULL)
				return -2;
			
			//发送Buff
			tUpdate.pTxBuff = &tPrintTxBuff;
			lwrb_reset(tUpdate.pTxBuff);
			
			//接受Buff
			tUpdate.pRxBuff = &tpPrintProtoRx->tRxBuff;
			cBaiku_ResetRxBuff(tpPrintProtoRx);
			
			//接受协议
			tUpdate.tpProtoRx = tpPrintProtoRx;
			
			//发送协议
			tUpdate.tpProtoTx = tpPrintProtoTx;
			
			if(cUpdate_ProtoSelect(proto_type) == false)
				return -1;
		}
		break;
		#endif
		
		#if(boardWIFI_USARTX)
		case CT_WIFI:
		{
			
		}
		break;
		#endif
		
		default:
			log_e("当前通道%d未定义,请重新选择通道",ch_type);
			ch_type = CT_NULL;
			break;
	}
	
	tUpdate.eChType = ch_type;
	
	return c_result;
}

/***********************************************************************************************************************
-----函数功能    协议选择
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
s8 cUpdate_ProtoSelect(ProtoType_E type)
{
	switch(type)
	{
		case PT_XMODEM:
		{
			tUpdate.usTotalFrmValue = 800;
			tUpdate.eProtoType = PT_XMODEM;
			bXmodem_Reset(&tXmodem);
		}
		break;
		
		case PT_BAIKU:
		{
//			tUpdate.usTotalFrmValue = 400;
			tUpdate.eProtoType = PT_BAIKU;
			bBaiKuProto_Reset(&tBaiKuProto, tUpdate.tpProtoRx);
		}
		break;
		
		default:
			break;
	}
	
	return 1;
}

/***********************************************************************************************************************
-----函数功能    升级任务Tick计时
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vUpdate_TickTimer(void)
{
	switch(tUpdate.eProtoType)
	{
		case PT_XMODEM:
		{
			vXmodem_TickTime(&tXmodem);
		}
		break;
		
		case PT_BAIKU:
		{
			vBaiKuProto_TickTime(&tBaiKuProto);
		}
		break;
		
		default:
			break;
	}
}

#endif //boardUPDATE

