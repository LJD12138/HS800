/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Dc
 * File    : dc_queue_task_init.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : DC 队列任务: 初始化实现文件
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
 * 函数功能    : DC 队列任务: 初始化
 * 说明(备注)  : 等待 APP 信息就绪后读取记忆参数，完成后置位初始化完成标志并切换至关闭完成
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dc_queue_task_init(Task_T *p_task)
{
	s8 c_ret = 0;

	/* 注意: 初始化任务不做队列抢占让出。管理函数在初始化完成标志置位前
	 * 优先装载本任务，若此处让出会因标志未置位陷入"让出-重装"忙转死锁；
	 * 排队命令将在初始化完成后按序弹出执行 */
	switch (p_task->ucStep)
	{
		case 0:
		{
			vDc_ParamInit();
			dcPOWER_EN_OFF();

			/* 等待获取 APP 信息 */
			if (tSysInfo.uInit.tFinish.bIF_AppInfo == false)
				break;

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			static bool s_b_ret = true;

			c_ret = cDc_InfoInit();
			if (c_ret > 0)
			{
				if ((uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant) && s_b_ret == false)
					log_w("bDcTask:tDC获取错误清除");

				s_b_ret = true;
			}
			else
			{
				if ((uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant) && s_b_ret == true)
				{
					log_w("bDcTask:tDC初始化失败 代码%d", c_ret);
					s_b_ret = false;
				}
				break;
			}

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 2:
		{
			tSysInfo.uInit.tFinish.bIF_DcTask = true;
			vDc_SetWorkState(DS_SHUT_DOWN);
			cQueue_GotoStep(p_task, STEP_END);
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
