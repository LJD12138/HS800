/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统初始化任务                                                        *
*                                                                                                                *
******************************************************************************************************************/
#include "board_config.h"
#include "timer_task.h"
#include "gpio_init.h"
#include "app_info.h"

#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Flash/flash_iface.h"
#include "..\..\BOOT\Application\flash_allot_table.h"

#if(boardDISPLAY_EN)
#include "Middlewares/LVGL/lvgl.h"
#endif

#if(boardADC_EN)
#include "Adc/adc_task.h"
#endif  //boardADC_EN

#if(boardKEY_EN)
#include "Key/key_task.h"
#endif  //boardKEY_EN

#if(boardLED_EN)
#include "Led/led_task.h"
#endif  //boardLED_EN

#if(boardDC_EN)
#include "Dc/dc_task.h"
#endif  //boardDC_EN

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //boardUSB_EN

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif  //boardLIGHT_EN

#if(boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_hm_task.h"
#endif  //boardHEAT_MANAGE_EN

#if(boardUSE_OS)
#include "FreeRTOS.h"
#include "task.h"
#endif  //boardUSE_OS

#if(boardCM_BACKTRACE)
#include <cm_backtrace.h>
#endif  //boardCM_BACKTRACE

#if(boardEASY_LOGGER)
#include "elog_init.h"
#endif  //boardEASY_LOGGER

#if(boardEASY_FLASH)
#include "easyflash.h"
#endif  //boardEASY_FLASH

#if(boardWDGT_EN)
#include "fwdgt.h"
#endif  //boardWDGT_EN

#if(boardPRINT_IFACE)
#include "Print/print_iface.h"
#endif  //boardPRINT_IFACE

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  //boardBMS_EN

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#endif  //boardMPPT_EN

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#endif  //boardDCAC_EN

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE


void SysParamInit(void)
{
	#if(boardPRINT_IFACE)
	uPrint.tFlag.bImportant = 1;
	uPrint.tFlag.bAppInfo   = 1;
	uPrint.tFlag.bSysTask   = 1;
	uPrint.tFlag.bKeyTask   = 1;
	uPrint.tFlag.bBmsRecTask= 1;
	uPrint.tFlag.bBmsTask   = 1;
	uPrint.tFlag.bDcacTask  = 1;
	uPrint.tFlag.bDcacRecTask= 1;
	uPrint.tFlag.bMpptTask  = 1;
	uPrint.tFlag.bMpptRecTask= 1;
	uPrint.tFlag.bUsbTask   = 0;
	uPrint.tFlag.bDcTask    = 0;
	uPrint.tFlag.bDispTask   = 0;
	
	#if(boardUSE_OS_DEBUG_OUT)
	uPrint.tFlag.bFreeRTOS = 1;
	#endif  //boardUSE_OS_DEBUG_OUT
	
	#endif  //boardPRINT_IFACE
}


