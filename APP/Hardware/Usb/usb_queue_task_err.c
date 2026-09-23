/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_queue_task_err.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB 队列任务: 故障错误处理实现文件
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
#include "Usb/usb_iface.h"
#include "Usb/usb_prot_frame.h"
#include "Sys/sys_task.h"

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  /* boardPRINT_IFACE */

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			usbTASK_ERR_CYCLE_TIME					1000

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : USB 队列任务: 错误
 * 说明(备注)  : 设置错误状态并在错误自动清除后重新开机或关机
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_usb_queue_task_err(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		/* 初始化 */
		case 0:
		{
			bUsb_SetDevState(DS_ERR);

			if (tUsb.uErrCode.tCode.bBootFault ||
			    tUsb.uErrCode.tCode.bOT ||
			    tUsb.uErrCode.tCode.bPowerErr ||
			    tUsb.uErrCode.tCode.bIc1Lost ||
			    tUsb.uErrCode.tCode.bIc2Lost ||
			    tUsb.uErrCode.tCode.bBatUV)
				cQueue_GotoStep(p_task, STEP_NEXT);
			else
				cQueue_GotoStep(p_task, 2);

			return;
		}

		/* 等待恢复 */
		case 1:
		{
			/* 有任务, 退出 */
			if (lwrb_get_full(&p_task->tQueueBuff))
			{
				cQueue_GotoStep(p_task, STEP_END);
				return;
			}

			/* 错误清除, 重新开机 */
			if (tUsb.uErrCode.ucErrCode == 0)
				cUsb_Switch(ST_ON, true);
		}
		break;

		/* 等待关闭 */
		case 2:
		{
			/* 有任务, 退出 */
			if (lwrb_get_full(&p_task->tQueueBuff))
			{
				cQueue_GotoStep(p_task, STEP_END);
				return;
			}

			/* 错误清除, 关闭 */
			if (tUsb.uErrCode.ucErrCode == 0)
				cUsb_Switch(ST_OFF, true);
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_ERR_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

#endif  /* boardUSB_EN */
