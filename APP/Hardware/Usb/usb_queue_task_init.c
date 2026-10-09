/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_queue_task_init.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB 队列任务: 初始化实现文件
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

#include "Print/print_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			usbTASK_INIT_CYCLE_TIME					100

//****************************************************Function Declaration******************************************************//
static s8 c_usb_info_init(void);


/***********************************************************************************************************************
 * 函数功能    : USB 队列任务: 初始化
 * 说明(备注)  : 等待获取 APP 信息并读取 USB 记忆参数
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_usb_queue_task_init(Task_T *p_task)
{
	s8 c_ret = 0;

	switch (p_task->ucStep)
	{
		case 0:
		{
			bUsb_SetDevState(DS_INIT);

			/* 等待获取 APP 信息 */
			if (tSysInfo.uInit.tFinish.bIF_AppInfo == false)
				break;

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			static bool s_b_ret = true;

			c_ret = c_usb_info_init();
			if (c_ret > 0)
			{
				if ((uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant) && s_b_ret == false)
					log_w("bUsbTask:tUSB获取错误清除");

				s_b_ret = true;
			}
			else
			{
				if ((uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant) && s_b_ret == true)
				{
					log_w("bUsbTask:tUSB初始化失败 代码%d", c_ret);
					s_b_ret = false;
				}
				break;
			}

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 2:
		{
			tSysInfo.uInit.tFinish.bIF_UsbTask = true;
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
	if (bQueue_IsTaskTimeoutMs(p_task, 3000))
	{
		if (uPrint.tFlag.bUsbTask)
			log_w("bUsbTask:初始化任务等待超时");

		cQueue_GotoStep(p_task, STEP_END);
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_INIT_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 获取并更新 USB 记忆参数
 * 说明(备注)  : 获取或重新初始化 Flash 存储参数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 小于0: 失败; 0: 未完成; 大于0: 完成
 ************************************************************************************************************************/
static s8 c_usb_info_init(void)
{
	s8 ret                = 0;
	const char *p_obj_str = tUsbMemParamStr;
	static bool s_b_ret   = true;

	/* 已经初始化 */
	if (tSysInfo.uInit.tFinish.bIF_SysInit == true)
	{
		ret = cApp_GetMemParam(p_obj_str);
		if (ret > 0)
			return 1;

		if ((uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant) && s_b_ret == true)
		{
			log_e("bUsbTask:当前系统已经初始化完成,但是tUSB读取依旧为空,准备重置");
			s_b_ret = false;
		}
	}

	/* 重新初始化 */
	ret = cApp_MemParamInit(p_obj_str);
	if (ret <= 0)
		return -1;

	ret = cApp_UpdateMemParam(p_obj_str);
	if (ret <= 0)
		return -2;

	s_b_ret = true;
	return 2;
}

#endif  /* boardUSB_EN */
