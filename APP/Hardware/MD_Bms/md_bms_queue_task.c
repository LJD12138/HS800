/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务的队列函数                                                  *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_queue_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"


//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	Task_T *tpBmsTask = NULL;  	//队列任务


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
bool bBms_QueueInit(void)
{
	s8 c_result = 1;
	
	//任务队列初始化，队列大小为8，回复缓存器大小为256
	c_result = cQueue_TaskInit(&tpBmsTask, 8, 256, b_task_manage_func_cb, v_add_task_return_func_cb);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_e("bBmsTask:tpBmsTask任务对象初始化失败,代码&d",c_result);
		
		return false;
	}
	else if(tpBmsTask == NULL)
	{
		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_e("bBmsTask:tpBmsTask任务对象创建失败");
		
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
		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_e("bBmsTask:任务队列长度异常 长度%d",uc_temp);
		lwrb_reset(&tp_task->tQueueBuff);
		return false;
	}
	
	if(tSysInfo.uInit.tFinish.bIF_BmsTask == 0)
	{
		tp_task->ucID = BTI_INIT;
		tp_task->usInParam = 0;
	}
    else if(uc_temp)    
    {
        lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->ucID, 1);
		lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->usInParam, 2);
    }
    else
    {
		tp_task->ucID = BTI_MAIN;
		tp_task->usInParam = 0;
    }
    
    switch (tp_task->ucID)
    {
        case BTI_INIT:
        {
            tp_task->vp_func = v_bms_queue_task_init;
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:----装载初始化任务----\r\n");
        }break;
        
        case BTI_MAIN:
        {			
            tp_task->vp_func = v_bms_queue_task_main;
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:----装载主任务----\r\n");
        }break; 
           
        case BTI_CTRL_BMS_SW:
        {  
			tp_task->vp_func = v_bms_queue_task_clt_switch;
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:----装载控制开关任务----\r\n");
        }break;
		
		case BTI_ERR_PROCESS:
        {			
            tp_task->vp_func = v_bms_queue_task_err;
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:----装载错误处理任务----\r\n");
        }break;
		
		case BTI_REQ_SET_CMD:
        {			
            tp_task->vp_func = v_bms_queue_task_req_set_cmd;
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:----装载请求设置BMS任务----\r\n");
        }break;
		
		case BTI_GET_INFO:
        {			
            tp_task->vp_func = v_bms_queue_task_get_app_info;
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:----装载获取Bms信息任务----\r\n");
        }break;
		
		case BTI_CALI:
        {			
            tp_task->vp_func = v_bms_queue_task_cali;
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:----装载校准任务----\r\n");
        }break;

		#if(boardUPDATE)
		case BTI_UPDATE:
        {			
            tp_task->vp_func = v_bms_queue_task_update;
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:----装载升级任务----\r\n");
        }break;
		#endif  //boardUPDATE
        
		case BTI_NULL:
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
			xTaskNotifyGive(tBmsTaskHandler);
			#endif  //boardUSE_OS
		}
		break;
		
		default:
			break;
	}
}

#endif  //boardBMS_EN
