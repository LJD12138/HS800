/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_queue_task.h"

#if(boardBMS_EN && boardUPDATE)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "MD_Bms/md_bms_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"

#define       	bmsTASK_UPDATE_TIME               		50

//****************************************************函数声明****************************************************//

	
/*****************************************************************************************************************
-----函数功能    任务函数:更新
-----说明(备注)  none
-----传入参数    BMS_ErrState_N:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_update(Task_T *tp_task)
{
	//退出升级模式
	if(tSysInfo.eDevState != DS_UPDATE_MODE)
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
		return;
	}		
	
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(tBms.eDevState != DS_UPDATE_MODE)
				bBms_SetDevState(DS_UPDATE_MODE);

			if(tp_task->tReplyBuff.buff == NULL)
			{
				if(uPrint.tFlag.bBmsTask)
					log_w("bBmsTask:任务返回参数缓存器异常");

				cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}
			
			cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		
		//开始透传
		case 1:
		{
			//校验数据
			u16 len = lwrb_get_full(&tp_task->tReplyBuff);
			if(len == 0)
				break;

			//长度超过buff
			u8 uca_buff[256] = {0};
			if(len > sizeof(uca_buff))
			{
				cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}

			//读取数据
			lwrb_read(&tp_task->tReplyBuff, uca_buff, len);
			
			//开始传输
			bBms_DataSendStart(uca_buff, len);
		}
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, bmsTASK_UPDATE_TIME);
	#endif  //boardUSE_OS
}
#endif  //boardBMS_EN  && boardUPDATE
