/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

/***********************************************************************************************************************
-----函数功能    工作
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_reset(Task_T *tp_task)
{
	switch (tp_task->ucStep)
    {
		//************************************步骤零:关闭BMS**********************************************
		case 0:
		{
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
		}break ;
		
		case 1:
		{
			NVIC_SystemReset();//重启
		}break;
		
        default:
				cQueue_GotoStep( tp_task, STEP_END );  //结束
			break;
    }
	
	//等待5S,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000/sysTASK_CYCLE_TIME) && tp_task->ucStep != STEP_END)
	{
		if(uPrint.tFlag.bSysTask)
			log_w("bSysTask:设置重置任务等待超时,退出");
		
		cQueue_GotoStep( tp_task, STEP_END );  //结束
	}
}