/***********************************************************************************************************************
-----函数功能    初始化任务的任务参数    第一步
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vBoard_SysInit(void)
{
	vGPIO_Init();
	
	bFlash_IfaceInit();
	
	#if boardWDGT_EN
	vFwdgt_Init();
	#endif  //boardWDGT_EN
	
	#if(boardPRINT_IFACE)
	vPrint_Init();
	#endif  //boardPRINT_IFACE
	
	#if boardSEGGER
	SEGGER_RTT_Init();      //初始化Seeger RTT 输出 
	SEGGER_SYSVIEW_Conf();  //SystemView 初始化
	#endif  //boardSEGGER
	
	#if(boardCM_BACKTRACE)
	cm_backtrace_init("APP", boardHARDWARE_VERSION, boardSOFTWARE_VERSION);
	#endif  //boardCM_BACKTRACE
	
	#if(boardEASY_LOGGER)
	vElog_Init();
	#endif  //boardEASY_LOGGER
	
	#if(boardEASY_FLASH)
	EfErrCode ret = easyflash_init();
	if(ret != EF_NO_ERR)
		printf("EasyFlash init fail, EfErrCode = %d.r\n", ret);
	#endif  //boardEASY_FLASH
	
	SysParamInit();        //初始化系统参数
}





/***********************************************************************************************************************
-----函数功能    初始化任务的任务    第二步
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vBoard_StartTask(void *pvParameters)
{
    #if(boardUSE_OS)
    taskENTER_CRITICAL();
	#endif  //boardUSE_OS
	
	vSys_TaskInit();            //系统总任务
	
	#if(boardPRINT_IFACE)
	if(bPrint_TaskInit() == false)
		uPrint.ulFlag = 0;	//关闭所有调试
	#endif

	#if(boardADC_EN)
	vAdc_TaskInit();             //ADC处理任务
	#endif  //boardADC_EN
	
	#if(boardDISPLAY_EN)
	bDisp_TaskInit();             //显示任务
	#endif  //boardDISPLAY_EN
	
	#if(boardBUZ_EN)
   	bBuz_TaskInit();            //蜂鸣器任务 1
	#endif  //boardBUZ_EN
	
	#if(boardKEY_EN)
   	vKey_TaskInit();             //按键任务
	#endif  //boardKEY_EN
	
	#if(boardLED_EN)
	vLed_TaskInit();             //指示灯任务 1
	#endif  //boardLED_EN
	
	#if(boardLIGHT_EN)
	vLight_TaskInit();           //照明灯任务 1
	#endif  //boardLIGHT_EN
	
	#if(boardHEAT_MANAGE_EN)
	bHM_TaskInit();             //风扇散热任务 1
	#endif  //boardHEAT_MANAGE_EN
	
	#if(boardUSB_EN)
	bUsb_TaskInit();             //USB任务
	#endif  //boardUSB_EN
	
	#if(boardDC_EN)
	vDc_TaskInit();              //DC任务
	#endif  //boardDC_EN
	
	#if(boardDCAC_EN)
	bDcac_TaskInit();           //逆变器发送任务
	bDcac_RecTaskInit();        //逆变器接收任务
	#endif  //boardDCAC_EN
	
	#if(boardBMS_EN)
	bBms_TaskInit();            //BMS发送任务
	bBms_RecTaskInit();         //BMS接收任务
	#endif  //boardBMS_EN
	
	#if(boardMPPT_EN)
	bMppt_TaskInit();           //MPPT发送任务
	bMppt_RecTaskInit();        //MPPT接收任务
	#endif  //boardMPPT_EN
	
	#if(boardWIFI_IFACE)
	vWifi_TaskInit();           //WiFi发送任务
	vWiFi_RecTaskInit();        //WiFi接收任务
	#endif  //boardWIFI_IFACE
	
	vTimer_TaskInit();           //系统计时

    #if(boardUSE_OS)
    vTaskDelete(NULL);   
    taskEXIT_CRITICAL();
	#endif  //boardUSE_OS
}


/*
1.执行一些低优先级的、后台的、需要连续执行的函数
2.测量系统的空闲时间：空闲任务能被执行就意味着所有的高优先级任务都停止了，所以测量空闲任
务占据的时间，就可以算出处理器占用率。
3.让系统进入省电模式：空闲任务能被执行就意味着没有重要的事情要做，当然可以进入省电模式
了。
4.钩子函数不可以调用会引起空闲任务阻塞的 API 函数（例如：vTaskDelay()、带有阻塞时间的队列和信号量函数）
*/
void vApplicationIdleHook( void )  //钩子函数
{
    
}

void vApplicationTickHook( void )
{
   /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */
	// 告诉lvgl已经过去1毫秒
	#if(boardDISPLAY_EN)
	lv_tick_inc(1);
	#endif  //boardDISPLAY_EN
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	(void)xTask;

	printf("\r\nStack overflow: %s\r\n", (pcTaskName != NULL) ? pcTaskName : "unknown");
	taskDISABLE_INTERRUPTS();
	for(;;)
	{
	}
}



