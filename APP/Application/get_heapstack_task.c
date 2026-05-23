/*****************************************************************************************************************
*                                                                                                                *
 *                                         堆栈获取任务                                                         *
*                                                                                                                *
******************************************************************************************************************/
#include "get_heapstack_task.h"
#include "freertos.h"
#include "task.h"



//****************************************************任务初始化**************************************************//
#define         GET_HEAPSTACK_TASK_PRIO                  1     //任务优先级 
#define         GET_HEAPSTACK_TASK_SIZE                  64   //任务堆栈  实际字节数 *4
TaskHandle_t    Get_HeapStack_Task_Handler = NULL; 
void            get_heapstack_task(void *pvParameters);



/***********************************************************************************************************************
-----函数功能    按键任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void KEY_TaskInit(void)
{
    xTaskCreate((TaskFunction_t )get_heapstack_task,              //任务函数 (1)
                (const char* )"HeapStackTask",                    //任务名称
                (uint16_t ) GET_HEAPSTACK_TASK_SIZE,              //任务堆栈大小
                (void* )NULL,                                     //传递给任务函数的参数
                (UBaseType_t ) GET_HEAPSTACK_TASK_PRIO,           //任务优先级
                (TaskHandle_t*)&Get_HeapStack_Task_Handler);      //任务句柄
}


void get_heapstack_task(void *pvParameters)
{
	for(;;)
	{
		
		vTaskDelay(2000);
	}
}










