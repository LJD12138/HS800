/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"

#if(boardLOW_POWER)
#include "Print/print_task.h"
#include "rtc_wakeup.h"

//****************************************************函数声明****************************************************//
void v_enter_sleep(uint16_t time)

/***********************************************************************************************************************
-----函数功能    系统关闭中
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/  
void v_sys_queue_task_low_power(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
		//************************************步骤零:上报控制台关闭**********************************************
		case 0:
		{
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
		}
		break ;
		
		
		//************************************步骤2:关闭BMS**********************************************
		case 1:
		{
			cQueue_GotoStep(tp_task, STEP_END);
		}
		break ;
		
		default:
			cQueue_GotoStep(tp_task, STEP_END);
			break;
    }
}


/*****************************************************************************************************************
-----函数功能    进入休眠
-----说明(备注)  none
-----传入参数    time:进入休眠延时计数
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void v_enter_sleep(uint16_t time)
{
	static uint16_t enter_sleep_cnt; 
		
	if(++enter_sleep_cnt >= time)  //30S 350
	{
		enter_sleep_cnt = 0;
		
		bExti_SensorTriFlag = false;
		
		//***************************进入低功耗**********************************
		 
		//放在中断配置前面
		rtc_configuration(ALARM_TIME_INTERVAL); //会先重置所有外部中断,配置RTC唤醒
		
		#if boardWDGT_EN
		vFwdgt_EnterLowPower();
		#endif
		
		vPrint_EnterLowPower();     //关闭串口
		
		vLcd_EnterLowPower();
		
		vAdc_IoEnterLowPower();
		
		vKey_EnterLowPower();       //配置按键中断出发
		
		/* 关闭滴答定时器，复位到默认值 */
		SysTick->CTRL = 0;
		SysTick->LOAD = 0;
		SysTick->VAL = 0; 
		
		system_reset_clock_8m_irc8m();  //系统时钟
		
		//开始进入休眠
		pmu_to_sleepmode(WFI_CMD);  //任何中断都可以唤醒系统
		
		//****************************退出低功耗********************************
		SystemInit();               //初始化主时钟
		
		vSys_TickConfig();           //初始化系统tick
		
		vPrint_TaskInit();          //串口任务初始化
		
//		vGPIO_EnterApp();           //关闭中断
		vGPIO_Init();               //IO口初始化
		
		vAdc_Init();                //电池电压采集
		
		vLcd_ExitLowPower();
		
		rtc_close();                //关闭RTC唤醒中断
		
		#if boardWDGT_EN
		vFwdgt_ExitLowPower();      //
		#endif
		
		usBootRun1MsCnt = 0;
		
		uPrint.tFlag.SysTask = 0;
		uPrint.tFlag.Sensor = 0;
		if(uPrint.tFlag.SysTask)
			sMyPrint("退出低功耗\r\n");
	}
	
}


/*****************************************************************************************************************
-----函数功能    退出低功耗要处理部分
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void PostSleepProcessing(uint32_t ulExpectedIdleTime)
{
    //重新打开外设
	SystemCoreClock =((uint32_t)120000000);
	system_reset_clock_120m_irc8m();       //恢复主频时钟
	
	vGPIO_ExitLowPower();
	#if(boardPRINT_IFACE)
	vPrint_Init();
	sMyPrint("CK_SYS is %d\r\n", rcu_clock_freq_get(CK_SYS));
	sMyPrint("CK_AHB is %d\r\n", rcu_clock_freq_get(CK_AHB));
	sMyPrint("CK_APB1 is %d\r\n", rcu_clock_freq_get(CK_APB1));
	sMyPrint("CK_APB2 is %d\r\n", rcu_clock_freq_get(CK_APB2));
	#endif  //boardPRINT_IFACE
//	bBat_ExitLowPower();
	vLcd_ExitLowPower();
	vBuz_ExitLowPower();
	vKey_ExitLowPower();
	bAdc_ExitLowPower();
	vCount_ExitLowPower();
//	vMppt_ExitLowPower();
	vDCAC_ExitLowPower();
	vLight_ExitLowPower ();	
	vUSB_ExitLowPower();
	vTaskResume(Sys_Task_Handler);  //恢复
	tSysInfo.eWaitBootStep = WBS_WAIT;
}
#endif

