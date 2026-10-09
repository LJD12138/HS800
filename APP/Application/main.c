/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : main.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 应用程序主入口，负责时钟中断初始化、向量表偏移配置及RTOS启动任务创建
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc

                   _ooOoo_
                  o8888888o
                  88" . "88
                  (| -_- |)
                  O\  =  /O
               ____/`---'\____
            .'  \\|     |//  `.
            /  \\|||  :  |||//  \
           /  _||||| -:- |||||-  \
           |   | \\\  -  /// |   |
           | \_|  ''\---/''  |   |
           \  .-\__  `-`  ___/-. /
         ___`. .'  /--.--\  `. . __
      ."" '<  `.___\_<|>_/___.'  >'"".
     | | :  `- \`.;`\ _ /`;.`/ - ` : | |
     \  \ `-.   \_ __\ /__ _/   .-` /  /
======`-.____`-.___\_____/___.-`____.-'======
                   `=---='
===============================================================
    佛祖保佑       永不宕机     永无BUG
===============================================================
*/

//****************************************************Includes******************************************************************//
#include "main.h"
#include "..\..\BOOT\Application\flash_allot_table.h"
#include "freertos.h"
#include "task.h"

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#elif (boardSEGGER)
#include "SEGGER_RTT.h"
#endif  //boardPRINT_IFACE

#if (boardWDGT_EN)
#include "fwdgt.h"
#endif  //boardWDGT_EN



//****************************************************Macros********************************************************************//
//APP中断向量表地址偏移
#define			USER_BOOT_EXIST							1		//是否有bootloader
#if (USER_BOOT_EXIST)
#define			VECT_TAB_OFFSET							((uint32_t)(flashAPP_START - FLASH_BASE))	//APP中断向量表相对Flash基址偏移量
#endif  //USER_BOOT_EXIST

#define			START_TASK_PRIO							1		//任务优先级

//****************************************************Parameter Initialization**************************************************//
static TaskHandle_t s_start_task_handler;

//****************************************************Function Declaration******************************************************//
void vBoard_StartTask(void *pvParameters);
void vBoard_SysInit(void);

/***********************************************************************************************************************
 * 函数功能    : 中断优先级分组初始化
 * 说明(备注)  : 配置 4 位抢占优先级，0 位响应优先级
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void nvic_init(void)
{
	/* configure 4 bits pre-emption priority */
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
}

/***********************************************************************************************************************
 * 函数功能    : 应用程序主入口
 * 说明(备注)  : 完成系统时钟、向量表偏移重定位、板级外设初始化，并创建 FreeRTOS 根任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : int
 ************************************************************************************************************************/
int main(void)
{
	SystemInit();

	#if (USER_BOOT_EXIST == 1)
	nvic_vector_table_set(NVIC_VECTTAB_FLASH, VECT_TAB_OFFSET);  //设置NVIC中断向量表的偏移
	__enable_irq(); //解除中断屏蔽
	#endif  //USER_BOOT_EXIST == 1

	nvic_init();        //中断初始化

	vBoard_SysInit();

	#if (boardWDGT_EN)
	vFwdgt_PrintResetReason();
	#endif  //boardWDGT_EN

	#if (boardSEGGER)
	SEGGER_RTT_printf(0, "------------------APP OK--------------------!\r\n");
	#endif  //boardSEGGER

	//创建开始任务
	if (xTaskCreate((TaskFunction_t)vBoard_StartTask,  //任务函数
	            (const char *)"StartTask",          //任务名称
	            (uint16_t)512,                     //任务堆栈大小
	            (void *)NULL,                      //传递给任务函数的参数
	            (UBaseType_t)START_TASK_PRIO,      //任务优先级
	            (TaskHandle_t *)&s_start_task_handler) != pdPASS) //任务句柄
	{
		/* 根任务创建失败(堆耗尽),复位重试,避免无人喂狗前的静默挂死 */
		NVIC_SystemReset();
	}

	vTaskStartScheduler();

	while (1)
	{
	}
}
