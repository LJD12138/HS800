/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统总任务队列调度分发实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (1)

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Task_T *tpSysTask = NULL;      /* 系统总任务控制块指针 */

//****************************************************Function Declaration******************************************************//
static bool b_task_manage_func_cb(Task_T *p_task);
static void v_add_task_return_func_cb(Task_T *p_task, u8 num);

/***********************************************************************************************************************
 * 函数功能    : 系统任务队列初始化
 * 说明(备注)  : 初始化系统主任务队列对象并注册装载与返回回调函数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 成功, false 失败
 ************************************************************************************************************************/
bool bSys_QueueInit(void)
{
    if (cQueue_TaskInit(&tpSysTask, 8, 12, b_task_manage_func_cb, v_add_task_return_func_cb) <= 0)
    {
        if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
            log_e("bSysTask:tpSysTask任务对象初始化失败");

        return false;
    }
    else if (tpSysTask == NULL)
    {
        if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
            log_e("bSysTask:tpSysTask任务对象创建失败");

        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 任务管理装载回调函数
 * 说明(备注)  : 根据系统当前状态或队列消息分发装载下一任务处理函数
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 装载目标任务函数与参数
 * 返回值      : bool: true 成功, false 失败
 ************************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *p_task)
{
	TaskItem_T t_item;

	if (p_task == NULL)
		return false;

	vQueue_ResetTaskState(p_task);

	if (tSysInfo.uInit.tFinish.bIF_SysTask == 0)
	{
		p_task->ucID = STI_INIT;
		p_task->usInParam = 0;
	}
	else if (bQueue_PopTask(p_task, &t_item)) //队列里面有任务
	{
		p_task->ucID = t_item.ucId;
		p_task->usInParam = t_item.usParam;
	}
	else
	{
		if (tSysInfo.eDevState == DS_WORK)
		{
			p_task->ucID = STI_WORK;
			p_task->usInParam = 0;
		}
		else if (tSysInfo.eDevState == DS_ERR)
		{
			p_task->ucID = STI_ERR;
			p_task->usInParam = 0;
		}
		else
		{
			p_task->ucID = STI_SHUT_DOWN;
			p_task->usInParam = 0;
		}
	}

	switch (p_task->ucID)
	{
		case STI_INIT:
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("\r\n bSysTask:----装载初始化任务---- \r\n");
			p_task->vp_func = v_sys_queue_task_init;
		}break;

		case STI_CLOSING:
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载关闭任务----\r\n");
			p_task->vp_func = v_sys_queue_task_closing;
		}break;

		case STI_SHUT_DOWN:
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载关闭完成任务----\r\n");
			p_task->vp_func = v_sys_queue_task_shut_down;
		}break;

		case STI_ERR:
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载错误任务----\r\n");
			p_task->vp_func = v_sys_queue_task_err;
		}break;

		case STI_RESET:
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载重置任务----\r\n");
			p_task->vp_func = v_sys_queue_task_reset;
		}break;

		case STI_BOOTING:
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载启动任务----\r\n");
			p_task->vp_func = v_sys_queue_task_booting;
		}break;

		case STI_WORK:
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载工作任务----\r\n");
			p_task->vp_func = v_sys_queue_task_work;
		}break;

		#if (boardUPDATE)
		case STI_UPDATE:
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载升级任务----\r\n");
			p_task->vp_func = v_sys_queue_task_update;
		}break;

		case STI_UPDATE_ERR:
		{
			p_task->vp_func = v_sys_queue_task_update_err;
		}break;
		#endif  //boardUPDATE

		#if (boardENG_MODE_EN)
		case STI_ENG:
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载工程模式任务----\r\n");
			p_task->vp_func = v_sys_queue_task_eng;
			bSys_SetDevState(DS_ENG_MODE, false);  //进入工程模式
		}break;
		#endif  //boardENG_MODE_EN

		case STI_NULL:
		default:
		{
			p_task->vp_func = NULL;
			p_task->usInParam = 0;
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----装载空任务----\r\n");
		}
		break;
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 添加任务返回回调函数
 * 说明(备注)  : 投递新任务成功时唤醒队列任务调度执行
 * 传入参数    : p_task: 队列任务指针, num: 状态码
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_add_task_return_func_cb(Task_T *p_task, u8 num)
{
	(void)p_task;

	switch (num)
	{
		//添加了任务
		case 2:
		{
			#if (boardUSE_OS)
			/* 任务句柄判空兜底:创建失败时放弃通知,防止空句柄触发断言死机 */
			if (tSysTaskHandler != NULL)
				xTaskNotifyGive(tSysTaskHandler);
			#endif  //boardUSE_OS
		}break;

		default:
		{
		}
		break;
	}
}
#endif  /* 1 */

