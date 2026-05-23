/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task.h"
#include "Print/print_task.h"

#include "gpio_init.h"
#include "timer_task.h"

#if(boardENG_MODE_EN)
#include "eng_mode.h"
#endif


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define     	SYS_TASK_PRIO                  			2     //任务优先级 
#define      	SYS_TASK_STK_SIZE              			256   //任务堆栈  实际字节数 *4
TaskHandle_t  	tSysTaskHandler = NULL; 
void         	vSys_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
__ALIGNED(4) SysInfo_T tSysInfo;
static Task_T *tp_task = NULL;

//****************************************************函数声明****************************************************//
static void v_task_param_init(void);

/*****************************************************************************************************************
-----函数功能    系统任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vSys_TaskInit(void)
{
	if(bSys_QueueInit() == false)
		return;
	
	v_task_param_init();
	
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vSys_Task,              //任务函数 (1)
                (const char* )"bSysTask",                //任务名称
                (uint16_t ) SYS_TASK_STK_SIZE,          //任务堆栈大小
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) SYS_TASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tSysTaskHandler);      //任务句柄
	#endif  //#endif  //boardUSE_OS
}


/***********************************************************************************************************************
-----函数功能    任务参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_task_param_init(void)
{
	tp_task = tpSysTask;

	//系统任务参数
	memset(&tSysInfo, 0, sizeof(tSysInfo));
	
	tSysInfo.sMaxTemp = 25;                   //设置默认最高温度
	tSysInfo.sMinTemp = 25;                   //设置默认最低温度
}

/***********************************************************************************************************************
-----函数功能    系统循环任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vSys_Task(void *pvParameters)
{
	#if(boardUSE_OS)
    for(;;)
	#endif
    {
		if(tp_task == NULL)
		{
			v_task_param_init();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif
		}
		
		if(tp_task->vp_func != NULL && tp_task ->bNowRun == false)
			tp_task->vp_func(tp_task);
		else if(tp_task->bp_task_manage_func != NULL)
		{
			if(tp_task->bp_task_manage_func(tp_task) == false)
			{
				if(uPrint.tFlag.bSysTask)
						log_w("bSysTask:任务调度失败,等待任务长度%d",
								lwrb_get_full(&tp_task->tQueueBuff));
				
				#if(boardUSE_OS)
				vTaskDelay(1000);
				#endif  //boardUSE_OS
			}
		}
		
		#if(boardUSE_OS)
		vTaskDelay(sysTASK_CYCLE_TIME);
		#endif  //boardUSE_OS
    }
}
















/************************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/

#if(boardLOW_POWER)
/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void PreSleepProcessing(uint32_t ulExpectedIdleTime)
{
	tSysInfo.bIntFlag = false;
}





/*****************************************************************************************************************
-----函数功能    系统电源类型选择
-----说明(备注)  这个要比较快判断处理  建议50MS
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vSys_PowerTypeSelect(void)
{
	//外接电池了
    if( bAdc_CheckSysInputVolt() == VS_NORMAL)
    {
        tSysInfo.ePowerType = SPT_BMS;
    }
//	//照明开启
//	else if (tLight.eState > LS_OFF)
//	{
//		tSysInfo.ePowerType = SPT_10V;
//	}
	else 
	{
		tSysInfo.ePowerType = SPT_5V;
	}	
}
#endif

/***********************************************************************************************************************
-----函数功能    设置系统运行状态
-----说明(备注)  none
-----传入参数    step:
					  DS_INIT = 0,          // 初始化
					  DS_CLOSING ,          // 关闭中
					  DS_SHUT_DOWN,         // 关机状态
					  DS_ERR,               // 错误状态
					  DS_BOOTING,           // 装载中
					  DS_WORK,              // 工作状态
					  DS_ENG_MODE,          // 工程模式
				 bz:
					  true 打开 蜂鸣器
-----输出参数    none
-----返回值      true:操作成功   false:操作失败
************************************************************************************************************************/
bool bSys_SetDevState(DevState_E step, bool bz)
{
	if(tSysInfo.eDevState != step)
	{
		tSysInfo.eDevState = step;
		if(tSysInfo.eDevState == DS_INIT)  //初始化
		{
			gpioASSIST_OPEN_ON();
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为初始化\r\n");
		}
		else if(tSysInfo.eDevState == DS_CLOSING)  //关闭中
		{
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为关闭中\r\n");
		}
		else if(tSysInfo.eDevState == DS_SHUT_DOWN)  //关闭
		{
			gpioASSIST_OPEN_OFF();
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为关闭\r\n");
		}
		else if(tSysInfo.eDevState == DS_ERR)  //错误
		{
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为错误\r\n");
		}
		else if(tSysInfo.eDevState == DS_BOOTING)    //启动中
		{
			gpioASSIST_OPEN_ON();
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为启动中\r\n");
		}
		else if(tSysInfo.eDevState == DS_WORK)    //工作
		{
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为工作\r\n");
		}
		#if(boardENG_MODE_EN)
		else if(tSysInfo.eDevState == DS_ENG_MODE)  //工程模式
		{
			vEng_ParamInit();
			bEng_SetEngStep(EMS_LCD);
		}
		#endif //boardENG_MODE_EN
	}	
	
	//关机状态下
	if(tSysInfo.eDevState == DS_SHUT_DOWN)
	{
//		bLCD_DispalySwitch(false);
	}
	else 
	{
//		vLCD_RefreshDisplayParam();  //显示屏息屏倒计时
	}
		
	
	if(bz)
    {
//        bBuz_Tweet(LONG_1);
    }
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    工作过程中检测是否进入关机或重启,并进入倒计时
-----说明(备注)  没有此功能
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vSys_TickTimer(void) 
{
	static  u8 uc_cnt = 0;
	
	uc_cnt++;
	if(uc_cnt >= 2)
	{
		uc_cnt = 0;
		switch(tpSysTask->ucID)
		{
			case STI_INIT:  
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_INIT !\r\n");
			}
			break;
			
			case STI_ENTER_APP:  
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_ENTER_APP !\r\n");
			}
			break;
			
			case STI_RESET:
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_RESET !\r\n");
			}
			break;
			
			case STI_ERR:  
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_ERR !\r\n");
			}
			break;
			
			#if(boardUPDATE)
			case STI_UPDATE:
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_UPDATE !\r\n");
			}
			break;
			#endif
			
			#if(boardDISPLAY_EN)
			case STI_DISPLAY:
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_DISPLAY !\r\n");
			}
			break;
			#endif
			
			#if(boardLOW_POWER)
			case STI_LOW_POWER:
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_LOW_POWER !\r\n");
			}
			break;
			#endif
			
			default:
				break;
		}
	}
