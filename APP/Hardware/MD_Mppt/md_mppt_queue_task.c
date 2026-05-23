/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务的队列函数                                                  *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Mppt/md_mppt_queue_task.h"

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"


//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	Task_T *tpMpptTask = NULL;  	//队列任务


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
bool bMppt_QueueInit(void)
{
	s8 c_result = 1;
	
	//任务队列初始化，队列大小为8，回复缓存器大小为0
	c_result = cQueue_TaskInit(&tpMpptTask, 8, 12, b_task_manage_func_cb, v_add_task_return_func_cb);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
			log_e("bMpptTask:tpMpptTask任务对象初始化失败,代码&d",c_result);
		
		return false;
	}
	else if(tpMpptTask == NULL)
	{
		if(uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
			log_e("bMpptTask:tpMpptTask任务对象创建失败");
		
		return false;
	}
	
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
		if(uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
			log_e("bMpptTask:任务队列长度异常 长度%d",uc_temp);
		lwrb_reset(&tp_task->tQueueBuff);
		return false;
	}
	
	if(tSysInfo.uInit.tFinish.bIF_MpptTask == 0)
	{
		tp_task->ucID = MTI_INIT;
		tp_task->usInParam = 0;
	}
    else if(uc_temp)    
    {
        lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->ucID, 1);
		lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->usInParam, 2);
    }
    else
    {
		tp_task->ucID = MTI_MAIN;
		tp_task->usInParam = 0;
    }
    
    switch (tp_task->ucID)
    {
        case MTI_INIT:
        {
            tp_task->vp_func = v_mppt_queue_task_init;
			
			if(uPrint.tFlag.bMpptTask)
				sMyPrint("bMpptTask:----装载初始化任务----\r\n");
        }break;
        
        case MTI_MAIN:
        {			
            tp_task->vp_func = v_mppt_queue_task_main;
			
			if(uPrint.tFlag.bMpptTask)
				sMyPrint("bMpptTask:----装载主任务----\r\n");
        }break; 
           
        case MTI_SET_CHG_PWR:
        {  
			tp_task->vp_func = v_mppt_queue_task_set_chg_pwr;
			
			if(uPrint.tFlag.bMpptTask)
				sMyPrint("bMpptTask:----装载设置充电功率任务 参数%dW----\r\n",tp_task->usInParam);
        }break;
		
		case MTI_ERR_PROCESS:
        {			
            tp_task->vp_func = v_mppt_queue_task_err_process;
			
			if(uPrint.tFlag.bMpptTask)
				sMyPrint("bMpptTask:----装载错误处理任务----\r\n");
        }break;
		
		case MTI_NULL:
        default:
            tp_task->vp_func = NULL;
			tp_task->usInParam = 0;
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
			xTaskNotifyGive(tMpptTaskHandler); //发通知
			#endif  //boardUSE_OS
		}
		break;
		
		default:
			break;
	}
}

#endif  //boardMPPT_EN



