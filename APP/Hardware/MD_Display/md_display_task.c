/*****************************************************************************************************************
*                                                                                                                *
 *                                         Disp显示任务 - TFT+LVGL版本                                          *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_task.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "MD_Display/lv_port_disp.h"
#include "MD_Display/lv_port_timer.h"
#include "MD_Display/md_display_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Middlewares/LVGL/lvgl.h"

#include "app_info.h"

//****************************************************任务参数初始化**********************************************//
#if(boardUSE_OS)
#define			dispTASK_PRIO                   2       //任务优先级 
#define			dispTASK_STK_SIZE               256     //任务堆栈  实际字节数 *4
TaskHandle_t tDispTaskHandler = NULL; 
void vDisp_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
Disp_T tDisp; 
bool g_bDispPageDirty = true;   //页面脏标志

//****************************************************局部函数定义************************************************//
static void v_disp_param_init(void);
static void v_disp_lvgl_init(void);
static void v_disp_apply_mem_param(void);

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
	
	tDisp.eDevState = DS_INIT;
	tDisp.bSleepShow = true;    //待机强制打开亮屏
    v_disp_apply_mem_param();
}

static void v_disp_apply_mem_param(void)
{
    tDisp.usAutoOffTime = tAppMemParam.tDISP.usAutoOffTime;
    tDisp.usAutoOffCnt = tDisp.usAutoOffTime;
}

/***********************************************************************************************************************
 -----函数功能    LVGL初始化
 -----说明(备注)  TFT+LVGL版本简化初始化
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_disp_lvgl_init(void)
{
    vDisp_Init();
}

/***********************************************************************************************************************
 -----函数功能    Disp显示任务初始化
 -----说明(备注)  TFT+LVGL版本简化初始化
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
bool bDisp_TaskInit(void)
{
    v_disp_param_init();

    /* 初始化LVGL */
    v_disp_lvgl_init();

    #if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vDisp_Task,            // 任务函数
                (const char* )"DispTask",             // 任务名称
                (u16 ) dispTASK_STK_SIZE,             // 任务堆栈大小
                (void* )NULL,                         // 传递给任务函数的参数
                (UBaseType_t ) dispTASK_PRIO,         // 任务优先级
                (TaskHandle_t*)&tDispTaskHandler);    // 任务句柄
    #endif  //boardUSE_OS

    return true;
}

/***********************************************************************************************************************
 -----函数功能    设置显示设备运行状态
 -----说明(备注)  none
 -----传入参数    state: 设备状态
 -----输出参数    none
 -----返回值      true:操作成功   false:操作失败
 ************************************************************************************************************************/
bool bDisp_SetDevState(DevState_E state)
{
	if(tDisp.eDevState != state)
	{
		tDisp.eDevState = state;
		if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
			sMyPrint("bDispTask:显示任务状态变更 = %d\r\n", (int)state);
	}
	
	return true;
}

/***********************************************************************************************************************
 -----函数功能    tDisp显示任务
 -----说明(备注)  TFT+LVGL版本简化任务
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
        uint32_t wait_ms;

		/* 处理LVGL任务 */
		wait_ms = lv_timer_handler();
        if(wait_ms == LV_NO_TIMER_READY)
            wait_ms = 20U;
        else if(wait_ms == 0U)
            wait_ms = 1U;
		
		#if(boardUSE_OS)
		vTaskDelay(pdMS_TO_TICKS(wait_ms));
		#else
		v_disp_delay_ms((u16)wait_ms);
		#endif  //boardUSE_OS
	}
}

/***********************************************************************************************************************
 -----函数功能    显示开关
 -----说明(备注)  TFT+LVGL版本简化显示控制
 -----传入参数    type:类型   fore_en:强制打开
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
bool bDisp_Switch(SwitchType_E type, bool fore_en)
{
	switch(type)
	{
		case ST_ON:
			if(tDisp.bLight == false)
			{
				tDisp.bLight = true;
                vDisp_SetPower(true);
				if(tDisp.usAutoOffTime)
					tDisp.usAutoOffCnt = tDisp.usAutoOffTime;
				g_bDispPageDirty = true;
			}
			break;
		
		case ST_OFF:
			if(tDisp.bLight)
			{
				tDisp.bLight = false;
                vDisp_SetPower(false);
				if(tDisp.usAutoOffTime)
					tDisp.usAutoOffCnt = tDisp.usAutoOffTime;
				g_bDispPageDirty = true;
			}
			break;
		
		default:
			break;
	}
	
	return true;
}

/***********************************************************************************************************************
 -----函数功能    请求页面刷新
 -----说明(备注)  设置页面脏标志
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vDisp_RequestRefresh(void)
{
	g_bDispPageDirty = true;

	#if(boardUSE_OS)
	if(tDispTaskHandler != NULL)
		xTaskNotifyGive(tDispTaskHandler);
	#endif
}

/***********************************************************************************************************************
 -----函数功能    背光自动关闭计时
 -----说明(备注)  本函数由系统定时器或任务定时调用，用于：
				  1) 在系统处于工作态且屏幕处于亮屏时，递减自动息屏计数；
				  2) 当计数归零时自动关闭背光并记录日志；
				  3) 每次计时变更置页面脏标志以触发必要的刷新或状态更新。
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
			g_bDispPageDirty = true;
			if(tDisp.usAutoOffCnt == 0)
			{
				bDisp_Switch(ST_OFF, false);
				if(uPrint.tFlag.bDispTask|| uPrint.tFlag.bImportant)
					sMyPrint("Lcd_Task:倒计时结束,进入息屏 时间 = %dS\r\n",tDisp.usAutoOffTime);
			}
		}
	}
}

/***********************************************************************************************************************
-----函数功能    初始化显示记忆参数
-----说明(备注)  写入显示模块默认亮度和自动息屏时间
-----传入参数    p_disp_mem:显示记忆参数结构体指针
-----输出参数    none
-----返回值      true:设置成功 false:设置失败
************************************************************************************************************************/
bool bDisp_MemParamInit(DispMemParam_T* p_disp_mem)
{
	p_disp_mem->ucHighLightValue = boardDISP_HIGH_LIGHT_VALUE;
	p_disp_mem->ucLowLightValue = boardDISP_LOW_LIGHT_VALUE;
	p_disp_mem->usAutoOffTime = boardDISP_OFF_TIME;
	return true;
}

#if(boardLOW_POWER)
/***********************************************************************************************************************
-----函数功能    选择显示供电通路
-----说明(备注)  根据外部输入电源状态控制显示屏电池供电开关
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
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

/***********************************************************************************************************************
-----函数功能    进入显示低功耗
-----说明(备注)  挂起显示任务以降低低功耗模式下的运行消耗
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vLcd_EnterLowPower(void)
{
	vTaskSuspend(tDispTaskHandler);
}

/***********************************************************************************************************************
-----函数功能    退出显示低功耗
-----说明(备注)  恢复显示任务运行
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vLcd_ExitLowPower(void)
{
	vTaskResume(tDispTaskHandler);
}
#endif //boardLOW_POWER

#endif //boardDISPLAY_EN
