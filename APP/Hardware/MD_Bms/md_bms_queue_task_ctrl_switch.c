/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task_ctrl_switch.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 开关控制队列子任务实现
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
#define			bmsTASK_CTRL_SWITCH_CYCLE_TIME			50

//****************************************************Function Declaration******************************************************//
static void v_bms_sw_fail_cleanup(Task_T *p_task, TaskInParam_U *p_param);

//****************************************************Function Declaration******************************************************//



/***********************************************************************************************************************
 * 函数功能    : 控制开关任务
 * 说明(备注)  : 处理 BMS 开关状态机与重试
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_clt_switch(Task_T *p_task)
{
    s8 c_ret = 0;
    TaskInParam_U u_param;
    SwitchType_E e_sw_type = ST_OFF;
    
    u_param.usTaskInParam = p_task->usInParam;       /* 要设置BMS的状态 */

    if (u_param.tTaskParam.ucObj == 0)
        e_sw_type = (SwitchType_E)u_param.tTaskParam.ucParam;
    
    switch (p_task->ucStep)
    {
        /*------------------------------发送指令------------------------------------*/
        case 0:
        {
            if (bQueue_IsStepRetryOver(p_task, 3))
            {
                if (uPrint.tFlag.bBmsTask)
                    log_w("bBmsTask:对象%d数据发送失败次数过多,退出开关任务", u_param.tTaskParam.ucObj);
                
                v_bms_sw_fail_cleanup(p_task, &u_param);
                break;
            }
            
            switch (u_param.tTaskParam.ucObj)
            {
                /* 系统 */
                case 0:
                {
                    if (e_sw_type == ST_ON)
                        bBms_SetDevState(DS_BOOTING);
                    else 
                    {
                        bBms_SetErrCode(BEC_CLEAR_ALL, false);  /* 清除所有错误 */
                        bBms_SetDevState(DS_CLOSING);
                    }
                    c_ret = c_bms_cs_switch(u_param);
                }
                break;

                default:
                {
                    if (uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
                        log_w("bBmsTask:开关对象%d未定义", u_param.tTaskParam.ucObj);
                    cQueue_GotoStep(p_task, STEP_END);          /* 结束 */
                }
                break;
            }

            if (c_ret > 0)
                cQueue_GotoStep(p_task, STEP_NEXT);             /* 下一步 */
        }
        break;
        
        /*------------------------------处理返回数据------------------------------------*/
        case 1:
        {
            TaskInParam_U u_reply_param;

            if (usQueue_ReadReply(p_task, &u_reply_param, sizeof(u_reply_param)) != sizeof(u_reply_param))
            {
                if (uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
                    log_w("bBmsTask:BMS返回数据错误");
                cQueue_GotoStep(p_task, STEP_FORWARD);
                break;
            }
            
            /* 校验数据 */
            if (u_param.tTaskParam.ucObj != u_reply_param.tTaskParam.ucObj)
            {
                if (uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
                    log_w("bBmsTask:BMS返回对象%d错误,当前操作对象%d", 
                        u_reply_param.tTaskParam.ucObj, u_param.tTaskParam.ucObj);
                cQueue_GotoStep(p_task, STEP_FORWARD);
                break;
            }

            /* 处理数据 */
            switch (u_param.tTaskParam.ucObj)
            {
                /* 系统 */
                case 0:
                {
                    if (u_reply_param.tTaskParam.ucParam > DS_UPDATE_MODE)
                    {
                        if (uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
                            log_w("bBmsTask:BMS返回对象%d的参数%d超范围", 
                                u_reply_param.tTaskParam.ucObj, u_reply_param.tTaskParam.ucParam);
                        cQueue_GotoStep(p_task, STEP_FORWARD);
                        break;
                    }

                    if (e_sw_type == ST_ON)
                    {
                        if (u_reply_param.tTaskParam.ucParam >= DS_BOOTING)
                        {
                            bBms_SetDevState(DS_WORK);
                            cQueue_GotoStep(p_task, STEP_NEXT); /* 下一步 */
                            break;
                        }
                    }
                    else 
                    {
                        if (u_reply_param.tTaskParam.ucParam <= DS_SHUT_DOWN)
                        {
                            bBms_SetDevState(DS_SHUT_DOWN);
                            cQueue_GotoStep(p_task, STEP_NEXT); /* 下一步 */
                            break;
                        }    
                    }

                    cQueue_GotoStep(p_task, STEP_FORWARD);
                }
                break;

                default:
                {
                    if (uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
                        log_w("bBmsTask:BMS返回对象%d未定义,当前对象%d", 
                            u_reply_param.tTaskParam.ucObj, u_param.tTaskParam.ucObj);
                    cQueue_GotoStep(p_task, STEP_FORWARD);
                }
                break;
            }
        }
        break;
        
        /*-------------------------------------完成---------------------------------------*/
        case 2:
        {
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:对象%d----开关完成----\r\n", u_param.tTaskParam.ucObj);
            
            cQueue_GotoStep(p_task, STEP_END);                  /* 结束 */
        }
        break;
            
        default:
        {
            cQueue_GotoStep(p_task, STEP_END);                  /* 结束 */
        }
        break;
    }
    
    if (bQueue_IsTaskTimeoutMs(p_task, 3000))  /* 等待超时 */
    {
        if (uPrint.tFlag.bBmsTask)
            sMyPrint("bBmsTask:对象%d开关任务等待超时退出,步骤%d", u_param.tTaskParam.ucObj, p_task->ucStep);
        
        v_bms_sw_fail_cleanup(p_task, &u_param);
    }
    
    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, bmsTASK_CTRL_SWITCH_CYCLE_TIME);   /* 周期节拍；新任务投递立即唤醒抢占 */
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : BMS 开关任务失败/超时清理
 * 说明(备注)  : 重置开关并结束当前任务步进
 * 传入参数    : p_task: 队列任务指针, p_param: 任务输入参数指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_bms_sw_fail_cleanup(Task_T *p_task, TaskInParam_U *p_param)
{
    if (p_param->tTaskParam.ucParam != 0)
    {
        p_param->tTaskParam.ucParam = 0;
        c_bms_cs_switch(*p_param);

        if (p_param->tTaskParam.ucObj == 0)
            bBms_SetDevState(DS_SHUT_DOWN);
    }
    cQueue_GotoStep(p_task, STEP_END);
}

#endif  /* boardBMS_EN */
