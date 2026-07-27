/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_queue_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

#define       	bmsTASK_INIT_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//
static s8 c_bms_info_init(void);



/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_init(Task_T *tp_task)
{
	s8 c_ret = 0;
	
	switch (tp_task->ucStep)
    {
		case 0:
        {
			//获取参数,用来判断是否是充电唤醒
			if(c_bms_cs_get_param(bmsGET_PARAM_OBJ) > 0 || G_TestMode == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else
				break;
        }

		case 1:
		{
			//等待获取APP信息
			if(tSysInfo.uInit.tFinish.bIF_AppInfo == false)
				break;
			
			static bool b_ret = true;
			c_ret = c_bms_info_init();
			if(c_ret > 0)
			{
				if((uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant) && b_ret == false)
					log_w("bBmsTask:BMS获取错误清除");
				
				b_ret = true;
			}
			else
			{
				if((uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant) && b_ret == true)
				{
					log_w("bBmsTask:BMS初始化失败 代码%d",c_ret);
					b_ret = false;
				}
				break;
			}
			cQueue_GotoStep(tp_task, STEP_NEXT);
		}
		break;
		
		case 2:
		{
			tSysInfo.uInit.tFinish.bIF_BmsTask = 1;
			cBms_CheckPerm();
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:初始化BMS----初始化完成----\r\n");
			
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
        break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000 / bmsTASK_INIT_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bBmsTask)
			log_w("bBmsTask:BMS初始化任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	vTaskDelay(bmsTASK_INIT_CYCLE_TIME);
	#endif  //boardUSE_OS
}

/*****************************************************************************************************************
-----函数功能   初始化DCAC信息
-----说明(备注)	none
-----传入参数	none
-----输出参数	none
-----返回值		小于0:失败	
				0:未完成
				大于0:完成
******************************************************************************************************************/
static s8 c_bms_info_init(void)
{
	s8 ret = 0;
	const char* p_obj_str = tBmsMemParamStr;
	static bool b_ret = true;
	
	//已经初始化
	if(tSysInfo.uInit.tFinish.bIF_SysInit == true)
	{
		ret = cApp_GetMemParam(p_obj_str);
		if(ret > 0)//成功
			return 1;

		if((uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant) && b_ret == true)
		{
			log_e("bBmsTask:当前系统已经初始化完成,但是tBMS读取依旧为空,准备重置");
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

#endif  //boardBMS_EN
