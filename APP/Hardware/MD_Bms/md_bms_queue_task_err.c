/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task_err.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 错误处理队列子任务实现
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
#include "Buz/buz_task.h"

//****************************************************Macros********************************************************************//
#define			bmsTASK_ERR_CYCLE_TIME					50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : BMS 错误处理队列子任务
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_err(Task_T *p_task)
{
    switch (p_task->ucStep)
    {
        case 0:
            /* 设备丢失 */
            if (tBms.uErrCode.tCode.bSysDevLost == 1)
            {
                #if (boardBUZ_EN)
                bBuz_Tweet(LONG_3);
                #endif  /* boardBUZ_EN */
                
                bBms_SetDevState(DS_LOST);
            }
            else
                bBms_SetDevState(DS_ERR);
            
            cQueue_GotoStep(p_task, STEP_NEXT);         /* 下一步 */
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
    
    if (bQueue_IsTaskTimeoutMs(p_task, 3000))  /* 等待超时 */
    {
        cQueue_GotoStep(p_task, STEP_END);              /* 结束 */
        if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
            log_w("bBmsTask:错误处理任务超时,步骤%d", p_task->ucStep);
    }
    
    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, bmsTASK_ERR_CYCLE_TIME);   /* 周期节拍；新任务投递立即唤醒抢占 */
    #endif  /* boardUSE_OS */
}

#endif  /* boardBMS_EN */
