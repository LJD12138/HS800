/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Mppt/md_mppt_queue_task.h"

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"


#define       	mpptTASK_SET_PWR_CYCLE_TIME               		100


//****************************************************函数声明****************************************************//


/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_mppt_queue_task_set_chg_pwr(Task_T *tp_task)
{
	vu16 us_chg_pwr = tp_task->usInParam;
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(us_chg_pwr && tMppt.eDevState != DS_WORK)
				bMppt_SetDevState(DS_BOOTING);
			else if((us_chg_pwr == 0) && tMppt.eDevState != DS_SHUT_DOWN)
				bMppt_SetDevState(DS_CLOSING);
				

			if(c_mppt_cs_set_pwr(us_chg_pwr) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else//发送函数要单独计数
			{
				tp_task->usStepRepeatCnt++;
				if(tp_task->usStepRepeatCnt >= 2)
				{
					if(uPrint.tFlag.bMpptTask)
						log_w("bMpptTask:发送失败次数过多,退出设置充电功率任务");
					
					cQueue_GotoStep(tp_task, STEP_END);  //结束
				}
				break;
			}
		}
		
		case 1:
		{
			tp_task->usStepWaitCnt++;
			if(tp_task->usStepWaitCnt >= (1000 / mpptTASK_SET_PWR_CYCLE_TIME))
			{
				if(uPrint.tFlag.bMpptTask)
					log_w("bMpptTask:等待设置充电功率回复超时");
				
				cQueue_GotoStep(tp_task, STEP_FORWARD);
				break;
			}

			if(tp_task->tReplyBuff.buff == NULL)
			{
				if(uPrint.tFlag.bMpptTask)
					log_w("bMpptTask:任务返回参数缓存器异常");

				cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}

			#pragma pack (1)   //强制进行1字节对齐
			struct
			{
//				u8 uc_obj;
				u16 us_in_pwr;
			}t_mppt_chg;
			#pragma pack() //取消一个字节对齐

			//校验数据
			u8 len = lwrb_get_full(&tp_task->tReplyBuff);
			if(len != sizeof(t_mppt_chg) || tp_task->tReplyBuff.buff == NULL)
			{
				if(uPrint.tFlag.bMpptTask && uPrint.tFlag.bImportant)
					log_w("bMpptTask:返回数据错误,长度%d", len);
				cQueue_GotoStep(tp_task, STEP_FORWARD);
				break;
			}
			
			//读取数据
			lwrb_read(&tp_task->tReplyBuff, (u8*)&t_mppt_chg, sizeof(t_mppt_chg));
			
			//校验数据
//			if(t_mppt_chg.uc_obj != 0)
//			{
//				if(uPrint.tFlag.bMpptTask && uPrint.tFlag.bImportant)
//					log_w("bMpptTask:返回对象%d错误", t_mppt_chg.uc_obj);
//				cQueue_GotoStep(tp_task, STEP_FORWARD);
//				break;
//			}

			if(t_mppt_chg.us_in_pwr != us_chg_pwr)
			{
				if(uPrint.tFlag.bMpptTask)
					log_w("bMpptTask:回复功率%d和设置功率%d不一致",t_mppt_chg.us_in_pwr,us_chg_pwr);
				
				cQueue_GotoStep(tp_task, STEP_FORWARD);
				break;
			}

			if(us_chg_pwr)
				bMppt_SetDevState(DS_WORK);
			else
				bMppt_SetDevState(DS_SHUT_DOWN);
			
			cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		
		case 2:
        {
			tMpptRx.usMaxInPwr = us_chg_pwr * 10;
			
			if(uPrint.tFlag.bMpptTask)
				sMyPrint("bMpptTask:----设置MPPT充电功率%dW完成----\r\n",us_chg_pwr);

			cQueue_GotoStep(tp_task, STEP_END);  //结束
			return;
        }
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	//等待超时
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt>(3000/mpptTASK_SET_PWR_CYCLE_TIME)) 
	{
		if(uPrint.tFlag.bMpptTask)
			log_w("bMpptTask:设置MPPT充电功率任务等待超时,步骤%d", tp_task->ucStep);
		
		if(tMppt.eDevState == DS_BOOTING)
		{
			bMppt_SetDevState(DS_SHUT_DOWN);
			c_mppt_cs_set_pwr(0);
		}
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, mpptTASK_SET_PWR_CYCLE_TIME);
	#endif  //boardUSE_OS
}

#endif  //boardMPPT_EN

