/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_queue_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print模块任务队列初始化、任务分发与管理
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_queue_task.h"

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_iface.h"
#include "Sys/sys_task.h"

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Task_T *tpPrintTask = NULL;  	//队列任务

//****************************************************Function Declaration******************************************************//
static bool b_task_manage_func_cb(Task_T *tp_task);
static void v_add_task_return_func_cb(Task_T *tp_task, u8 num);


/***********************************************************************************************************************
 * 函数功能    : 队列初始化
 * 说明(备注)  : 任务队列大小为8, 回复缓存器大小为256
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bPrint_QueueInit(void)
{
	s8 c_result = 1;
	
	c_result = cQueue_TaskInit(&tpPrintTask, 8, 256, b_task_manage_func_cb, v_add_task_return_func_cb);
	if (c_result <= 0)
		return false;
	else if (tpPrintTask == NULL)
		return false;
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 装载任务回调函数
 * 说明(备注)  : 任务队列出队并匹配执行对应的子任务函数
 * 传入参数    : tp_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *tp_task)
{
	TaskItem_T t_item;
	
	if (tp_task == NULL)
		return false;
	
	vQueue_ResetTaskState(tp_task);
	
	if (bQueue_PopTask(tp_task, &t_item))    
    {
        tp_task->ucID = t_item.ucId;
		tp_task->usInParam = t_item.usParam;
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
			v_print_queue_task_main(tp_task);
		}
        break; 

		case PTI_REPLY_APP_INFO:
		{
            tp_task->vp_func = v_print_queue_task_reply_app_info;
		}
		break;
		
		case PTI_REPLY_CALI:
		{
            tp_task->vp_func = v_print_queue_task_reply_cali;
		}
		break;

		#if (boardBMS_EN && boardRUN_LOG_EN)
		case PTI_REPLY_LOG_UNLOCK:
		case PTI_REPLY_RUN_LOG_SLOT:
		case PTI_REPLY_RESET_RUN_LOG:
		{
			tp_task->vp_func = v_print_queue_task_reply_run_log;
		}
		break;
		#endif  /* boardBMS_EN && boardRUN_LOG_EN */

		#if (boardUPDATE)
		case PTI_UPDATE:
		{
            tp_task->vp_func = v_print_queue_task_update;
		}
		break;
		#endif  /* boardUPDATE */
        
		case PTI_NULL:
        default:
            tp_task->vp_func = NULL;
			tp_task->usInParam = 0;
			break;
    }
    
    return true;       
}

/***********************************************************************************************************************
 * 函数功能    : 添加任务返回函数回调
 * 说明(备注)  : none
 * 传入参数    : tp_task: 任务结构体指针, num: 事件类型编号
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_add_task_return_func_cb(Task_T *tp_task, u8 num)
{
	switch (num)
	{
		/* 添加了任务 */
		case 2:
		{
			#if (boardUSE_OS)
			xTaskNotifyGive(tPrintTaskHandler);
			#endif  /* boardUSE_OS */
		}
		break;
		
		default:
		{
		}
		break;
	}
}

#endif  /* boardPRINT_IFACE */
