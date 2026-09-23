/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_err.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统故障保护状态队列任务实现
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
#include "Key/Key_task.h"
#endif  //boardKEY_EN

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

//****************************************************Macros********************************************************************//
#define			sysTASK_ERR_CYCLE_TIME					sysTASK_CYCLE_TIME	//任务时间

/***********************************************************************************************************************
 * 函数功能    : 系统故障保护队列任务执行函数
 * 说明(备注)  : 处理系统错误报警、欠压/0%SOC倒计时关机以及充电唤醒保护
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 更新任务状态
 * 返回值      : void
 ************************************************************************************************************************/
void v_sys_queue_task_err(Task_T *p_task)
{
	//队列里面有任务
	if (lwrb_get_full(&p_task->tQueueBuff))
	{
		cQueue_GotoStep(p_task, STEP_END);  //结束
		return;
	}

	switch (p_task->ucStep)
	{
		//************************************步骤0:初始化*************************************************
		case 0:
		{
			bSys_SetDevState(DS_ERR, false);
			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
		}break;

		//************************************步骤1:等待关闭*************************************************
		case 1:
		{
			#if (boardKEY_EN)
			//按键按下即可重置
			if (bKey_IsAnyPress() == true)
				p_task->usStepWaitCnt = 0;
			#endif  //boardKEY_EN

			//存在充电或者错误清除
			if ((bSys_ExistInVolt() == true) || (tSysInfo.uErrCode.usCode == 0))
			{
				cQueue_GotoStep(p_task, 3);
				return;
			}

			//倒计时退出
			if ((tSysInfo.uErrCode.tCode.bUV == 1) || (tSysInfo.uErrCode.tCode.b0SOC == 1))
			{
				p_task->usStepWaitCnt++;
				if (p_task->usStepWaitCnt >= (10000 / sysTASK_ERR_CYCLE_TIME))
				{
					p_task->usStepWaitCnt = 0;
					bSys_SetDevState(DS_SHUT_DOWN, true);
					cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
				}
			}
			else
			{
				p_task->usStepWaitCnt = 0;
				cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
			}
		}break;

		//************************************步骤2:进入关机**************************************************
		case 2:
		{
			//存在充电
			if (bSys_ExistInVolt() == true)
			{
				cQueue_GotoStep(p_task, 3);
				return;
			}

			bSys_SetDevState(DS_SHUT_DOWN, false);

			#if (boardBMS_EN)
			cBms_Switch(SO_KEY, ST_OFF, true);
			#endif  //boardBMS_EN

			#if (boardUSE_OS)
			vTaskDelay(1000);
			#endif  //boardUSE_OS
		}break;

		//************************************步骤3:充电开机**************************************************
		case 3:
		{
			cSys_Switch(SO_MPPT, ST_ON, true); //开机
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:开启充电唤醒或错误清除\r\n");

			cQueue_GotoStep(p_task, STEP_END);  //结束
			return;
		}

		default:
		{
			if (lwrb_get_full(&p_task->tQueueBuff))  //队列里面有任务
				cQueue_GotoStep(p_task, STEP_END);  //结束
		}
		break;
	}

	//等待60S,超时退出
	p_task->usTaskWaitCnt++;
	if ((p_task->usTaskWaitCnt > (60000 / sysTASK_ERR_CYCLE_TIME)) && (p_task->ucStep != STEP_END))
	{
		bSys_SetDevState(DS_SHUT_DOWN, false);

		#if (boardBMS_EN)
		cBms_Switch(SO_KEY, ST_OFF, false);
		#endif  //boardBMS_EN
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, sysTASK_ERR_CYCLE_TIME);   /* 周期节拍；新任务投递立即唤醒抢占 */
	#endif  //boardUSE_OS
}
