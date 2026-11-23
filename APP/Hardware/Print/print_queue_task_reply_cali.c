/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_queue_task_reply_cali.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 回复上位机校准命令队列任务
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_queue_task.h"

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"

//****************************************************Macros********************************************************************//
#define			printTASK_CALI_CYCLE_TIME				50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 回复上位机校准结果队列任务
 * 说明(备注)  : 打包并向上位机回复校准执行结果
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_print_queue_task_reply_cali(Task_T *p_task)
{
	u16 us_data_len = p_task->usInParam;
	__ALIGNED(4) u8 uca_buff[8] = {0};
	
	switch (p_task->ucStep)
	{
		case 0:
		{
			if (p_task->tReplyBuff.buff == NULL || us_data_len == 0 || us_data_len > sizeof(uca_buff))
			{
				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			if (usQueue_GetReplyLen(p_task) != us_data_len)
			{
				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			if (usQueue_PeekReply(p_task, 0, uca_buff, us_data_len) != us_data_len)
			{
				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			if (c_relay45_cali(uca_buff, (u8)us_data_len) > 0)
			{
				vQueue_ResetReply(p_task);
				cQueue_GotoStep(p_task, STEP_NEXT);
			}
			else
				break;
		}
		
		case 1:
			cQueue_GotoStep(p_task, STEP_END);  //结束
			return;
		
		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  //结束
		}
		break;
	}
	
	if (bQueue_IsTaskTimeoutMs(p_task, 3000))  //等待超时
		cQueue_GotoStep(p_task, STEP_END);  //结束
	
	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, printTASK_CALI_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

#endif  /* boardPRINT_IFACE */
