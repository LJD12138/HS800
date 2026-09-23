/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_init.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统上电初始化队列任务实现
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

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if (boardADC_EN)
#include "Adc/adc_task.h"
#endif  //boardADC_EN

#include "gpio_init.h"
#include "app_info.h"

//****************************************************Macros********************************************************************//
#define			sysTASK_INIT_CYCLE_TIME					100		//任务时间

/***********************************************************************************************************************
 * 函数功能    : 系统上电初始化队列任务执行函数
 * 说明(备注)  : 依次初始化 Boot/APP 记忆参数，等待 ADC 就绪，检测外部充电唤醒、按键唤醒、工程/工厂模式
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 更新任务状态
 * 返回值      : void
 ************************************************************************************************************************/
void v_sys_queue_task_init(Task_T *p_task)
{
	s8          c_ret = 0;
	static bool s_b_ret = true;
	static vu8  s_uc_tri_init_cnt = 0;
	static vu8  s_uc_tri_type = 0;

	//记录长按时间
	#if (boardKEY_EN)
	if (bKey_IsPressById(keyPOWER) == true)
	{
		if (s_uc_tri_init_cnt < 0xff)
			s_uc_tri_init_cnt++;
	}
	else
		s_uc_tri_init_cnt = 0;
	#endif  //boardKEY_EN

	switch (p_task->ucStep)
	{
		case 0:
		{
			tSysInfo.uInit.tFinish.bIF_AppInfo = false;

			c_ret = cApp_BootInfoInit();
			if (c_ret > 0)
			{
				if ((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && (s_b_ret == false))
					log_w("bSysTask:Boot记忆消息获取错误清除");

				s_b_ret = true;
				cQueue_GotoStep(p_task, STEP_NEXT);
			}
			else
			{
				if ((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && (s_b_ret == true))
				{
					log_w("bSysTask:Boot记忆消息初始化失败 代码%d", c_ret);
					s_b_ret = false;
				}

				#if (boardUSE_OS)
				vTaskDelay(100);
				#endif  //boardUSE_OS
				break;
			}
		}break;

		case 1:
		{
			c_ret = cApp_AppInfoInit();
			if (c_ret > 0)
			{
				if ((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && (s_b_ret == false))
					log_w("bSysTask:App记忆消息获取错误清除");

				s_b_ret = true;
				tSysInfo.uInit.tFinish.bIF_AppInfo = true;
				cQueue_GotoStep(p_task, STEP_NEXT);
			}
			else
			{
				if ((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && (s_b_ret == true))
				{
					log_w("bSysTask:App记忆消息初始化失败 代码%d", c_ret);
					s_b_ret = false;
				}

				#if (boardUSE_OS)
				vTaskDelay(100);
				#endif  //boardUSE_OS
				break;
			}
		}break;

		case 2:
		{
			if (
				#if (boardADC_EN)
				tSysInfo.uInit.tFinish.bIF_AdcTask
				#else
				true
				#endif  //boardADC_EN
			)
				cQueue_GotoStep(p_task, STEP_NEXT);
			else
			{
				#if (boardUSE_OS)
				vTaskDelay(500);
				#endif  //boardUSE_OS
				break;
			}
		}break;

		case 3:
		{
			if (bSys_ExistInVolt() == true)
			{
				bSys_ChgWakeUp(SO_MPPT);
				cQueue_GotoStep(p_task, STEP_NEXT);
			}
			#if (boardKEY_EN)
			else if (bKey_IsPressById(keyPOWER) == true)
				cQueue_GotoStep(p_task, STEP_NEXT);
			#endif  //boardKEY_EN
			#if (boardADC_EN)
			else if ((tAdcSamp.usSysInVolt > (tAppMemParam.tSYS.usMinOpenVolt / 2))
				#if (boardBMS_EN)
			         && (tBms.eDevState == DS_LOST)
				#endif  //boardBMS_EN
			)
			{
				//充电激活
				p_task->usStepWaitCnt++;
				if (p_task->usStepWaitCnt > (5000 / sysTASK_INIT_CYCLE_TIME))
				{
					bSys_ChgWakeUp(SO_MPPT);
					cQueue_GotoStep(p_task, STEP_NEXT);
				}
				else
					break;
			}
			#endif  //boardADC_EN
			else
			{
				p_task->usStepWaitCnt = 0;
				break;
			}
		}break;

		case 4:
		{
			if (
				#if (boardKEY_EN)
				bKey_IsFactoryModePress() == true  //工厂模式
				#else
				false
				#endif  //boardKEY_EN
			)
			{
				if (s_uc_tri_type != 2)
				{
					s_uc_tri_type = 2;
					p_task->usStepWaitCnt = 0;
				}

				p_task->usStepWaitCnt++;
			}
			#if (boardKEY_EN)  //工程模式
			else if (bKey_IsEngModePress() == true)
			{
				if (s_uc_tri_type != 1)
				{
					s_uc_tri_type = 1;
					p_task->usStepWaitCnt = 0;
				}

				p_task->usStepWaitCnt++;
			}
			#endif  //boardKEY_EN
			else if (
				#if (boardBMS_EN)
				tSysInfo.uInit.tFinish.bIF_BmsTask
				#else
				true
				#endif  //boardBMS_EN
			)
			{
				if ((s_uc_tri_init_cnt > 1) && (bKey_IsPressById(keyPOWER) == true))
					cSys_Switch(SO_KEY, ST_ON, false);

				s_uc_tri_type = 0;
				p_task->usStepWaitCnt = 0;
				cQueue_GotoStep(p_task, STEP_NEXT);
			}
			else
			{
				s_uc_tri_type = 0;
				p_task->usStepWaitCnt = 0;
			}

			if (p_task->usStepWaitCnt > (3000 / sysTASK_INIT_CYCLE_TIME))
			{
				if (s_uc_tri_type == 2)  //工厂模式
				{
					G_TestMode = true;
					cSys_Switch(SO_KEY, ST_ON, false);
					cQueue_GotoStep(p_task, STEP_NEXT);
				}
				#if (boardENG_MODE_EN)  //工程模式
				else if (s_uc_tri_type == 1)
				{
					#if (boardBUZ_EN)
					bBuz_Tweet(SHORT_2);
					#endif  //boardBUZ_EN

					cQueue_AddQueueTask(tpSysTask, STI_ENG, NULL, false);

					#if (boardBMS_EN)
					cBms_Switch(SO_KEY, ST_ON, false);
					#endif  //boardBMS_EN

					cQueue_GotoStep(p_task, STEP_NEXT);
				}
				#endif  //boardENG_MODE_EN
			}
		}break;

		case 5:
		{
			tSysInfo.uInit.tFinish.bIF_SysTask = 1;
			tSysInfo.uInit.tFinish.bIF_SysInit = 1;

			//没有任务就调度关闭任务
			if (tSysInfo.eDevState != DS_BOOTING)
				bSys_SetDevState(DS_SHUT_DOWN, false);

			cQueue_GotoStep(p_task, STEP_END);
		}break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	//初始化等待10S,超时强制关机退出
	p_task->usTaskWaitCnt++;
	if (p_task->usTaskWaitCnt > (10000 / sysTASK_INIT_CYCLE_TIME))
	{
		gpioASSIST_OPEN_OFF();

		#if (boardBMS_EN)
		cBms_Switch(SO_KEY, ST_OFF, false);
		#endif  //boardBMS_EN
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, sysTASK_INIT_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
	#endif  //boardUSE_OS
}
