/*****************************************************************************************************************
*                                                                                                                *
 *                                         照明灯任务                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Light/md_light_task.h"

#if(boardLIGHT_EN)
#include "MD_Light/md_light_iface.h"
#include "Buz/buz_task.h"
#include "Sys/sys_task.h"

#include "MD_Display/md_display_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define        	LIGHT_TASK_PRIO                  		1   		//任务优先级 
#define        	LIGHT_TASK_STK_SIZE              		64   		//任务堆栈  实际字节数 *4
TaskHandle_t    tLightTaskHandler = NULL; 
void           	vLight_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
Light_T         tLight;

//****************************************************函数声明****************************************************//
static void v_light_pwm_set(u16 level);
static void v_light_set_state(LightWorkMode_E mode);


/*****************************************************************************************************************
-----函数功能    照明任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vLight_TaskInit(void)
{
	vLight_IfaceInit();
	
	v_light_set_state(LWM_OFF);
	
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vLight_Task,				//任务函数
                (const char* )"LightTask",					//任务名称
                (uint16_t ) LIGHT_TASK_STK_SIZE,			//任务堆栈大小
                (void* )NULL,								//传递给任务函数的参数
                (UBaseType_t ) LIGHT_TASK_PRIO,           	//任务优先级
                (TaskHandle_t*)&tLightTaskHandler);      	//任务句柄
	#endif  //boardUSE_OS
}

/*****************************************************************************************************************
-----函数功能    照明任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vLight_Task(void *pvParameters)
{
	#if(lightSIMPLE_MODE)
	static u8 num1=0,num2=0;
	static u8 value1=2,value2=4;
	#endif //lightSIMPLE_MODE
	 
	for(;;)
	{		
	    if(tSysInfo.uPerm.tPerm.bDisChgPerm == true)
		{
//			lightPWM_EN_ON();
			if(tLight.eWordMode==LWM_HALF)
			{
				tLight.usPower = 2;
				#if(lightSIMPLE_MODE)
				num1=0;num2=0;
				#endif //lightSIMPLE_MODE
			}
			else if(tLight.eWordMode==LWM_FULL)
			{
				tLight.usPower = 4;
				#if(lightSIMPLE_MODE)
				num1=0;num2=0;
				#endif //lightSIMPLE_MODE
			}
			
			#if(lightSIMPLE_MODE)
			else if(tLight.eWordMode==LWM_SOS)
			{
				tLight.usPower = 2;
				num1=0;
				if(num2++==0)
					tLight.usValue=lightPWM_FULL_VALUE;
				else if(num2==value1*1)
					tLight.usValue=0;
				else if(num2==(value1*2))
					tLight.usValue=lightPWM_FULL_VALUE;
				else if(num2==(value1*3))
					tLight.usValue=0;
				else if(num2==(value1*4))
					tLight.usValue=lightPWM_FULL_VALUE;
				else if(num2==(value1*5))
					tLight.usValue=0;
				else if(num2==(value2*6))
					tLight.usValue=lightPWM_FULL_VALUE;
				else if(num2==(value2*7))
					tLight.usValue=0;
				else if(num2==(value2*8))
					tLight.usValue=lightPWM_FULL_VALUE;
				else if(num2==(value2*9))
					tLight.usValue=0;
				else if(num2==(value2*10))
					tLight.usValue=lightPWM_FULL_VALUE;
				else if(num2==(value2*11))
					tLight.usValue=0;
				else if(num2==(value2*12))
				{
					tLight.usValue=lightPWM_FULL_VALUE;
					num2=0;
				}
					
			}
			else if(tLight.eWordMode==LWM_TWINKLE)
			{
				tLight.usPower = 2;
				num2=0;
				if(num1==0)
				{
					tLight.usValue=lightPWM_FULL_VALUE;
					num1=1;
				}
				else
				{
					tLight.usValue=0;
					num1=0;
				}
			}
			else
			{
				tLight.usValue=0;
				tLight.usPower = 0;
			}
			#endif //lightSIMPLE_MODE
			
			v_light_pwm_set(tLight.usValue);
		}
		else  //其他模式为关闭状态
		{
//			lightPWM_EN_ON();
			tLight.usPower = 0;
			if(tLight.eWordMode!=LWM_OFF)  //照明灯开的情况下关闭,照明灯也会关闭
			{
				tLight.usLastValue=tLight.usValue;
				v_light_set_state(LWM_OFF);
				v_light_pwm_set(tLight.usValue);
			}				 
		}
		vTaskDelay(100);
	}
	
}

/*****************************************************************************************************************
-----函数功能    照明设置状态
-----说明(备注)  none
-----传入参数       LS_OFF = 0,
					LS_HALF,
					LS_FULL,
					LS_SOS,
					LS_TWINKLE,
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_light_set_state(LightWorkMode_E mode) 
{
	if(mode == LWM_OFF)  //关闭
	{
		timer_disable(lightTIMER);
		tLight.eDevState = DS_SHUT_DOWN;
	}
	else                 //打开
	{
		timer_enable(lightTIMER);
		#if (boardIC_TYPE == boardIC_GD32F50X)
		timer_primary_output_config(lightTIMER, ENABLE);
		timer_channel_primary_output_config(lightTIMER, lightTIMER_CH, ENABLE);
		#endif
		tLight.eDevState = DS_WORK;
	}
	
    switch(mode)
	{
		case LWM_HALF:
		{
			tLight.usValue=lightPWM_SEMI_VALUE;
		}
		break;
		
		case LWM_FULL:
		{
			tLight.usValue=lightPWM_FULL_VALUE;
		}
		break;
		
		#if(lightSIMPLE_MODE)
		case LWM_SOS:
		{
			tLight.usValue=0;
		}
		break;
		
		case LWM_TWINKLE:
		{
			tLight.usValue=0;
		}
		break;
		#endif
		
		case LWM_OFF:
		default:
			tLight.usValue=0;
			
			break;
	}
	
	tLight.eWordMode = mode;
}


/*****************************************************************************************************************
-----函数功能    照明设置PWM值
-----说明(备注)  none
-----传入参数    level:PWM值
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_light_pwm_set(u16 level)    //无极输入 max为1000
{
    level = LIMIT_MAX(level, lightPWM_MAX_VALUE);
    lightPWM_SET(level);
}






























































/************************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/



/*****************************************************************************************************************
-----函数功能    照明的开关
-----说明(备注)  none
-----传入参数    
				 LST_NULL=0,//进行取反
				 LST_ON,
				 LST_OFF,
-----输出参数    none
-----返回值      执行结果 true:成功  false失败
******************************************************************************************************************/
bool bLight_Switch(SwitchType_E type)   // 长按 ON/OFF
{
	switch(type)
	{
		case ST_ON:
		{
			if(tLight.eDevState == DS_WORK)
				return true;
			goto Loop1;
		}
		
		case ST_OFF:
		{
			if(tLight.eDevState == DS_SHUT_DOWN)
				return true;
			goto Loop2;
		}
		
		default:
			if(tLight.eDevState == DS_SHUT_DOWN)  //开灯
			{ 
				Loop1:
				
				if(tSysInfo.uPerm.tPerm.bDisChgPerm == false)
				{
					#if(boardBUZ_EN)
					bBuz_Tweet(SHORT_2);
					#endif  //boardBUZ_EN

					return false;
				}
				
				tLight.usValue=tLight.usLastValue;   //取出上一次关闭前的值
				v_light_set_state(LWM_HALF);
			}
			else                                                   //关灯
			{
				Loop2:
				tLight.usLastValue=tLight.usValue;
				v_light_set_state(LWM_OFF);
			}
			
			break;
	}
	
	#if(boardSYS_DATA_UPADATA)
	Sys_Update_Element(AT_LIGHT_SWITCH_ADDR,tLight.eWordMode,true,true);   //打开照明ICON
	#endif
	
	#if(boardBUZ_EN)
	bBuz_Tweet(LONG_1);
	#endif  //boardBUZ_EN

	return true ;
}

