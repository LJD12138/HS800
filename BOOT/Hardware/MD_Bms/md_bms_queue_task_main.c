/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
 * File    : md_bms_queue_task_main.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 队列主任务函数实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_queue_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Print/print_task.h"
#include "Update/update_main.h"

//****************************************************Function Declaration******************************************************//


/***********************************************************************************************************************
 * 函数功能    : BMS队列主任务
 * 说明(备注)  : 轮询BMS运行参数
 * 传入参数    : p_task: 任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_main(Task_T *p_task)
{
	/* 若队列中有其他紧急任务，则立即结束当前主任务 */
	if (!bQueue_IsQueueEmpty(p_task))
	{
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	switch (p_task->ucStep)
	{
		case 0:
		{
			if (bQueue_IsStepTimeout(p_task, 1000 / bmsTASK_CYCLE_TIME))
				cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			c_bms_cs_get_param(0);
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	p_task->usTaskWaitCnt++;
	if (p_task->usTaskWaitCnt > (3000 / bmsTASK_CYCLE_TIME))
	{
		if (uPrint.tFlag.bBmsTask)
			sMyPrintWarn("bBmsTask:获取数据任务等待超时,退出");
		
		cQueue_GotoStep(p_task, STEP_END);
	}
}

#endif  /* boardBMS_EN */
