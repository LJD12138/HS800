/*****************************************************************************************************************
*                                                                                                                *
 *                                         按键处理任务                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "Key/key_task.h"

#if(boardKEY_EN)
#include "Key/key_func.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define       	KEY_TASK_PRIO                  			2     	//任务优先级 
#define        	KEY_TASK_STK_SIZE              			256   	//任务堆栈  实际字节数 *4
TaskHandle_t    tKeyTaskHandler = NULL; 
void          	vKey_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
KeyHandler_t 	tKeyPower;

#if(boardDCAC_EN)
KeyHandler_t 	tKeyAC;
#endif  //boardDCAC_EN

#if(boardLIGHT_EN)
KeyHandler_t 	tKeyLight;
#endif  //boardLIGHT_EN

#if(boardUSB_EN)
KeyHandler_t 	tKeyUSB;
#endif  //boardUSB_EN

#if(boardDC_EN)
KeyHandler_t 	tKeyDC;
#endif  //boardDC_EN

u8 Key_TriTypeBuff[ keyGROUP_NUM ] = {0};      	//按键功能
vu16 Key_UnPressTim = 0 , Key_TriTypeCnt = 0;


//****************************************************函数声明**************************************************//
static void v_key_gpio_init(void);
static void v_key_shot_press(KeyHandler_t* keyHandler);
static void v_key_long_press(KeyHandler_t* keyHandler);
//static void v_key_super_long_press(KeyHandler_t* keyHandler);
static bool v_key_check_other_is_tri(void);

