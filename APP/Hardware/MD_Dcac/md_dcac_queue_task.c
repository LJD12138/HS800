/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务的队列函数                                                  *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"


//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	Task_T *tpDcacTask = NULL;  		//队列任务


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
bool bDcac_QueueInit(void)
{
	s8 c_result = 1;
	
	//任务队列初始化，队列大小为8，回复缓存器大小为0
	c_result = cQueue_TaskInit(&tpDcacTask, 8, 0, b_task_manage_func_cb, v_add_task_return_func_cb);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_e("bDcacTask:tpDcacTask任务对象初始化失败,代码&d",c_result);
		
		return false;
	}
	else if(tpDcacTask == NULL)
	{
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_e("bDcacTask:tpDcacTask任务对象创建失败");
		
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
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_e("bDcacTask:任务队列长度异常 长度%d",uc_temp);
		lwrb_reset(&tp_task->tQueueBuff);
		return false;
	}
	
	if(tSysInfo.uInit.tFinish.bIF_DcacTask == 0)
	{
		tp_task->ucID = DTI_INIT;
		tp_task->usInParam = 0;
	}
    else if(uc_temp)    
    {
        lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->ucID, 1);
		lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->usInParam, 2);
    }
    else
    {
		tp_task->ucID = DTI_MAIN;
		tp_task->usInParam = 0;
    }
    
    switch (tp_task->ucID)
    {
        case DTI_INIT:
        {
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:----装载初始化任务----\r\n");
			
            tp_task->vp_func = v_dcac_queue_task_init;
        }
        break;
        
        case DTI_MAIN:
        {	
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:----装载主任务----\r\n");
			
            tp_task->vp_func = v_dcac_queue_task_main;
        }
        break;    
        
        case DTI_CTRL_DCAC_OUT:
        {
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:----装载交流输出任务 参数0x%x----\r\n", tp_task->usInParam);
			
			tp_task->vp_func = v_dcac_queue_task_dcac_out;
        }
        break;

        case DTI_CTRL_DCAC_IN:
		{
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:----装载开启充电任务 参数0x%x----\r\n", tp_task->usInParam);
			
			tp_task->vp_func = v_dcac_queue_task_dcac_in;
		}
		break;
		
		case DTI_CTRL_PARA_IN:
        {	
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:----装载并网放电任务 参数0x%x----\r\n", tp_task->usInParam);
			
            tp_task->vp_func = v_dcac_queue_task_para_in;
        }
        break;
		
		case DTI_ERR_PROC:
        {	
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:----装载错误处理任务----\r\n");
			
            tp_task->vp_func = v_dcac_queue_task_err_proc;
        }
        break;

		case DTI_UPDATE:
		{
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:----装载升级任务----\r\n");
			
			tp_task->vp_func = v_dcac_queue_task_update;
		}
		break;
		
		case DTI_NULL:
        default:
			bDcac_SetAcState(OO_ALL, IOS_SHUT_DOWN);
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
			xTaskNotifyGive(tDcacTaskHandler);
			#endif  //boardUSE_OS
		}
		break;
		
		default:
			break;
	}
}
#endif  //boardDCAC_EN
