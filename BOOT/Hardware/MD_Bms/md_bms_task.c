/*****************************************************************************************************************
*                                                                                                                *
 *                                         电池包发送任务                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_queue_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE

//****************************************************任务参数初始化**********************************************//
#if(boardUSE_OS)
#define        	BMS_TASK_PRIO                         	2                       	//任务优先级 
#define        	BMS_TASK_SIZE                         	256                      	//任务堆栈  实际字节数 *4
TaskHandle_t    tBmsTaskHandler = NULL; 
void            vBms_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
//结构体
__ALIGNED(4)	Bms_T tBms;			//任务
static Task_T	*tp_task = NULL;


//****************************************************函数声明****************************************************//


/*****************************************************************************************************************
-----函数功能    电池包任务参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_bms_task_param_init(void)
{
	if(tpBmsTask == NULL)
		return false;
	
	memset(&tBms, 0, sizeof(tBms));
	
	lwrb_reset(&tpBmsTask->tQueueBuff);
	
	tp_task = tpBmsTask;
	
	return true;
}


/*****************************************************************************************************************
-----函数功能    电池包任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool bBms_TaskInit(void)
{ 
	//发送协议初始化
	if(bBms_SendProtInit() == false)
		return false;
	
	//任务队列初始化
	if(bBms_QueueInit() == false)
		return false;
	
	//任务参数初始化
	if(b_bms_task_param_init() == false)
		return false;
	
	//任务初始化
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vBms_Task,            	//任务函数
                (const char* )"BmsTask",              	//任务名称
                (uint16_t ) BMS_TASK_SIZE,              //任务堆栈大小
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) BMS_TASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tBmsTaskHandler);       //任务句柄
	#endif  //boardUSE_OS
				
	return true;
}



/*****************************************************************************************************************
-----函数功能    电池包任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vBms_Task(void *pvParameters)
{
	#if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
		if(tp_task == NULL
			#if(boardUPDATE)
			|| (tSysInfo.eDevState == DS_UPDATE_MODE)
			#endif  //boardUPDATE
		)
		{
			if(tp_task == NULL)
				b_bms_task_param_init();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif  //boardUSE_OS
		}
		
		if(tp_task->vp_func != NULL && tp_task ->bNowRun == false)
			tp_task->vp_func(tp_task);
		else if(tp_task->vp_func == NULL || tp_task ->bNowRun == true)
		{
			#if(boardUSE_OS)
			if(lwrb_get_full(&tp_task->tQueueBuff) == 0)
				ulTaskNotifyTake(pdFALSE, bmsTASK_CYCLE_TIME);//pdFALSE:任务通知多少次就执行多少次
			#endif  //boardUSE_OS
			
			if(tp_task->bp_task_manage_func != NULL)
				tp_task->bp_task_manage_func(tp_task);
		}
    }
}

#endif  //boardBMS_EN
