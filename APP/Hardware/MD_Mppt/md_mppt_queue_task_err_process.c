/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_queue_task_err_process.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 错误处理与状态管理任务
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
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//
#define			mpptTASK_ERR_CYCLE_TIME					100

/***********************************************************************************************************************
 * 函数功能    : 任务函数:错误处理
 * 说明(备注)  : 发生故障时关断充电功率并置为错误状态
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_mppt_queue_task_err_process(Task_T *p_task)
{
    MpptErrCode_E e_err_code;
    
    e_err_code = (MpptErrCode_E)p_task->usInParam;  /* 要设置MPPT的状态 */
    
    /* 丢失状态不需要去关闭 */
    if (e_err_code == MEC_SYS_DEV_LOST)
        p_task->ucStep = 2;
    
    bMppt_SetDevState(DS_ERR);
    
    switch (p_task->ucStep)
    {
        case 0:
        {
            if (c_mppt_cs_set_pwr(0) > 0)
                cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */
        }
        break;
        
        case 1:
        {
            if (tMpptRx.usMaxInPwr == 0)
                cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */
        }
        break;
        
        case 2:
        {
            cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
        }
        break;
            
        default:
        {
            cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
        }
        break;
    }
    
    /* 等待超时 */
    if (bQueue_IsTaskTimeoutMs(p_task, 3000))
    {
        if (uPrint.tFlag.bMpptTask)
            log_w("bMpptTask:错误处理任务等待超时,步骤%d", p_task->ucStep);
        
        cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
    }
    
    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, mpptTASK_ERR_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
    #endif  /* boardUSE_OS */
}

#endif  /* boardMPPT_EN */
