/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Dc
 * File    : dc_queue_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : DC 任务队列管理与状态调度实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Dc/dc_queue_task.h"

#if (boardDC_EN)
#include "Dc/dc_task.h"
#include "Dc/dc_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Task_T *tpDcTask = NULL;                                           /* DC 队列任务对象 */

//****************************************************Function Declaration******************************************************//
static bool b_task_manage_func_cb(Task_T *p_task);
static void v_add_task_return_func_cb(Task_T *p_task, uint8_t uc_num);


/***********************************************************************************************************************
 * 函数功能    : DC 队列初始化
 * 说明(备注)  : 创建 DC 任务队列，队列容量为 8，绑定任务管理与添加通知回调
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bDc_QueueInit(void)
{
	s8 c_result = 1;

	c_result = cQueue_TaskInit(&tpDcTask, 8, 0, b_task_manage_func_cb, v_add_task_return_func_cb);
	if (c_result <= 0)
	{
		if (uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
			log_e("bDcTask:tpDcTask任务对象初始化失败,代码%d", c_result);
		return false;
	}
	else if (tpDcTask == NULL)
	{
		if (uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
			log_e("bDcTask:tpDcTask任务对象创建失败");
		return false;
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : DC 任务管理回调函数
 * 说明(备注)  : 队列空闲时按设备状态装载默认任务(工作/错误/关机完成)
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *p_task)
{
	TaskItem_T t_item;

	if (p_task == NULL)
		return false;

	vQueue_ResetTaskState(p_task);

	if (tSysInfo.uInit.tFinish.bIF_DcTask == 0)
	{
		p_task->ucID      = DCTI_INIT;
		p_task->usInParam = 0;
	}
	else if (bQueue_PopTask(p_task, &t_item))
	{
		p_task->ucID      = t_item.ucId;
		p_task->usInParam = t_item.usParam;
	}
	else if (tDc.eDevState == DS_WORK)
	{
		p_task->ucID      = DCTI_WORK;
		p_task->usInParam = 0;
	}
	else if (tDc.eDevState == DS_ERR)
	{
		p_task->ucID      = DCTI_ERR;
		p_task->usInParam = 0;
	}
	else
	{
		p_task->ucID      = DCTI_SHUT_DOWN;
		p_task->usInParam = 0;
	}

	switch (p_task->ucID)
	{
		case DCTI_INIT:
		{
			p_task->vp_func = v_dc_queue_task_init;
			if (uPrint.tFlag.bDcTask)
				sMyPrint("bDcTask:----装载初始化任务----\r\n");
		}
		break;

		case DCTI_CLOSING:
		{
			p_task->vp_func = v_dc_queue_task_closing;
			if (uPrint.tFlag.bDcTask)
				sMyPrint("bDcTask:----装载关闭任务----\r\n");
		}
		break;

		case DCTI_SHUT_DOWN:
		{
			p_task->vp_func = v_dc_queue_task_shut_down;
			if (uPrint.tFlag.bDcTask)
				sMyPrint("bDcTask:----装载关闭完成任务----\r\n");
		}
		break;

		case DCTI_ERR:
		{
			p_task->vp_func = v_dc_queue_task_err;
			if (uPrint.tFlag.bDcTask)
				sMyPrint("bDcTask:----装载错误任务----\r\n");
		}
		break;

		case DCTI_BOOTING:
		{
			p_task->vp_func = v_dc_queue_task_booting;
			if (uPrint.tFlag.bDcTask)
				sMyPrint("bDcTask:----装载启动任务----\r\n");
		}
		break;

		case DCTI_WORK:
		{
			p_task->vp_func = v_dc_queue_task_work;
			if (uPrint.tFlag.bDcTask)
				sMyPrint("bDcTask:----装载工作任务----\r\n");
		}
		break;

		case DCTI_NULL:
		default:
		{
			p_task->vp_func   = NULL;
			p_task->usInParam = 0;
		}
		break;
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 添加任务返回回调
 * 说明(备注)  : 新任务压入队列后发送通知唤醒 OS 任务
 * 传入参数    : p_task: 队列任务控制块, uc_num: 事件类型
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_add_task_return_func_cb(Task_T *p_task, uint8_t uc_num)
{
	(void)p_task;

	switch (uc_num)
	{
		case 2:                                                         /* 添加了任务 (QE_TASK_POSTED) */
		{
			#if (boardUSE_OS)
			xTaskNotifyGive(tDcTaskHandler);
			#endif  /* boardUSE_OS */
		}
		break;

		default:
		{
		}
		break;
	}
}

#endif  /* boardDC_EN */
