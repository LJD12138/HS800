/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_queue_task_reply_app_info.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 回复BMS固件版本信息队列任务
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
#define			printTASK_APP_INFO_CYCLE_TIME			50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 回复BMS固件版本信息队列任务
 * 说明(备注)  : 从回复缓存区读取BMS版本数据并打包发送给上位机
 * 传入参数    : tp_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_print_queue_task_reply_app_info(Task_T *tp_task)
{
	u8 us_char_len = tp_task->usInParam;

	__ALIGNED(4) u8 uca_buff[256] = {0};
	
	switch (tp_task->ucStep)
	{
		case 0:
		{
			if (tp_task->tReplyBuff.buff == NULL)
			{
				cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}

			//校验数据
			u8 len = lwrb_get_full(&tp_task->tReplyBuff);
			if (len != us_char_len || tp_task->tReplyBuff.buff == NULL)
			{
				cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}
			
			//读取数据
			lwrb_read(&tp_task->tReplyBuff, (u8*)&uca_buff, len);

			if (c_relay_bms_app_info(uca_buff, us_char_len) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else
				break;
		}

		case 2:
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;

		default:
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
	}
	
	tp_task->usTaskWaitCnt++;
	if (tp_task->usTaskWaitCnt > (5000 / printTASK_APP_INFO_CYCLE_TIME))  //等待超时
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	
	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, printTASK_APP_INFO_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

#endif  /* boardPRINT_IFACE */
