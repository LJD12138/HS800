/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_queue_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Print/print_task.h"

#define       	bmsTASK_APP_INFO_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//

	
/*****************************************************************************************************************
-----函数功能    任务函数:错误处理任务
-----说明(备注)  none
-----传入参数    BMS_ErrState_N:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_get_app_info(Task_T *tp_task)
{
	TaskInParam_U u_in_param;
	u_in_param.usTaskInParam = tp_task->usInParam;
	
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(c_bms_cs_get_app_info(u_in_param.usTaskInParam) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		break;
		
		case 1:
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt>(5000/bmsTASK_APP_INFO_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bBmsTask)
			log_w("bBmsTask:回复信息任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	vTaskDelay(bmsTASK_APP_INFO_CYCLE_TIME);
	#endif  //boardUSE_OS
}
#endif  //boardBMS_EN
