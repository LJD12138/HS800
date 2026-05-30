/*****************************************************************************************************************
*                                                                                                                *
 *                                         PD100W任务                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "Usb/usb_task.h"

#if(boardUSB_EN)
#include "Usb/usb_queue_task.h"
#include "Usb/usb_iface.h"

#include "app_info.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  //boardPRINT_IFACE

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if(boardADC_EN)
#include "Adc/adc_task.h"
#endif  //boardADC_EN


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define        	USB_TASK_PRIO                 			1   	//任务优先级 
#define        	USB_TASK_SIZE                 			256   	//任务堆栈  实际字节数 *4
TaskHandle_t    tUsbTaskHandler = NULL; 
void           	vUsb_Task(void *pvParameters);
#endif  //boardUSE_OS


//****************************************************参数初始化**************************************************//
__ALIGNED(4) Usb_T tUsb;
static Task_T *tp_task = NULL;


//****************************************************函数声明****************************************************//
static bool b_task_param_init(void);
static void v_usb_check_prote(void);
static void v_usb_param_update(void);


/*****************************************************************************************************************
-----函数功能    参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static bool b_task_param_init(void )
{
	if(tpUsbTask == NULL)
		return false;

	memset((u8*)&tUsb, 0, sizeof(tUsb));

	tUsb.usAutoOffTime = tAppMemParam.tUSB.usAutoOffTime;

	tp_task = tpUsbTask;

	return true;
}

/*****************************************************************************************************************
-----函数功能    系统任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool bUsb_TaskInit(void)
{
	vUsb_IfaceInit();

	if(bUsb_QueueInit() == false)
		return false;
	
	if(b_task_param_init() == false)
		return false;
	
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vUsb_Task,				//任务函数
                (const char* )"UsbTask",				//任务名称
                (uint16_t ) USB_TASK_SIZE,				//任务堆栈大小
                (void* )NULL,							//传递给任务函数的参数
                (UBaseType_t ) USB_TASK_PRIO,			//任务优先级
                (TaskHandle_t*)&tUsbTaskHandler);		//任务句柄
	#endif  //boardUSE_OS
	return true;
}





/*****************************************************************************************************************
-----函数功能    USB任务
-----说明(备注)  none
-----传入参数    value单位是W
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vUsb_Task(void *pvParameters)
{
    #if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
		if(tp_task == NULL)
		{
			b_task_param_init();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif
		}

		v_usb_param_update();
		v_usb_check_prote();

		if(tp_task->vp_func != NULL && tp_task ->bNowRun == false)
			tp_task->vp_func(tp_task);
		else if(tp_task->vp_func == NULL || tp_task ->bNowRun == true)
		{
			#if(boardUSE_OS)
			if(lwrb_get_full(&tp_task->tQueueBuff) == 0)
				ulTaskNotifyTake(pdFALSE, usbTASK_CYCLE_TIME);//pdFALSE:任务通知多少次就执行多少次
			#endif  //boardUSE_OS
			
			if(tp_task->bp_task_manage_func != NULL)
				tp_task->bp_task_manage_func(tp_task);
		}

		#if(boardUSE_OS)
		vTaskDelay(usbTASK_CYCLE_TIME);
		#endif  //boardUSE_OS
    }
}












/*****************************************************************************************************************
-----函数功能    保护处理
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_usb_check_prote(void)
{
	static u8 uc_pwr_err_cnt = 0;
	static u8 uc_over_temp_cnt = 0;

	//关闭USB
	if(tSysInfo.uPerm.tPerm.bDisChgPerm == false || 
		tSysInfo.eDevState == DS_CLOSING || 
		tSysInfo.eDevState == DS_SHUT_DOWN)
	{
		//USB处于开机
		if(tUsb.eDevState >= DS_BOOTING)  
			cUsb_Switch(ST_OFF, true);
	}

	if(tUsb.eDevState != DS_WORK && tUsb.eDevState != DS_ERR)
		return;

	//电源错误检查
	if(cUsb_CheckInVolt() != 0)
	{
		if(tUsb.uErrCode.tCode.bPowerErr == 0)
		{
			uc_pwr_err_cnt++;
			if(uc_pwr_err_cnt >= 10)
			{
				uc_pwr_err_cnt = 0;
				bUsb_SetErrCode(UEC_POWER_ERR,true); //设置错误
			}
		}
		else 
			uc_pwr_err_cnt = 0;
	}
	else
	{
		if(tUsb.uErrCode.tCode.bPowerErr == 1)
		{
			uc_pwr_err_cnt++;
			if(uc_pwr_err_cnt >= 5)
			{
				uc_pwr_err_cnt = 0;
				bUsb_SetErrCode(UEC_POWER_ERR,false); //清除错误
			}
		}
		else 
			uc_pwr_err_cnt = 0;
	}
	
	//温度检查
	if(tUsb.sMaxTemp > tAppMemParam.tUSB.sMaxTemp)
	{
		if(tUsb.uErrCode.tCode.bOT == 0)
		{
			uc_over_temp_cnt++;
			if(uc_over_temp_cnt >= 5)
			{
				uc_over_temp_cnt = 0;
				bUsb_SetErrCode(UEC_OT,true);  //设置错误
			}
		}
		else 
		{
			uc_over_temp_cnt = 0;
		}
	}
	//相差10摄氏度则开始退出高温报警
	else  if(tUsb.sMaxTemp < (tAppMemParam.tUSB.sMaxTemp - 10))
	{
		if(tUsb.uErrCode.tCode.bOT == 1)
		{
			uc_over_temp_cnt++;
			if(uc_over_temp_cnt >= 5)
			{
				uc_over_temp_cnt = 0;
				bUsb_SetErrCode(UEC_OT,false);   //清除错误
			}
		}
		else 
		{
			uc_over_temp_cnt = 0;
		}
	}
}

/***********************************************************************************************************************
-----函数功能	更新参数
-----作者       LJD
-----日期       2026-04-10
************************************************************************************************************************/
static void v_usb_param_update(void)
{
	tUsb.usAutoOffTime = tAppMemParam.tUSB.usAutoOffTime;
	// tUsb.sMaxTemp = tAdcSamp.sUsbTemp;
	tUsb.sMaxTemp = 25;//固定25摄氏度
	tUsb.usInVolt = tAdcSamp.usSysInVolt;//0.1V
	
	if(tUsb.eDevState == DS_WORK)
	{
		tUsb.usInCurr = 0;//0.1A
		// tUsb.usPdPwr = tAdcSamp.fUsbPdCurr * tAdcSamp.usUsbPdVolt / 10;//W
		// tUsb.usWcPwr = tAdcSamp.fUsbWcCurr * tAdcSamp.usUsbWcVolt / 10;//W
	}
	else
	{
		tUsb.usInCurr = 0;//0.1A
		tUsb.usPdPwr = 0;//W
		tUsb.usWcPwr = 0;//W
		tUsb.usOutPwr = 0;//W
	}
}

