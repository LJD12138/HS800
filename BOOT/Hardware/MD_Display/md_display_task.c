/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Display
 * File    : md_display_task.c
 * Date    : 2026-09-29
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示屏任务管理与状态控制实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_task.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_iface.h"
#include "Update/update_main.h"
#include "MD_Display/user_ui/update_mode_ui.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "boot_info.h"

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  /* boardUPDATE */


//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			dispTASK_PRIO							2		/* 任务优先级 */
#define			dispTASK_STK_SIZE						256		/* 任务堆栈(字) */
TaskHandle_t	tDispTaskHandler = NULL;						/* 任务句柄 */
void 			vDisp_Task(void *pvParameters);					/* 任务函数 */
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
Disp_T   tDisp; 

bool bDispIfaceInit = false;

//****************************************************Function Declaration******************************************************//
static void v_disp_param_init(void);


/***********************************************************************************************************************
 * 函数功能    : 参数初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_disp_param_init(void)
{
	memset(&tDisp, 0, sizeof(tDisp));
	
	tDisp.usAutoOffTime = boardDISP_OFF_TIME;
	tDisp.bSleepShow    = true;	/* 待机强制打开亮屏 */
}

/***********************************************************************************************************************
 * 函数功能    : Disp显示任务初始化
 * 说明(备注)  : 初始化参数、硬件GPIO引脚与显示驱动, 若使能OS则创建显示任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, 负数: 失败步骤码
 ************************************************************************************************************************/
s8 cDisp_TaskInit(void)
{
	v_disp_param_init();
	
	vDisp_IfaceInit();
	
	vDisp_Init();
	
	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t)vDisp_Task,				/* 任务函数 */
	                (const char *)"DispTask",				/* 任务名称 */
	                (uint16_t)dispTASK_STK_SIZE,			/* 任务堆栈大小 */
	                (void *)NULL,							/* 传递给任务函数的参数 */
	                (UBaseType_t)dispTASK_PRIO,				/* 任务优先级 */
	                (TaskHandle_t *)&tDispTaskHandler) != pdPASS)	/* 任务句柄 */
		return -1;
	#endif  /* boardUSE_OS */
	bDispIfaceInit = true;
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : tDisp显示任务
 * 说明(备注)  : 依据系统主任务状态路由不同界面的显示与刷新;
 *               支持非OS周期调用与FreeRTOS多任务运行
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_Task(void *pvParameters)
{
	#if (boardUSE_OS)
	for (;;)
	#endif  /* boardUSE_OS */
	{
		if(tpSysTask == NULL || bDispIfaceInit == false)
		{
			#if(boardUSE_OS)
			vTaskDelay(100);
			continue;
			#else
			return;
			#endif  /* boardUSE_OS */
		}
		
		switch (tpSysTask->ucID)
		{
			case STI_INIT:
			{
				bDisp_Switch(ST_OFF, false);
			}
			break;
			
			case STI_ENTER_APP:
			{
				if (tDisp.bLight)
					bDisp_Switch(ST_OFF, false);
			}
			break;
			
			case STI_ERR:
			case STI_RESET:
			{
				if (!tDisp.bLight)
					bDisp_Switch(ST_ON, false);
				
			}
			break;
			
			#if (boardUPDATE)
			case STI_UPDATE:
			{
				if (!tDisp.bLight)
					bDisp_Switch(ST_ON, true);
				
				vDisp_UpdateModeUi();
			}
			break;
			#endif  /* boardUPDATE */
			
			#if (boardDISPLAY_EN)
			case STI_DISPLAY:
			{
				if (!tDisp.bLight)
					bDisp_Switch(ST_ON, false);
				
			}
			break;
			#endif  /* boardDISPLAY_EN */
			
			#if (boardLOW_POWER)
			case STI_LOW_POWER:
			{
				if (tDisp.bLight)
					bDisp_Switch(ST_OFF, false);
			}
			break;
			#endif  /* boardLOW_POWER */
			
			default:
			{
			}
			break;
		}
		
		#if(boardUSE_OS)
		vTaskDelay(50);
		#endif  /* boardUSE_OS */
	}
	
}


