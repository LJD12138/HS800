/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_queue_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器队列任务管理与子任务分发实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Dcac/md_dcac_queue_task.h"

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Task_T *tpDcacTask = NULL;                     /* 队列任务 */

//****************************************************Function Declaration******************************************************//
static bool b_task_manage_func_cb(Task_T *p_task);
static void v_add_task_return_func_cb(Task_T *p_task, uint8_t uc_num);

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 逆变器队列初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool bDcac_QueueInit(void)
{
    s8 c_result = 1;
    
    /* 任务队列初始化，队列大小为8，回复缓存器大小为256 */
    c_result = cQueue_TaskInit(&tpDcacTask, 8, 256, b_task_manage_func_cb, v_add_task_return_func_cb);
    if (c_result <= 0)
    {
        if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
            log_e("bDcacTask:tpDcacTask任务对象初始化失败,代码%d", c_result);
        
        return false;
    }
    else if (tpDcacTask == NULL)
    {
        if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
            log_e("bDcacTask:tpDcacTask任务对象创建失败");
        
        return false;
    }
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 装载任务函数回调
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *p_task)
{
    TaskItem_T t_item;
    
    if (p_task == NULL)
        return false;
    
    vQueue_ResetTaskState(p_task);
    
    if (tSysInfo.uInit.tFinish.bIF_DcacTask == 0)
    {
        p_task->ucID = DTI_INIT;
        p_task->usInParam = 0;
    }
    else if (bQueue_PopTask(p_task, &t_item))    
    {
        p_task->ucID = t_item.ucId;
        p_task->usInParam = t_item.usParam;
    }
    else
    {
        if (tSysInfo.eDevState == DS_UPDATE_MODE)
        {
            p_task->ucID = DTI_NULL;
            p_task->usInParam = 0;
        }
        else
        {
            p_task->ucID = DTI_MAIN;
            p_task->usInParam = 0;
        }
    }
    
    switch (p_task->ucID)
    {
        case DTI_INIT:
        {
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----装载初始化任务----\r\n");
            p_task->vp_func = v_dcac_queue_task_init;
        }
        break;
        
        case DTI_MAIN:
        {
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----装载主任务----\r\n");
            p_task->vp_func = v_dcac_queue_task_main;
        }
        break;    
        
        case DTI_CTRL_DCAC_OUT:
        {
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----装载交流输出任务 参数0x%x----\r\n", p_task->usInParam);
            p_task->vp_func = v_dcac_queue_task_dcac_out;
        }
        break;

        case DTI_CTRL_DCAC_IN:
        {
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----装载开启充电任务 参数0x%x----\r\n", p_task->usInParam);
            p_task->vp_func = v_dcac_queue_task_dcac_in;
        }
        break;
        
        case DTI_CTRL_PARA_IN:
        {
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----装载并网放电任务 参数0x%x----\r\n", p_task->usInParam);
            p_task->vp_func = v_dcac_queue_task_para_in;
        }
        break;
        
        case DTI_ERR_PROC:
        {
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----装载错误处理任务----\r\n");
            p_task->vp_func = v_dcac_queue_task_err_proc;
        }
        break;

        case DTI_UPDATE:
        {
            #if (boardUPDATE)
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----装载升级任务----\r\n");
            p_task->vp_func = v_dcac_queue_task_update;
            #endif  /* boardUPDATE */
        }
        break;
        
        case DTI_NULL:
        default:
        {
            bDcac_SetAcState(OO_ALL, IOS_SHUT_DOWN);
            p_task->vp_func = NULL;
            p_task->usInParam = 0;
        }
        break;
    }
    
    return true;       
}

/***********************************************************************************************************************
 * 函数功能    : 添加任务后回调函数
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务指针, uc_num: 操作码
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_add_task_return_func_cb(Task_T *p_task, uint8_t uc_num)
{
    switch (uc_num)
    {
        /* 添加了任务 */
        case 2:
            #if (boardUSE_OS)
            xTaskNotifyGive(tDcacTaskHandler);
            #endif  /* boardUSE_OS */
            break;
        
        default:
        {
        }
        break;
    }
}

#endif  /* boardDCAC_EN */
