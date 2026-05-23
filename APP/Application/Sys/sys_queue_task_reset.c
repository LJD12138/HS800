/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "gpio_init.h"
#include "app_info.h"

#define     	sysTASK_RESET_CYCLE_TIME					sysTASK_CYCLE_TIME //任务时间

/***********************************************************************************************************************
-----函数功能    工作
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_reset(Task_T *tp_task)
{
	s8 c_ret = 0;
	
    switch (tp_task->ucStep)
    {
		case 0:
		{
			//初始化APP数据
			c_ret = cApp_MemParamInit(tAppMemParamStr);
			if(c_ret <= 0)
			{
				if(uPrint.tFlag.bAppInfo)
					sMyPrint("bAppInfo:APP参数初始化失败 代码%d\r\n",c_ret);
				break;
			}
			
			//更新参数
			c_ret = cApp_UpdateMemParam(tAppMemParamStr);
			if(c_ret <= 0)
			{
				if(uPrint.tFlag.bAppInfo)
					sMyPrint("bAppInfo:APP参数更新失败 代码%d\r\n",c_ret);
				break;
			}
			
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
		}break;
		
		case 1:
		{
			NVIC_SystemReset();//重启
		}break;
		
        default:
				cQueue_GotoStep( tp_task, STEP_END );  //结束
			break;
    }
	
	//等待5S,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000 / sysTASK_RESET_CYCLE_TIME) && tp_task->ucStep != STEP_END)
	{
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_w("bSysTask:设置重置任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep( tp_task, STEP_END );  //结束
	}
	
	#if(boardUSE_OS)
	vTaskDelay(sysTASK_RESET_CYCLE_TIME);
	#endif  //boardUSE_OS
}