/***********************************************************************************************************************
 * 函数功能    : 显示开关控制
 * 说明(备注)  : 控制显示背光电源与工作状态
 * 传入参数    : type: 开关类型, fore_en: 强制常亮(不息屏)
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bDisp_Switch(SwitchType_E type, bool fore_en)
{
	bool turn_on = false;

	switch (type)
	{
		case ST_ON:
		{
			turn_on = true;
		}
		break;
		
		case ST_OFF:
		{
			turn_on = false;
		}
		break;
		
		default:
		{
			turn_on = (tDisp.bLight == false);
		}
		break;
	}
	
	if (turn_on)
	{
		if (tDisp.bLight == false)
		{
			// 清屏，静态标题由 vDisp_UpdateModeUi 首次运行时绘制
			if(bDispIfaceInit == true)
			vDisp_DrawFillRect(0, 0, dispTFT_WIDTH, dispTFT_HEIGHT, 0x0000);

			vDisp_SetBacklight(true);
			tDisp.bLight = true;
		}
		
		/* 强制常亮或设置息屏时间 */
		if (fore_en == true)
		{
			tDisp.usAutoOffTime = 0;
			tDisp.usAutoOffCnt  = 0;
		}
		else
		{
			tDisp.usAutoOffTime = boardDISP_OFF_TIME;
			tDisp.usAutoOffCnt  = tDisp.usAutoOffTime;
		}
	}
	else
	{
		if (tDisp.bLight != false)
		{
			vDisp_SetBacklight(false);
			tDisp.bLight = false;
		}
		v_disp_param_init();
	}
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 背光自动关闭计时
 * 说明(备注)  : 1秒周期调用, 倒计时归零时自动熄灭屏幕
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_TickTimer(void) 
{
	/* 非亮屏幕状态 */
	if (tDisp.bLight == false)   
		return;
	
	/* 自动关闭背光倒计时 */
	if (tDisp.usAutoOffTime)
	{
		if (tDisp.usAutoOffCnt)
		{
			tDisp.usAutoOffCnt--;
			if (tDisp.usAutoOffCnt == 0)
			{
				if (uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
					sMyPrint("Lcd_Task:倒计时结束,进入息屏 时间 = %dS\r\n", tDisp.usAutoOffTime);
				bDisp_Switch(ST_OFF, false);
			}
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 初始化参数
 * 说明(备注)  : 填充显示默认记忆参数
 * 传入参数    : p_disp_mem: disp记忆参数结构体指针
 * 输出参数    : p_disp_mem: 回填参数
 * 返回值      : true: 设置成功, 反之失败
 ************************************************************************************************************************/
bool bDisp_MemParamInit(DispMemParam_T *p_disp_mem)
{
	if (p_disp_mem == NULL)
		return false;

	p_disp_mem->ucHighLightValue = boardDISP_HIGH_LIGHT_VALUE;
	p_disp_mem->ucLowLightValue  = boardDISP_LOW_LIGHT_VALUE;
	p_disp_mem->usAutoOffTime    = boardDISP_OFF_TIME;
	return true;
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 检查系统的输入电源:外接电池
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void v_dis_power_select(void)
{
	if (tDisp.bLight)
	{
		/* 电源有输入 */
		if (tAdcSamp.usSysInVolt >= boardBMS_MIN_VOLT)   
			Disp_EN_OFF();          /* 关闭显示屏的电池供电 */
		else
			Disp_EN_ON();           /* 打开显示屏的电池供电 */
	}
	else 
		Disp_EN_OFF();              /* 关闭显示屏的电池供电 */
}

/***********************************************************************************************************************
 * 函数功能    : 进入低功耗
 * 说明(备注)  : 挂起显示任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLcd_EnterLowPower(void)
{
	#if (boardUSE_OS)
	if (tDispTaskHandler != NULL)
		vTaskSuspend(tDispTaskHandler);
	#endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 退出低功耗
 * 说明(备注)  : 恢复显示任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLcd_ExitLowPower(void)
{
	#if (boardUSE_OS)
	if (tDispTaskHandler != NULL)
		vTaskResume(tDispTaskHandler);
	#endif  /* boardUSE_OS */
}
#endif  /* boardLOW_POWER */

#endif  /* boardDISPLAY_EN */

