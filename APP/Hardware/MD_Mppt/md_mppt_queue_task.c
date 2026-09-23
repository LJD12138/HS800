/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_queue_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 任务队列管理实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Mppt/md_mppt_queue_task.h"

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Task_T *tpMpptTask = NULL;  /* 队列任务 */

//****************************************************Function Declaration******************************************************//
static bool b_task_manage_func_cb(Task_T *tp_task);
static void v_add_task_return_func_cb(Task_T *tp_task, u8 num);

/***********************************************************************************************************************
 * 函数功能    : MPPT队列初始化
 * 说明(备注)  : 创建MPPT任务队列与回调绑定
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bMppt_QueueInit(void)
{
    s8 c_result = 1;
    
    /* 任务队列初始化，队列大小为8，回复缓存器大小为12 */
    c_result = cQueue_TaskInit(&tpMpptTask, 8, 12, b_task_manage_func_cb, v_add_task_return_func_cb);
    if (c_result <= 0)
    {
        if (uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
            log_e("bMpptTask:tpMpptTask任务对象初始化失败,代码%d", c_result);
        
        return false;
    }
    else if (tpMpptTask == NULL)
    {
        if (uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
            log_e("bMpptTask:tpMpptTask任务对象创建失败");
        
        return false;
    }
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 装载任务函数回调
 * 说明(备注)  : 弹出队列任务项或装载默认轮询主任务
 * 传入参数    : tp_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *tp_task)
{
    TaskItem_T t_item;
    
    if (tp_task == NULL)
        return false;
    
    vQueue_ResetTaskState(tp_task);
    
    if (tSysInfo.uInit.tFinish.bIF_MpptTask == 0)
    {
        tp_task->ucID      = MTI_INIT;
        tp_task->usInParam = 0;
    }
    else if (bQueue_PopTask(tp_task, &t_item))    
    {
        tp_task->ucID      = t_item.ucId;
        tp_task->usInParam = t_item.usParam;
    }
    else
    {
        tp_task->ucID      = MTI_MAIN;
        tp_task->usInParam = 0;
    }
    
    switch (tp_task->ucID)
    {
        case MTI_INIT:
        {
            tp_task->vp_func = v_mppt_queue_task_init;
            if (uPrint.tFlag.bMpptTask)
                sMyPrint("bMpptTask:----装载初始化任务----\r\n");
        }
        break;
        
        case MTI_MAIN:
        {
            tp_task->vp_func = v_mppt_queue_task_main;
            if (uPrint.tFlag.bMpptTask)
                sMyPrint("bMpptTask:----装载主任务----\r\n");
        }
        break; 
           
        case MTI_SET_CHG_PWR:
        {
            tp_task->vp_func = v_mppt_queue_task_set_chg_pwr;
            if (uPrint.tFlag.bMpptTask)
                sMyPrint("bMpptTask:----装载设置充电功率任务 参数%dW----\r\n", tp_task->usInParam);
        }
        break;
        
        case MTI_ERR_PROCESS:
        {
            tp_task->vp_func = v_mppt_queue_task_err_process;
            if (uPrint.tFlag.bMpptTask)
                sMyPrint("bMpptTask:----装载错误处理任务----\r\n");
        }
        break;
        
        case MTI_NULL:
        default:
        {
            tp_task->vp_func   = NULL;
            tp_task->usInParam = 0;
        }
        break;
    }
    
    return true;       
}

/***********************************************************************************************************************
 * 函数功能    : 添加任务返回回调函数
 * 说明(备注)  : 新任务推入队列时唤醒OS任务执行
 * 传入参数    : tp_task: 队列任务指针, num: 事件代码
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_add_task_return_func_cb(Task_T *tp_task, u8 num)
{
    switch (num)
    {
        /* 添加了任务 */
        case 2:
            #if (boardUSE_OS)
            xTaskNotifyGive(tMpptTaskHandler);  /* 发通知 */
            #endif  /* boardUSE_OS */
            break;
        
        default:
        {
        }
        break;
    }
}

#endif  /* boardMPPT_EN */