/*****************************************************************************************************************
-----函数功能    快充开关
-----说明(备注)  none
-----传入参数    UST_NULL=0,//进行取反
				 ST_ON,
				 ST_OFF,
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 cUsb_Switch(SwitchType_E Tri_Type, bool fore_en)
{
	switch(Tri_Type)
	{
		case ST_ON:
		{
			if((tUsb.eDevState == DS_WORK || 
				tUsb.eDevState == DS_BOOTING) &&
				fore_en == false)
			{
				if(uPrint.tFlag.bUsbTask)
					sMyPrint("bUsbTask:当前状态为工作,不允许开机\r\n");
				 
				return 0;
			}
			
			goto LoopOn;
		}
		
		case ST_OFF:
		{
			if((tUsb.eDevState == DS_SHUT_DOWN || 
				tUsb.eDevState == DS_CLOSING) &&
				fore_en == false)
			{
				if(uPrint.tFlag.bUsbTask)
					sMyPrint("bUsbTask:当前状态为关闭,不允许关机\r\n");
				 
				return 0;
			}
			
			goto LoopOff;
		}
		
		default:
		{
		    if(tUsb.eDevState == DS_SHUT_DOWN || tUsb.eDevState == DS_CLOSING)
			{
				LoopOn:
				if(tSysInfo.uPerm.tPerm.bDisChgPerm == false)
				{
					#if(boardBUZ_EN)
					bBuz_Tweet(SHORT_2);
					#endif  //boardBUZ_EN
					
					if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
						log_w("bUsbTask:系统不允许开启放电");
					
					return -1;
				}

				if(cUsb_CheckBatVolt() <= 0)  //电池电压异常
				{
					bUsb_SetErrCode(UEC_BAT_VOLT_LOW,true);
					
					if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
						log_w("bUsbTask:电池电压过低 %.2fV",tAdcSamp.usSysInVolt / 10.0f);
					
					return -2;
				} 
				#if(boardBUZ_EN)
				bBuz_Tweet(LONG_1);
				#endif  //boardBUZ_EN

				if(tUsb.uErrCode.ucErrCode)
					bUsb_SetErrCode(UEC_CLEAR_ALL, false);

				cQueue_AddQueueTask(tpUsbTask, UTI_BOOTING, NULL, fore_en);
			}
			else                       //其他情况都是关闭
			{
				LoopOff:
				#if(boardBUZ_EN)
				bBuz_Tweet(LONG_1);
				#endif  //boardBUZ_EN

				if(tUsb.uErrCode.ucErrCode)
					bUsb_SetErrCode(UEC_CLEAR_ALL, false);

				cQueue_AddQueueTask(tpUsbTask, UTI_CLOSING, NULL, fore_en);
			}
		}
		break;
	}
	
	#if(boardSYS_DATA_UPADATA)
	Sys_Update_Mod(USB_Mod,true);
	#endif
	
	return 1 ;
}

/*****************************************************************************************************************
-----函数功能    快充工作状态设置
-----说明(备注)  none
-----传入参数    
				 DS_CLOSING = 0,
				 DS_SHUT_DOWN,
				 DS_ERR,
				 DS_BOOTING,
				 DS_WORK,
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void bUsb_SetDevState(DevState_E stat)
{
    tUsb.eDevState = stat;
	
	//启动和关闭都清除一次错误
	if(stat == DS_BOOTING)
	{
		// usbPOWER_EN_ON();
		usbPD_EN_ON();
		usbPD2_EN_ON();
		// usbA_EN_ON();
	}
	else if(stat == DS_CLOSING)
	{
		// usbPOWER_EN_OFF();
		usbPD_EN_OFF();
		usbPD2_EN_OFF();
		// usbA_EN_OFF();
	}
	else if(stat == DS_SHUT_DOWN)
	{
		// usbPOWER_EN_OFF();
		usbPD_EN_OFF();
		usbPD2_EN_OFF();
		// usbA_EN_OFF();
	}
		
}

/*****************************************************************************************************************
-----函数功能    设置错误状态设置
-----说明(备注)  none
-----传入参数    code:错误位   set:设置,反之清除
				 UEC_CLEAR_ALL = 0,
				 UEC_POWER_ERR,
				 UEC_OVERTEMP,
				 UEC_OVERLOAD,
				 UEC_SW3518_LOST,
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void bUsb_SetErrCode(UsbErrCode_E code, bool set)
{
	static UsbErrCode_E e_next_code;
	static bool b_next_set;
	
	if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
	{
		if(e_next_code != code || b_next_set != set)
		{
			log_e("bUsbTask:任务错误 代码%d 类型%d",code,set);
			e_next_code = code;
			b_next_set = set;
		}
	}
	
	if(code == UEC_CLEAR_ALL)
	{
		tUsb.uErrCode.ucErrCode = 0;
		return;
	}

	if(set)
	{
		#if(boardBUZ_EN)
		bBuz_Tweet(LONG_3);
		#endif  //boardBUZ_EN
		
		ERR_SET(tUsb.uErrCode.ucErrCode, (code - 1));
	}
	else
		ERR_CLR(tUsb.uErrCode.ucErrCode, (code - 1));
	
	if(tUsb.uErrCode.ucErrCode)
	{
		UsbErrCode_U u_err_code;
		u_err_code.ucErrCode = tUsb.uErrCode.ucErrCode;
		u_err_code.tCode.bIc1Lost = 0;
		u_err_code.tCode.bIc2Lost = 0;

		if(tpUsbTask->ucID != UTI_ERR && u_err_code.ucErrCode)
			cQueue_AddQueueTask(tpUsbTask, UTI_ERR, NULL, true);
	}
}

/***********************************************************************************************************************
-----函数功能    自动关闭计时
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vUsb_TickTimer(void) 
{
	//非工作状态下退出
	if(bSys_IsWorkState() == false) 
		return;
	
	//非工作模式不计时
	if(tUsb.eDevState != DS_WORK)
		return;
	
	//-----自动关闭--------------------------------------   
	if(tUsb.usAutoOffTime)
	{
		if(tUsb.usAutoOffCnt)
		{
			tUsb.usAutoOffCnt--;
			
			if(tUsb.usAutoOffCnt == 0)
			{
				cUsb_Switch(ST_OFF, false);
				
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					sMyPrint("bUsbTask:倒计时结束,关闭USB  时间=%dS\r\n",tUsb.usAutoOffTime);
			}
		}
	}
}


/***********************************************************************************************************************
-----函数功能    刷新关闭时间
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vUsb_RefreshOffTime(void) 
{  
	if(tUsb.usAutoOffTime)
	{
		tUsb.usAutoOffCnt = tUsb.usAutoOffTime;
	}
}



/*****************************************************************************************************************
-----函数功能    初始化参数
-----说明(备注)  none
-----传入参数    p_lcd_mem : lcd记忆参数结构体
-----输出参数    none
-----返回值      true:设置成功  反之失败
*****************************************************************************************************************/
bool bUsb_MemParamInit(UsbMemParam_T* p_usb_mem)
{
	p_usb_mem->usAutoOffTime = boardUSB_OFF_TIME;
	p_usb_mem->usMaxInVolt = boardUSB_MAX_IN_VOLT;
	p_usb_mem->usMinInVolt = boardUSB_MIN_IN_VOLT;
	p_usb_mem->usMinOpenVolt = boardUSB_OPEN_MIN_VOLT;
	p_usb_mem->sMaxTemp = boardUSB_MAX_TEMP;
	return true;
}

