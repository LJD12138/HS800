/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if(boardKEY_EN)
#include "Key/Key_task.h"
#endif

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif

#include "gpio_init.h"
#include "app_info.h"

#define     	sysTASK_INIT_CYCLE_TIME					100 //任务时间

/***********************************************************************************************************************
-----函数功能    系统初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/  
void v_sys_queue_task_init(Task_T *tp_task)
{
	s8 c_ret = 0;
	static bool b_ret = true;
	static vu8 uc_tri_init_cnt = 0;
	static vu8 uc_tri_type = 0;
	
	//记录长按时间
	#if(boardKEY_EN)
	if(bKey_PowerIsPress() == true)
	{
		if(uc_tri_init_cnt < 0xff)
			uc_tri_init_cnt++;
	}
	else 
		uc_tri_init_cnt = 0;
	#endif
	
    switch (tp_task->ucStep)
    {
		case 0:
		{
			tSysInfo.uInit.tFinish.bIF_AppInfo = false;
			
			c_ret = cApp_BootInfoInit();
			if(c_ret > 0)
			{
				if((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && b_ret == false)
					log_w("bSysTask:Boot记忆消息获取错误清除");
				
				b_ret = true;
				
				cQueue_GotoStep(tp_task, STEP_NEXT);
			}
			else
			{
				if((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && b_ret == true)
				{
					log_w("bSysTask:Boot记忆消息初始化失败 代码%d",c_ret);
					b_ret = false;
				}
				
				#if(boardUSE_OS)
				vTaskDelay(100);
				#endif  //boardUSE_OS
				break;
			}
		}
		
		case 1:
		{
			c_ret = cApp_AppInfoInit();
			if(c_ret > 0)
			{
				if((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && b_ret == false)
					log_w("bSysTask:App记忆消息获取错误清除");
				
				b_ret = true;
				tSysInfo.uInit.tFinish.bIF_AppInfo = true;
				
				cQueue_GotoStep(tp_task, STEP_NEXT);
			}
			else
			{
				if((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && b_ret == true)
				{
					log_w("bSysTask:App记忆消息初始化失败 代码%d",c_ret);
					b_ret = false;
				}
		
				#if(boardUSE_OS)
				vTaskDelay(100);
				#endif  //boardUSE_OS
				break;
			}
		}
		
		case 2:
		{
			if(
				#if(boardADC_EN)
				tSysInfo.uInit.tFinish.bIF_AdcTask
				#else
				true
				#endif  //boardADC_EN
			)
				cQueue_GotoStep(tp_task, STEP_NEXT);
			else
			{
				#if(boardUSE_OS)
				vTaskDelay(500);
				#endif  //boardUSE_OS
				break;
			}
		}
		
		case 3:
		{
			if(bSys_ExistInVolt() == true)
			{
				bSys_ChgWakeUp(SO_MPPT);
				cQueue_GotoStep(tp_task, STEP_NEXT);
			}
			
			#if(boardKEY_EN)
			else if(bKey_PowerIsPress() == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);
			#endif  //boardKEY_EN

			#if(boardADC_EN)
			else if(tAdcSamp.usSysInVolt > (tAppMemParam.tSYS.usMinOpenVolt / 2)
					#if(boardBMS_EN)
					&& tBms.eDevState == DS_LOST
					#endif  //boardBMS_EN
				)
			{
				//充电激活
				tp_task->usStepWaitCnt++;
				if(tp_task->usStepWaitCnt > (5000 / sysTASK_INIT_CYCLE_TIME))
				{
					bSys_ChgWakeUp(SO_MPPT);
					cQueue_GotoStep(tp_task, STEP_NEXT);
				}
				else 
					break;
			}
			#endif  //boardADC_EN
			
			else
			{
				tp_task->usStepWaitCnt = 0;
				break;
			}
		}
		
		case 4:
		{
			if(
				#if(boardKEY_EN)
				bKey_AcIsPress() == false &&  //工厂模式
				bKey_PowerIsPress() == true &&
				bKey_UsbIsPress() == true &&
				bKey_DcIsPress() == false &&
				bKey_LightIsPress() == false
				#else
				false
				#endif  //boardKEY_EN
			)
			{
				if(uc_tri_type != 2)
				{
					uc_tri_type = 2;
					tp_task->usStepWaitCnt = 0;
				}
				
				tp_task->usStepWaitCnt++;
			}

			#if(boardKEY_EN)  //工程模式
			else if(bKey_AcIsPress() == false &&
				bKey_PowerIsPress() == true &&
				bKey_UsbIsPress() == false &&
				bKey_DcIsPress() == true &&
				bKey_LightIsPress() == false)
			{
				if(uc_tri_type != 1)
				{
					uc_tri_type = 1;
					tp_task->usStepWaitCnt = 0;
				}
				
				tp_task->usStepWaitCnt++;
			}
			#endif  //boardKEY_EN

			else if(
				#if(boardBMS_EN)
				tSysInfo.uInit.tFinish.bIF_BmsTask
				#else
				true
				#endif  //boardBMS_EN
				)
			{
				if(uc_tri_init_cnt && bKey_PowerIsPress() == true)
					cSys_Switch(SO_KEY, ST_ON, false);
				
				uc_tri_type = 0;
				tp_task->usStepWaitCnt = 0;
				cQueue_GotoStep(tp_task, STEP_NEXT);
			}
			else 
			{
				uc_tri_type = 0;
				tp_task->usStepWaitCnt = 0;
			}
				
			if(tp_task->usStepWaitCnt > (3000 / sysTASK_INIT_CYCLE_TIME))
			{
				if(uc_tri_type == 2)  //工厂模式
				{
					G_TestMode = true;
					cSys_Switch(SO_KEY, ST_ON, false);
					cQueue_GotoStep(tp_task, STEP_NEXT);
				}
				#if(boardENG_MODE_EN)  //工程模式
				else if(uc_tri_type == 1)
				{
					#if(boardBUZ_EN)
					bBuz_Tweet(SHORT_2);
					#endif  //boardBUZ_EN

					cQueue_AddQueueTask(tpSysTask, STI_ENG, NULL, false);

					#if(boardBMS_EN)
					cBms_Switch(SO_KEY, ST_ON, false);
					#endif  //boardBMS_EN

					cQueue_GotoStep(tp_task, STEP_NEXT);
				}
				#endif  //boardENG_MODE_EN
			}
		}break;
		
		case 5:
		{
			tSysInfo.uInit.tFinish.bIF_SysTask = 1;
			tSysInfo.uInit.tFinish.bIF_SysInit = 1;

			//没有任务就调度关闭任务
			if(tSysInfo.eDevState != DS_BOOTING)
				bSys_SetDevState(DS_SHUT_DOWN,false);
			
			cQueue_GotoStep(tp_task, STEP_END);
		}break;
		
        default:
			cQueue_GotoStep(tp_task, STEP_END);
			break;
    }
	
	//初始化等待5S,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000 / sysTASK_INIT_CYCLE_TIME))
	{
//		gpioASSIST_OPEN_OFF();
	}
	
	#if(boardUSE_OS)
	vTaskDelay(sysTASK_INIT_CYCLE_TIME);
	#endif  //boardUSE_OS
}