/***********************************************************************************************************************
-----函数功能    登记按键信息
-----说明(备注)  此注册有问题,会导致Num超过最大值,数据溢出
-----传入参数    按键结构体
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static KeyHandler_t* KeyHandlerList[keyNUM];
static vu8 KeyHandlerListNum = 0;
static void v_key_register(KeyHandler_t* keyHandler)
{
    KeyHandlerList[KeyHandlerListNum] = keyHandler;
	
    keyHandler->sOnPressCnt = 0;
	
    KeyHandlerListNum++;
}

/***********************************************************************************************************************
-----函数功能    按键任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vKey_TaskInit(void)
{
	v_key_gpio_init();
	
	#if(boardUSE_OS)
	xTaskCreate((TaskFunction_t )vKey_Task,				//任务函数
                (const char* )"bKeyTask",				//任务名称
                (uint16_t ) KEY_TASK_STK_SIZE,          //任务堆栈大小
                (void* )NULL,							//传递给任务函数的参数
                (UBaseType_t ) KEY_TASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tKeyTaskHandler);      	//任务句柄
	#endif  //boardUSE_OS
}


/***********************************************************************************************************************
-----函数功能    按键初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_key_gpio_init(void)
{
	#if(!boardADC_EN)
	rcu_periph_clock_enable(keyGPIO_POWER_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(keyGPIO_POWER_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, keyGPIO_POWER_PIN);
	#else
	gpio_init(keyGPIO_POWER_PORT,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_2MHZ,keyGPIO_POWER_PIN);
	#endif
	#endif  //boardADC_EN
	
	#if(boardDCAC_EN)
	rcu_periph_clock_enable(keyGPIO_AC_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(keyGPIO_AC_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, keyGPIO_AC_PIN);
	#else
	gpio_init(keyGPIO_AC_PORT,GPIO_MODE_IPU,GPIO_OSPEED_2MHZ,keyGPIO_AC_PIN);
	#endif
	#endif  //boardDCAC_EN

	#if(boardLIGHT_EN)
	rcu_periph_clock_enable(keyGPIO_LIGHT_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(keyGPIO_LIGHT_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, keyGPIO_LIGHT_PIN);
	#else
	gpio_init(keyGPIO_LIGHT_PORT,GPIO_MODE_IPU,GPIO_OSPEED_2MHZ,keyGPIO_LIGHT_PIN);
	#endif
	#endif  //boardLIGHT_EN

	#if(boardUSB_EN)
	rcu_periph_clock_enable(keyGPIO_USB_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(keyGPIO_USB_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, keyGPIO_USB_PIN);
	#else
	gpio_init(keyGPIO_USB_PORT,GPIO_MODE_IPU,GPIO_OSPEED_2MHZ,keyGPIO_USB_PIN);
	#endif
	#endif  //boardUSB_EN

	#if(boardDC_EN)
	rcu_periph_clock_enable(keyGPIO_DC_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(keyGPIO_DC_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, keyGPIO_DC_PIN);
	#else
	gpio_init(keyGPIO_DC_PORT,GPIO_MODE_IPU,GPIO_OSPEED_2MHZ,keyGPIO_DC_PIN);
	#endif
	#endif  //boardDC_EN

	//true:长按累加功能                 false:关闭
	tKeyPower.bEnLongPressAdd = false;
	//true:多功能按键:双击等前后触发的   false:可以使用一直长按可以触发长按功能,也可以识别同时触发的
    tKeyPower.bEnMulitFunKey = false;
    tKeyPower.IsPress = bKey_PowerIsPress;
    v_key_register(&tKeyPower);
   
	#if(boardDCAC_EN)
    tKeyAC.bEnLongPressAdd = false;
    tKeyAC.bEnMulitFunKey = true;
    tKeyAC.IsPress = bKey_AcIsPress;
    v_key_register(&tKeyAC);
	#endif  //boardDCAC_EN

	#if(boardLIGHT_EN)
	tKeyLight.bEnLongPressAdd = false;
    tKeyLight.bEnMulitFunKey = false;
    tKeyLight.IsPress = bKey_LightIsPress;
    v_key_register(&tKeyLight);
	#endif  //boardLIGHT_EN

	#if(boardUSB_EN)
	tKeyUSB.bEnLongPressAdd = false;
    tKeyUSB.bEnMulitFunKey = false;
    tKeyUSB.IsPress = bKey_UsbIsPress;
    v_key_register(&tKeyUSB);
	#endif  //boardUSB_EN

	#if(boardDC_EN)
	tKeyDC.bEnLongPressAdd = false;
    tKeyDC.bEnMulitFunKey = false;
    tKeyDC.IsPress = bKey_DcIsPress;
    v_key_register(&tKeyDC);
	#endif  //boardDC_EN
}


/***********************************************************************************************************************
-----函数功能    按键循环任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vKey_Task(void *pvParameters)
{
    static vu8 Currkey = 0;
	
	#if(boardUSE_OS)
	for(;;)
	#endif  //boardUSE_OS
    {
		//GPIO初始化未完成
		if(tSysInfo.uInit.tFinish.bIF_Gpio == 0 || tpSysTask->ucID == STI_UPDATE)
		{
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif
		}
		
		for(Currkey = 0; Currkey < KeyHandlerListNum; Currkey++)
		{
			//******************************************按键 按下状态***********************************************
            if(KeyHandlerList[Currkey]->IsPress())        
            {
				//记录按下的时间--------------------------------------------------------------------------
                if(KeyHandlerList[Currkey]->sOnPressCnt < 0xfff && 
				    KeyHandlerList[Currkey]->sOnPressCnt >= 0 )
					{
						KeyHandlerList[Currkey]->sOnPressCnt++;
					}
				
				//使能长按累加按键-----------------------------------------------------------------------
                if( KeyHandlerList[Currkey]->bEnLongPressAdd == true )  
				{
					if( KeyHandlerList[Currkey]->sOnPressCnt >= keyLONG_PRESS_TIME) //满足长按时长
					{
						v_key_shot_press(KeyHandlerList[Currkey]);
						
						vKey_ProcKeyFunc(Key_TriTypeBuff); //立刻处理
						
						KeyHandlerList[Currkey]->sOnPressCnt = keyLONG_PRESS_TIME - keyADD_SPACE_TIME;
					}
				}
				//不使能组合按键--------------------------------------------------------------------------
				else if( KeyHandlerList[Currkey]->bEnMulitFunKey == false )  
				{				
					if( KeyHandlerList[Currkey]->sOnPressCnt >= keyLONG_PRESS_TIME)   //满足长按事件,记录 
					{
						v_key_long_press(KeyHandlerList[Currkey]);     //执行 长按 事件

						if(v_key_check_other_is_tri() == true)
						{
							continue;  //结束本次循环
						}
						
						vKey_ProcKeyFunc(Key_TriTypeBuff);  //立刻处理
					}
				}
				else
				{
					if( KeyHandlerList[Currkey]->sOnPressCnt >= keySUPER_LONG_PRESS_TIME ) //满足超长按事件,提示
					{
						if(uPrint.tFlag.bKeyTask)
							sMyPrint("Key_Task:触发长按事件\r\n");
						
//						v_key_super_long_press(KeyHandlerList[Currkey]);  //记录长按事件 
//						
//						vKey_ProcKeyFunc(Key_TriTypeBuff);  //立刻处理
						
						v_key_long_press(KeyHandlerList[Currkey]);  //记录长按事件 
						
						#if(boardBUZ_EN)
						bBuz_Tweet(SHORT_1);
						#endif  //boardBUZ_EN
					}
				}
				Key_UnPressTim = 0;	
				tSysInfo.usNeedSleepCnt = 0;
            }
			//****************************************************按键 放开状态*******************************************
            else                                   
            { 
				//按键已经松开,记录当前按键事件,并等待是否还有组合按键触发------------------------------------------------
				 if( Key_UnPressTim < keyNUPRESS_MAX_TIME && Key_UnPressTim >= 4) 
				 {
					 //短按  :按下时间在 keySHORT_PRESS_TIME ~ KeyLongPressTime 之间
					 if(RANGE( KeyHandlerList[Currkey]->sOnPressCnt,  keySHORT_PRESS_TIME,
						 ( keyLONG_PRESS_TIME - keyADD_SPACE_TIME -1 )))   
					 {  
						 v_key_shot_press(KeyHandlerList[Currkey]);
						 if(KeyHandlerList[Currkey]->bEnMulitFunKey == false) //没有使能多功能按键,就不需要等待,直接触发按键
							goto KeyTri;
					 }
					 else if( KeyHandlerList[Currkey]->sOnPressCnt >= keyLONG_PRESS_TIME)  //长按
					 {
						 v_key_long_press(KeyHandlerList[Currkey]);	
						 if(KeyHandlerList[Currkey]->bEnMulitFunKey == false) //没有使能多功能按键,就不需要等待,直接触发按键
							goto KeyTri;
					 }
				 }
				 //已经处理完--------------------------------------------------------------------------------------------
				 else  if( Key_UnPressTim == keyNUPRESS_MAX_TIME) 
				 {
					 KeyTri:
				     vKey_ProcKeyFunc(Key_TriTypeBuff);
				 }
				 
				 
				 //每遍历一次---------------------------------------------------------------------------------------------
				 if(Currkey == 0)  
				 {
					 //按键松开计时
					 if( Key_UnPressTim < 0xffff) 
						 Key_UnPressTim ++ ; 
				 }
				 
				 if(Key_UnPressTim ==5)
					KeyHandlerList[Currkey]->sOnPressCnt = 0;
            }
        }
		#if(boardUSE_OS)
		vTaskDelay(keyTASK_CYCLE_TIME);
		#endif
	}
}

/***********************************************************************************************************************
-----函数功能    检查其他按键是否按下
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:还有其他按键按下没有触发,反之false
************************************************************************************************************************/
static bool v_key_check_other_is_tri(void)
{
	vu8 Currkey = 0;	  
    for(Currkey = 0; Currkey < KeyHandlerListNum; Currkey++)
	{
		if(KeyHandlerList[Currkey]->sOnPressCnt > 0)
			return true ;
	}
	return false;
}


