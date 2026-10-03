/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_queue_task_dcac_in.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器开关充电队列任务实现
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

#include "app_info.h"

//****************************************************Macros********************************************************************//
#define			dcacTASK_IN_CYCLE_TIME					50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 任务函数:开关充电
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dcac_queue_task_dcac_in(Task_T *p_task)
{
	uint16_t us_temp = 0;
	SwitchType_E e_type = (SwitchType_E)p_task->usInParam;

	switch (p_task->ucStep)
	{
		case 0:
		{
			if (e_type == ST_ON)
				us_temp = tAppMemParam.tDCAC.usMinInPwr;

			if (b_dcac_cs_set_chg_pwr(us_temp) == true)
				cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */

			p_task->usStepRepeatCnt++;
			if (p_task->usStepRepeatCnt > 3)
			{
				if (uPrint.tFlag.bDcacTask)
					log_w("bDcacTask:数据发送失败次数过多,退出开关逆变充电任务");

				goto loop_end;
			}
		}
		break;

		case 1:
		{
			cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */
		}
		break;

		case 2:
			if (uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:充电开关操作完成\r\n");

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
			log_w("bDcacTask:控制逆变充电超时,步骤%d", p_task->ucStep);

loop_end:
		if (e_type == ST_ON)
		{
			b_dcac_cs_set_chg_pwr(0);
			bDcac_SetAcState(OO_CHG, IOS_SHUT_DOWN);
		}

		cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
	}

	ulTaskNotifyTake(pdTRUE, dcacTASK_IN_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
}

#endif  /* boardDCAC_EN */
