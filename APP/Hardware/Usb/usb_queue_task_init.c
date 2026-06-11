/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Usb/usb_queue_task.h"

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#include "Usb/usb_prot_frame.h"
#include "Sys/sys_task.h"

#include "app_info.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  //boardPRINT_IFACE

#define       	usbTASK_INIT_CYCLE_TIME               		100

static s8 c_usb_info_init(void);


/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_usb_queue_task_init(Task_T *tp_task)
{
	s8 c_ret = 0;

	switch (tp_task->ucStep)
    {
		case 0:
        {
			bUsb_SetDevState(DS_INIT);

			//等待获取APP信息
			if(tSysInfo.uInit.tFinish.bIF_AppInfo == false)
				break;

			cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
        }
		break;

		case 1:
        {
			static bool b_ret = true;
			c_ret = c_usb_info_init();
			if(c_ret > 0)
			{
				if((uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant) && b_ret == false)
					log_w("bUsbTask:tUSB获取错误清除");
				
				b_ret = true;
			}
			else
			{
				if((uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant) && b_ret == true)
				{
					log_w("bUsbTask:tUSB初始化失败 代码%d",c_ret);
					b_ret = false;
				}
				break;
			}

			cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
        }
		break;
	
		case 2:
		{
			tSysInfo.uInit.tFinish.bIF_UsbTask = true;
			bUsb_SetDevState(DS_SHUT_DOWN);
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	//等待超时
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000 / usbTASK_INIT_CYCLE_TIME)) 
	{
		if(uPrint.tFlag.bUsbTask)
			log_w("bUsbTask:初始化任务等待超时");
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_INIT_CYCLE_TIME);
	#endif  //boardUSE_OS
}

/*****************************************************************************************************************
-----函数功能	获取信息
-----说明(备注)	none
-----传入参数	none
-----输出参数	none
-----返回值		小于0:失败	
				0:未完成
				大于0:完成
******************************************************************************************************************/
static s8 c_usb_info_init(void)
{
	s8 ret = 0;
	const char* p_obj_str = tUsbMemParamStr;
	static bool b_ret = true;
	
	//已经初始化
	if(tSysInfo.uInit.tFinish.bIF_SysInit == true)
	{
		ret = cApp_GetMemParam(p_obj_str);
		if(ret > 0)//成功
			return 1;

		if((uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant) && b_ret == true)
		{
			log_e("bUsbTask:当前系统已经初始化完成,但是tUSB读取依旧为空,准备重置");
			b_ret = false;
		}	
	}
	
	//重新初始化
	ret = cApp_MemParamInit(p_obj_str);
	if(ret <= 0)//失败
		return -1;
	
	ret = cApp_UpdateMemParam(p_obj_str);
	if(ret <= 0)//失败
		return -2;
	
	b_ret = true;
	return 2;
}
#endif  //boardUSB_EN

