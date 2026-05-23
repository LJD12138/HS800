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
#include "Buz/buz_task.h"

#define       	bmsTASK_ERR_CYCLE_TIME               		50

/*****************************************************************************************************************
-----函数功能    任务函数:错误处理任务
-----说明(备注)  none
-----传入参数    BMS_ErrState_N:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_err(Task_T *tp_task)
{
	switch(tp_task->ucStep)
	{
		case 0:
		{	
			//设备丢失
			if(tBms.uErrCode.tCode.bSysDevLost == 1)
			{
				#if(boardBUZ_EN)
				bBuz_Tweet(LONG_3);
				#endif  //boardBUZ_EN
				
				bBms_SetDevState(DS_LOST);
			}
			else
				bBms_SetDevState(DS_ERR);
			
			cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
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
	if(tp_task->usTaskWaitCnt > (3000/bmsTASK_ERR_CYCLE_TIME))  //等待超时
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_w("bBmsTask:错误处理任务超时,步骤%d", tp_task->ucStep);
	}
	
	#if(boardUSE_OS)
	vTaskDelay(bmsTASK_ERR_CYCLE_TIME);
	#endif  //boardUSE_OS
}
#endif  //boardBMS_EN
