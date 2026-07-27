/*****************************************************************************************************************
*                                                                                                                *
 *                                         队列函数                                                  			*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

#define       	dcacTASK_INIT_CYCLE_TIME               		100

//****************************************************函数声明****************************************************//
static s8 c_dcac_info_init(void);


/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_init(Task_T *tp_task)
{
	s8 c_ret = 0;

	switch (tp_task->ucStep)
    {
		case 0:
		{
			//等待获取APP信息
			if(tSysInfo.uInit.tFinish.bIF_AppInfo == false)
				break;
			
			static bool b_ret = true;
			c_ret = c_dcac_info_init();
			if(c_ret > 0)
			{
				if((uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant) && b_ret == false)
					log_w("bDcacTask:tDCAC获取错误清除");
				
				b_ret = true;
			}
			else
			{
				if((uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant) && b_ret == true)
				{
					log_w("bDcacTask:tDCAC初始化失败 代码%d",c_ret);
					b_ret = false;
				}
				break;
			}
			cQueue_GotoStep(tp_task, STEP_NEXT);
		}
		break;

		case 1:
        {
			if(b_dcac_cs_init() == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);
			else
			{
				vTaskDelay(500);
				break;
			}
        }

		case 2:
		{
			// if(b_dcac_cs_set_chg_pwr(tAppMemParam.tDCAC.usInPwrRating) == true)  //获取参数
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			// else
			// {
			// 	vTaskDelay(500);
			// 	break;
			// }
		}

		case 3:
        {
			tSysInfo.uInit.tFinish.bIF_DcacTask = 1;
			bDcac_SetAcState(OO_ALL, IOS_SHUT_DOWN);
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:初始化DCAC----初始化完成----\r\n");
			
			cQueue_GotoStep(tp_task, STEP_END);
        }
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000 / dcacTASK_INIT_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:初始化任务等待超时,步骤%d", tp_task->ucStep);
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	vTaskDelay(dcacTASK_INIT_CYCLE_TIME);
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
static s8 c_dcac_info_init(void)
{
	s8 ret = 0;
	const char* p_obj_str = tDcacMemParamStr;
	static bool b_ret = true;
	
	//已经初始化
	if(tSysInfo.uInit.tFinish.bIF_SysInit == true)
	{
		ret = cApp_GetMemParam(p_obj_str);
		if(ret > 0)//成功
			return 1;

		if((uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant) && b_ret == true)
		{
			log_e("bDcacTask:当前系统已经初始化完成,但是tDCAC读取依旧为空,准备重置");
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

#endif  //boardDCAC_EN
