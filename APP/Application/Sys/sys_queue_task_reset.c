/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_reset.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统重置队列任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "gpio_init.h"
#include "app_info.h"

#if (1)

//****************************************************Macros********************************************************************//
#define			sysTASK_RESET_CYCLE_TIME				sysTASK_CYCLE_TIME	//任务时间

/***********************************************************************************************************************
 * 函数功能    : 系统重置队列任务执行函数
 * 说明(备注)  : 初始化并更新 APP 记忆参数，执行系统复位
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 更新任务状态
 * 返回值      : void
 ************************************************************************************************************************/
void v_sys_queue_task_reset(Task_T *p_task)
{
	s8 c_ret = 0;

	switch (p_task->ucStep)
	{
		case 0:
		{
			//初始化APP数据
			c_ret = cApp_MemParamInit(tAppMemParamStr);
			if (c_ret <= 0)
			{
				if (uPrint.tFlag.bAppInfo)
					sMyPrint("bAppInfo:APP参数初始化失败 代码%d\r\n", c_ret);
				break;
			}

			//更新参数
			c_ret = cApp_UpdateMemParam(tAppMemParamStr);
			if (c_ret <= 0)
			{
				if (uPrint.tFlag.bAppInfo)
					sMyPrint("bAppInfo:APP参数更新失败 代码%d\r\n", c_ret);
				break;
			}

			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
		}break;

		case 1:
		{
			NVIC_SystemReset(); //重启
		}break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  //结束
		}
		break;
	}

	//等待5S,超时退出
	if (bQueue_IsTaskTimeoutMs(p_task, 5000) && (p_task->ucStep != STEP_END))
	{
		if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_w("bSysTask:设置重置任务等待超时,步骤%d", p_task->ucStep);

		cQueue_GotoStep(p_task, STEP_END);  //结束
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, sysTASK_RESET_CYCLE_TIME); /* 周期节拍；新任务投递立即唤醒抢占 */
	#endif  //boardUSE_OS
}

#endif  /* 1 */

