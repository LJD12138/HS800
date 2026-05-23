/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

/***********************************************************************************************************************
-----函数功能    错误任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_err(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
		case 0:
		{
			//队列里面有任务
			if(lwrb_get_full(&tp_task->tQueueBuff))  
			{
				cQueue_GotoStep( tp_task, STEP_END );  //结束
				break;
			}
			
			if(tSysInfo.eDevState != DS_ERR)
				bSys_SetDevState(DS_ERR, false);
			
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
			break;
		}
		
		//************************************步骤1:关闭并机功能*********************************************
		case 1:
		{
			cQueue_GotoStep( tp_task, STEP_END );  //结束
		}break ;
		
        default:
			if(lwrb_get_full(&tp_task->tQueueBuff))  //队列里面有任务
				cQueue_GotoStep( tp_task, STEP_END );  //结束
			break;
    }
	
	//等待5S,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000/sysTASK_CYCLE_TIME) && tp_task->ucStep != STEP_END)
	{
		if(uPrint.tFlag.bSysTask)
			log_w("bSysTask:错误任务等待超时,退出");

		cQueue_GotoStep( tp_task, STEP_END );  //结束
	}
}

