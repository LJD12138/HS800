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


//****************************************************任务参数初始化**********************************************//
#if(boardUSE_OS)
#define			dispTASK_PRIO                   2       //任务优先级 
#define			dispTASK_STK_SIZE               256     //任务堆栈  实际字节数 *4
TaskHandle_t	tDispTaskHandler = NULL; 
void vDisp_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
Disp_T   tDisp; 

//****************************************************局部函数定义************************************************//
static void v_disp_update(void);

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
	
	Display_ClearData();   //Buff清零,避免残留
	
	tDisp.usAutoOffTime = boardDISP_OFF_TIME;
	tDisp.bSleepShow =true;//待机强制打开亮屏
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
	
	HT1621_IfaceInit();
	
	#if(boardUSE_OS)
	xTaskCreate((TaskFunction_t )vDisp_Task,			//任务函数
                (const char* )"DispTask",				//任务名称
                (u16 ) dispTASK_STK_SIZE,				//任务堆栈大小
                (void* )NULL,							//传递给任务函数的参数
                (UBaseType_t ) dispTASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tDispTaskHandler);      //任务句柄
	#endif  //boardUSE_OS
	
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
		if(tpSysTask == NULL)
			return;
		
		if(tpSysTask->ucID != STI_UPDATE)
		{
			bDisp_Switch(ST_OFF, false);
			return;
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
				v_disp_update();
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
	}
	
}


/***********************************************************************************************************************
-----函数功能    参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
__STATIC_INLINE void v_disp_update(void)
{
	static vu16 us_delay_cnt = 0;
	us_delay_cnt++;
	if(us_delay_cnt >= 5)
	{
		us_delay_cnt = 0;
		
		Display_IconUpdate();
		

		
		Display_ShowErrCode(tBootMemParam.tParam.eAppState);
		Display_UpdateState(1, tUpdate.eProtoType, 0);
		Display_UpdateProgress(tUpdate.usRecFrameCnt, tUpdate.usTotalFrmValue);
		Display_UpdateTime(tUpdate.tpProtoRx->usLostOverTimeCnt/100);
		Display_UpdateAnimation();
		
		Display_RefreshData();            //发送数据
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
	Display_ClearData();   //Buff清零,避免残留
	
	switch(type)
	{
		case ST_ON:
			goto LoopOn;
		
		case ST_OFF:
			goto LoopOff;
		
		default:
		{
			if(tDisp.bLight == false)
			{
				LoopOn:
				dispLIGHT_POWER_ON();
				tDisp.bLight = true;
				
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
				dispLIGHT_POWER_OFF();
				v_disp_param_init();
				
				if(tDisp.bLight != false)
					Display_RefreshData(); //发送数据
				
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
				// v_disp_shut_down();
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
-----函数功能    检查系统的输入电源:外接电池
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void v_dis_power_select( void )
{
	if(tDisp.bLight)
	{
		/*************************************电源有输入*********************************************************/
		if( tAdcSamp.usBMS_Vin >= boardBMS_MIN_VOLT )   
		{
			Disp_EN_OFF();          //关闭显示屏的电池供电
		}
		/*************************************没有电源输入*******************************************************/
		else
		{
			Disp_EN_ON();          //打开显示屏的电池供电
		}	
	}
	else 
	{
		Disp_EN_OFF();          //关闭显示屏的电池供电
	}
}


/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vLcd_EnterLowPower(void)
{
//	vLcd_IoEnterLowPower();
//	bAtti_EnterLowPower();
//	vExRTC_EnterLowPower();
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
//	vExRTC_ExitLowPower();
	vTaskResume(tDispTaskHandler);
}
#endif //boardLOW_POWER

#endif //boardDISPLAY_EN