/*****************************************************************************************************************
-----函数功能    设置记忆参数
-----说明(备注)  none
-----传入参数    add:true 增加   false:减少
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vUsb_MemParamSet(u8 item, bool add)
{
	switch(item)
	{
		case 0:
		{
			if(add == true)
			{
				if(tAppMemParam.tUSB.usAutoOffTime < 3600)
					tAppMemParam.tUSB.usAutoOffTime++;
			}
			else
			{
				if(tAppMemParam.tUSB.usAutoOffTime > 0)
					tAppMemParam.tUSB.usAutoOffTime--;
			}
		}
		break;
		
		case 1:
		{
			if(add == true)
				tAppMemParam.tUSB.usMaxInVolt++;
			else
				tAppMemParam.tUSB.usMaxInVolt--;
		}
		break;
		
		case 2:
		{
			if(add == true)
				tAppMemParam.tUSB.usMinInVolt++;
			else
				tAppMemParam.tUSB.usMinInVolt--;
		}
		break;
		
		case 3:
		{
			if(add == true)
				tAppMemParam.tUSB.usMinOpenVolt++;
			else
				tAppMemParam.tUSB.usMinOpenVolt--;
		}
		break;
		
		case 4:
		{
			if(add == true)
			{
				if(tAppMemParam.tUSB.sMaxTemp < 127)
					tAppMemParam.tUSB.sMaxTemp++;
			}
			else
			{
				if(tAppMemParam.tUSB.sMaxTemp > -127)
					tAppMemParam.tUSB.sMaxTemp--;
			}
		}
		break;
	}
}

/***********************************************************************************************************************
-----函数功能    检查USB供电状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      电压状态 小于0;欠压  0:电压正常  1:过压
************************************************************************************************************************/
s8 cUsb_CheckInVolt(void)
{
	if(RANGE(tUsb.usInVolt, 
	   tAppMemParam.tUSB.usMinInVolt, 
	   tAppMemParam.tUSB.usMaxInVolt))
	{
		return 0;
	}
	else if(tUsb.usInVolt > tAppMemParam.tUSB.usMaxInVolt)
	{
		return 1;
	}
	else
		return -1;
}

