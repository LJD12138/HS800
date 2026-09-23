/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_queue_task_closing.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB 队列任务: 关闭实现文件
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
#include "Usb/usb_prot_frame.h"
#include "Sys/sys_task.h"
#include "app_info.h"

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  /* boardPRINT_IFACE */

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			usbTASK_CLOSING_CYCLE_TIME				100

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : USB 队列任务: 关闭
 * 说明(备注)  : 执行关闭流程并切入关机完成状态
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_usb_queue_task_closing(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		case 0:
		{
			bUsb_SetDevState(DS_CLOSING);
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			bUsb_SetDevState(DS_SHUT_DOWN);
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	/* 等待超时 */
	p_task->usTaskWaitCnt++;
	if (p_task->usTaskWaitCnt > (5000 / usbTASK_CLOSING_CYCLE_TIME))
	{
		bUsb_SetErrCode(UEC_COLSE_FAULT, true);

		if (uPrint.tFlag.bUsbTask)
			log_w("bUsbTask:关闭任务等待超时,步骤%d", p_task->ucStep);

		cQueue_GotoStep(p_task, STEP_END);
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_CLOSING_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

#endif  /* boardUSB_EN */
