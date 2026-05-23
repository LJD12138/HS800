/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Mppt/md_mppt_queue_task.h"

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"


#define       	mpptTASK_INIT_CYCLE_TIME               		100


/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_mppt_queue_task_init(Task_T *tp_task)
{
	switch (tp_task->ucStep)
    {
		case 0:
        {
			if(1)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else
				break;
        }
		break;
	
		case 1:
		{
			tSysInfo.uInit.tFinish.bIF_MpptTask = true;
			bMppt_SetDevState(DS_SHUT_DOWN);
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			return;
		}
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	//等待超时
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt>(3000/mpptTASK_INIT_CYCLE_TIME)) 
	{
		if(uPrint.tFlag.bMpptTask)
			log_w("bMpptTask:初始化任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, mpptTASK_INIT_CYCLE_TIME);
	#endif  //boardUSE_OS
}

#endif  //boardMPPT_EN
