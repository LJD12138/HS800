/*!
    \file    main.c
    \brief   GPIO running led demo

    \version 2020-09-04, V2.0.0, demo for GD32F4xx
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.

****************************************************/

    /*                                             
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
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    佛祖保佑       永不宕机     永无BUG
***************************************************/

#include "main.h"
#include "..\..\BOOT\Application\flash_allot_table.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board_config.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  //boardPRINT_IFACE

//****************************************************局部宏定义**************************************************//
//APP中断向量表地址偏移 需要在Linker选项卡勾选“USE Memory Layout from Target Dialog”，
//并修改Target选项卡中的ROM的Start地址

#define 		USER_BOOT_EXIST				         	1			         		//是否有bootloader
#if(USER_BOOT_EXIST)
#define 		NVIC_VECTTAB_RAM1                   	((uint32_t)SRAM_START) 		//RAM首地址
#define 		NVIC_VECTTAB_FLASH1                 	((uint32_t)flashAPP_START) 	//Flash首地址
#define 		VECT_TAB_OFFSET				         	flashAPP_START      		//偏移量
#endif	//USER_BOOT_EXIST

//****************************************************任务初始化**************************************************//
TaskHandle_t    StartTask_Handler; 
#define      	START_TASK_PRIO             			1                   		//任务优先级 
void vBoard_StartTask(void *pvParameters);

/*****************************************************************************************************************
-----函数功能    中断初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void nvic_init(void)
{
     /* configure 4 bits pre-emption priority */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
}


/*****************************************************************************************************************
-----函数功能    APP主入口
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
int main(void)
{
	SystemInit();
	
	#if(USER_BOOT_EXIST == 1)
	nvic_vector_table_set(NVIC_VECTTAB_FLASH1, VECT_TAB_OFFSET);  //设置NVIC中断向量表的偏移
	__enable_irq();	//解除中断屏蔽
	#endif	//USER_BOOT_EXIST
	
	nvic_init();        //中断初始化
	
	vBoard_SysInit();
	
    #if		printSEGGER
    SEGGER_RTT_printf(0,"------------------APP OK--------------------!\r\n");
    #endif	//printSEGGER
	
	//创建开始任务
    xTaskCreate((TaskFunction_t )vBoard_StartTask,		//任务函数
                (const char* )"StartTask",          	//任务名称
                (uint16_t ) 512,                     	//任务堆栈大小
                (void* )NULL,                       	//传递给任务函数的参数
                (UBaseType_t ) START_TASK_PRIO,     	//任务优先级
                (TaskHandle_t*)&StartTask_Handler); 	//任务句柄
    
    vTaskStartScheduler(); 

    while(1) 
	{
		
    }
}
