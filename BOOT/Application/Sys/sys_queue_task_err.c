/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application\Sys
 * File    : sys_queue_task_err.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统错误处理队列任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (1)
//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 系统错误状态机执行函数
 * 说明(备注)  : 设置系统设备状态为 DS_ERR 并进行异常安全退出处理
 * 传入参数    : tp_task: 队列任务控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_sys_queue_task_err(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
        case 0:
        {
            /* 队列中有其他任务时优先响应 */
            if (!bQueue_IsQueueEmpty(tp_task))
            {
                cQueue_GotoStep(tp_task, STEP_END);
                break;
            }

            if (tSysInfo.eDevState != DS_ERR)
                bSys_SetDevState(DS_ERR, false);

            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;

        default:
        {
            if (!bQueue_IsQueueEmpty(tp_task))
                cQueue_GotoStep(tp_task, STEP_END);
        }
        break;
    }

    /* 等待 5S 超时退出 */
    if (bQueue_IsStepTimeout(tp_task, (5000 / sysTASK_CYCLE_TIME)) && tp_task->ucStep != STEP_END)
    {
        if (uPrint.tFlag.bSysTask)
            log_w("bSysTask:错误任务等待超时,退出");
        cQueue_GotoStep(tp_task, STEP_END);
    }
}

#endif  /* 1 */
