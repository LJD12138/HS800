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

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif

#define     	sysTASK_CLOSE_CYCLE_TIME					10 //任务时间

/***********************************************************************************************************************
-----函数功能    系统关闭中
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/  
void v_sys_queue_task_closing(Task_T *tp_task)
{
//	TaskInParam_U u_param;
//	SwitchObject_E e_obj;
//	SwitchType_E   e_type;
	
//	u_param.usTaskInParam = tp_task->usInParam;
//	e_obj = (SwitchObject_E)u_param.tTaskParam.ucObj;
//	e_type = (SwitchType_E)u_param.tTaskParam.ucParam;
	
    switch (tp_task->ucStep)
    {
		//************************************步骤0:初始化**********************************************
		case 0:
		{
			bSys_SetDevState(DS_CLOSING,true);
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
		}break ;
		
		
		//************************************步骤1:初始化逆变参数**********************************************
		case 1:
		{
			// #if(boardDCAC_EN)
			// //关闭前初始化DCAC参数
			// tSysInfo.uInit.tFinish.bIF_DcacTask = 0;
			// cQueue_AddQueueTask(tpDcacTask, DTI_INIT, NULL, true);   //初始化tDCAC
			// #endif  //boardDCAC_EN

			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
			
			//等待超时下一步
			tp_task->usStepWaitCnt++;
			if(tp_task->usStepWaitCnt >= 3)
			{
				tp_task->usStepWaitCnt = 0;
				cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
				
				if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) 
					log_w("bSysTask:关闭系统等待超时");
			}
		}
		break ;
		
		//************************************步骤3:等待设备关闭**********************************************
		case 2:
		{
			if(bSys_CheckActState() == false )  //等待关闭 tSysInfo.uInit.tFinish.bIF_DcacTask == 1
			{
				cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
				break;
			}				

			//等待超时下一步
			tp_task->usStepWaitCnt++;
			if(tp_task->usStepWaitCnt >= (5000 / sysTASK_CLOSE_CYCLE_TIME))
			{
				tp_task->usStepWaitCnt = 0;
				cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
				
				if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) 
					log_w("bSysTask:等待设备关闭超时,强制关闭");
			}
		}
		break ;
		
		//************************************步骤3:关闭BMS**********************************************
		case 3:
		{
			#if(boardBMS_EN)
			if(cBms_Switch(SO_KEY, ST_OFF, false) < 0)  //操作失败
			{
				if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) 
					sMyPrint("bSysTask:关闭BMS失败\r\n");
				
				#if(boardUSE_OS)
				vTaskDelay(500);
				#endif  //boardUSE_OS
				break;
			}
			#endif
			
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
		}break ;
		
		//************************************步骤4:等待BMS关闭**********************************************
		case 4:
		{
			#if(boardBMS_EN)
			if(tBms.eDevState == DS_SHUT_DOWN)
			{
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			}	

			//等待超时重新从第一步开始
			tp_task->usStepWaitCnt++;
			if(tp_task->usStepWaitCnt >= (5000 / sysTASK_CLOSE_CYCLE_TIME))
			{
				tp_task->usStepWaitCnt = 0;
				cQueue_GotoStep(tp_task, STEP_FORWARD);  //上一步
				
				if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) 
					sMyPrint("bSysTask:等待BMS关闭完成超时\r\n");
			}
			#else
			cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			#endif
		}break ;
		
		//************************************步骤4:关闭完成**********************************************
		case 5:
		{
			bSys_SetDevState(DS_SHUT_DOWN, false);
			
			cQueue_GotoStep( tp_task, STEP_END );  //结束
		}
		break ;
		
		default:
			cQueue_GotoStep(tp_task, STEP_END);
			break;
    }
	
	//初始化等待10S,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (10000 / sysTASK_CLOSE_CYCLE_TIME) && tp_task->ucStep != STEP_END)
	{
		
		bSys_SetErrCode(SEC_CLOSE_FAULT, true);
		
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_w("bSysTask:关闭系统任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep( tp_task, STEP_END );  //结束
	}
	
	#if(boardUSE_OS)
	vTaskDelay(sysTASK_CLOSE_CYCLE_TIME);
	#endif  //boardUSE_OS
}
