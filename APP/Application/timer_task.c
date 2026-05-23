/*****************************************************************************************************************
*                                                                                                                *
 *                                         软件定时器                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "timer_task.h"
#include "gpio_init.h"

#include "Sys/sys_task.h"

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#endif

#if(boardDC_EN)
#include "Dc/dc_task.h"
#endif

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#endif

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_rec_task.h"
#endif

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#endif


//****************************************************任务初始化**************************************************//
#if(boardBMS_485_IFACE_EN)	
TimerHandle_t 	tBmsRxEnTimer = NULL;     //单次定时器,BMS的458发送延时切换
#endif

#if(boardMPPT_485_IFACE_EN)
TimerHandle_t 	tMpptRxEnTimer = NULL;     //单次定时器,MPPT的458发送延时切换
#endif

#if(boardDCAC_485_IFACE_EN)
TimerHandle_t 	tDcacRxEnTimer = NULL;     //单次定时器,BMS的458发送延时切换
#endif

#if(boardBMS_EN)
TimerHandle_t 	tWakeUpBmsTimer = NULL;   //单次定时器,BMS的唤醒使能延时关闭
#endif  //boardBMS_EN

TimerHandle_t 	tRepetTimer   = NULL;     //重复定时器  repetition

static void vTimer_SignalCallback( TimerHandle_t xTimer );
static void vTimer_RepetCallback( TimerHandle_t xTimer );


//****************************************************参数初始化**************************************************//

/***********************************************************************************************************************
-----函数功能    定时器任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTimer_TaskInit(void)
{			
	/*创建单次定时器*/
	#if(boardBMS_485_IFACE_EN)
    tBmsRxEnTimer = xTimerCreate("bms_exit_485_tx_timer",  	//软件定时器的名字  
                            2,       	                    //定时器周期(ms),单位时钟节拍
                            pdFALSE,                        //定时器模式，pdTRUE为周期定时器，pdFALSE为单次定时器
                            (void*)1,        	            //定时器的ID号=1
                            vTimer_SignalCallback); 	    //定时器回调函数
	#endif  //boardBMS_485_IFACE_EN
	
	#if(boardBMS_EN)
	tWakeUpBmsTimer = xTimerCreate("wake_up_bms_timer",  	//软件定时器的名字  
                            3000,       	                //定时器周期(ms),单位时钟节拍
                            pdFALSE,                        //定时器模式，pdTRUE为周期定时器，pdFALSE为单次定时器
                            (void*)2,        	            //定时器的ID号=1
                            vTimer_SignalCallback); 	    //定时器回调函数
	#endif  //boardBMS_EN
							
	#if(boardDCAC_485_IFACE_EN)
    tDcacRxEnTimer = xTimerCreate("acdc_exit_485_tx_timer", //软件定时器的名字  
                            4,       	                    //定时器周期(ms),单位时钟节拍
                            pdFALSE,                        //定时器模式，pdTRUE为周期定时器，pdFALSE为单次定时器
                            (void*)3,        	            //定时器的ID号=1
                            vTimer_SignalCallback); 	    //定时器回调函数
	#endif  //boardDCAC_485_IFACE_EN
	
	#if(boardMPPT_485_IFACE_EN)
    tMpptRxEnTimer = xTimerCreate("mppt_exit_485_tx_timer", //软件定时器的名字  
                            2,       	                    //定时器周期(ms),单位时钟节拍
                            pdFALSE,                        //定时器模式，pdTRUE为周期定时器，pdFALSE为单次定时器
                            (void*)4,        	            //定时器的ID号=1
                            vTimer_SignalCallback); 	    //定时器回调函数
	#endif
							
	/*创建重复定时器*/
    tRepetTimer = xTimerCreate("repet_timer",  	            //软件定时器的名字  
                            boardREPET_TIMER_CYCLE_TMIE,    //定时器周期(ms),单位时钟节拍
                            pdTRUE,                         //定时器模式，pdTRUE为周期定时器，pdFALSE为单次定时器
                            (void*)1,        	            //定时器的ID号=1
                            vTimer_RepetCallback); 	        //定时器回调函数
	
	//开始定时器
	#if(boardBMS_485_IFACE_EN)
	xTimerStart(tBmsRxEnTimer, 0);
    #endif  //boardBMS_485_IFACE_EN
							
	#if(boardMPPT_485_IFACE_EN)
	xTimerStart(tMpptRxEnTimer, 0);
    #endif  //boardMPPT_485_IFACE_EN
	
	#if(boardBMS_EN)
	xTimerStart(tWakeUpBmsTimer, 0);
	#endif  //boardBMS_EN
							
	#if(boardDCAC_485_IFACE_EN)
	xTimerStart(tDcacRxEnTimer, 0);
    #endif  //boardDCAC_485_IFACE_EN
							
	xTimerStart(tRepetTimer, 0);
}



