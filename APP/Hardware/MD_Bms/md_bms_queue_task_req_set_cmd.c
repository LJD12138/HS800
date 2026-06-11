/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_queue_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Print/print_task.h"

#define       	bmsTASK_SET_CMD_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//

/*****************************************************************************************************************
-----函数功能    任务函数:请求升级任务
-----说明(备注)  none
-----传入参数    BMS_ErrState_N:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_req_set_cmd(Task_T *tp_task)
{
	static tSysSetParam t_sys_set_param;
	
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(tp_task->tReplyBuff.buff == NULL)
			{
				if(uPrint.tFlag.bBmsTask)
					log_w("bBmsTask:任务返回参数缓存器异常");

				cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}

			//校验数据
			u8 len = lwrb_get_full(&tp_task->tReplyBuff);
			if(len != sizeof(t_sys_set_param) || tp_task->tReplyBuff.buff == NULL)
			{
				if(uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
					log_w("bBmsTask:BMS返回数据错误,长度%d", len);

				cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}
			
			//读取数据
			lwrb_read(&tp_task->tReplyBuff, (u8*)&t_sys_set_param, sizeof(t_sys_set_param));

			if(c_bms_cs_sys_set(&t_sys_set_param) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		break;
		
		case 1:
		{
			//等待1S
			tp_task->usStepWaitCnt++;
			if(tp_task->usStepWaitCnt < (1000 / bmsTASK_SET_CMD_CYCLE_TIME))
				break;
			
			if(c_bms_cs_sys_set(&t_sys_set_param) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		break;

		case 2:
		{
			if(t_sys_set_param.cmd == mainUPDATE_FLAG)
				cQueue_AddQueueTask(tpBmsTask, BTI_UPDATE, 0, false);

			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000 / bmsTASK_SET_CMD_CYCLE_TIME))  //等待超时
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_w("bBmsTask:设置指令任务超时,步骤%d", tp_task->ucStep);
	}
	
	#if(boardUSE_OS)
	vTaskDelay(bmsTASK_SET_CMD_CYCLE_TIME);
	#endif  //boardUSE_OS
}

#endif  //boardBMS_EN
