/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_shut_down.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统关机状态队列任务实现
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

#if (boardKEY_EN)
#include "Key/key_task.h"
#endif  //boardKEY_EN

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#include "gpio_init.h"

//****************************************************Macros********************************************************************//
#define			sysTASK_SHUT_DOWN_CYCLE_TIME			sysTASK_CYCLE_TIME	//任务时间

/***********************************************************************************************************************
 * 函数功能    : 系统关机就绪队列任务执行函数
 * 说明(备注)  : 维持关机状态，监听按键开机、充电唤醒与任务退出
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 更新任务状态
 * 返回值      : void
 ************************************************************************************************************************/
void v_sys_queue_task_shut_down(Task_T *p_task)
{
	if (tSysInfo.eDevState != DS_SHUT_DOWN)
		bSys_SetDevState(DS_SHUT_DOWN, false);

	if (tSysInfo.uErrCode.tCode.bCloseFault)
		bSys_SetErrCode(SEC_CLOSE_FAULT, false);

	if (bSys_ExistInVolt() == true)
	{
		bSys_ChgWakeUp(SO_MPPT);
		cQueue_GotoStep(p_task, STEP_END);  //结束
		return;
	}

	switch (p_task->ucStep)
	{
		//有按键按下
		case 0:
		{
			#if (boardKEY_EN)
			if (bKey_IsPressById(keyPOWER) == true)
				p_task->usStepWaitCnt = 0;
			#endif  //boardKEY_EN

			//有任务退出
			if (lwrb_get_full(&p_task->tQueueBuff))                 //队列里面有任务
				cQueue_GotoStep(p_task, STEP_END);  //结束

			//等待主机请求关闭
			p_task->usStepWaitCnt++;
			if (p_task->usStepWaitCnt >= (6000 / sysTASK_SHUT_DOWN_CYCLE_TIME))
			{
				p_task->usStepWaitCnt = 0;
				cQueue_GotoStep(p_task, STEP_NEXT);  //下一步

				if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
					sMyPrint("bSysTask:等待主机关闭超时,再次关闭BMS关闭\r\n");
			}
		}break;

		case 1:
		{
			#if (boardBMS_EN)
			if (cBms_Switch(SO_KEY, ST_OFF, false) < 0)  //操作失败
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:关闭BMS失败\r\n");

				#if (boardUSE_OS)
				vTaskDelay(500);
				#endif  //boardUSE_OS
			}
			else
				cQueue_GotoStep(p_task, STEP_FORWARD);  //上一步
			#else
			cQueue_GotoStep(p_task, STEP_FORWARD);  //上一步
			#endif  //boardBMS_EN
		}break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  //结束
		}
		break;
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, sysTASK_SHUT_DOWN_CYCLE_TIME); /* 周期节拍；新任务投递立即唤醒抢占 */
	#endif  //boardUSE_OS
}
