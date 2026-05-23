/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Mppt/md_mppt_queue_task.h"

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#define       	mpptTASK_ERR_CYCLE_TIME               		100

//****************************************************函数声明****************************************************//


/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_mppt_queue_task_err_process(Task_T *tp_task)
{
	MpptErrCode_E e_err_code;
	
	e_err_code = (MpptErrCode_E)tp_task->usInParam;       //要设置MPPT的状态
	
	//丢失状态不需要去关闭
	if(e_err_code == MEC_SYS_DEV_LOST)
	{
		tp_task->ucStep = 2;
	}
	
	bMppt_SetDevState(DS_ERR);
	
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(c_mppt_cs_set_pwr(0) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		break;
		
		case 1:
		{
			if(tMpptRx.usMaxInPwr == 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		break;
		
		case 2:
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	//等待超时
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt>(3000/mpptTASK_ERR_CYCLE_TIME)) 
	{
		if(uPrint.tFlag.bMpptTask)
			log_w("bMpptTask:错误处理任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	vTaskDelay(mpptTASK_ERR_CYCLE_TIME);
	#endif  //boardUSE_OS
}

#endif  //boardMPPT_EN