/***********************************************************************************************************************
-----函数功能    单次定时器回调函数
-----说明(备注)  none
-----传入参数    xTimer:调用函数的句柄
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void vTimer_SignalCallback( TimerHandle_t xTimer )
{
	if(pvTimerGetTimerID(xTimer) == ((void *)1))
	{
		#if(boardBMS_485_IFACE_EN)
		vBms_485TransEnable(false);
		#endif
	}	
	else if(pvTimerGetTimerID(xTimer) == ((void *)2))
	{
		vGPIO_AssistBmsOpen(false);
	}
	else if(pvTimerGetTimerID(xTimer) == ((void *)3))
	{
		#if(boardDCAC_485_IFACE_EN)
		vDcac_485TransEnable(false);
		#endif  //boardDCAC_485_IFACE_EN
	}
	else if(pvTimerGetTimerID(xTimer) == ((void *)4))
	{
		#if(boardMPPT_485_IFACE_EN)
		vMppt_485TransEnable(false);
		#endif
	}		
}


/***********************************************************************************************************************
-----函数功能    重复定时器回调函数
-----说明(备注)  none
-----传入参数    xTimer:调用函数的句柄
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static vu8 timer_cnt = 0;
static void vTimer_RepetCallback( TimerHandle_t xTimer )
{
	#if(boardUPDATE)
	if(tSysInfo.eDevState ==DS_UPDATE_MODE)
	{
		vUpdate_TickTimer();
		return;
	}
	#endif  //boardUPDATE
	
	timer_cnt++;
	if(timer_cnt >= (1000/boardREPET_TIMER_CYCLE_TMIE)) //1S计时 
	{
		timer_cnt = 0;
		vSys_TickTimer();
		
		#if(boardDCAC_EN)
		vDcac_TickTimer();
		#endif
		
		#if(boardDISPLAY_EN)
		vDisp_TickTimer();
		#endif
		
		#if(boardUSB_EN)
		vUsb_TickTimer();
		#endif
		
		#if(boardDC_EN)
		vDc_TickTimer();
		#endif
		
		#if(boardWDGT_EN && boardPRINT_IFACE == 0)
		vFwdgt_Reload();
		#endif
		
		#if(boardENG_MODE_EN)
//		vEng_ExitEngModeCnt();
		#endif
		
		#if(boardUSE_OS_DEBUG_OUT)
		if(uPrint.tFlag.bFreeRTOS)
		{
			size_t num = xPortGetFreeHeapSize();	         //获取当前未分配的内存堆大小
			sMyPrint("bFreeRTOS:未分配的内存堆 = %d word\r\n",num);
			
			num = xPortGetMinimumEverFreeHeapSize();	 	//获取未分配的内存堆历史最小值
			sMyPrint("bFreeRTOS:未分配的内存堆最小值 = %d word\r\n",num);
			
			char InfoBuffer[1024] = {0};
			vTaskList((char *) &InfoBuffer);
			printf("\r\n任务名      任务状态  优先级  剩余栈  任务序号\r\n");
			printf("\r\n %s \r\n", InfoBuffer);
		}
		#endif  //boardUSE_OS_DEBUG_OUT
	}
	
	#if(boardBMS_EN && (!boardDEBUG))
	vBms_RecTickTimer();
	#endif
	
	#if(boardMPPT_EN && (!boardDEBUG))
	vMppt_RecTickTimer();
	#endif
	
	#if(boardDCAC_EN && (!boardDEBUG))
	vDcac_RecTickTimer();
	#endif
	
	#if(boardWIFI_IFACE && (!boardDEBUG))
	vWiFi_RecTickTimer();
	#endif
	
	#if(boardPRINT_IFACE && (!boardDEBUG))
	vPrint_RecTickTimer();
	#endif
}

#if(boardLOW_POWER)
//进入低功耗
void vCount_EnterLowPower(void)
{
	xTimerDelete(tSignalTimer,100);
	xTimerDelete(tRepetTimer,100);
}

//退出低功耗
void vCount_ExitLowPower(void)
{
	vTimer_TaskInit();
}
#endif

