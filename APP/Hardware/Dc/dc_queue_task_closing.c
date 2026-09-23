/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Dc
 * File    : dc_queue_task_closing.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : DC 队列任务: 关闭实现文件
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
 * 函数功能    : DC 队列任务: 关闭
 * 说明(备注)  : 关闭输出并等待放电完成，期间持续保护监测(关断失败检测)
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dc_queue_task_closing(Task_T *p_task)
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
			vDc_SetWorkState(DS_CLOSING);
			dcPOWER_EN_OFF();
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			/* 输出已放电完毕, 回到关机完成状态 */
			if (cDc_CheckOutVolt() < 0)
			{
				vDc_ParamInit();
				vDc_SetWorkState(DS_SHUT_DOWN);
				cQueue_GotoStep(p_task, STEP_END);
				return;
			}

			/* 放电等待中, 继续保护监测 */
			vDc_ProtectProcess();
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
