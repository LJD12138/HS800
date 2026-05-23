/*****************************************************************************************************************
*                                                                                                                *
 *                                         队列函数                                                  			*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Print/print_task.h"

//#include "app_info.h"

#define       	dcacTASK_ERR_PROC_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//



/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_err_proc(Task_T *tp_task)
{
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(b_dcac_cs_sys_switch(dcacSWITCH_REG_OFF) == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
		}
		break;

		case 1:
		{
			bDcac_SetAcState(OO_CHG, IOS_ERR);
			bDcac_SetAcState(OO_DISCHG, IOS_ERR);
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
             
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt>(3000 / dcacTASK_ERR_PROC_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:错误处理任务处理超时,步骤%d",tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	vTaskDelay(dcacTASK_ERR_PROC_CYCLE_TIME);
}


