/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务的队列函数                                                  *
*                                                                                                                *
******************************************************************************************************************/
#include "Usb/usb_queue_task.h"

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  //boardPRINT_IFACE
#include "Sys/sys_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
__ALIGNED(4) Task_T *tpUsbTask = NULL;  	//队列任务


//****************************************************函数声明****************************************************//
static bool b_task_manage_func_cb(Task_T *tp_task);
static void v_add_task_return_func_cb(Task_T *tp_task, u8 num);
static void v_usb_queue_task_shut_down(Task_T *tp_task);



/***********************************************************************************************************************
-----函数功能    队列初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bUsb_QueueInit(void)
{
	s8 c_result = 1;
	
	//任务队列初始化，队列大小为8，回复缓存器大小为64
	c_result = cQueue_TaskInit(&tpUsbTask, 8, 0, b_task_manage_func_cb, v_add_task_return_func_cb);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_e("bUsbTask:tpUsbTask任务对象初始化失败,代码&d",c_result);
		
		return false;
	}
	else if(tpUsbTask == NULL)
	{
		if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_e("bUsbTask:tpUsbTask任务对象创建失败");
		
		return false;
	}
	
	return true;
}


/*****************************************************************************************************************
-----函数功能    装载任务函数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:成功   false:失败 
******************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *tp_task)
{
	vu16 uc_temp = 0;
	
	if(tp_task == NULL)
		return false;
	
	tp_task->bNowRun = false;
	tp_task->ucStep = 0;
	tp_task->usStepWaitCnt = 0;
	tp_task->usTaskWaitCnt = 0;
	tp_task->usStepRepeatCnt = 0;
	
	uc_temp = lwrb_get_full(&tp_task->tQueueBuff);
	if(uc_temp%3 != 0 && uc_temp != 0)
	{
		if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_e("bUsbTask:任务队列长度异常 长度%d",uc_temp);
		lwrb_reset(&tp_task->tQueueBuff);
		return false;
	}
	
	if(tSysInfo.uInit.tFinish.bIF_UsbTask == 0)
	{
		tp_task->ucID = UTI_INIT;
		tp_task->usInParam = 0;
	}
    else if(uc_temp)    
    {
        lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->ucID, 1);
		lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->usInParam, 2);
    }
	else if(tUsb.eDevState ==DS_WORK)
    {
		tp_task->ucID = UTI_WORK;
		tp_task->usInParam = 0;
    }
    else
    {
		tp_task->ucID = UTI_SHUT_DOWN;
		tp_task->usInParam = 0;
    }
    
    switch (tp_task->ucID)
    {
        case UTI_INIT:
        {
            tp_task->vp_func = v_usb_queue_task_init;
			
			if(uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载初始化任务----\r\n");
        }break;
        
        case UTI_CLOSING:
        {			
            tp_task->vp_func = v_usb_queue_task_closing;
			
			if(uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载关闭任务----\r\n");
        }break; 
           
        case UTI_SHUT_DOWN:
        {  
			tp_task->vp_func = v_usb_queue_task_shut_down;
			
			if(uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载关闭完成任务----\r\n",tp_task->usInParam);
        }break;
		
		case UTI_ERR:
        {			
            tp_task->vp_func = v_usb_queue_task_err;
			
			if(uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载错误任务----\r\n");
        }break;

		case UTI_BOOTING:
        {  
			if(uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载启动任务----\r\n");
			tp_task->vp_func = v_usb_queue_task_booting;
        }
        break;

		case UTI_WORK:
        {  
			if(uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:----装载工作任务----\r\n");
			tp_task->vp_func = v_usb_queue_task_work;
        }
        break;
		
		case UTI_NULL:
        default:
            tp_task->vp_func = NULL;
			tp_task->usInParam = 0;
        break;
    }
    
    return true;       
}

static void v_add_task_return_func_cb(Task_T *tp_task, u8 num)
{
	switch(num)
	{
		//添加了任务
		case 2:
		{
			#if(boardUSE_OS)
			xTaskNotifyGive(tUsbTaskHandler); //发通知
			#endif  //boardUSE_OS
		}
		break;
		
		default:
			break;
	}
}

/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_usb_queue_task_shut_down(Task_T *tp_task)
{
	memset((u8*)&tUsb, 0, sizeof(tUsb));
	bUsb_SetDevState(DS_SHUT_DOWN);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //等待任务通知  一直等待,直到释放通知
	cQueue_GotoStep(tp_task, STEP_END);  //结束
}
#endif  //boardUSB_EN
