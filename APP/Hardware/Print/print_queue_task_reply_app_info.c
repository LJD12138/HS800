/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_queue_task.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "Print/print_task.h"

#define       	printTASK_APP_INFO_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//

	
/*****************************************************************************************************************
-----函数功能    任务函数:任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_print_queue_task_reply_app_info(Task_T *tp_task)
{
	u8 us_char_len = tp_task->usInParam;

	__ALIGNED(4) u8 uca_buff[256] = {0};
	
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(tp_task->tReplyBuff.buff == NULL)
			{
				cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}

			//校验数据
			u8 len = lwrb_get_full(&tp_task->tReplyBuff);
			if(len != us_char_len || tp_task->tReplyBuff.buff == NULL)
			{
				cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}
			
			//读取数据
			lwrb_read(&tp_task->tReplyBuff, (u8*)&uca_buff, len);

			if(c_relay_bms_app_info(uca_buff, us_char_len) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else
				break;
		}

		case 2:
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000 / printTASK_APP_INFO_CYCLE_TIME))  //等待超时
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, printTASK_APP_INFO_CYCLE_TIME);
	#endif  //boardUSE_OS
}
#endif  //boardPRINT_IFACE
