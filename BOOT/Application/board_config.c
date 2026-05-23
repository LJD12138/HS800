/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统初始化任务                                                        *
*                                                                                                                *
******************************************************************************************************************/
#include "board_config.h"
#include "timer_task.h"
#include "gpio_init.h"
#include "boot_info.h"

#include "Sys/sys_task.h"
#include "Led/led_task.h"
#include "Flash/flash_iface.h"

#if(boardADC_EN)
#include "Adc/adc_task.h"
#endif  //boardADC_EN

#if(boardKEY_EN)
#include "Key/key_task.h"
#endif  //boardKEY_EN

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
#include "Print/print_task.h"
#include "Print/print_iface.h"
#endif  //boardPRINT_IFACE

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif //boardBMS_EN

void SysParamInit(void)
{
	#if(boardWDGT_EN)
	vFwdgt_Init();
	#endif
	
	#if(boardPRINT_IFACE)
	uPrint.tFlag.bImportant = 1;
	uPrint.tFlag.bBootInfo  = 1;
	uPrint.tFlag.bSysTask   = 1;
	uPrint.tFlag.bKeyTask   = 1;
	uPrint.tFlag.bAdcTask   = 0;
	uPrint.tFlag.bOperFlash = 0;
	#endif
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
	
	#if(boardDISPLAY_EN)
	#endif  //boardDISPLAY_EN
	
	bFlash_IfaceInit();
	
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
	
	#if(boardUPDATE)
	bUpdate_Init();
	#endif
	
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
	vAdc_TaskInit();			//ADC处理任务
	#endif  //boardADC_EN

	#if(boardKEY_EN)
    vKey_TaskInit();			//按键任务
	#endif  //boardKEY_EN

	#if(boardLED_EN)
	vLed_TaskInit();			//指示灯任务
    #endif  //boardLED_EN

	#if(boardBMS_EN)
	bBms_TaskInit();         //主控发送任务
	bBms_RecTaskInit();      //主控接收任务
	#endif
	
	#if(boardDISPLAY_EN)
	bDisp_TaskInit();//显示任务初始化
	#endif  //boardDISPLAY_EN
	
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







