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
//	s8 result = 1;
//	static bool b_flag = false;
	
	switch(tp_task->ucStep)
	{
		case EMS_INIT:
		{
			
		}break;
		
		case EMS_SYS:
		{
			if(tEngMode.ucEngModeItem == 1)  //控制风扇
			{
				if(tEngMode.cEngModeState == 1)
					vFan_ForceOpenFan(true);
				else if(tEngMode.cEngModeState == 0)
					vFan_ForceOpenFan(false);
			}
			else 
				vFan_ForceOpenFan(false);
			
			if(tEngMode.ucEngModeItem == 6)
			{
				if(tEngMode.cEngModeState == 1)
					tAppMemParam.tSYS.bBuzSwitchOff = 0;
				else if(tEngMode.cEngModeState == 0)
					tAppMemParam.tSYS.bBuzSwitchOff = 1;
			}
			
		}break;
		
		case EMS_LCD:
		{
			
		}break;
		
		case EMS_BAT:
		{
			
		}break;
		
		case EMS_MPPT:
		{
			
		}break;
		
		case EMS_DCAC:
		{
			
		}break;
		
		case EMS_ADC:
		{
			
		}break;
		
		case EMS_USB:
		{
			
		}break;
		
		case EMS_DC:
		{
			
		}break;
		
		case EMS_LIGHT:
		{
			
		}break;

		case EMS_SET:
		{
			if(tEngMode.ucEngModeItem == 0)  //保存
			{
				if(tEngMode.cEngModeState == 1)
				{
//					if(bApp_MemParamUpdate(NULL,NULL,false) == true)
						goto shut_down;
				}
			}
			else if(tEngMode.ucEngModeItem == 1)  //重置
			{
				if(tEngMode.cEngModeState == 1)
				{
//					if(bApp_SysInfoInit(true) == true)
						goto shut_down;
				}
			}
			else if(tEngMode.ucEngModeItem == 2)  //升级
			{
				if(tEngMode.cEngModeState == 1)
				{
					vApp_JumpToBoot(mainUPDATE_FLAG);
				}
			}
		}
		break;
		
		default:
		case EMS_FINISH:
		{
			if(tEngMode.cEngModeState == 1)
			{
				shut_down:
				cSys_Switch(SO_KEY, ST_OFF, false);
				cQueue_GotoStep(tpSysTask, STEP_END);  //结束
			}
		}break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (60000/sysTASK_ENG_CYCLE_TIME))  //等待超时
	{
		cSys_Switch(SO_KEY, ST_OFF, false);
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	vTaskDelay(sysTASK_ENG_CYCLE_TIME);
}

void vEng_RefreshEngModeTime(void)
{
	tpSysTask->usTaskWaitCnt = 0;
}

#endif



