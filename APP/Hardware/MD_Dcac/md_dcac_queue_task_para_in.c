/*****************************************************************************************************************
*                                                                                                                *
 *                                         队列函数                                                  			*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Print/print_task.h"

//#include "app_info.h"

#define       	dcacTASK_PARA_IN_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//



/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_para_in(Task_T *tp_task)
{
	static u16 us_temp = 0;
	SwitchType_E type = (SwitchType_E)tp_task->usInParam;
	
	switch(tp_task->ucStep)
	{
		case 0:
		{
//			if(type == ST_ON)
//			{
//				bDcac_SetAcState(OO_PARA_IN, IOS_STARTING);
//				us_temp = tAppMemParam.tDCAC.usParaInPwr;
//			}
//			else 
//			{
//				bDcac_SetAcState(OO_PARA_IN, IOS_CLOSING);
//				us_temp = 0;
//			}
			
			if(b_dcac_cs_set_para_in_pwr(us_temp) == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			
			tp_task->usStepRepeatCnt++;
			if(tp_task->usStepRepeatCnt > 3)
			{
				if(uPrint.tFlag.bDcacTask)
					log_w("bDcacTask:数据发送失败次数过多,退出设置并网放电任务");
				
				goto loop_para_in_end;
			}
		}
		break;

		case 1:
		{
			if(type == ST_ON)
				bDcac_SetAcState(OO_PARA_IN, IOS_WORK);   //开启
			else 
				bDcac_SetAcState(OO_PARA_IN, IOS_SHUT_DOWN);    //关闭	
			cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步	
		}
		break;
             
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt>(3000 / dcacTASK_PARA_IN_CYCLE_TIME))  //等待超时
	{
		loop_para_in_end:
		b_dcac_cs_set_para_in_pwr(0);
		bDcac_SetAcState(OO_PARA_IN, IOS_SHUT_DOWN);    //关闭	
		
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:并网控制任务处理超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	vTaskDelay(dcacTASK_PARA_IN_CYCLE_TIME);
}
#endif  //boardDCAC_EN
