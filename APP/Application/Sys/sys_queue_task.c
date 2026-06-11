/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务的队列函数                                                  *
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"


//****************************************************参数初始化**************************************************//
//结构体
__ALIGNED(4) 	Task_T *tpSysTask = NULL;  	//队列任务


//****************************************************函数声明****************************************************//
static bool b_task_manage_func_cb(Task_T *tp_task);
static void v_add_task_return_func_cb(Task_T *tp_task, u8 num);

/***********************************************************************************************************************
-----函数功能    任务参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bSys_QueueInit(void)
{
	//任务队列初始化
	if(cQueue_TaskInit(&tpSysTask, 8, 12, b_task_manage_func_cb, v_add_task_return_func_cb) <= 0)
	{
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_e("bSysTask:tpSysTask任务对象初始化失败");
		
		return false;
	}
	else if(tpSysTask == NULL)
	{
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_e("bSysTask:tpSysTask任务对象创建失败");
		
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
	static vu16 uc_temp = 0;
	
	tp_task->bNowRun = false;
	tp_task->ucStep = 0;
	tp_task->usTaskWaitCnt = 0;
	tp_task->usTaskWaitCnt = 0;
	tp_task->usStepRepeatCnt = 0;
	
	uc_temp = lwrb_get_full(&tp_task->tQueueBuff);
	if(uc_temp%3 != 0 && uc_temp != 0)
	{
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_e("bSysTask:任务队列长度异常 长度%d",uc_temp);
		lwrb_reset(&tp_task->tQueueBuff);
		return false;
	}
	
	if(tSysInfo.uInit.tFinish.bIF_SysTask == 0)
	{
		tp_task->ucID = STI_INIT;           
		tp_task->usInParam = 0;
	}
    else if(uc_temp)//队列里面有任务   
    {
        lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->ucID, 1);
		lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->usInParam, 2);
    }
    else
    {
		if( tSysInfo.eDevState == DS_WORK)
		{
			tp_task->ucID = STI_WORK;           
			tp_task->usInParam = 0;
		}
		else if( tSysInfo.eDevState == DS_ERR)
		{
			tp_task->ucID = STI_ERR;           
			tp_task->usInParam = 0;
		}
		else 
		{
			tp_task->ucID = STI_SHUT_DOWN;           
			tp_task->usInParam = 0;
		}
    }
    
    switch (tp_task->ucID)
    {
        case STI_INIT:
        {
			if(uPrint.tFlag.bSysTask)
				sMyPrint("\r\n bSysTask:----装载初始化任务---- \r\n");
            tp_task->vp_func = v_sys_queue_task_init;
        }
        break;
		
		case STI_CLOSING:
        {	
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载关闭任务----\r\n");
			tp_task->vp_func = v_sys_queue_task_closing;
        }
        break;

		case STI_SHUT_DOWN:
        {	
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载关闭完成任务----\r\n");
			tp_task->vp_func = v_sys_queue_task_shut_down;
        }
        break;
		
		case STI_ERR:
        {  
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载错误任务----\r\n");
			tp_task->vp_func = v_sys_queue_task_err;
        }
        break;
		
		case STI_RESET:
        {  
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载重置任务----\r\n");
			tp_task->vp_func = v_sys_queue_task_reset;
        }
        break;
        
        case STI_BOOTING:
        {  
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载启动任务----\r\n");
			tp_task->vp_func = v_sys_queue_task_booting;
        }
        break;

		case STI_WORK:
        {  
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载工作任务----\r\n");
			tp_task->vp_func = v_sys_queue_task_work;
        }
        break;

		#if(boardUPDATE)
		case STI_UPDATE:
        {  
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载升级任务----\r\n");
			tp_task->vp_func = v_sys_queue_task_update;
        }
        break;
		#endif  //boardUPDATE
		
		#if(boardENG_MODE_EN)
		case STI_ENG:
        {  
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载工程模式任务----\r\n");
			tp_task->vp_func = v_sys_queue_task_eng;
			bSys_SetDevState(DS_ENG_MODE, false);  //进入工程模式
        }
        break;
		#endif
		
		case STI_NULL:
        default:
            tp_task->vp_func = NULL;
			tp_task->usInParam = 0;
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载空任务----\r\n");
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
			xTaskNotifyGive(tSysTaskHandler);
			#endif  //boardUSE_OS
		}
		break;
		
		default:
			break;
	}
}

