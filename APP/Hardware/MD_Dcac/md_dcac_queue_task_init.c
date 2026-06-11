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
#include "app_info.h"

#define       	dcacTASK_INIT_CYCLE_TIME               		100

//****************************************************函数声明****************************************************//



/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_init(Task_T *tp_task)
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
			// if(b_dcac_cs_set_chg_pwr(tAppMemParam.tDCAC.usInPwrRating) == true)  //获取参数
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			// else
			// {
			// 	vTaskDelay(500);
			// 	break;
			// }
		}

		case 2:
        {
			tSysInfo.uInit.tFinish.bIF_DcacTask = 1;
			bDcac_SetAcState(OO_ALL, IOS_SHUT_DOWN);
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:初始化DCAC----初始化完成----\r\n");
			
			cQueue_GotoStep(tp_task, STEP_END);
        }
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000 / dcacTASK_INIT_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:初始化任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	vTaskDelay(dcacTASK_INIT_CYCLE_TIME);
}

#endif  //boardDCAC_EN
