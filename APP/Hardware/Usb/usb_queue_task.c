/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_queue_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB 任务队列管理与调度实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Usb/usb_queue_task.h"

#if (boardUSB_EN)
#include "Usb/usb_task.h"
#include "Sys/sys_task.h"

#include "Print/print_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Task_T *tpUsbTask = NULL;                                          /* USB 队列任务对象 */

//****************************************************Function Declaration******************************************************//
static bool b_task_manage_func_cb(Task_T *p_task);
static void v_add_task_return_func_cb(Task_T *p_task, uint8_t uc_num);
static void v_usb_queue_task_shut_down(Task_T *p_task);

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : USB 队列初始化
 * 说明(备注)  : 初始化 USB 任务队列，队列大小为 8
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bUsb_QueueInit(void)
{
	s8 c_result = 1;

	c_result = cQueue_TaskInit(&tpUsbTask, 8, 0, b_task_manage_func_cb, v_add_task_return_func_cb);
	if (c_result <= 0)
	{
		if (uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_e("bUsbTask:tpUsbTask任务对象初始化失败,代码%d", c_result);

		return false;
	}
	else if (tpUsbTask == NULL)
	{
		if (uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_e("bUsbTask:tpUsbTask任务对象创建失败");

		return false;
	}

	return true;
}


/***********************************************************************************************************************
 * 函数功能    : USB 任务管理回调函数
 * 说明(备注)  : 根据当前工作状态与队列事件动态装载对应的子任务函数
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

	if (tSysInfo.uInit.tFinish.bIF_UsbTask == 0)
	{
		p_task->ucID      = UTI_INIT;
		p_task->usInParam = 0;
	}
	else if (bQueue_PopTask(p_task, &t_item))
	{
		p_task->ucID      = t_item.ucId;
		p_task->usInParam = t_item.usParam;
	}
	else if (tUsb.eDevState == DS_WORK)
	{
		p_task->ucID      = UTI_WORK;
		p_task->usInParam = 0;
	}
	else
	{
		p_task->ucID      = UTI_SHUT_DOWN;
		p_task->usInParam = 0;
	}

	switch (p_task->ucID)
	{
		case UTI_INIT:
		{
			p_task->vp_func = v_usb_queue_task_init;
			if (uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载初始化任务----\r\n");
		}
		break;

		case UTI_CLOSING:
		{
			p_task->vp_func = v_usb_queue_task_closing;
			if (uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载关闭任务----\r\n");
		}
		break;

		case UTI_SHUT_DOWN:
		{
			p_task->vp_func = v_usb_queue_task_shut_down;
			if (uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载关闭完成任务----\r\n");
		}
		break;

		case UTI_ERR:
		{
			p_task->vp_func = v_usb_queue_task_err;
			if (uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载错误任务----\r\n");
		}
		break;

		case UTI_BOOTING:
		{
			if (uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载启动任务----\r\n");
			p_task->vp_func = v_usb_queue_task_booting;
		}
		break;

		case UTI_WORK:
		{
			if (uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载工作任务----\r\n");
			p_task->vp_func = v_usb_queue_task_work;
		}
		break;

		case UTI_NULL:
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
		case 2:                                                         /* 添加了任务 */
		{
			#if (boardUSE_OS)
			xTaskNotifyGive(tUsbTaskHandler);
			#endif  /* boardUSE_OS */
		}
		break;

		default:
		{
		}
		break;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 队列任务: 关机完成
 * 说明(备注)  : 重置 USB 结构体，配置状态并阻塞等待新命令唤醒
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_usb_queue_task_shut_down(Task_T *p_task)
{
	memset((u8*)&tUsb, 0, sizeof(tUsb));
	bUsb_SetDevState(DS_SHUT_DOWN);

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	#endif  /* boardUSE_OS */

	cQueue_GotoStep(p_task, STEP_END);
}

#endif  /* boardUSB_EN */
