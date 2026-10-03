/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task_req_set_cmd.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 设置指令队列子任务实现
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
#define			bmsTASK_SET_CMD_CYCLE_TIME				50

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : BMS 请求设置指令队列子任务
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_req_set_cmd(Task_T *p_task)
{
    static tSysSetParam s_t_sys_set_param;
    
    switch (p_task->ucStep)
    {
        case 0:
        {
            if (usQueue_ReadReply(p_task, &s_t_sys_set_param, sizeof(s_t_sys_set_param)) != sizeof(s_t_sys_set_param))
            {
                if (uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
                    log_w("bBmsTask:BMS返回数据错误");

                cQueue_GotoStep(p_task, STEP_END);      /* 结束 */
                break;
            }

            if (c_bms_cs_sys_set(&s_t_sys_set_param) > 0)
                cQueue_GotoStep(p_task, STEP_NEXT);     /* 下一步 */
        }
        break;
        
        case 1:
        {
            /* 等待1S */
            if (bQueue_IsStepTimeoutMs(p_task, 1000) == false)
                break;
            
            if (c_bms_cs_sys_set(&s_t_sys_set_param) > 0)
                cQueue_GotoStep(p_task, STEP_NEXT);     /* 下一步 */
        }
        break;

        case 2:
        {
            if (s_t_sys_set_param.cmd == mainUPDATE_FLAG)
                cQueue_AddQueueTask(tpBmsTask, BTI_UPDATE, 0, false);

            cQueue_GotoStep(p_task, STEP_END);          /* 结束 */
        }
        break;
            
        default:
        {
            cQueue_GotoStep(p_task, STEP_END);          /* 结束 */
        }
        break;
    }
    
    if (bQueue_IsTaskTimeoutMs(p_task, 5000))  /* 等待超时 */
    {
        cQueue_GotoStep(p_task, STEP_END);              /* 结束 */
        if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
            log_w("bBmsTask:设置指令任务超时,步骤%d", p_task->ucStep);
    }
    
    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, bmsTASK_SET_CMD_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
    #endif  /* boardUSE_OS */
}

#endif  /* boardBMS_EN */
