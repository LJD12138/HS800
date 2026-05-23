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

#define       	bmsTASK_CALI_CYCLE_TIME               		50

/*****************************************************************************************************************
-----函数功能    任务函数:校准任务
-----说明(备注)  none
-----传入参数    none:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_cali(Task_T *tp_task)
{
	u8 temp = tp_task->usInParam;
	
	switch(tp_task->ucStep)
	{
		case 0 :
		{
			c_bms_cs_set_cali(temp);
			cQueue_GotoStep(tp_task, STEP_NEXT);
		}
		break;
		
		case 1 :
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
		
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000/bmsTASK_CALI_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bBmsTask)
			log_w("bBmsTask:校准任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	vTaskDelay(bmsTASK_CALI_CYCLE_TIME);
	#endif  //boardUSE_OS
}

#endif  //boardBMS_EN