/***********************************************************************************************************************
-----函数功能    录入短按按键事件
-----说明(备注)  none
-----传入参数    按键结构体
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_key_shot_press(KeyHandler_t* keyHandler)
{
	#if(boardDISPLAY_EN)
	if(!tDisp.bLight)   //息屏第一个功能不执行
	{
		bDisp_Switch(ST_ON, false);
		keyHandler->sOnPressCnt = -1;
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:当前息屏,按键功能退出\r\n");
		return;
	}
	#endif  //boardDISPLAY_EN
	
	if(keyHandler == &tKeyPower)
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_POWER_SHORT ;  
		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:电源短按\r\n");
	}
	
	#if(boardDCAC_EN)	
	else if(keyHandler == &tKeyAC) 
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_AC_SHORT ; 
		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:AC短按\r\n");
	}
	#endif  //boardDCAC_EN

	#if(boardLIGHT_EN)
	else if(keyHandler == &tKeyLight) 
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_LIGHT_SHORT ; 
		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:Light短按\r\n");
	}
	#endif  //boardLIGHT_EN

	#if(boardUSB_EN)
	else if(keyHandler == &tKeyUSB) 
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_USB_SHORT ; 
		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:USB短按\r\n");
	}
	#endif  //boardUSB_EN

	#if(boardDC_EN)
	else if(keyHandler == &tKeyDC) 
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_DC_SHORT ; 
		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:DC短按\r\n");
	}
	#endif  //boardDC_EN
}



/***********************************************************************************************************************
-----函数功能    录入长按按键事件
-----说明(备注)  none
-----传入参数    按键结构体
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_key_long_press(KeyHandler_t* keyHandler)
{
	#if(boardDISPLAY_EN)
	if(!tDisp.bLight)   //非关机状态下,息屏第一个功能不执行
	{
		bDisp_Switch(ST_ON, false);
		keyHandler->sOnPressCnt = -1;
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:当前息屏,按键功能退出\r\n");
		return;
	}
	#endif  //boardDISPLAY_EN
	
	if(keyHandler == &tKeyPower) 
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_POWER_LONG ;  
		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:电源长按\r\n");
		
	}

	#if(boardDCAC_EN)
	else if(keyHandler == &tKeyAC) 
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_AC_LONG ; 
		if((keyGROUP_NUM-1)>Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:AC长按\r\n");
	}
	#endif  //boardDCAC_EN

	#if(boardLIGHT_EN)
	else if(keyHandler == &tKeyLight) 
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_LIGHT_LONG ; 
		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:Light长按\r\n");
	}
	#endif  //boardLIGHT_EN

	#if(boardUSB_EN)
	else if(keyHandler == &tKeyUSB) 
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_USB_LONG ; 
		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:USB长按\r\n");
	}
	#endif  //boardUSB_EN

	#if(boardDC_EN)
	else if(keyHandler == &tKeyDC) 
	{
		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_DC_LONG ; 
		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
		
		keyHandler->sOnPressCnt = -1;
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:DC长按\r\n");
	}
	#endif  //boardDC_EN
}

/***********************************************************************************************************************
-----函数功能    录入超长按按键事件
-----说明(备注)  none
-----传入参数    按键结构体
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
//static void v_key_super_long_press(KeyHandler_t* keyHandler)
//{
//	if(!tLCD.bLight && bSys_IsShutDownState() == false)   //非关机状态下,息屏第一个功能不执行
//	{
//		vLCD_RefreshDisplayParam();
//		keyHandler->sOnPressCnt = -1;
//		if(uPrint.tFlag.bKeyTask)
//			sMyPrint("Key_Task:当前息屏,按键功能退出\r\n");
//		return;
//	}
	
//	if(keyHandler == &tKeyPower) 
//	{
//		Key_TriTypeBuff[Key_TriTypeCnt] = KTE_POWER_SUPER_LONG ;  
//		if((keyGROUP_NUM - 1) > Key_TriTypeCnt) Key_TriTypeCnt ++;
//		
//		keyHandler->sOnPressCnt = -1;
//		
//		if(uPrint.tFlag.bKeyTask)
//			sMyPrint("Key_Task:电源超长按\r\n");
//		
//	}
//}


/***********************************************************************************************************************
-----函数功能    电源按键已经被触发
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vKey_PowerIsTri(void)
{
	tKeyPower.sOnPressCnt = -1;
}

/*****************************************************************************************************************
-----函数功能    参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vKey_ParamInit(void )
{
	Key_TriTypeCnt = 0;
	memset (Key_TriTypeBuff, KTE_FUN_NULL, sizeof( Key_TriTypeBuff));  //按键事件Buff清零
}
	

#if(boardLOW_POWER)
/***********************************************************************************************************************
-----函数功能    按键进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vKey_EnterLowPower(void)
{
	rcu_periph_clock_enable(RCU_PMU);
	rcu_periph_clock_enable(keyGPIO_POWER_RCU);
	rcu_periph_clock_enable(keyGPIO_WP_RCU);
	rcu_periph_clock_enable(RCU_AF);
	
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(keyGPIO_POWER_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, keyGPIO_POWER_PIN);
	gpio_mode_set(keyGPIO_WP_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_NONE, keyGPIO_WP_PIN);
	gpio_mode_set(keyGPIO_AC_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, keyGPIO_AC_PIN);
	#else
	gpio_init(keyGPIO_POWER_PORT,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_2MHZ,keyGPIO_POWER_PIN);
	gpio_init(keyGPIO_WP_GPIO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_2MHZ,keyGPIO_WP_PIN);
	gpio_init(keyGPIO_AC_PORT,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,keyGPIO_AC_PIN);
	#endif
	
	/* enable and set key EXTI interrupt to the lowest priority */
	nvic_irq_enable(EXTI10_15_IRQn, 2U, 0U);
	nvic_irq_enable(EXTI0_IRQn, 2U, 0U);

	/* connect key EXTI line to key GPIO pin */
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOC, GPIO_PIN_SOURCE_13); //PC13
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_0); //PA0

	/* configure key EXTI line */
	exti_init(EXTI_13, EXTI_INTERRUPT, EXTI_TRIG_FALLING); //下降沿触发
	exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_RISING); //上升沿触发
	exti_interrupt_flag_clear(EXTI_13);
	exti_interrupt_flag_clear(EXTI_0);
	
	vTaskSuspend(tKeyTaskHandler);  //挂起任务
}


