/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_queue_task.h"

#if(boardPRINT_IFACE && boardUPDATE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "Sys/sys_queue_task_update.h"
#include "Sys/sys_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#define       	printTASK_UPDATE_CYCLE_TIME               		50

/*****************************************************************************************************************
-----函数功能    任务函数:升级任务
-----说明(备注)  none
-----传入参数    none:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_print_queue_task_update(Task_T *tp_task)
{
	UpdateObj_E e_update_obj = (UpdateObj_E)tp_task->usInParam;	
	
	//超范围
	if(tp_task->usInParam >= UO_INVAILD)
	{
		cQueue_GotoStep( tp_task, STEP_END );  //结束
		return;
	}
	
	//退出升级模式 || 队列里面有任务
	if(tSysInfo.eDevState != DS_UPDATE_MODE || 
		lwrb_get_full(&tp_task->tQueueBuff))
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
		return;
	}		

	switch(tp_task->ucStep)
	{
		//开始透传
		case 0 :
		{
			u16 us_char_send_bms_len = lwrb_get_full(&tpPrintProtoRx->tRxBuff);
			
			u16 us_char_send_print_len = lwrb_get_full(&tp_task->tReplyBuff);

			#if(boardBMS_EN)
			if(e_update_obj == UO_BMS)
			{
				if(us_char_send_bms_len)
				{
					lwrb_reset(&tpBmsTask->tReplyBuff);
					lwrb_move(&tpBmsTask->tReplyBuff, &tpPrintProtoRx->tRxBuff);

					#if(boardUSE_OS)
					xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
					#endif  //boardUSE_OS
				}
				
				if(us_char_send_print_len)
				{
					lwrb_reset(&tPrintTxBuff);
					lwrb_move(&tPrintTxBuff, &tp_task->tReplyBuff);
					bPrint_SendDataToUsart();
				}
			}
			#endif  //boardBMS_EN
		}
		break;
		
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	
	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, printTASK_UPDATE_CYCLE_TIME);
	#endif  //boardUSE_OS
}
#endif  //boardPRINT_IFACE && boardUPDATE
