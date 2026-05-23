#include "gpio_init.h"
#include "freertos.h"
#include "task.h"

#include "Sys/sys_task.h"
#include "timer_task.h"
#include "Print/print_task.h"

/***********************************************************************************************************************
-----函数功能    IO口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void vGPIO_Init(void)
{
//	rcu_periph_clock_enable(gpioVCC21_EN_RCU);
//	gpio_init(gpioVCC21_EN_GPIO, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ,gpioVCC21_EN_PIN);
//    gpioVCC21_EN_OFF();
	
	rcu_periph_clock_enable(gpioASSIST_OPEN_RCU);
	gpio_init(gpioASSIST_OPEN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, gpioASSIST_OPEN_PIN);
	gpioASSIST_OPEN_OFF();
}




/***********************************************************************************************************************
-----函数功能    21V电源开关
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:操作成功    false:操作失败
************************************************************************************************************************/ 
//bool vGPIO_V21PowerSwitch(bool en)
//{
//	if(en == true)
//	{
//		gpioVCC21_EN_ON();
//		return  true;
//	}
//	else 
//	{
//		gpioVCC21_EN_OFF();
//		return  false;
//	}
//}

/***********************************************************************************************************************
-----函数功能    协助BMS开启
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:操作成功    false:操作失败
************************************************************************************************************************/ 
bool vGPIO_AssistBmsOpen(bool en)
{
	if(en == true)
	{
		tSysInfo.uInit.tFinish.bIF_Gpio = 0;  //关闭初始化,预防按键误触发
		gpioASSIST_OPEN_ON();

		#if(boardBMS_EN)
		xTimerReset(tWakeUpBmsTimer,0);  //开启软件定时,2S后关闭
		#endif  //boardBMS_EN
		
		if(uPrint.tFlag.bSysTask)
			sMyPrint("BMS电源辅助开启打开\r\n");
		
		return  true;
	}
	else 
	{
		tSysInfo.uInit.tFinish.bIF_Gpio = 1;  //初始化完成
		gpioASSIST_OPEN_OFF();
		
		if(uPrint.tFlag.bSysTask)
			sMyPrint("定时2S到达,关闭BMS辅助开启\r\n");
		
		return  true;
	}
}

/***********************************************************************************************************************
-----函数功能    控制21V电源输出
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:正常    false:错误
************************************************************************************************************************/  
//bool vGPIO_CtrlV21Power(void)
//{
//	static  u8  v21_power_cnt;
//	#if(boardENG_MODE_EN)
//	//工程模式不打开
//	if(tSysInfo.eDevState == DS_ENG_MODE)  
//	{
//		vGPIO_V21PowerSwitch(false);
//	}
//	//内部电池需要充电 || USB需要打开
//	else 
//	#endif
//	if(tUsb.eState >= US_STARTING)
//	{
//		//关闭状态重新打开
//		if(bAdc_CheckV21Power() == VOS_OFF)
//		{ 
//			vGPIO_V21PowerSwitch(true);
//			v21_power_cnt = 0;
//		}
//		//存在错误.关闭21V(即电压不在范围)
//		else if(bAdc_CheckV21Power() == VOS_ERR)
//		{
//			if(++ v21_power_cnt > 5)
//			{
//				v21_power_cnt = 0;
//				vGPIO_V21PowerSwitch(false);
//				return false;
//			}	
//		}
//	}
//	else 
//	{
//		vGPIO_V21PowerSwitch(false);
//	}
//	return true;
//}



#if(boardLOW_POWER)
/***********************************************************************************************************************
-----函数功能    IO口进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void vGPIO_EnterLowPower(void)
{
	rcu_periph_clock_enable(gpioVCC21_EN_RCU);
	gpio_init(gpioVCC21_EN_GPIO, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ,gpioVCC21_EN_PIN);
}


/***********************************************************************************************************************
-----函数功能    IO口退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void vGPIO_ExitLowPower(void)
{
	vGPIO_Init();
}
#endif
