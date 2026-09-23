/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_booting.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统启动开机状态队列任务实现
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

#include "gpio_init.h"

//****************************************************Macros********************************************************************//
#define			sysTASK_BOOTING_CYCLE_TIME				10		//任务时间

/***********************************************************************************************************************
 * 函数功能    : 系统启动中队列任务执行函数
 * 说明(备注)  : 控制系统由初始化/休眠状态依次唤醒 BMS 并进入正常工作状态
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 更新任务状态
 * 返回值      : void
 ************************************************************************************************************************/
void v_sys_queue_task_booting(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		//************************************步骤0:开启系统**********************************************
		case 0:
		{
			if (tSysInfo.eDevState < DS_BOOTING)
				bSys_SetDevState(DS_BOOTING, false);

			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
		}break;

		//************************************步骤1:开启BMS**********************************************
		case 1:
		{
			#if (boardBMS_EN)
			if(cBms_Switch(SO_KEY, ST_ON, false) < 0)  //操作失败
			{
				if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
					log_w("bSysTask:开启BMS失败");

				#if (boardUSE_OS)
				vTaskDelay(500);
				#endif  //boardUSE_OS
				break;
			}
			#endif  //boardBMS_EN

			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
		}break;

		//************************************步骤2:等待BMS开启并保证开机画面显示时长*************************
		case 2:
		{
			#if (boardBMS_EN)
			bool b_bms_ready = (tBms.eDevState == DS_WORK ||
			                    tBms.eDevState == DS_ERR ||
			                    bSys_LowVoltReqChg() || //电池欠压请求充电
			                    G_TestMode == true);     //测试模式

			p_task->usStepWaitCnt++;
			if (b_bms_ready)
			{
				/* BMS 就绪后, 且开机启动画面至少展示 1000ms (两帧动画) 后进入工作态 */
				if (p_task->usStepWaitCnt >= (1000 / sysTASK_BOOTING_CYCLE_TIME))
					cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
			}
			else
			{
				//等待超时重新从第一步开始
				p_task->usStepRepeatCnt++;
				if (p_task->usStepRepeatCnt >= (1000 / sysTASK_BOOTING_CYCLE_TIME))
				{
					p_task->usStepRepeatCnt = 0;
					cQueue_GotoStep(p_task, STEP_FORWARD);  //上一步

					if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
						log_w("bSysTask:等待BMS开启完成超时");
				}
			}
			#else
			p_task->usStepWaitCnt++;
			if (p_task->usStepWaitCnt >= (1000 / sysTASK_BOOTING_CYCLE_TIME))
				cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
			#endif  //boardBMS_EN
		}break;

		//************************************步骤3:开启完成**********************************************
		case 3:
		{
			bSys_SetDevState(DS_WORK, false); //进入工作
			cQueue_GotoStep(p_task, STEP_END); //结束
		}break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	//等待10S,超时退出
	p_task->usTaskWaitCnt++;
	if ((p_task->usTaskWaitCnt > (10000 / sysTASK_BOOTING_CYCLE_TIME)) && (p_task->ucStep != STEP_END))
	{
		if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_w("bSysTask:启动任务等待超时,步骤%d", p_task->ucStep);

		bSys_SetErrCode(SEC_BOOT_FAULT, true);
		cQueue_GotoStep(p_task, STEP_END);  //结束
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, sysTASK_BOOTING_CYCLE_TIME);   /* 周期节拍；新任务投递立即唤醒抢占 */
	#endif  //boardUSE_OS
}
