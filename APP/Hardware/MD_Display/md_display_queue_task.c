/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务管理                                                      *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"
#include "MD_Display/md_display_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"


//****************************************************参数初始化**************************************************//
//结构体
__ALIGNED(4) 	Task_T *tpDispTask = NULL;  	//队列任务


//****************************************************函数声明****************************************************//
static bool b_task_manage_func_cb(Task_T *tp_task);
static void v_add_task_return_func_cb(Task_T *tp_task, u8 num);

/***********************************************************************************************************************
-----函数功能    任务参数初始化
-----说明(备注)  创建显示任务队列并绑定任务装载和入队回调函数
-----传入参数    none
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
bool bDisp_QueueInit(void)
{
	//任务队列初始化
	if(cQueue_TaskInit(&tpDispTask, 8, 12, b_task_manage_func_cb, v_add_task_return_func_cb) <= 0)
	{
		if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
			log_e("bDispTask:tpDispTask任务对象初始化失败");
		
		return false;
	}
	else if(tpDispTask == NULL)
	{
		if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
			log_e("bDispTask:tpDispTask任务对象创建失败");
		
		return false;
	}
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    装载任务函数
-----说明(备注)  复位任务运行参数, 从队列读取任务ID并绑定对应处理函数
-----传入参数    tp_task:任务对象指针
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *tp_task)
{
	static vu16 uc_temp = 0;
	
	tp_task->bNowRun = false;
	tp_task->ucStep = 0;
	tp_task->usTaskWaitCnt = 0;
	tp_task->usTaskWaitCnt = 0;
	tp_task->usStepRepeatCnt = 0;
	
	uc_temp = lwrb_get_full(&tp_task->tQueueBuff);
	if(uc_temp%3 != 0 && uc_temp != 0)
	{
		if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
			log_e("bDispTask:任务队列长度异常 长度%d",uc_temp);
		lwrb_reset(&tp_task->tQueueBuff);
		return false;
	}
	
	if(tSysInfo.uInit.tFinish.bIF_DispTask == 0) //初始化未完成
	{
		tp_task->ucID = DTI_INIT;           
		tp_task->usInParam = 0;
	}
	else if(uc_temp)//队列里面有任务   
	{
		lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->ucID, 1);
		lwrb_read(&tp_task->tQueueBuff, (u8*)&tp_task->usInParam, 2);
	}
	else
	{
		tp_task->ucID = DTI_NULL;           
		tp_task->usInParam = 0;
	}
	
    switch (tp_task->ucID)
    {
        case DTI_INIT:
        {
			if(uPrint.tFlag.bDispTask)
				sMyPrint("\r\n bDispTask:----装载初始化显示任务---- \r\n");
            tp_task->vp_func = v_disp_queue_task_init;
        }
        break;
		
		case DTI_CLOSING:
        {	
			if(uPrint.tFlag.bDispTask)
				sMyPrint("bDispTask:----装载关闭中显示任务----\r\n");
			tp_task->vp_func = v_disp_queue_task_closing;
        }
        break;

		case DTI_SHUT_DOWN:
        {	
			if(uPrint.tFlag.bDispTask)
				sMyPrint("bDispTask:----装载关闭完成显示任务----\r\n");
			tp_task->vp_func = v_disp_queue_task_shut_down;
        }
        break;
		
		case DTI_ERR:
        {  
			if(uPrint.tFlag.bDispTask)
				sMyPrint("bDispTask:----装载错误显示任务----\r\n");
			tp_task->vp_func = v_disp_queue_task_err;
        }
        break;
        
        case DTI_BOOTING:
        {  
			if(uPrint.tFlag.bDispTask)
				sMyPrint("bDispTask:----装载启动显示任务----\r\n");
			tp_task->vp_func = v_disp_queue_task_booting;
        }
        break;

		case DTI_WORK:
        {  
			if(uPrint.tFlag.bDispTask)
				sMyPrint("bDispTask:----装载工作显示任务----\r\n");
			tp_task->vp_func = v_disp_queue_task_work;
        }
        break;

		#if(boardUPDATA)
		case DTI_UPDATA:
        {  
			if(uPrint.tFlag.bDispTask)
				sMyPrint("bDispTask:----装载升级显示任务----\r\n");
			tp_task->vp_func = v_disp_queue_task_updata;
        }
        break;
		#endif  //boardUPDATA
		
		#if(boardENG_MODE_EN)
		case DTI_ENG:
        {  
			if(uPrint.tFlag.bDispTask)
				sMyPrint("bDispTask:----装载工程模式显示任务----\r\n");
			tp_task->vp_func = v_disp_queue_task_eng;
        }
        break;
		#endif
		
		case DTI_NULL:
        default:
            tp_task->vp_func = NULL;
			tp_task->usInParam = 0;
			if(uPrint.tFlag.bDispTask)
				sMyPrint("bDispTask:----装载空任务----\r\n");
        break;
    }

    return true;       
}

/***********************************************************************************************************************
-----函数功能    添加任务返回回调
-----说明(备注)  队列加入新任务后唤醒显示任务
-----传入参数    tp_task:任务对象指针  num:回调类型
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_add_task_return_func_cb(Task_T *tp_task, u8 num)
{
	switch(num)
	{
		//添加了任务
		case 2:
		{
			#if(boardUSE_OS)
			xTaskNotifyGive(tDispTaskHandler);
			#endif  //boardUSE_OS
		}
		break;
		
		default:
			break;
	}
}
