/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_queue_task_reply_run_log.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 回复BMS运行日志队列任务(解锁/读槽/重置)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_queue_task.h"

#if (boardPRINT_IFACE && boardBMS_EN && boardRUN_LOG_EN)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"

//****************************************************Macros********************************************************************//
#define			printTASK_RUN_LOG_CYCLE_TIME			50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 回复BMS运行日志队列任务
 * 说明(备注)  : 处理解锁(0x8B)、读槽(0xB7)、重置(0xB9)等运行日志指令的回复
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_print_queue_task_reply_run_log(Task_T *p_task)
{
	u16 us_param = p_task->usInParam;

	switch (p_task->ucStep)
	{
		case 0:
		{
			s8 c_ret = 0;
			if (p_task->ucID == PTI_REPLY_LOG_UNLOCK)
			{
				u8 uc_res = (u8)us_param;
				c_ret = c_print_cs_reply_log_unlock(uc_res);
			}
			else if (p_task->ucID == PTI_REPLY_RESET_RUN_LOG)
			{
				u8 uc_res = (u8)us_param;
				c_ret = c_print_cs_reply_reset_run_log(uc_res);
			}
			else if (p_task->ucID == PTI_REPLY_RUN_LOG_SLOT)
			{
				u8 buff[64] = {0};
				u8 len = lwrb_get_full(&p_task->tReplyBuff);
				if (len > 0 && len <= sizeof(buff))
				{
					lwrb_read(&p_task->tReplyBuff, buff, len);
					c_ret = c_print_cs_reply_run_log_slot(buff, len);
				}
				else
				{
					cQueue_GotoStep(p_task, STEP_END);
					break;
				}
			}

			if (c_ret > 0)
				cQueue_GotoStep(p_task, STEP_NEXT);
			else
				break;
		}
		break;

		case 1:
		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	if (bQueue_IsTaskTimeoutMs(p_task, 3000))
		cQueue_GotoStep(p_task, STEP_END);

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, printTASK_RUN_LOG_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

#endif  /* boardPRINT_IFACE && boardBMS_EN && boardRUN_LOG_EN */
