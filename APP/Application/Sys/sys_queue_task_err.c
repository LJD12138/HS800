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

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif


#define     	sysTASK_ERR_CYCLE_TIME					sysTASK_CYCLE_TIME //任务时间


/***********************************************************************************************************************
-----函数功能    错误任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_err(Task_T *tp_task)
{
//	s8 c_ret = 0;
//	BmsErrCode_N u_err_code;
//	
//	u_err_code = tBms.uErrCode;
	
//	BIT_CLR(u_err_code.ulCode, BEC_AFE_DISCHG_UV);
//	BIT_CLR(u_err_code.ulCode, BEC_BMS_CELL_UV);
	
	//队列里面有任务
	if(lwrb_get_full(&tp_task->tQueueBuff))  
	{
		cQueue_GotoStep( tp_task, STEP_END );  //结束
		return;
	}
					
    switch (tp_task->ucStep)
    {
		//************************************步骤0:初始化*************************************************
		case 0:
		{
			bSys_SetDevState(DS_ERR, false);
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
		}
		break;
		
		//************************************步骤2:等待关闭*************************************************
		case 1:
		{
			#if(boardKEY_EN)
			//按键按下即可重置
			if(bKey_AcIsPress() == true ||
				bKey_PowerIsPress() == true ||
				bKey_UsbIsPress() == true ||
				bKey_DcIsPress() == true ||
				bKey_LightIsPress() == true)
				tp_task->usStepWaitCnt = 0;
			#endif
				
			//存在充电
			if(bSys_ExistInVolt() == true)
			{
				cQueue_GotoStep(tp_task, 3);
				return;
			}

			//倒计时退出
			if(tSysInfo.uErrCode.tCode.bUV == 1 || tSysInfo.uErrCode.tCode.b0SOC == 1)
			{
				tp_task->usStepWaitCnt++;
				if(tp_task->usStepWaitCnt >= (10000 / sysTASK_ERR_CYCLE_TIME))
				{
					tp_task->usStepWaitCnt = 0;
					bSys_SetDevState(DS_SHUT_DOWN, true);
					cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
				}
			}
			else
			{
				tp_task->usStepWaitCnt = 0;
				cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
			}
		}
		break;
		
		//************************************步骤2:进入关机**************************************************
		case 2:
		{
			//存在充电
			if(bSys_ExistInVolt() == true)
			{
				cQueue_GotoStep(tp_task, 3);
				return;
			}

			bSys_SetDevState(DS_SHUT_DOWN, false);

			#if(boardBMS_EN)
			cBms_Switch(SO_KEY, ST_OFF, false);
			#endif  //boardBMS_EN

			#if(boardUSE_OS)
			vTaskDelay(1000);
			#endif  //boardUSE_OS
		}
		break ;

		//************************************步骤3:充电开机**************************************************
		case 3:
		{
			cSys_Switch(SO_MPPT, ST_ON, false); //开机
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:开启充电唤醒\r\n");

			cQueue_GotoStep( tp_task, STEP_END );  //结束
			return;
		}
		
        default:
			if(lwrb_get_full(&tp_task->tQueueBuff))  //队列里面有任务
				cQueue_GotoStep( tp_task, STEP_END );  //结束
			break;
    }
	
	//等待60S,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (60000 / sysTASK_ERR_CYCLE_TIME) && tp_task->ucStep != STEP_END)
	{
		bSys_SetDevState(DS_SHUT_DOWN, false);

		#if(boardBMS_EN)
		cBms_Switch(SO_KEY, ST_OFF, false);
		#endif  //boardBMS_EN
	}
	
	#if(boardUSE_OS)
	vTaskDelay(sysTASK_ERR_CYCLE_TIME);
	#endif  //boardUSE_OS
}

