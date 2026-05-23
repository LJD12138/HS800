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
#include "Update/update_main.h"

/*****************************************************************************************************************
-----函数功能    任务函数:主任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_main(Task_T *tp_task)
{	
	switch(tp_task->ucStep)
	{
		case 0:
		{	
			tp_task->usStepWaitCnt++;
			if(tp_task->usStepWaitCnt >= (1000/bmsTASK_CYCLE_TIME))
				cQueue_GotoStep(tp_task, STEP_NEXT);
		}
		break;
		
		case 1:
		{	
			c_bms_cs_send_update();
			cQueue_GotoStep(tp_task, STEP_END);
		}
		break;
		
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000/bmsTASK_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bBmsTask)
			sMyPrintWarn("bBmsTask:获取数据任务等待超时,退出");
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
}

#endif  //boardBMS_EN