/***********************************************************************************************************************
-----函数功能    按键退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vKey_ExitLowPower(void)
{
	rcu_periph_clock_enable(keyGPIO_POWER_RCU);
	rcu_periph_clock_enable(keyGPIO_AC_RCU);
	
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(keyGPIO_POWER_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, keyGPIO_POWER_PIN);
	gpio_mode_set(keyGPIO_AC_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, keyGPIO_AC_PIN);
	gpio_mode_set(keyGPIO_LIGHT_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, keyGPIO_LIGHT_PIN);
	gpio_mode_set(keyGPIO_USB_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, keyGPIO_USB_PIN);
	#else
	gpio_init(keyGPIO_POWER_PORT,GPIO_MODE_IPU,GPIO_OSPEED_2MHZ,keyGPIO_POWER_PIN);
	gpio_init(keyGPIO_AC_PORT,GPIO_MODE_IPU,GPIO_OSPEED_2MHZ,keyGPIO_AC_PIN);
	gpio_init(keyGPIO_LIGHT_PORT,GPIO_MODE_IPU,GPIO_OSPEED_2MHZ,keyGPIO_LIGHT_PIN);
	gpio_init(keyGPIO_USB_PORT,GPIO_MODE_IPU,GPIO_OSPEED_2MHZ,keyGPIO_USB_PIN);
	#endif

	vTaskResume(tKeyTaskHandler);  //恢复任务
}
#endif  //boardLOW_POWER

#endif  //boardKEY_EN

