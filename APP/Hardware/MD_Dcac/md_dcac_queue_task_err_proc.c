/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_queue_task_err_proc.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器错误处理队列任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Dcac/md_dcac_queue_task.h"

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//
#define			dcacTASK_ERR_PROC_CYCLE_TIME			50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 任务函数:错误处理
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dcac_queue_task_err_proc(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		case 0:
		{
			if (b_dcac_cs_sys_switch(dcacSWITCH_REG_OFF) == true)
				cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */
		}
		break;

		case 1:
		{
			bDcac_SetAcState(OO_CHG, IOS_ERR);
			bDcac_SetAcState(OO_DISCHG, IOS_ERR);
			cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
		}
		break;
	}

	if (bQueue_IsTaskTimeoutMs(p_task, 3000))  /* 等待超时 */
	{
		if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:错误处理任务处理超时,步骤%d", p_task->ucStep);

		cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
	}

	ulTaskNotifyTake(pdTRUE, dcacTASK_ERR_PROC_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
}

#endif  /* boardDCAC_EN */
