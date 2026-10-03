/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application\Sys
 * File    : sys_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统总任务调度实现，负责系统任务轮询、状态机切换、周期节拍及运行参数管理
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task.h"
#include "Print/print_task.h"
#include "gpio_init.h"
#include "timer_task.h"

#if (boardENG_MODE_EN)
#include "eng_mode.h"
#endif  /* boardENG_MODE_EN */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			SYS_TASK_PRIO							2		/* 任务优先级 */
#define			SYS_TASK_STK_SIZE						256		/* 任务堆栈(字) */
TaskHandle_t  	tSysTaskHandler = NULL;							/* 任务句柄 */
void         	vSys_Task(void *pvParameters);					/* 任务函数 */
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) SysInfo_T tSysInfo;
static Task_T *p_task = NULL;

//****************************************************Function Declaration******************************************************//
static void v_task_param_init(void);


/***********************************************************************************************************************
 * 函数功能    : 系统任务初始化
 * 说明(备注)  : 初始化队列任务并在支持RTOS时创建总任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 初始化成功; 负数: 对应初始化步骤失败
 ************************************************************************************************************************/
s8 cSys_TaskInit(void)
{
	if (bSys_QueueInit() == false)
		return -1;
	
	v_task_param_init();
	
	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t)vSys_Task,               //任务函数
	                (const char*)"bSysTask",                 //任务名称
	                (uint16_t)SYS_TASK_STK_SIZE,             //任务堆栈大小
	                (void*)NULL,                            //传递给任务函数的参数
	                (UBaseType_t)SYS_TASK_PRIO,            //任务优先级
	                (TaskHandle_t*)&tSysTaskHandler) != pdPASS)       //任务句柄
		return -2;

	vQueue_BindTaskHandler(tpSysTask, tSysTaskHandler);
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 任务内部参数初始化
 * 说明(备注)  : 重置系统信息结构体及温度默认值
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_task_param_init(void)
{
	p_task = tpSysTask;

	//系统任务参数
	memset(&tSysInfo, 0, sizeof(tSysInfo));
	
	tSysInfo.sMaxTemp = 25;                   //设置默认最高温度
	tSysInfo.sMinTemp = 25;                   //设置默认最低温度
}

/***********************************************************************************************************************
 * 函数功能    : 系统总任务循环轮询
 * 说明(备注)  : 轮询队列任务周期事件
 * 传入参数    : pvParameters: 任务传入参数指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vSys_Task(void *pvParameters)
{
	#if (boardUSE_OS)
    for (;;)
	#endif  /* boardUSE_OS */
    {
		if (p_task == NULL)
		{
			v_task_param_init();
			
			#if (boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif  /* boardUSE_OS */
		}
		
		vQueue_TaskPoll(p_task, sysTASK_CYCLE_TIME);
    }
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 进入睡眠前的系统准备
 * 说明(备注)  : 低功耗勾子回调
 * 传入参数    : ulExpectedIdleTime: 预计空闲时间
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void PreSleepProcessing(uint32_t ulExpectedIdleTime)
{
	tSysInfo.bIntFlag = false;
}

/***********************************************************************************************************************
 * 函数功能    : 系统供电电源类型选择
 * 说明(备注)  : 建议50ms周期判定电池输入或5V供电状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vSys_PowerTypeSelect(void)
{
	//外接电池判定
    if (bAdc_CheckSysInputVolt() == VS_NORMAL)
        tSysInfo.ePowerType = SPT_BMS;
	else 
		tSysInfo.ePowerType = SPT_5V;
}
#endif  /* boardLOW_POWER */

/***********************************************************************************************************************
 * 函数功能    : 设置系统设备运行状态
 * 说明(备注)  : 控制辅助电源开关、输出调试日志及蜂鸣器提醒
 * 传入参数    : step: 目标状态代码(DevState_E); bz: 是否鸣叫蜂鸣器
 * 输出参数    : 无
 * 返回值      : true: 操作成功, false: 操作失败
 ************************************************************************************************************************/
bool bSys_SetDevState(DevState_E step, bool bz)
{
	if (tSysInfo.eDevState != step)
	{
		tSysInfo.eDevState = step;
		if (tSysInfo.eDevState == DS_INIT)  //初始化
		{
			gpioASSIST_OPEN_ON();
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为初始化\r\n");
		}
		else if (tSysInfo.eDevState == DS_CLOSING)  //关闭中
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为关闭中\r\n");
		}
		else if (tSysInfo.eDevState == DS_SHUT_DOWN)  //关闭
		{
			gpioASSIST_OPEN_OFF();
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为关闭\r\n");
		}
		else if (tSysInfo.eDevState == DS_ERR)  //错误
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为错误\r\n");
		}
		else if (tSysInfo.eDevState == DS_BOOTING)    //启动中
		{
			gpioASSIST_OPEN_ON();
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为启动中\r\n");
		}
		else if (tSysInfo.eDevState == DS_WORK)    //工作
		{
			if (uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:系统任务状态为工作\r\n");
		}
		#if (boardENG_MODE_EN)
		else if (tSysInfo.eDevState == DS_ENG_MODE)  //工程模式
		{
			vEng_ParamInit();
			bEng_SetEngStep(EMS_LCD);
		}
		#endif  /* boardENG_MODE_EN */
	}	
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 系统任务周期节拍处理
 * 说明(备注)  : 检测关机倒计时并在调试模式下打印当前系统队列任务ID
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vSys_TickTimer(void) 
{
	static u8 uc_cnt = 0;
	
	uc_cnt++;
	if (uc_cnt >= 2)
	{
		uc_cnt = 0;
		switch (tpSysTask->ucID)
		{
			case STI_INIT:  
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_INIT !\r\n");
			}
			break;
			
			case STI_ENTER_APP:  
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_ENTER_APP !\r\n");
			}
			break;
			
			case STI_RESET:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_RESET !\r\n");
			}
			break;
			
			case STI_ERR:  
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_ERR !\r\n");
			}
			break;
			
			#if (boardUPDATE)
			case STI_UPDATE:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_UPDATE !\r\n");
			}
			break;
			#endif  /* boardUPDATE */
			
			#if (boardDISPLAY_EN)
			case STI_DISPLAY:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_DISPLAY !\r\n");
			}
			break;
			#endif  /* boardDISPLAY_EN */
			
			#if (boardLOW_POWER)
			case STI_LOW_POWER:
			{
				if (uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = STI_LOW_POWER !\r\n");
			}
			break;
			#endif  /* boardLOW_POWER */
			
			default:
			{
			}
			break;
		}
	}

	//关机倒计时
	if (tSysInfo.usAutoOffTime) 
	{
		if (tSysInfo.usAutoOffCnt)
		{
			tSysInfo.usAutoOffCnt--;
			if (tSysInfo.usAutoOffCnt == 0) //倒计时为0进入
			{
				if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
					sMyPrint("Sys_Task:====倒计时结束,进入关机 时间=%dS====\r\n", tSysInfo.usAutoOffTime);
			}
		}	
	}
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 低功耗电源有效性检测
 * 说明(备注)  : 判断是否允许开启照明或Type-C应急输出
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 允许, false: 不允许
 ************************************************************************************************************************/
bool bSys_LowPowerExist(void)
{
	if (tAdc_SysSamp.usBMS_Vin >= boardBMS_MIN_VOLT || eMPPT_GetVinState() == MVS_NORMAL)
		return true;
	else 
		return false;
}
#endif  /* boardLOW_POWER */
