/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务的队列函数                                                  *
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_queue_task.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_iface.h"
#include "Sys/sys_task.h"


//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	Task_T *tpPrintTask = NULL;  	//队列任务


//****************************************************函数声明****************************************************//
static bool b_task_manage_func_cb(Task_T *tp_task);
static void v_add_task_return_func_cb(Task_T *tp_task, u8 num);


/***********************************************************************************************************************
-----函数功能    队列初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bPrint_QueueInit(void)
{
	s8 c_result = 1;
	
	//任务队列初始化，队列大小为8，回复缓存器大小为256
	c_result = cQueue_TaskInit(&tpPrintTask, 8, 256, b_task_manage_func_cb, v_add_task_return_func_cb);
	if(c_result <= 0)
		return false;
	else if(tpPrintTask == NULL)
		return false;
	
	return true;
}


/*****************************************************************************************************************
-----函数功能    装载任务函数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:成功   false:失败 
******************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *tp_task)
{
	vu16 uc_temp = 0;
	
	if(tp_task == NULL)
		return false;
	
	tp_task->bNowRun = false;
	tp_task->ucStep = 0;
	tp_task->usStepWaitCnt = 0;
	tp_task->usTaskWaitCnt = 0;
	tp_task->usStepRepeatCnt = 0;
	
	uc_temp = lwrb_get_full(&tp_task->tQueueBuff);
	if(uc_temp%3 != 0 && uc_temp != 0)
	{
		lwrb_reset(&tp_task->tQueueBuff);
		return false;
	}
	
	if(uc_temp)    
    {
        lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->ucID, 1);
		lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->usInParam, 2);
    }
    else
    {
		tp_task->ucID = PTI_MAIN;
		tp_task->usInParam = 0;
    }
    
    switch (tp_task->ucID)
    {
        case PTI_MAIN:
        {			
            tp_task->vp_func = v_print_queue_task_main;
        }
		break; 

    }
    
    return true;       
}

static void v_add_task_return_func_cb(Task_T *tp_task, u8 num)
{
	switch(num)
	{
		//添加了任务
		case 2:
		{
			#if(boardUSE_OS)
			xTaskNotifyGive(tPrintTaskHandler);
			#endif  //boardUSE_OS
		}
		break;
		
		default:
			break;
	}
}

#endif  //boardPRINT_IFACE


