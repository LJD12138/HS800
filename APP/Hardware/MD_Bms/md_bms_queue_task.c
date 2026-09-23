/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 任务队列管理与子任务分发实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_queue_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Task_T *tpBmsTask = NULL;                      /* 队列任务 */

//****************************************************Function Declaration******************************************************//
static bool b_task_manage_func_cb(Task_T *p_task);
static void v_add_task_return_func_cb(Task_T *p_task, uint8_t uc_num);

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : BMS 队列初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool bBms_QueueInit(void)
{
    s8 c_result = 1;
    
    /* 任务队列初始化，队列大小为8，回复缓存器大小为256 */
    c_result = cQueue_TaskInit(&tpBmsTask, 8, 256, b_task_manage_func_cb, v_add_task_return_func_cb);
    if (c_result <= 0)
    {
        if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
            log_e("bBmsTask:tpBmsTask任务对象初始化失败,代码%d", c_result);
        
        return false;
    }
    else if (tpBmsTask == NULL)
    {
        if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
            log_e("bBmsTask:tpBmsTask任务对象创建失败");
        
        return false;
    }
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 装载任务函数回调
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *p_task)
{
    TaskItem_T t_item;
    
    if (p_task == NULL)
        return false;
    
    vQueue_ResetTaskState(p_task);
    
    if (tSysInfo.uInit.tFinish.bIF_BmsTask == 0)
    {
        p_task->ucID = BTI_INIT;
        p_task->usInParam = 0;
    }
    else if (bQueue_PopTask(p_task, &t_item))    
    {
        p_task->ucID = t_item.ucId;
        p_task->usInParam = t_item.usParam;
    }
    else
    {
        if (tSysInfo.eDevState == DS_UPDATE_MODE)
        {
            p_task->ucID = BTI_NULL;
            p_task->usInParam = 0;
        }
        else
        {
            p_task->ucID = BTI_MAIN;
            p_task->usInParam = 0;
        }
    }
    
    switch (p_task->ucID)
    {
        case BTI_INIT:
        {
            p_task->vp_func = v_bms_queue_task_init;
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:----装载初始化任务----\r\n");
        }
        break;
        
        case BTI_MAIN:
        {
            p_task->vp_func = v_bms_queue_task_main;
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:----装载主任务----\r\n");
        }
        break; 
           
        case BTI_CTRL_BMS_SW:
        {
            p_task->vp_func = v_bms_queue_task_clt_switch;
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:----装载控制开关任务----\r\n");
        }
        break;
        
        case BTI_ERR_PROCESS:
        {
            p_task->vp_func = v_bms_queue_task_err;
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:----装载错误处理任务----\r\n");
        }
        break;
        
        case BTI_REQ_SET_CMD:
        {
            p_task->vp_func = v_bms_queue_task_req_set_cmd;
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:----装载请求设置BMS任务----\r\n");
        }
        break;
        
        case BTI_GET_INFO:
        {
            p_task->vp_func = v_bms_queue_task_get_app_info;
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:----装载获取Bms信息任务----\r\n");
        }
        break;
        
        case BTI_CALI:
        {
             p_task->vp_func = v_bms_queue_task_cali;
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:----装载校准任务----\r\n");
        }
        break;

        #if (boardRUN_LOG_EN)
        case BTI_LOG_UNLOCK:
        case BTI_GET_RUN_LOG:
        case BTI_READ_RUN_LOG_SLOT:
        case BTI_RESET_RUN_LOG:
        {
            p_task->vp_func = v_bms_queue_task_run_log;
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:----装载运行日志任务----\r\n");
        }
        break;  
        #endif  /* boardRUN_LOG_EN */

		#if (boardUPDATE)
		case BTI_UPDATE:
		{
			p_task->vp_func = v_bms_queue_task_update;
			if (uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:----装载升级任务----\r\n");
		}
		break;
		#endif  /* boardUPDATE */

		case BTI_NULL:
		default:
		{
			p_task->vp_func = NULL;
			p_task->usInParam = 0;
		}
		break;
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 添加任务后回调函数
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务指针, uc_num: 操作码
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_add_task_return_func_cb(Task_T *p_task, uint8_t uc_num)
{
    switch (uc_num)
	{
		/* 添加了任务 */
		case 2:
		{
			#if (boardUSE_OS)
			xTaskNotifyGive(tBmsTaskHandler);
			#endif  /* boardUSE_OS */
		}
		break;

		default:
		{
		}
		break;
	}
}

#endif  /* boardBMS_EN */
