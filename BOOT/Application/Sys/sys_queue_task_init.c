/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application\Sys
 * File    : sys_queue_task_init.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统初始化队列任务实现
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
#include "Flash/flash_iface.h"
#include "boot_info.h"

#if (1)
//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//
static void v_print_logo(void);


/***********************************************************************************************************************
 * 函数功能    : 打印开机Logo信息
 * 说明(备注)  : 启动提示
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_print_logo(void)
{
    sMyPrint("Boot Start!!!\r\n");
}

/***********************************************************************************************************************
 * 函数功能    : 系统初始化队列任务函数
 * 说明(备注)  : 依次执行启动信息打印、Boot配置校验与引导后续目标任务入队
 * 传入参数    : tp_task: 队列任务控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_sys_queue_task_init(Task_T *tp_task)
{
    SysTaskId_E e_task_id;

    switch (tp_task->ucStep)
    {
        case 0:
        {
            if (uPrint.tFlag.bSysTask)
                v_print_logo();
            tSysInfo.uInit.tFinish.bIF_BootInfo = false;
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            e_task_id = eBoot_InfoInit(false);
            if (e_task_id != STI_ERR && e_task_id >= STI_INIT)
                tSysInfo.uInit.tFinish.bIF_BootInfo = true;
            cQueue_AddQueueTask(tpSysTask, e_task_id, 0, false);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        case 2:
        {
            tSysInfo.uInit.tFinish.bIF_SysTask = 1;
            tSysInfo.uInit.tFinish.bIF_SysInit = 1;
            tSysInfo.uInit.tFinish.bIF_AT24Cxx = 1;
            tSysInfo.uInit.tFinish.bIF_Gpio    = 1;
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;

        default:
        {
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;
    }

    /* 初始化等待 5S 超时保护 */
    if (bQueue_IsStepTimeout(tp_task, (5000 / sysTASK_CYCLE_TIME)))
        cQueue_GotoStep(tp_task, STEP_END);
}

#endif  /* 1 */
