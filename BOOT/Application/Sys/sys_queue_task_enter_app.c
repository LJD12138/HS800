/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_queue_task_update.h"
#include "Print/print_task.h"
#include "Print/print_iface.h"

#include "boot_info.h"

/***********************************************************************************************************************
-----函数功能    系统初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/  
void v_sys_queue_task_enter_app(Task_T *tp_task)
{
	static s8 ret = 0;
	
	#if(boardUPDATE)
	static uint8_t u8_illegal_addr_cnt;
	#endif
	
    switch (tp_task->ucStep)
    {
		case 0:
		{
			//等待Print打印完成
			#if(boardPRINT_IFACE)
			if(bPrint_CheckSendFinish() == false)
				break;
			#endif
			
			//开始映射到外部Flash
			#if(boardIC_TYPE == boardIC_STM32H7XX)
			ret = cQSPI_MemoryMapped();
			#else
			ret = 1;
			#endif
			
			if(ret > 0)
				cQueue_GotoStep( tp_task, STEP_NEXT);  //下一步
		}
		break;
		
		case 1:
		{
			ret = cSys_JumpToApp();
			if(ret <= 0)
				cQueue_GotoStep( tp_task, STEP_NEXT);  //下一步
		}
		break;
		
		case 2:
		{
			//延时1S
			tpSysTask->usStepWaitCnt++;
			if(tpSysTask->usStepWaitCnt < (1000 / sysTASK_CYCLE_TIME))
				break;
			tpSysTask->usStepWaitCnt = 0;
			
			#if(boardUPDATE)
			if(++u8_illegal_addr_cnt > 10)
			{
				//关闭内存映射(不关闭写Flash会卡死,还不知道原因)
				#if(boardIC_TYPE == boardIC_STM32H7XX)
				cQSPI_QuitMemoryMapped();
				#endif  //boardIC_STM32H7XX
				
				u8_illegal_addr_cnt = 0;
				cBoot_CtrlUpdate(true, AS_ERASE);
				#if(boardCONSOLE_EN)
				cUpdate_ChSelect(CT_CONSOLE, PT_BAIKU);
				#elif(boardPRINT_IFACE)
				cUpdate_ChSelect(CT_PRINT, PT_XMODEM);
				#endif
				cQueue_GotoStep( tp_task, STEP_END);  //结束
				break;
			}
			#endif
			
			//开始映射到外部Flash
			#if(boardIC_TYPE == boardIC_STM32H7XX)
			ret = cQSPI_MemoryMapped();
			#else
			ret = 1;
			#endif
			
//			if(uPrint.tFlag.bSysTask == 1)
				log_e("BOOT跳转APP失败,错误代码%d!!!\0",ret);
			
			cQueue_GotoStep( tp_task, STEP_FORWARD);  //返回上一步重新检测
		}break;
		
        default:
				cQueue_GotoStep( tp_task, STEP_END);  //结束
			break;
    } 
}

