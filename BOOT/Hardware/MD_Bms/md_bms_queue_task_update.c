/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
 * File    : md_bms_queue_task_update.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS队列升级任务
 * -------------------------------------------------------
 * todo    :
 * 1. none
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
 * 函数功能    : BMS队列升级任务
 * 说明(备注)  : 向BMS发送升级指令
 * 传入参数    : tp_task: 任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_update(Task_T *tp_task)
{
	switch (tp_task->ucStep)
	{
		case 0:
		{
			if (bQueue_IsStepTimeout(tp_task, 1000 / bmsTASK_CYCLE_TIME))
				cQueue_GotoStep(tp_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			c_bms_cs_send_update();
			cQueue_GotoStep(tp_task, STEP_END);
		}
		break;

		default:
		{
			cQueue_GotoStep(tp_task, STEP_END);
		}
		break;
	}

	tp_task->usTaskWaitCnt++;
	if (tp_task->usTaskWaitCnt > (3000 / bmsTASK_CYCLE_TIME))
	{
		if (uPrint.tFlag.bBmsTask)
			sMyPrintWarn("bBmsTask:升级任务等待超时,退出");
		
		cQueue_GotoStep(tp_task, STEP_END);
	}
}

#endif  /* boardBMS_EN */
