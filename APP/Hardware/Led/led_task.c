/*****************************************************************************************************************
*                                                                                                                *
 *                                         指示灯处理任务                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "Led/led_task.h"

#if(boardLED_EN)
#include "Led/led_iface.h"
#include "Sys/sys_task.h"

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //boardUSB_EN

#if(boardDC_EN)
#include "Dc/dc_task.h"
#endif  //boardDC_EN

#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif  //boardLIGHT_EN

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  //boardBMS_EN

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#endif  //DCAC使能

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS


#define     	ledTASK_CYCLE_TIME                		1000  //任务时间


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define        	LED_TASK_PRIO                 			1     //任务优先级 
#define        	LED_TASK_STK_SIZE              			64   //任务堆栈  实际字节数 *4
TaskHandle_t    tLedTaskHandler = NULL; 
void           	vLed_Task(void *pvParameters);
#endif  //boardUSE_OS


//****************************************************函数声明****************************************************//
static void v_led_breathing(void);


/***********************************************************************************************************************
-----函数功能    按键任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vLed_TaskInit(void)
{
	vLed_IfaceInit();
	
	#if(boardUSE_OS)
	xTaskCreate((TaskFunction_t )vLed_Task,				//任务函数
                (const char* )"LedTask",              	//任务名称
                (uint16_t ) LED_TASK_STK_SIZE,          //任务堆栈大小
                (void* )NULL,							//传递给任务函数的参数
                (UBaseType_t ) LED_TASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tLedTaskHandler);      	//任务句柄
	#endif  //boardUSE_OS
}


/***********************************************************************************************************************
-----函数功能    按键循环任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vLed_Task(void *pvParameters)
{
    #if(boardUSE_OS)
	for(;;)
	#endif  //boardUSE_OS
    {
		switch(tSysInfo.eDevState)
		{
			default:
			
			case DS_INIT:
			case DS_SHUT_DOWN:
			{	
				ledPWR_SW_OFF();
				ledAC_SW_OFF();
				ledUSB_SW_OFF();
				ledLight_SW_OFF();
				ledDC_SW_OFF();
				#if(boardUSE_OS)
				vTaskDelay(ledTASK_CYCLE_TIME);
				#endif  //boardUSE_OS
			}
			break;
			
			case DS_BOOTING:
			{
				ledPWR_SW_ON();
				#if(boardUSE_OS)
				vTaskDelay(ledTASK_CYCLE_TIME);
				#endif  //boardUSE_OS
			}
			break;
			
			case DS_CLOSING:
			case DS_ERR:
			case DS_WORK:
			{
				#if(boardDISPLAY_EN)
				if(tDisp.bLight == true)
					ledPWR_SW_ON();
				else 
				#endif  //boardDISPLAY_EN
					v_led_breathing();
				
				#if(boardDCAC_EN)
				if(tDcac.eDisChgState >= IOS_STARTING)
					ledAC_SW_ON();
				else 
					ledAC_SW_OFF();
				
				
				
				if(tLight.eDevState == DS_WORK)
					ledLight_SW_ON();
				else 
					ledLight_SW_OFF();
				
				
				
				#if(boardDCAC_PARA_IN)
				//USB DC
				if(tUsb.eDevState >= DS_BOOTING || tDc.eDevState >= DS_BOOTING)
					ledUSB_SW_ON();
				else 
					ledUSB_SW_OFF();
				//并网
				if(tDcac.eParanInState >= IOS_STARTING)
					ledDC_SW_ON();
				else 
					ledDC_SW_OFF();
				#else
				//USB
				if(tUsb.eDevState >= DS_BOOTING)
					ledUSB_SW_ON();
				else 
					ledUSB_SW_OFF();
				//DC
				if(tDc.eDevState >= DS_BOOTING)
					ledDC_SW_ON();
				else 
					ledDC_SW_OFF();
				#endif
				
				#if(boardUSE_OS)
				vTaskDelay(50);
				#endif  //boardUSE_OS
				#endif  //boardDCAC_EN
			}
			break;
			
			#if(boardENG_MODE_EN)
			case DS_ENG_MODE:
			{
				static bool b_twinkle_flag = false;
				if(b_twinkle_flag)
				{
					ledPWR_SW_ON();
					ledAC_SW_ON();
					ledUSB_SW_ON();
					ledLight_SW_ON();
					ledDC_SW_ON();
					b_twinkle_flag = false;
				}
				else 
				{
					ledPWR_SW_OFF();
					ledAC_SW_OFF();
					ledUSB_SW_OFF();
					ledLight_SW_OFF();
					ledDC_SW_OFF();
					b_twinkle_flag = true;
				}
				#if(boardUSE_OS)
				vTaskDelay(ledTASK_CYCLE_TIME);
				#endif  //boardUSE_OS
			}
			break;
			#endif	
		}
	}
}

/***********************************************************************************************************************
-----函数功能    电源指示灯呼吸
-----说明(备注)  IO模拟PWM  2mS调用
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
//static void v_led_breathing(void)
//{
//	static int pwmCounter = 0;
//    static int directionChangeCounter = 0;
//	
//	static int brightness = 0; // 当前亮度级别
//	static bool increase = true; // 标记是增加还是减少亮度

//    // 每2ms执行一次
//    pwmCounter++;
//    directionChangeCounter++;

//    if (pwmCounter < brightness) {
//        // 设置IO口为高电平
//        ledPWR_SW_ON();
//    } else {
//        // 设置IO口为低电平
//        ledPWR_SW_OFF();
//    }

//    if (pwmCounter >= 10) { // 假设最大亮度级别为100
//        pwmCounter = 0;
//    }

//    if (directionChangeCounter >= 30) { // 控制亮度变化速度
//        directionChangeCounter = 0;
//        if (increase) {
//            if (brightness < 10) {
//                brightness++; // 增加亮度
//            } else {
//                increase = false; // 开始减少亮度
//            }
//        } else {
//            if (brightness > 0) {
//                brightness--; // 减少亮度
//            } else {
//                increase = true; // 再次开始增加亮度
//            }
//        }
//    }
//}

/***********************************************************************************************************************
-----函数功能    电源指示灯呼吸
-----说明(备注)  50mS调用
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_led_breathing(void)
{
	static u8 breath_cnt = 0;
	static bool breath_flag = 0;
	
	ledPWR_SW_PWM_SET(breath_cnt * 50);
	
	if(breath_flag == 0) breath_cnt++;
	else 
	{
		if(breath_cnt) breath_cnt--;
	}
	
	if(breath_cnt >= 20)
		breath_flag = 1;
	else if(breath_cnt == 0)
		breath_flag = 0;
}

#endif  //boardLED_EN
