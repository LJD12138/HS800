/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_eng.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统工程模式队列任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task_eng.h"

#if (boardENG_MODE_EN)
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Adc/adc_task.h"
#include "Buz/buz_task.h"
#include "Usb/usb_task.h"
#include "Dc/dc_task.h"
#include "MD_Light/md_light_task.h"
#include "MD_HeatManage/md_hm_task.h"
#include "..\..\BOOT\Application\flash_allot_table.h"

#include "app_info.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  /* boardBMS_EN */

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#endif  /* boardMPPT_EN */

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  /* boardDCAC_EN */

//****************************************************Macros********************************************************************//
#define			sysTASK_ENG_CYCLE_TIME					sysTASK_CYCLE_TIME	/* 任务调度节拍周期 */

//****************************************************Parameter Initialization**************************************************//
EngMode_T tEngMode;

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 工程模式关机退出
 * 说明(备注)  : 复位强制风扇、触发系统关机并结束队列任务
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_sys_eng_shutdown(Task_T *p_task)
{
	#if (boardHEAT_MANAGE_EN)
	vFan_ForceOpenFan(false);
	#endif  /* boardHEAT_MANAGE_EN */

	cSys_Switch(SO_KEY, ST_OFF, false);
	cQueue_GotoStep(p_task, STEP_END);
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式系统队列任务执行函数
 * 说明(备注)  : 周期维护超时退出与工程步骤状态机推进
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void v_sys_queue_task_eng(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		case EMS_INIT:
		{
		}
		break;

		case EMS_SYS:
		{
			/* 风扇/蜂鸣器控制由 vEng_AdjustParam 动态执行 */
		}
		break;

		case EMS_LCD:
		case EMS_BAT:
		case EMS_DCAC:
		case EMS_MPPT:
		case EMS_USB:
		case EMS_DC:
		case EMS_ADC:
		{
		}
		break;

		case EMS_SET:
		case EMS_FINISH:
		{
			if (tEngMode.cEngModeState == 1)
			{
				v_sys_eng_shutdown(p_task);
				return;
			}
		}
		break;

		default:
		{
		}
		break;
	}

	/* 超时检测: 60秒无操作自动关机(超时保护统一由系统任务管理) */
	if (bQueue_IsTaskTimeoutMs(p_task, 60 * 1000))
	{
		v_sys_eng_shutdown(p_task);
		return;
	}

	ulTaskNotifyTake(pdTRUE, sysTASK_ENG_CYCLE_TIME);   /* 周期节拍；新任务投递立即唤醒抢占 */
}

/***********************************************************************************************************************
 * 函数功能    : 刷新工程模式等待超时时间
 * 说明(备注)  : 重置系统主任务的无操作超时时间戳(超时保护统一由系统任务管理)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vEng_RefreshEngModeTime(void)
{
	vQueue_RefreshTaskTick(tpSysTask);
}


/***********************************************************************************************************************
 * 函数功能    : 调整工程模式记忆参数
 * 说明(备注)  : 按键任务上下文(只改后端 tEngMode / tAppMemParam, 不调 LVGL); b_add=true 增加/置1, false 减少/置0;
 *               同步 ucEngModeItem + cEngModeState; 只读项(版本号)直接忽略; 风扇强制开关走 vFan_ForceOpenFan; UI 刷新由调用方设置标记触发
 * 传入参数    : uc_tab: 参数 Tab 索引(0=SYS, 1=LCD, 2=BAT, 3=MPPT, 4=DCAC, 5=USB, 6=DC); uc_item: Tab 内参数索引;
 *               b_add: true=增加/置1, false=减少/置0
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vEng_AdjustParam(uint8_t uc_tab, uint8_t uc_item, bool b_add)
{
	/* 同步 tEngMode 以便后台任务处理 */
	tEngMode.ucEngModeItem = uc_item;

	switch (uc_tab)
	{
		case 0: /* SYS */
		{
			if (uc_item == 0)
			{
				/* 版本号: 只读, 不调整 */
			}
			else if (uc_item == 1)
			{
				/* 风扇控制: 开关类型, 按键时直接执行 */
				#if (boardHEAT_MANAGE_EN)
				vFan_ForceOpenFan(b_add);
				#endif  /* boardHEAT_MANAGE_EN */
				tEngMode.cEngModeState = b_add ? 1 : 0;
			}
			else if (uc_item == 6)
			{
				/* 蜂鸣器开关: 按键时直接执行 */
				tAppMemParam.tSYS.bBuzSwitchOff = b_add ? 0 : 1;
				tEngMode.cEngModeState = b_add ? 1 : 0;
			}
			else
			{
				/* 可调参数项 2~5 */
				vSys_MemParamSet(uc_item, b_add);
				tEngMode.cEngModeState = b_add ? 1 : -1;
			}
		}break;

		#if (boardDISPLAY_EN)
		case 1: /* LCD */
		{
			vDisp_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif  /* boardDISPLAY_EN */

		#if (boardBMS_EN)
		case 2: /* BAT */
		{
			vBms_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif  /* boardBMS_EN */

		#if (boardMPPT_EN)
		case 3: /* MPPT */
		{
			vMppt_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif  /* boardMPPT_EN */

		#if (boardDCAC_EN)
		case 4: /* DCAC */
		{
			vDcac_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif  /* boardDCAC_EN */

		#if (boardUSB_EN)
		case 5: /* USB */
		{
			vUsb_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif  /* boardUSB_EN */

		#if (boardDC_EN)
		case 6: /* DC */
		{
			vDc_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif  /* boardDC_EN */

		default:
		{
		}
		break;
	}
}

#endif  /* boardENG_MODE_EN */
