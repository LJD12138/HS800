/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_queue_task_work.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB 队列任务: 工作实现文件
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
#include "filtration.h"

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  /* boardPRINT_IFACE */

//****************************************************Macros********************************************************************//
#define			usbTASK_WORK_CYCLE_TIME					100

/* USB 电压滤波器 */
#define			adcUSB_PWR_FILTER_BUFF_SIZE				4

//****************************************************Parameter Initialization**************************************************//
s32 us_usb_total_out_pwr = 0;
s16 s_max_temp           = 0;

//****************************************************Parameter Initialization**************************************************//
static s32             s_usa_adc_usb_pwr_buff[adcUSB_PWR_FILTER_BUFF_SIZE];
static FilterHandler_T s_t_adc_usb_pwr_filter_mad_avg = {s_usa_adc_usb_pwr_buff, adcUSB_PWR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : USB 队列任务: 工作
 * 说明(备注)  : 轮询读取快充 IC 参数并计算滤波输出功率
 * 传入参数    : p_task: 队列任务控制块
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_usb_queue_task_work(Task_T *p_task)
{
	/* 有新任务投递, 让出执行 */
	if (lwrb_get_full(&p_task->tQueueBuff))
	{
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	switch (p_task->ucStep)
	{
		case 0:
		{
			us_usb_total_out_pwr = 0;
			s_max_temp           = 25;
			if (tUsb.uErrCode.tCode.bIc1Lost == false)
				c_usb_cs_get_ic_param(&tUSB_IC1_I2C);
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			if (tUsb.uErrCode.tCode.bIc2Lost == false)
				c_usb_cs_get_ic_param(&tUSB_IC2_I2C);
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 2:
		{
			tUsb.usOutPwr = lFilter_MadianAverage(&s_t_adc_usb_pwr_filter_mad_avg, &us_usb_total_out_pwr);

			vTaskDelay(400);
			cQueue_GotoStep(p_task, 0);
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_WORK_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

#endif  /* boardUSB_EN */