//	int num1 = 10;
//	float num2 = 1.123f;
//	char num3[] = "zheshaoishdoiashjdncuoasd";
//	sMyPrint("bSysTask = DS_BOOTING %s \r\n",num3);
//	log_e("这个是错误");
//	log_w("这个是警告");
//	log_i("这是一条提示");

	//***************************************************关机倒计时*****************************************************
	if(tSysInfo.usAutoOffTime) 
	{
		if(tSysInfo.usAutoOffCnt)
		{
			tSysInfo.usAutoOffCnt--;
			if(tSysInfo.usAutoOffCnt == 0)//倒计时为0进入
			{
				if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
					sMyPrint("Sys_Task:====倒计时结束,进入关机  时间=%dS====\r\n",tSysInfo.usAutoOffTime);
			}
		}	
	}
}

#if(boardLOW_POWER)
/***********************************************************************************************************************
-----函数功能    低功耗电源是否存在,判断是否可以打开照明和type-c应急输出
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:允许  false:不允许
************************************************************************************************************************/
bool bSys_LowPowerExist(void)
{
	if(tAdc_SysSamp.usBMS_Vin >= boardBMS_MIN_VOLT || 
		eMPPT_GetVinState() == MVS_NORMAL
	    )
	{
		return true;
	}
	else 
	{
		return false;
	}
}
#endif