/*****************************************************************************************************************
-----函数功能    照明模式选择
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vLight_CircSelectMode(void) 
{
	//关闭状态下需要长按启动
	if(tLight.eDevState != DS_WORK) return ;  
	
	//获取当前状态
	LightWorkMode_E mode = tLight.eWordMode;  
	
	//循环选择状态
	mode++;
	#if(lightSIMPLE_MODE)
	if(mode>LWM_TWINKLE)
		mode=LWM_OFF;
	#else 
	if(mode>LWM_FULL)
		mode=LWM_OFF;
	#endif  //lightSIMPLE_MODE
	
	v_light_set_state(mode);
	
	#if(boardBUZ_EN)
	bBuz_Tweet(LONG_1);
	#endif  //boardBUZ_EN
	
	//打开照明元素
	#if(boardSYS_DATA_UPADATA)
	Sys_Update_Element(AT_LIGHT_SWITCH_ADDR,tLight.eWordMode,true,true);
    #endif	
}


#if(boardLOW_POWER)
/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vLight_EnterLowPower(void) 
{
	vLight_IoEnterLowPower();
	vTaskSuspend(LIGHT_Task_Handler);
}


/*****************************************************************************************************************
-----函数功能    退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vLight_ExitLowPower(void) 
{
	vLight_IfaceInit ();
	vTaskResume(LIGHT_Task_Handler);
}
#endif  //boardLOW_POWER

#endif  //boardLIGHT_EN