/***********************************************************************************************************************
-----函数功能    检查USB供电状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      电压状态 小于0;欠压  0:电压正常  1:过压
************************************************************************************************************************/
s8 cUsb_CheckBatVolt(void)
{
	if(tAdcSamp.usSysInVolt > tAppMemParam.tUSB.usMinOpenVolt)
	{
		return 1;
	}
	else
		return -1;
}


#if(boardLOW_POWER)
/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vUsb_EnterLowPower(void)
{
	rcu_periph_clock_enable(usbPD_EN_RCU);
	gpio_init(usbPD_EN_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ,usbPD_EN_PIN);
	
	rcu_periph_clock_enable(usbCHG_EN_RCU);
	gpio_init(usbCHG_EN_GPIO, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ,usbCHG_EN_PIN);
	
	rcu_periph_clock_enable(usbIC1_SCL_RCU);
	gpio_init(usbIC1_SCL_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ,usbIC1_SCL_PIN);
	
	rcu_periph_clock_enable(usbIC1_SDA_RCU);
	gpio_init(usbIC1_SDA_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ,usbIC1_SDA_PIN);
	
	rcu_periph_clock_enable(usbIC1_SCL_RCU);
	gpio_init(usbIC2_SCL_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ,usbIC2_SCL_PIN);
	
	rcu_periph_clock_enable(usbIC2_SDA_RCU);
	gpio_init(usbIC2_SDA_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ,usbIC2_SDA_PIN);
	
	vTaskSuspend(tUsbTaskHandler); //暂停任务
}



/*****************************************************************************************************************
-----函数功能    退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vUsb_ExitLowPower(void)
{
	v_usb_gpio_init();
	vTaskResume(tUsbTaskHandler); //恢复任务
}
#endif  //boardLOW_POWER

#endif  //boardUSB_EN


