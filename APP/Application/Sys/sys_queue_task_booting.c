/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif

#include "gpio_init.h"

#define     	sysTASK_BOOTING_CYCLE_TIME					10 //任务时间

/***********************************************************************************************************************
-----函数功能    系统启动中任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/  
void v_sys_queue_task_booting(Task_T *tp_task)
{
//	TaskInParam_U 	u_param;
//	SwitchObject_E 	e_obj;
//	SwitchType_E 	e_type;
	
//	u_param.usTaskInParam = tp_task->usInParam;
//	e_obj = (SwitchObject_E)u_param.tTaskParam.ucObj;
//	e_type = (SwitchType_E)u_param.tTaskParam.ucParam;
	
    switch (tp_task->ucStep)
    {
		//************************************步骤0:开启系统**********************************************
		case 0:
		{
			if(tSysInfo.eDevState < DS_BOOTING)
				bSys_SetDevState(DS_BOOTING,false);

			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
		}
		
		//************************************步骤1:开启BMS**********************************************
		case 1:
		{
			#if(boardBMS_EN)
			if(cBms_Switch(SO_KEY, ST_ON, false) < 0)  //操作失败
			{
				if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) 
					log_w("bSysTask:开启BMS失败");
				
				#if(boardUSE_OS)
				vTaskDelay(500);
				#endif  //boardUSE_OS
				break;
			}
			#endif
			
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
		}break ;
		
		//************************************步骤2:等待BMS开启**********************************************
		case 2:
		{
			#if(boardBMS_EN)
			//等待超时重新从第一步开始
			tp_task->usStepRepeatCnt++;
			if(tp_task->usStepRepeatCnt >= (1000 / sysTASK_BOOTING_CYCLE_TIME))
			{
				tp_task->usStepRepeatCnt = 0;
				cQueue_GotoStep(tp_task, STEP_FORWARD);  //上一步
				
				if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) 
					log_w("bSysTask:等待BMS开启完成超时");
			}

			if(tBms.eDevState == DS_WORK || 
				tBms.eDevState == DS_ERR ||
				bSys_LowVoltReqChg() || //电池欠压请求充电
				G_TestMode == true) //测试模式
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			else
				break;
			#else
			cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			#endif
		}
		
		//************************************步骤五:开启完成**********************************************
		case 3:
		{
			bSys_SetDevState(DS_WORK, false);//进入工作

			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break ;
		
		default:
			break;
    }
	
	
	//等待10S,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (10000 / sysTASK_BOOTING_CYCLE_TIME) && tp_task->ucStep != STEP_END)
	{
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_w("bSysTask:启动任务等待超时,步骤%d", tp_task->ucStep);
		
		bSys_SetErrCode(SEC_BOOT_FAULT, true);
		
		cQueue_GotoStep( tp_task, STEP_END );  //结束
	}

	#if(boardUSE_OS)
	vTaskDelay(sysTASK_BOOTING_CYCLE_TIME);
	#endif  //boardUSE_OS
}

