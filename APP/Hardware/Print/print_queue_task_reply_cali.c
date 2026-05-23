/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_queue_task.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"

#define       	printTASK_CALI_CYCLE_TIME               		50

/*****************************************************************************************************************
-----函数功能    任务函数:校准任务
-----说明(备注)  none
-----传入参数    none:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_print_queue_task_reply_cali(Task_T *tp_task)
{
	u16 us_temp = tp_task->usInParam;
	
	switch(tp_task->ucStep)
	{
		case 0 :
		{
			if(c_relay45_cali(us_temp) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);
			else
				break;
		}
		
		case 1 :
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			return;
		}
		
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000/printTASK_CALI_CYCLE_TIME))  //等待超时
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, printTASK_CALI_CYCLE_TIME);
	#endif  //boardUSE_OS
}

#endif  //boardPRINT_IFACE
