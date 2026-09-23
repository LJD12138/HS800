/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Dc
 * File    : dc_queue_task_booting.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : DC 队列任务: 启动实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Dc/dc_queue_task.h"

#if (boardDC_EN)
#include "Dc/dc_task.h"
#include "Dc/dc_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : DC 队列任务: 启动
 * 说明(备注)  : 使能输出并等待电压建立，3S 未建立报输出低错误
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dc_queue_task_booting(Task_T *p_task)
{
	/* 有更高优先级任务投递, 让出执行 */
	if (lwrb_get_full(&p_task->tQueueBuff))
	{
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	switch (p_task->ucStep)
	{
		case 0:
		{
			vDc_SetWorkState(DS_BOOTING);
			dcPOWER_EN_ON();
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			/* 输出电压已建立, 进入工作状态 */
			if (cDc_CheckOutVolt() == 0)
			{
				vDc_SetWorkState(DS_WORK);
				cQueue_GotoStep(p_task, STEP_END);
				return;
			}

			p_task->usTaskWaitCnt++;
			if (p_task->usTaskWaitCnt >= (3000 / dcTASK_CYCLE_TIME))
			{
				p_task->usTaskWaitCnt = 0;
				vDc_SetErrCode(DC_EC_OUT_LOW, true);
				cQueue_GotoStep(p_task, STEP_END);
				return;
			}
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, dcTASK_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

#endif  /* boardDC_EN */
