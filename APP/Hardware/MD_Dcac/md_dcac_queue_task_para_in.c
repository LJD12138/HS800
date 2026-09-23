/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_queue_task_para_in.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器并网控制队列任务实现
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
#define			dcacTASK_PARA_IN_CYCLE_TIME				50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 任务函数:并网放电控制
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dcac_queue_task_para_in(Task_T *p_task)
{
	static uint16_t s_us_temp = 0;
	SwitchType_E e_type = (SwitchType_E)p_task->usInParam;

	switch (p_task->ucStep)
	{
		case 0:
		{
			/*
			if (e_type == ST_ON)
			{
				bDcac_SetAcState(OO_PARA_IN, IOS_STARTING);
				s_us_temp = tAppMemParam.tDCAC.usParaInPwr;
			}
			else
			{
				bDcac_SetAcState(OO_PARA_IN, IOS_CLOSING);
				s_us_temp = 0;
			}
			*/

			if (b_dcac_cs_set_para_in_pwr(s_us_temp) == true)
				cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */

			p_task->usStepRepeatCnt++;
			if (p_task->usStepRepeatCnt > 3)
			{
				if (uPrint.tFlag.bDcacTask)
					log_w("bDcacTask:数据发送失败次数过多,退出设置并网放电任务");

				goto loop_para_in_end;
			}
		}
		break;

		case 1:
		{
			if (e_type == ST_ON)
				bDcac_SetAcState(OO_PARA_IN, IOS_WORK);       /* 开启 */
			else
				bDcac_SetAcState(OO_PARA_IN, IOS_SHUT_DOWN);  /* 关闭 */

			cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
		}
		break;
	}

	p_task->usTaskWaitCnt++;
	if (p_task->usTaskWaitCnt > (3000 / dcacTASK_PARA_IN_CYCLE_TIME))  /* 等待超时 */
	{
loop_para_in_end:
		b_dcac_cs_set_para_in_pwr(0);
		bDcac_SetAcState(OO_PARA_IN, IOS_SHUT_DOWN);  /* 关闭 */

		if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:并网控制任务处理超时,步骤%d", p_task->ucStep);

		cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
	}

	ulTaskNotifyTake(pdTRUE, dcacTASK_PARA_IN_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
}

#endif  /* boardDCAC_EN */
