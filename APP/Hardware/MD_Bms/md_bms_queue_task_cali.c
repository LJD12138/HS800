/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task_cali.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 校准队列子任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_queue_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//
#define			bmsTASK_CALI_CYCLE_TIME					50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : BMS 校准队列子任务
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_cali(Task_T *p_task)
{
    uint8_t temp = (uint8_t)p_task->usInParam;
    
    switch (p_task->ucStep)
    {
        case 0:
        {
            c_bms_cs_set_cali(temp);
            cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;
        
        case 1:
        {
            cQueue_GotoStep(p_task, STEP_END);          /* 结束 */
        }
        break;
        
        default:
        {
            cQueue_GotoStep(p_task, STEP_END);          /* 结束 */
        }
        break;
    }
    
    p_task->usTaskWaitCnt++;
    if (p_task->usTaskWaitCnt > (3000 / bmsTASK_CALI_CYCLE_TIME))  /* 等待超时 */
    {
        if (uPrint.tFlag.bBmsTask)
            log_w("bBmsTask:校准任务等待超时,步骤%d", p_task->ucStep);
        
        cQueue_GotoStep(p_task, STEP_END);              /* 结束 */
    }
    
    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, bmsTASK_CALI_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
    #endif  /* boardUSE_OS */
}

#endif  /* boardBMS_EN */
