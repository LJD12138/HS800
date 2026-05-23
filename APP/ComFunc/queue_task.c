/*****************************************************************************************************************
*                                                                                                                *
 *                                         队列任务                                                            *
*                                                                                                                *
******************************************************************************************************************/
#include "queue_task.h"
#include "check.h"
#include "board_config.h"


#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif


/*****************************************************************************************************************
-----函数功能	队列任务参数化
-----说明(备注)	none
-----传入参数	none
-----输出参数	none
-----返回值		小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cQueue_TaskInit(Task_T** task,
	u16 task_queue_size,
	u16 reply_buff_size,
	bpTaskManageFunc mfunc,
	vpAddTaskReturnFunc rFunc)
{
	s8 result = 1;
	
	if (mfunc == NULL || 
		task_queue_size == 0 ||
		task_queue_size > 100) 
		return -1;
	
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	
	// 动态分配内存，数组大小在编译时由task_queue_size和reply_buff_size确定
    size_t total_size = sizeof(Task_T) + task_queue_size * 2 + reply_buff_size;
	#if(boardUSE_OS)
    *task = (Task_T *)pvPortMalloc(total_size);
    #else
	*task = (Task_T *)malloc(total_size);
	#endif
    if (*task != NULL) 
	{
		(*task)->ucID = 0;
		(*task)->ucStep = 0;
		(*task)->usInParam = 0;
		(*task)->usStepWaitCnt = 0;
		(*task)->usStepRepeatCnt = 0;
		(*task)->usTaskWaitCnt = 0;
		(*task)->bp_task_manage_func = mfunc;
		(*task)->vp_func = NULL;
		(*task)->vp_return_func = rFunc;
		(*task)->bNowRun = false;
		(*task)->tQueueBuff.buff = NULL;
		(*task)->tReplyBuff.buff = NULL;
		
		// 初始化任务队列缓存器（使用柔性数组的前部分）
		lwrb_init(&(*task)->tQueueBuff, &(*task)->uac_buff[0], task_queue_size * 2);
		lwrb_reset(&(*task)->tQueueBuff);
		
		// 初始化回复缓存器（使用柔性数组的后部分）
		if(reply_buff_size)
		{
			lwrb_init(&(*task)->tReplyBuff, &(*task)->uac_buff[task_queue_size * 2], reply_buff_size);
			lwrb_reset(&(*task)->tReplyBuff);
		}
    }
	else
	{
		#if(boardUSE_OS)
		vPortFree((*task));
		#else
		free((*task));
		#endif
		result =  -2;
	}
	
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
	
	return result;
}


/*****************************************************************************************************************
-----函数功能    让任务执行到下个步骤
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:进行了任务调度   反之没有
******************************************************************************************************************/
s8 cQueue_GotoStep(Task_T* task, u8 toStep)
{
	s8 result = 0;
	if(task == NULL)
		return -1;
	
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	
	task->usStepWaitCnt = 0;
	task->usStepRepeatCnt = 0;
    switch(toStep)
    {
		case STEP_FORWARD:
		{
			if(task->vp_return_func != NULL)
				task->vp_return_func(task, STEP_FORWARD);
			
			if(task->ucStep > 0)
				task->ucStep--;
		}
        break;
		
        case STEP_NEXT:
		{
			if(task->vp_return_func != NULL)
				task->vp_return_func(task, STEP_NEXT);
			
            task->ucStep++;
		}
        break;
        
        case STEP_END:
		{
            task->ucStep = STEP_END; //总步骤数量
		}
        break;
        
        default:
		{
			if(task->vp_return_func != NULL)
				task->vp_return_func(task, 10);
			
            task->ucStep = toStep;
		}
        break;
    }
    
    if(task->ucStep >= STEP_END)       //步骤执行完毕
	{
		if(task->vp_return_func != NULL)
			task->vp_return_func(task, STEP_END);
		
		task ->bNowRun = true;
		result = 1;
	}
	
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
	
	return result;
}


/*****************************************************************************************************************
-----函数功能    添加队列任务
-----说明(备注)  往任务队列里面添加数据  添加后任务自动启动
-----传入参数    none
-----输出参数    none
-----返回值      小于0:有错误   等于0:没操作    大于0:操作成功 
******************************************************************************************************************/
s8 cQueue_AddQueueTask(Task_T* task, u8 task_id, u16 in_param, bool now_run)     
{    
	u8 uca_write_array[3] = {0};
	
	if(task == NULL)
		return -2;
	
	if(task->ucID == task_id && task->usInParam == in_param)
		return 0;
	
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	
	//立刻运行
	if(now_run)
	{    
		lwrb_reset(&task->tQueueBuff);
		
		if(task->vp_return_func != NULL)
			task->vp_return_func(task, 0);
	}
	
	uca_write_array[0] = task_id;
	memcpy(&uca_write_array[1], (u8*)&in_param, 2);
	
	//查重
	u16 us_len = lwrb_get_full(&task->tQueueBuff);
	if(us_len != 0)
	{
		//任务队列长度异常
		if(us_len%3 != 0)
		{
			lwrb_reset(&task->tQueueBuff);
			{
				#if(boardUSE_OS)
				taskEXIT_CRITICAL();
				#endif
				return -3;
			}
		}
		
		lwrb_sz_t found_idx;
		if(us_len > 3)
		{
			//存在相同,不写入
			if(lwrb_find(&task->tQueueBuff, uca_write_array, 3, (us_len - 3), &found_idx) > 0)
			{
				#if(boardUSE_OS)
				taskEXIT_CRITICAL();
				#endif
				return 0;
			}
		}
		else 
		{
			//存在相同,不写入
			if(lwrb_find(&task->tQueueBuff, uca_write_array, 3, 0, &found_idx) > 0)
			{
				#if(boardUSE_OS)
				taskEXIT_CRITICAL();
				#endif
				return 0;
			}
		}
	}	
	
	//写入队列
	lwrb_write(&task->tQueueBuff, uca_write_array, 3);
	
	if(now_run || task->ucID == 0)
	{
		if(task->vp_return_func != NULL)
			task->vp_return_func(task, 1);
		
		task ->bNowRun = true;
	}

	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
	
	if(task->vp_return_func != NULL)
		task->vp_return_func(task, 2);
	
	if(now_run)
		return 2;
	else
		return 1;
}

/*****************************************************************************************************************
-----函数功能    重置任务队列
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:成功, 反之失败
******************************************************************************************************************/
bool bQueue_Reset(Task_T* task)
{
	if(task == NULL)
		return false;
	
	lwrb_reset(&task->tQueueBuff);
	return true;
}
