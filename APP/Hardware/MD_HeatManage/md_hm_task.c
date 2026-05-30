/*****************************************************************************************************************
*                                                                                                                *
 *                                         散热任务                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_HeatManage/md_hm_task.h"
#include "MD_HeatManage/md_hm_iface.h"
#include "freertos.h"
#include "task.h"
#include "Sys/sys_task.h"
#include "board_config.h"
#include "Print/print_task.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Mppt/md_mppt_task.h"

//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define        	HM_TASK_PRIO                  			1    //任务优先级 
#define        	HM_TASK_STK_SIZE              			64   //任务堆栈  实际字节数 *4
TaskHandle_t    tHeatManageHandler = NULL; 
void           	vHW_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
HM_T           tHM;

static bool b_fan_stop_to_run_flag=0;
static u8 uc_update_delay = 0;
static u16 Temper = 0;

//****************************************************函数声明****************************************************//
static void v_fan_pwm_set(u16 level);
static u16 us_fan_set_work_mode(FanWorkMode_E mode);


/*****************************************************************************************************************
-----函数功能    照明任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool bHM_TaskInit(void)
{
	vFan_PwmInit ();
	
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vHW_Task,              //任务函数
                (const char* )"bHeatManage",			//任务名称
                (uint16_t ) HM_TASK_STK_SIZE,          	//任务堆栈大小
                (void* )NULL,							//传递给任务函数的参数
                (UBaseType_t ) HM_TASK_PRIO,           	//任务优先级
                (TaskHandle_t*)&tHeatManageHandler);	//任务句柄
	#endif  //boardUSE_OS
				
	return true;
}

/*****************************************************************************************************************
-----函数功能    照明任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vHW_Task(void *pvParameters)
{
	bool b_open_fan_flag = false;
	
	#if(boardUSE_OS)
	for(;;)
	#endif  //boardUSE_OS
	{		
	    if(bSys_IsWorkState() == true  ||
		   tSysInfo.eDevState == DS_ERR)
		{
			//3S更新一次温度
			if(++uc_update_delay >= 3)
			{
				Temper = tSysInfo.sMaxTemp;
				uc_update_delay = 0;
			}
			
			if(tSysInfo.usOutPwr > 100)
				b_open_fan_flag = true;
			if(tSysInfo.usOutPwr < 50)
				b_open_fan_flag = false;
			
			if(Temper < 41 &&
			   (
				#if(boardDCAC_EN)
				tDcac.eChgState == IOS_WORK ||
				#endif  //boardDCAC_EN

				b_open_fan_flag == true))
			{
				Temper = 41;
			}

			#if(boardDCAC_EN)
			if(tDcacRx.usOutPwr > 1500)
				Temper = 55;
			#endif  //boardDCAC_EN
			
			switch (tHM.eWordMode)
			{
				default:
				case FWM_OFF:         //关闭
				{
					if(Temper > 40)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_1);
				}
				break;
			
				case FWM_GEAR_1:         //
				{
					if(Temper < 38)
						tHM.usValue = us_fan_set_work_mode(FWM_OFF);
					else if(Temper > 44)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_2);
				}
				break;
				
				case FWM_GEAR_2:         //
				{
					if(Temper < 42)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_1);
					else if(Temper > 48)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_3);
				}
				break;
				
				case FWM_GEAR_3:         //
				{
					if(Temper < 46)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_2);
					else if(Temper > 52)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_FULL);
				}
				break;
				
				case 4:         //
				{
					if(Temper < 50)
						tHM.usValue = us_fan_set_work_mode(FWM_GEAR_3);
				}
				break;
			}
			
			if((tHM.eWordMode < FWM_GEAR_2 && tHM.eWordMode > FWM_OFF)&&b_fan_stop_to_run_flag==0)  //风扇 从停止启动并低于三档
			{
				b_fan_stop_to_run_flag=1;
				tHM.usValue = us_fan_set_work_mode(FWM_GEAR_2);  //从第2档启动,避免启动不成功
			}

			v_fan_pwm_set(tHM.usValue);
		}
		else  //其他模式为关闭状态
		{
			if((tHM.eWordMode!=FWM_OFF)||(tHM.usValue != 0))  //散热打开时候关机或关机时候PWM值不为0
			{
				tHM.usValue = us_fan_set_work_mode(FWM_OFF);
				v_fan_pwm_set(tHM.usValue);
			}
		}
		
		#if(boardUSE_OS)
		vTaskDelay(1000);
		#endif  //boardUSE_OS
	}
}



/*****************************************************************************************************************
-----函数功能    照明设置PWM值
-----说明(备注)  none
-----传入参数    level:PWM值
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_fan_pwm_set(u16 level)    //无极输入 max为1000
{
    level = LIMIT_MAX(level, fanPWM_MAX_VALUE);
    fanPWM_SET(level);
	if(!level)
	{
		fanPWM_EN_OFF();
	}
	else
	{
		fanPWM_EN_ON();
	}
}


/*****************************************************************************************************************
-----函数功能    设置工作模式
-----说明(备注)  none
-----传入参数    level:PWM值
-----输出参数    none
-----返回值      对应模式的PWM值
******************************************************************************************************************/
static u16 us_fan_set_work_mode(FanWorkMode_E mode)
{
	u16 temp = 0;
	
	if(mode == FWM_GEAR_1)
	{
		temp = 200;
	}
	else if(mode == FWM_GEAR_2)
	{
		temp = 500;
	}
	else if(mode == FWM_GEAR_3)
	{
		temp = 800;
	}
	else if(mode == FWM_GEAR_FULL)
	{
		temp = 1000;
	}
	else 
	{
		temp = 0;
		b_fan_stop_to_run_flag = 0;
		mode = FWM_OFF;
	}
	
	tHM.eWordMode = mode;
	return temp;
}






















































/************************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/


/*****************************************************************************************************************
-----函数功能    获取照明灯的状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      FWM_OFF = 0,
				 FWM_GEAR_1,
				 FWM_GEAR_2,
				 FWM_GEAR_3,
				 FWM_GEAR_FULL,
******************************************************************************************************************/
FanWorkMode_E eFan_GetWorkMode(void) 
{
	return tHM.eWordMode;
}



/*****************************************************************************************************************
-----函数功能    强制打开风扇
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vFan_ForceOpenFan(bool en)
{
	if(en == true)
	{
		if(Temper < 41)
		{
			Temper = 41;
			uc_update_delay = 0;
		}
	}
	else 
	{
		Temper = 25;
	}
	
}


#if(boardLOW_POWER)
/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vFan_EnterLowPower(void) 
{
	vFan_IoEnterLowPower();
	vTaskSuspend(LED_HM_Task_Handler);
}


/*****************************************************************************************************************
-----函数功能    退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vFan_ExitLowPower(void) 
{
	vFan_PwmInit ();
	vTaskResume(LED_HM_Task_Handler);
}
#endif
