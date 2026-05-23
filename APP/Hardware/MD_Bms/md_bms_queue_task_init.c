/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_queue_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#define       	bmsTASK_INIT_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//




/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_init(Task_T *tp_task)
{
//	s8 ret = 0;
	
	switch (tp_task->ucStep)
    {
		case 0:
        {
			//获取参数,用来判断是否是充电唤醒
			if(c_bms_cs_get_param(1) > 0 || G_TestMode == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else
				break;
        }
		
		case 1:
		{
			tSysInfo.uInit.tFinish.bIF_BmsTask = 1;
			cBms_CheckPerm();
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:初始化BMS----初始化完成----\r\n");
			
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
        break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000 / bmsTASK_INIT_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bBmsTask)
			log_w("bBmsTask:BMS初始化任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	vTaskDelay(bmsTASK_INIT_CYCLE_TIME);
	#endif  //boardUSE_OS
}
#endif  //boardBMS_EN
