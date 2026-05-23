/*****************************************************************************************************************
*                                                                                                                *
 *                                         指示灯处理任务                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "Led/led_task.h"

#if(boardLED_EN)
#include "Led/led_iface.h"
#include "Sys/sys_task.h"
#include "Update/update_main.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

#define     	ledTASK_CYCLE_TIME                		10  //任务时间

//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define        	LED_TASK_PRIO                 			1     //任务优先级 
#define        	LED_TASK_STK_SIZE              			64   //任务堆栈  实际字节数 *4
#define        	LED_TASK_CYCLE_TIME            			1000   //任务更新时间
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
	static int l_delay_cnt = 0;
	
	if(tpSysTask == NULL)
		return;
	
//	led_1_GPIO_OFF();
	switch (tpSysTask->ucID)
	{
		case STI_INIT:
		{
			ledPWR_SW_OFF();
		}break;
		
		case STI_ENTER_APP:
		{
//			led_1_GPIO_ON();
			ledPWR_SW_OFF();
		}break;
		
		case STI_ERR:
		case STI_RESET:
		{
		   l_delay_cnt++;
			if(l_delay_cnt < (200/ledTASK_CYCLE_TIME))//闪烁
			{
				ledPWR_SW_ON();
			}
			else if(l_delay_cnt < (400/ledTASK_CYCLE_TIME))
			{
				ledPWR_SW_OFF();
			}
			else if(l_delay_cnt >= (400/ledTASK_CYCLE_TIME))
			{
				l_delay_cnt = 0;
			}
		}break;
		
		#if(boardUPDATE)
		case STI_UPDATE:
		{
			l_delay_cnt++;
			if(l_delay_cnt > 0)//快闪
			{
				l_delay_cnt = 0;
				v_led_breathing();
			}
		}break;
		#endif
		
		#if(boardDISPLAY_EN)
		case STI_DISPLAY:
		{
			l_delay_cnt++;
			if(l_delay_cnt > (200/ledTASK_CYCLE_TIME))//慢闪
			{
				l_delay_cnt = 0;
				v_led_breathing();
			}
		}break;
		#endif
		
		#if(boardLOW_POWER)
		case STI_LOW_POWER:
        {  
			led_1_GPIO_OFF();
			ledPWR_SW_OFF();
        }
        break;
		#endif
		
		default:
			break;
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
