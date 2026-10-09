/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_queue_task_booting.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB 队列任务: 启动实现文件
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
#include "Usb/usb_iface.h"
#include "Sys/sys_task.h"
#include "app_info.h"

#include "Print/print_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			usbTASK_BOOTING_CYCLE_TIME				100

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : USB 队列任务: 启动
 * 说明(备注)  : 设置状态为启动中并校验供电电压
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_usb_queue_task_booting(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		case 0:
		{
			bUsb_SetDevState(DS_BOOTING);

			if (cUsb_CheckInVolt() == 0)
			{
				cQueue_GotoStep(p_task, STEP_NEXT);
				return;
			}
		}
		break;

		case 1:
		{
			bUsb_SetDevState(DS_WORK);
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
	if (bQueue_IsTaskTimeoutMs(p_task, 10000))
	{
		bUsb_SetErrCode(UEC_BOOT_FAULT, true);

		if (uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_w("bUsbTask:启动中任务等待超时,步骤%d", p_task->ucStep);

		cQueue_GotoStep(p_task, STEP_END);
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_BOOTING_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

#endif  /* boardUSB_EN */
