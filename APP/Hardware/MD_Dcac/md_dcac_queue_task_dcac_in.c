/*****************************************************************************************************************
*                                                                                                                *
 *                                         队列函数: 开关充电                                                  			*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "app_info.h"

#define       	dcacTASK_IN_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//



/*****************************************************************************************************************
-----函数功能    任务函数:开关充电
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_dcac_in(Task_T *tp_task)
{
	u16 us_temp = 0;
	SwitchType_E type = (SwitchType_E)tp_task->usInParam;
    
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(type == ST_ON)
				us_temp = tAppMemParam.tDCAC.usMinInPwr;
			
			if(b_dcac_cs_set_chg_pwr(us_temp) == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			
			tp_task->usStepRepeatCnt++;
			if(tp_task->usStepRepeatCnt > 3)
			{
				if(uPrint.tFlag.bDcacTask)
					log_w("bDcacTask:数据发送失败次数过多,退出开关逆变充电任务");
				
				goto loop_end;
			}
		}break;
		
		case 1:
		{
			cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步	
		}
		break;
		
		case 2:
		{	
			if(uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:充电开关操作完成\r\n");
			
			cQueue_GotoStep(tp_task, STEP_END);  //结束

			return;
		}
			
		default:
		    cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000 / dcacTASK_IN_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bDcacTask)
			log_w("bDcacTask:控制逆变充电超时,步骤%d", tp_task->ucStep);
		
		loop_end:
		if(type == ST_ON)
		{
			b_dcac_cs_set_chg_pwr(0);
			bDcac_SetAcState(OO_CHG, IOS_SHUT_DOWN);
		}
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	vTaskDelay(dcacTASK_IN_CYCLE_TIME);
}
#endif  //boardDCAC_EN
