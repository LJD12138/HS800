/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task_eng.h"

#if(boardENG_MODE_EN)
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

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_queue_task.h"
#include "MD_Display/md_display_eng_mode.h"
#endif  //boardDISPLAY_EN

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#endif  //boardMPPT_EN

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  //boardDCAC_EN

#define     	sysTASK_ENG_CYCLE_TIME					sysTASK_CYCLE_TIME //任务时间

//****************************************************参数初始化**************************************************//
EngMode_T tEngMode;


/***********************************************************************************************************************
-----函数功能    工作
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_eng(Task_T *tp_task)
{
	switch(tp_task->ucStep)
	{
		case EMS_INIT:
			break;

		case EMS_SYS:
			/* 风扇/蜂鸣器控制由vEng_AdjustParam按键时直接执行, 此处无需持续控制 */
			break;

		/* EMS_LCD/BAT/MPPT/DCAC/USB/DC: 参数调整由vEng_AdjustParam直接处理, 此处无需逻辑 */
		default:
			break;
	}

	/* 超时检测: 60秒无操作自动关机(显示任务另有独立超时, 此为备份) */
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (60000/sysTASK_ENG_CYCLE_TIME))
	{
		cSys_Switch(SO_KEY, ST_OFF, false);
		cQueue_GotoStep(tp_task, STEP_END);
	}

	vTaskDelay(sysTASK_ENG_CYCLE_TIME);
}

void vEng_RefreshEngModeTime(void)
{
	tpSysTask->usTaskWaitCnt = 0;
	#if(boardDISPLAY_EN)
	if(tpDispTask != NULL)
		tpDispTask->usTaskWaitCnt = 0;
	#endif  //boardDISPLAY_EN
}


/***********************************************************************************************************************
 -----函数功能    调整工程模式记忆参数
 -----说明(备注)  按键任务上下文(只改后端tEngMode/tAppMemParam, 不调LVGL);
				  b_add=true增加/置1, false减少/置0; 同步ucEngModeItem+cEngModeState;
				  只读项(版本号)直接忽略; 风扇强制开关走vFan_ForceOpenFan
				  UI刷新由调用方(eng_mode_ui)设置bNeedRefresh触发
 -----传入参数    uc_tab: 参数Tab索引(0=SYS,1=LCD,2=BAT,3=MPPT,4=DCAC,5=USB,6=DC)
				  uc_item: Tab内参数索引
				  b_add: true=增加/置1, false=减少/置0
 -----输出参数    none
 -----返回值      none
************************************************************************************************************************/
void vEng_AdjustParam(uint8_t uc_tab, uint8_t uc_item, bool b_add)
{
	/* 同步 tEngMode 以便后台任务处理 */
	tEngMode.ucEngModeItem = uc_item;

	switch(uc_tab)
	{
		case 0: /* SYS */
		{
			if(uc_item == 0)
			{
				/* 版本号: 只读, 不调整 */
			}
			else if(uc_item == 1)
			{
				/* 风扇控制: 开关类型, 按键时直接执行 */
				#if(boardHEAT_MANAGE_EN)
				vFan_ForceOpenFan(b_add);
				#endif
				tEngMode.cEngModeState = b_add ? 1 : 0;
			}
			else if(uc_item == 6)
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

		#if(boardDISPLAY_EN)
		case 1: /* LCD */
		{
			vDisp_MemParamSet(b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif

		#if(boardBMS_EN)
		case 2: /* BAT */
		{
			vBms_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif

		#if(boardMPPT_EN)
		case 3: /* MPPT */
		{
			vMppt_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif

		#if(boardDCAC_EN)
		case 4: /* DCAC */
		{
			vDcac_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif

		#if(boardUSB_EN)
		case 5: /* USB */
		{
			vUsb_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif

		#if(boardDC_EN)
		case 6: /* DC */
		{
			vDc_MemParamSet(uc_item, b_add);
			tEngMode.cEngModeState = b_add ? 1 : -1;
		}break;
		#endif

		default:
			break;
	}
}

#endif



