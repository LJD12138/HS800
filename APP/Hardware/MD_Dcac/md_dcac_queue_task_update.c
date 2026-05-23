/*****************************************************************************************************************
*                                                                                                                *
 *                                         队列函数                                                  			*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#define       	dcacTASK_UPDATE_CYCLE_TIME               		100

//****************************************************函数声明****************************************************//



/*****************************************************************************************************************
-----函数功能    任务函数:升级
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_update(Task_T *tp_task)
{
	switch (tp_task->ucStep)
    {
		case 0:
        {
			if(b_dcac_cs_init() == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);
			else
			{
				vTaskDelay(500);
				break;
			}
        }

		case 1:
        {
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:升级DCAC----升级完成----\r\n");
			
			cQueue_GotoStep(tp_task, STEP_END);
        }
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000 / dcacTASK_UPDATE_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:升级任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	vTaskDelay(dcacTASK_UPDATE_CYCLE_TIME);
}

#endif  //boardDCAC_EN
