/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_queue_task_dcac_out.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器开关输出队列任务实现
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
#include "Sys/sys_task.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//
#define			dcacTASK_OUT_CYCLE_TIME					50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 任务函数:开关输出
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dcac_queue_task_dcac_out(Task_T *p_task)
{
	uint16_t us_temp = dcacSWITCH_REG_OFF;
	SwitchType_E e_type = (SwitchType_E)p_task->usInParam;

	if (e_type == ST_ON)
		us_temp = dcacSWITCH_REG_ON;

	switch (p_task->ucStep)
	{
		case 0:
		{
			if (e_type == ST_ON)
				bDcac_SetAcState(OO_DISCHG, IOS_STARTING);   /* 开启 */
			else
				bDcac_SetAcState(OO_DISCHG, IOS_CLOSING);    /* 关闭 */

			if (b_dcac_cs_ac_output_switch(us_temp) == true)
				cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */

			p_task->usStepRepeatCnt++;
			if (p_task->usStepRepeatCnt > 3)
			{
				if (uPrint.tFlag.bDcacTask)
					log_w("bDcacTask:数据发送失败次数过多,退出开关逆变输出任务");

				goto loop_end;
			}
		}
		break;

		case 1:
		{
			b_dcac_cs_ac_output_switch(us_temp);
			if (e_type == ST_ON)
			{
				bDcac_SetAcState(OO_DISCHG, IOS_WORK);   /* 开启 */
				cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */
			}
			else
			{
				bDcac_SetAcState(OO_DISCHG, IOS_SHUT_DOWN);  /* 关闭 */
				cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */
			}
		}
		break;

		case 2:
			if (uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:输出开关操作完成\r\n");

			cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
			return;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
		}
		break;
	}

	if (bQueue_IsTaskTimeoutMs(p_task, 3000))  /* 等待超时 */
	{
		if (uPrint.tFlag.bDcacTask)
			log_w("bDcacTask:控制逆变输出超时,步骤%d", p_task->ucStep);

loop_end:
		if (e_type == ST_ON)
		{
			b_dcac_cs_ac_output_switch(0);
			bDcac_SetAcState(OO_DISCHG, IOS_SHUT_DOWN);
		}

		cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
	}

	ulTaskNotifyTake(pdTRUE, dcacTASK_OUT_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
}

#endif  /* boardDCAC_EN */
