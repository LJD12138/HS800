/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务的队列函数                                                  *
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Adc/adc_task.h"
#include "Led/led_iface.h"

#if(boardPRINT_IFACE)
#include "Print/print_iface.h"
#endif  //boardPRINT_IFACE

#if(boardBMS_EN)
#include "MD_Bms/md_bms_iface.h"
#endif //boardBMS_EN

#if(boardUPDATE)
#include "Update/update_main.h"
#endif

#include "systick.h"
#include "boot_info.h"
#include "flash_allot_table.h"


//****************************************************参数初始化**************************************************//
//结构体
__ALIGNED(4) 	Task_T *tpSysTask = NULL;  	//队列任务


//****************************************************函数声明****************************************************//
static bool b_task_manage_func_cb(Task_T *tp_task);

/***********************************************************************************************************************
-----函数功能    任务参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bSys_QueueInit(void)
{
	//任务队列初始化
	if(cQueue_TaskInit(&tpSysTask, 8, 0, b_task_manage_func_cb, NULL) <= 0)
	{
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_e("bSysTask:tpSysTask任务对象初始化失败");
		
		return false;
	}
	else if(tpSysTask == NULL)
	{
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_e("bSysTask:tpSysTask任务对象创建失败");
		
		return false;
	}
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    装载任务函数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:成功   false:失败 
******************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *tp_task)
{
	static vu16 uc_temp = 0;
	
	tp_task->bNowRun = false;
	tp_task->ucStep = 0;
	tp_task->usTaskWaitCnt = 0;
	tp_task->usTaskWaitCnt = 0;
	tp_task->usStepRepeatCnt = 0;
	
	uc_temp = lwrb_get_full(&tp_task->tQueueBuff);
	if(uc_temp%3 != 0 && uc_temp != 0)
	{
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_e("bSysTask:任务队列长度异常 长度%d",uc_temp);
		lwrb_reset(&tp_task->tQueueBuff);
		return false;
	}	
	
	if(tSysInfo.uInit.tFinish.bIF_SysTask == 0)
	{
		tp_task->ucID = STI_INIT;           
		tp_task->usInParam = 0;
	}
    else if(uc_temp)//队列里面有任务   
    {
        lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->ucID, 1);
		lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->usInParam, 2);
    }
    else
    {
		tp_task->ucID = STI_NULL;           
		tp_task->usInParam = 0;
    }
    
    switch (tp_task->ucID)
    {
        case STI_INIT:
            tp_task->vp_func = v_sys_queue_task_init;
        break;
		
		case STI_ENTER_APP:
			tp_task->vp_func = v_sys_queue_task_enter_app;
        break;
		
		case STI_ERR:
			tp_task->vp_func = v_sys_queue_task_err;
        break;
		
		case STI_RESET: 
			tp_task->vp_func = v_sys_queue_task_reset;
        break;
		
		#if(boardUPDATE)
		case STI_UPDATE:	
			tp_task->vp_func = v_sys_queue_task_update;
        break;
		#endif
		
		#if(boardDISPLAY_EN)
		case STI_DISPLAY: 
			tp_task->vp_func = v_sys_queue_task_disp;
        break;
		#endif
		
		#if(boardLOW_POWER)
		case STI_LOW_POWER: 
			tp_task->vp_func = v_sys_queue_task_low_power;
        break;
		#endif

		case STI_NULL:
        default:
            tp_task->vp_func = NULL;
			tp_task->usInParam = 0;
        break;
    }

    return true;       
}


/**********************************************************************************************************
*	函 数 名: JumpToApp
*	功能说明: 跳转到APP程序 0x0800 2000
*	形    参: 无
*	返 回 值: 无
**********************************************************************************************************/
/* 开关全局中断的宏 */
#define ENABLE_INT()	__set_PRIMASK(0)	/* 使能全局中断 */
#define DISABLE_INT()	__set_PRIMASK(1)	/* 禁止全局中断 */
typedef void (*pAppFunction) (void);
pAppFunction  application;
s8 cSys_JumpToApp(void)
{
	uint32_t JumpAddress = 0;
	
	/* APP栈顶指针合法 */
	if(0x20000000 != ((*(__IO uint32_t*)flashAPP_START) & 0x2FFE0000))
		return -1;
	
	if(tBootMemParam.tParam.eAppState == AS_ERASE)
		return -2;
	
//	1)关闭所有外设的时钟
//	2) 关闭使用的PLL
//	3) 禁用所有中断 
//	4) 清除所有挂起的中断标志位
	
	#if(boardADC_EN)
	vAdc_DeInit();
	#endif  //boardADC_EN
	
	#if(boardLED_EN)
	vLed_IfaceDeInit();
	#endif  //boardLED_EN
	
	#if(boardLOW_POWER)
	vPrint_EnterLowPower();                                 //关闭串口
	#endif
	
	#if(boardLOW_POWER)
	vGPIO_EnterApp();                                       //关闭中断
	vAdc_IoEnterLowPower();                                 //关闭AD
	#endif  //boardLOW_POWER
	
	#if(boardBMS_EN)
	vBms_IfaceDeInit();
	#endif
	
	#if( boardPRINT_IFACE )
	cBoot_CtrlUpdate(false, AS_OK);                               //把tBootInfo.ulCmd标志位设置为跳转到APP,下次重启就会直接进来APP
	vPrint_DeInit();
	#endif
	
	vSys_MsDelay(2);
	
	#if boardWDGT_EN
	vFwdgt_Reload();
	#endif
	
//		3) 禁用所有中断
//		4) 清除所有挂起的中断标志位
	__disable_irq();	                                    //关闭所有中断,如果有开外设也要关掉
	
	/* 关闭滴答定时器，复位到默认值 */
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
	
	JumpAddress = *(__IO uint32_t*)(flashAPP_START + 4);	//Reset_Handler 入口地址
	
	application = (pAppFunction) JumpAddress;
	
	__set_MSP(*(__IO uint32_t*) flashAPP_START);	        //APP程序堆栈指针起始(用户代码区的第一个字用于存放栈顶地址)
	
	application();	                                        //跳转到Reset_Handler即APP

	/* 跳转成功的话，不会执行到这里，用户可以在这里添加代码 */
	return -3;
}


























