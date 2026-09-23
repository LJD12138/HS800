/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Dc
 * File    : dc_queue_task_err.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : DC 队列任务: 故障错误处理实现文件
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
 * 函数功能    : DC 队列任务: 错误
 * 说明(备注)  : 关闭输出并周期执行保护检测, 等待外部命令或错误清除
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dc_queue_task_err(Task_T *p_task)
{
	/* 有更高优先级任务投递, 让出执行 */
	if (lwrb_get_full(&p_task->tQueueBuff))
	{
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	vDc_SetWorkState(DS_ERR);
	dcPOWER_EN_OFF();
	vDc_ProtectProcess();

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, dcTASK_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

#endif  /* boardDC_EN */
