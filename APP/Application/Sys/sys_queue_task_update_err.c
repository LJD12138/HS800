/*******************************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\1-Baiku_Projects\11-G24\1.software\G2404-3\APP\Application\Sys
 * File    : sys_queue_task_update_err.c
 * Date    : 2026-05-09 12:05:30
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"

#if(boardUPDATE)
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Sys/sys_queue_task_update.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif
//****************************************************Macros*******************************************************************//
#define     	sysTASK_UPDATE_ERR_CYCLE_TIME				sysTASK_CYCLE_TIME //任务时间


//****************************************************Parameter Initialization************************************************//



//****************************************************Function Declaration****************************************************//



/***********************************************************************************************************************
-----函数功能    系统升级错误任务处理函数
-----说明(备注)  根据任务的当前步骤和设备状态，处理系统升级错误任务。
-----传入参数    tp_task: 指向任务结构体的指针，包含任务的相关信息。
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_update_err(Task_T *tp_task)
{
    if(tp_task == NULL)
        return;

    switch(tp_task->ucStep)
    {
        //清除升级状态
        case 0:
        {
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        //等待用户操作,超时就进入关机
        case 1:
        {
            //有任务退出
            if(lwrb_get_full(&tp_task->tQueueBuff)) //队列里面有任务
			{
				tUpdate.eErrCode = 0;
				cQueue_GotoStep( tp_task, STEP_END ); //结束
			}
            
            tp_task->usStepWaitCnt++;
            if(tp_task->usStepWaitCnt >= ((60 * 1000) / sysTASK_UPDATE_ERR_CYCLE_TIME))
            {
                cQueue_GotoStep(tp_task, STEP_NEXT);
                return;
            }
        }
        break;

        case 2:
        {
            bUpdate_Init();
            cSys_Switch(SO_KEY, ST_OFF, false);
            cQueue_GotoStep(tp_task, STEP_END);

        }
        break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

	#if(boardUSE_OS)
	vTaskDelay(sysTASK_UPDATE_ERR_CYCLE_TIME);
	#endif  //boardUSE_OS
}


#endif  //boardUPDATE
