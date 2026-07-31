/*****************************************************************************************************************
*                                                                                                                *
*                                         Disp显示任务                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Display/md_display_task.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_iface.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
#include "Print/print_task.h"

#include "boot_info.h"
#include "Update/update_main.h"
#include "MD_Display/user_ui/update_mode_ui.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

//****************************************************任务参数初始化**********************************************//
#if(boardUSE_OS)
#define			dispTASK_PRIO                   2       //任务优先级 
#define			dispTASK_STK_SIZE               256     //任务堆栈  实际字节数 *4
TaskHandle_t	tDispTaskHandler = NULL; 
void vDisp_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
Disp_T   tDisp;

bool bDispIfaceInit = false;

/***********************************************************************************************************************
-----函数功能    参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_param_init(void)
{
	memset(&tDisp, 0, sizeof(tDisp));
	
	tDisp.usAutoOffTime = boardDISP_OFF_TIME;
	tDisp.bSleepShow = true;//待机强制打开亮屏
}

/***********************************************************************************************************************
-----函数功能    Disp显示任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bDisp_TaskInit(void)
{
	v_disp_param_init();
	
	vDisp_IfaceInit();
	vDisp_Init();
	
	#if(boardUSE_OS)
	xTaskCreate((TaskFunction_t )vDisp_Task,			//任务函数
                (const char* )"DispTask",				//任务名称
                (u16 ) dispTASK_STK_SIZE,				//任务堆栈大小
                (void* )NULL,							//传递给任务函数的参数
                (UBaseType_t ) dispTASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tDispTaskHandler);      //任务句柄
	#endif  //boardUSE_OS
	bDispIfaceInit = true;
	return true;
}

/***********************************************************************************************************************
-----函数功能    tDisp显示任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_Task(void *pvParameters)
{
	#if(boardUSE_OS)
	for(;;)
	#endif  //boardUSE_OS
	{
		if(tpSysTask == NULL || bDispIfaceInit == false)
		{
			#if(boardUSE_OS)
			vTaskDelay(100);
			continue;
			#else
			return;
			#endif
		}
		
		if(tpSysTask->ucID != STI_UPDATE)
		{
			bDisp_Switch(ST_OFF, false);
			#if(boardUSE_OS)
			vTaskDelay(100);
			continue;
			#else
			return;
			#endif
		}
			
		bDisp_Switch(ST_ON, true);
		
		switch (tpSysTask->ucID)
		{
			case STI_INIT:
			{
				
			}
			break;
			
			case STI_ENTER_APP:
			{
				
			}
			break;
			
			case STI_ERR:
			case STI_RESET:
			{
			   
			}
			break;
			
			#if(boardUPDATE)
			case STI_UPDATE:
			{
				vDisp_UpdateModeUi();
			}
			break;
			#endif
			
			#if(boardDISPLAY_EN)
			case STI_DISPLAY:
			{
				
			}
			break;
			#endif
			
			#if(boardLOW_POWER)
			case STI_LOW_POWER:
			{  
				
			}
			break;
			#endif
			
			default:
				break;
		}
		
		#if(boardUSE_OS)
		vTaskDelay(50);
		#endif
	}
	
}



/***********************************************************************************************************************
-----函数功能    显示开关
-----说明(备注)  none
-----传入参数    type:类型   fore_en:强制打开
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bDisp_Switch(SwitchType_E type, bool fore_en)
{
	switch(type)
	{
		case ST_ON:
			if(tDisp.bLight == true)
			{
				if(fore_en == true)
					tDisp.usAutoOffTime = 0;
				
				if(tDisp.usAutoOffTime)
					tDisp.usAutoOffCnt = tDisp.usAutoOffTime;
				
				return true;
			}
			goto LoopOn;
		
		case ST_OFF:
			if(tDisp.bLight == false)
			{
				return true;
			}
			goto LoopOff;
		
		default:
		{
			if(tDisp.bLight == false)
			{
				LoopOn:
				vDisp_TftSetBacklight(true);
				tDisp.bLight = true;
				
				// 清屏，静态标题由 vDisp_UpdateModeUi 首次运行时绘制
				if(bDispIfaceInit == true)
					vDisp_DrawFillRect(0, 0, dispTFT_WIDTH, dispTFT_HEIGHT, 0x0000);
				
				//关闭息屏
				if(fore_en == true)
					tDisp.usAutoOffTime = 0;
				
				//更新显示时间
				if(tDisp.usAutoOffTime)
					tDisp.usAutoOffCnt =  tDisp.usAutoOffTime;
			}
			else 
			{
				LoopOff:
				vDisp_TftSetBacklight(false);
				v_disp_param_init();
				tDisp.bLight = false;
			}
		}
		break;
	}
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    背光自动关闭计时
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TickTimer(void) 
{
	//非工作状态下退出
	if(tSysInfo.eDevState != DS_WORK) 
		return;
	
	//非亮屏幕状态
	if(tDisp.bLight == false)   
		return;
	
	//-----自动关闭背光--------------------------------------   
	if(tDisp.usAutoOffTime)
	{
		if(tDisp.usAutoOffCnt)
		{
			tDisp.usAutoOffCnt--;
			if(tDisp.usAutoOffCnt == 0)
			{
				if(uPrint.tFlag.bDispTask|| uPrint.tFlag.bImportant)
					sMyPrint("Lcd_Task:倒计时结束,进入息屏 时间 = %dS\r\n",tDisp.usAutoOffTime);
			}
		}
	}
}

/*****************************************************************************************************************
-----函数功能    初始化参数
-----说明(备注)  none
-----传入参数    p_disp_mem : disp记忆参数结构体
-----输出参数    none
-----返回值      true:设置成功  反之失败
*****************************************************************************************************************/
bool bDisp_MemParamInit(DispMemParam_T* p_disp_mem)
{
	p_disp_mem->ucHighLightValue = boardDISP_HIGH_LIGHT_VALUE;
	p_disp_mem->ucLowLightValue = boardDISP_LOW_LIGHT_VALUE;
	p_disp_mem->usAutoOffTime = boardDISP_OFF_TIME;
	return true;
}

#if(boardLOW_POWER)
/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vLcd_EnterLowPower(void)
{
	vTaskSuspend(tDispTaskHandler);
}

/*****************************************************************************************************************
-----函数功能    退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vLcd_ExitLowPower(void)
{
	vTaskResume(tDispTaskHandler);
}
#endif //boardLOW_POWER

#endif //boardDISPLAY_EN



