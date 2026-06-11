/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Usb/usb_queue_task.h"

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#include "Usb/usb_iface.h"
#include "Usb/usb_prot_frame.h"
#include "Sys/sys_task.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  //boardPRINT_IFACE


#define       	usbTASK_ERR_CYCLE_TIME               		1000

/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_usb_queue_task_err(Task_T *tp_task)
{
	switch (tp_task->ucStep)
    {
		//初始化
		case 0:
        {
			bUsb_SetDevState(DS_ERR);

			if(tUsb.uErrCode.tCode.bBootFault ||
				tUsb.uErrCode.tCode.bOT ||
				tUsb.uErrCode.tCode.bPowerErr ||
				tUsb.uErrCode.tCode.bIc1Lost ||
				tUsb.uErrCode.tCode.bIc2Lost ||
				tUsb.uErrCode.tCode.bBatUV)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else
				cQueue_GotoStep(tp_task, 2);
			
			return;
        }

		//等待恢复
		case 1:
        {
			// if(tUsb.uErrCode.tCode.bOT)
			// 	usbPOWER_EN_OFF();
			// else
			// 	usbPOWER_EN_ON();
			
			//有任务,退出
			if(lwrb_get_full(&tp_task->tQueueBuff))
			{
				cQueue_GotoStep(tp_task, STEP_END);  //结束
				return;
			}

			//错误清除,重新开机
			if(tUsb.uErrCode.ucErrCode == 0)
				cUsb_Switch(ST_ON, true);
        }
		break;

		//等待关闭
		case 2:
		{
			// usbPOWER_EN_OFF();

			//有任务,退出
			if(lwrb_get_full(&tp_task->tQueueBuff))
			{
				cQueue_GotoStep(tp_task, STEP_END);  //结束
				return;
			}
			
			//错误清除,关闭
			if(tUsb.uErrCode.ucErrCode == 0)
				cUsb_Switch(ST_OFF, true);
		}
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }

	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_ERR_CYCLE_TIME);
	#endif  //boardUSE_OS
}
#endif  //boardUSB_EN

