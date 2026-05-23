/*****************************************************************************************************************
*                                                                                                                *
 *                                         蜂鸣器任务                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "Buz/buz_task.h"
#if(boardBUZ_EN)
#include "Buz/buz_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "app_info.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN

//****************************************************局部定义**************************************************//
#define        	BUZ_ON()       						buzTIMER_PWM_SET(300);
#define        	BUZ_OFF()      						buzTIMER_PWM_SET(0);

//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define        	BUZ_TASK_PRIO                  		1   //任务优先级 
#define        	BUZ_TASK_STK_SIZE              		64   //任务堆栈  实际字节数 *4
TaskHandle_t    tBuzTaskHandler = NULL; 
void           	vBuz_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
static vs16 S_us_buz_num = 0;
static vu16 S_us_buz_on_time = 0;
static vu16 S_us_buz_off_time = 0;
static bool S_b_buz_tri_flag = false;

/***********************************************************************************************************************
-----函数功能    蜂鸣器任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void bBuz_TaskInit(void)
{
	vBuz_Init();
	
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vBuz_Task,				//任务函数
                (const char* )"BuzTask",              	//任务名称
                (uint16_t ) BUZ_TASK_STK_SIZE,          //任务堆栈大小
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) BUZ_TASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tBuzTaskHandler);      	//任务句柄
	#endif  //boardUSE_OS
}

/***********************************************************************************************************************
-----函数功能    蜂鸣器任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vBuz_Task(void *pvParameters)
{
	#if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
		#if(boardUSE_OS)
        if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == pdTRUE)        //等待任务通知,返回pdTRUE表示收到信号
		#endif  //boardUSE_OS
        {
            while(S_us_buz_num > 0)
            {
				if(!tAppMemParam.tSYS.bBuzSwitchOff) 
					BUZ_ON();
                vTaskDelay(S_us_buz_on_time);
                BUZ_OFF();
                vTaskDelay(S_us_buz_off_time);
                S_us_buz_num--;
            }
			if(S_us_buz_num == 0) S_b_buz_tri_flag = false;
        }
    }
}


/***********************************************************************************************************************
-----函数功能    记录蜂鸣器操作
-----说明(备注)  none
-----传入参数    buzzNum:响的次数
				 OnTime:响的时间
				 OffTime:间隔的时间
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void buz(u16 buzzNum, u16 OnTime, u16 OffTime)
{
    if(tBuzTaskHandler == NULL )
        return;

    if(!S_b_buz_tri_flag)
    {  
		S_us_buz_num = buzzNum;
		S_us_buz_on_time = OnTime;
		S_us_buz_off_time = OffTime;
        S_b_buz_tri_flag = true;
		
		#if(boardUSE_OS)
		xTaskNotifyGive(tBuzTaskHandler);
		#endif  //boardUSE_OS
    }
    
	#if(boardDISPLAY_EN)
	if(S_us_buz_num >= 3)       //报错蜂鸣器响的次数大于等于3次
		bDisp_Switch(ST_ON, false);
	#endif  //boardDISPLAY_EN
}


/***********************************************************************************************************************
-----函数功能    蜂鸣器响的类型
-----说明(备注)  none
-----传入参数    type:
				 Null = 0,
				 SHORT_1, //操作提示音(长按触发)
				 SHORT_2, //操作失败   
				 SHORT_3, //准备打开时的错误
				 LONG_1,  //重要操作提示:开关提示音
				 LONG_2,  //设备丢失:逆变器丢失
				 LONG_3,  //运行中的错误:运行导致的高低温报警,过压过流
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bBuz_Tweet(Buzz_E type)
{
	//sMyPrint("转载蜂鸣器次数为 = %d",type);
	
	switch(type)
	{
		case SHORT_1:
		{
			buz(1, 100, 0);
			break;
		}
		
		case SHORT_2:
		{
			buz(2, 100, 100);
			break;
		}
		
		case SHORT_3:
		{
			buz(3, 100, 100);
			break;
		}
		
		case LONG_1:
		{
			buz(1, 200, 0);
			break;
		}
		
		case LONG_2:
		{
			buz(2, 200, 200);
			break;
		}
		
		case LONG_3:
		{
			buz(3, 200, 200);
			break;
		}
		
		case LONG_5:
		{
			buz(5, 200, 200);
			break;
		}
		
		default:
			break;
	}
	return true;
}

#if(boardLOW_POWER)
/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:执行成功   false:执行失败
*****************************************************************************************************************/
void vBuz_EnterLowPower(void)
{
	vBuz_IoEnterLowPower();
	vTaskSuspend(tBuzTaskHandler);
}


/*****************************************************************************************************************
-----函数功能    退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:执行成功   false:执行失败
*****************************************************************************************************************/
void vBuz_ExitLowPower(void)
{
	BUZ_PWM_Init();
	vTaskResume(tBuzTaskHandler);
}
#endif  //boardLOW_POWER

#endif  //boardBUZ_EN



