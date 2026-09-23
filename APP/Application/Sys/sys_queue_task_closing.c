/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_closing.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统关机过程中队列任务实现
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

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  //boardDCAC_EN

//****************************************************Macros********************************************************************//
#define			sysTASK_CLOSE_CYCLE_TIME				10		//任务时间

/***********************************************************************************************************************
 * 函数功能    : 系统关机队列任务执行函数
 * 说明(备注)  : 控制系统由工作状态安全关闭各路外设、断开 BMS 并进入低功耗待机
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 更新任务状态
 * 返回值      : void
 ************************************************************************************************************************/
void v_sys_queue_task_closing(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		//************************************步骤0:初始化**********************************************
		case 0:
		{
			bSys_SetDevState(DS_CLOSING, true);
			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
		}break;

		//************************************步骤1:预留清理********************************************
		case 1:
		{
			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
		}break;

		//************************************步骤2:等待设备关闭**********************************************
		case 2:
		{
			if (bSys_CheckActState() == false)  //等待关闭 tSysInfo.uInit.tFinish.bIF_DcacTask == 1
			{
				cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
				break;
			}

			//等待超时下一步
			p_task->usStepWaitCnt++;
			if (p_task->usStepWaitCnt >= (5000 / sysTASK_CLOSE_CYCLE_TIME))
			{
				p_task->usStepWaitCnt = 0;
				cQueue_GotoStep(p_task, STEP_NEXT);  //下一步

				if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
					log_w("bSysTask:等待设备关闭超时,强制关闭");
			}
		}break;

		//************************************步骤3:关闭BMS**********************************************
		case 3:
		{
			#if (boardBMS_EN)
			if (cBms_Switch(SO_KEY, ST_OFF, false) < 0)  //操作失败
			{
				if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
					sMyPrint("bSysTask:关闭BMS失败\r\n");

				#if (boardUSE_OS)
				vTaskDelay(500);
				#endif  //boardUSE_OS
				break;
			}
			#endif  //boardBMS_EN

			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
		}break;

		//************************************步骤4:等待BMS关闭**********************************************
		case 4:
		{
			#if (boardBMS_EN)
			if (tBms.eDevState == DS_SHUT_DOWN)
				cQueue_GotoStep(p_task, STEP_NEXT);  //下一步

			//等待超时重新从第一步开始
			p_task->usStepWaitCnt++;
			if (p_task->usStepWaitCnt >= (5000 / sysTASK_CLOSE_CYCLE_TIME))
			{
				p_task->usStepWaitCnt = 0;
				cQueue_GotoStep(p_task, STEP_FORWARD);  //上一步

				if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
					sMyPrint("bSysTask:等待BMS关闭完成超时\r\n");
			}
			#else
			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
			#endif  //boardBMS_EN
		}break;

		//************************************步骤5:关闭完成**********************************************
		case 5:
		{
			bSys_SetDevState(DS_SHUT_DOWN, false);
			cQueue_GotoStep(p_task, STEP_END);  //结束
		}break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	//初始化等待10S,超时退出
	p_task->usTaskWaitCnt++;
	if ((p_task->usTaskWaitCnt > (10000 / sysTASK_CLOSE_CYCLE_TIME)) && (p_task->ucStep != STEP_END))
	{
		bSys_SetErrCode(SEC_CLOSE_FAULT, true);

		if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_w("bSysTask:关闭系统任务等待超时,步骤%d", p_task->ucStep);

		cQueue_GotoStep(p_task, STEP_END);  //结束
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, sysTASK_CLOSE_CYCLE_TIME); /* 周期节拍；新任务投递立即唤醒抢占 */
	#endif  //boardUSE_OS
}
