/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_queue_task_set_chg_pwr.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 充电功率设置队列任务
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
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//
#define			mpptTASK_SET_PWR_CYCLE_TIME				100

/***********************************************************************************************************************
 * 函数功能    : 任务函数:设置充电功率
 * 说明(备注)  : 向MPPT发送功率设定指令并校验回复结果
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_mppt_queue_task_set_chg_pwr(Task_T *p_task)
{
    vu16 us_chg_pwr = p_task->usInParam;

    switch (p_task->ucStep)
    {
        case 0:
        {
            if (us_chg_pwr && tMppt.eDevState != DS_WORK)
                bMppt_SetDevState(DS_BOOTING);
            else if ((us_chg_pwr == 0) && tMppt.eDevState != DS_SHUT_DOWN)
                bMppt_SetDevState(DS_CLOSING);

            if (c_mppt_cs_set_pwr(us_chg_pwr) > 0)
                cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */
            else
            {
                p_task->usStepRepeatCnt++;
                if (p_task->usStepRepeatCnt >= 2)
                {
                    if (uPrint.tFlag.bMpptTask)
                        log_w("bMpptTask:发送失败次数过多,退出设置充电功率任务");
                    
                    cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
                }
                break;
            }
        }
        break;
        
        case 1:
        {
            if (bQueue_IsStepTimeoutMs(p_task, 1000))
            {
                if (uPrint.tFlag.bMpptTask)
                    log_w("bMpptTask:等待设置充电功率回复超时");
                
                cQueue_GotoStep(p_task, STEP_FORWARD);
                break;
            }

            #pragma pack(1)
            struct
            {
				u16		us_in_pwr;
			}t_mppt_chg;
            #pragma pack()

            /* 校验并读取数据 */
            if (usQueue_ReadReply(p_task, &t_mppt_chg, sizeof(t_mppt_chg)) != sizeof(t_mppt_chg))
            {
                if (uPrint.tFlag.bMpptTask && uPrint.tFlag.bImportant)
                    log_w("bMpptTask:返回数据错误");
                cQueue_GotoStep(p_task, STEP_FORWARD);
                break;
            }

            if (t_mppt_chg.us_in_pwr != us_chg_pwr)
            {
                if (uPrint.tFlag.bMpptTask)
                    log_w("bMpptTask:回复功率%d和设置功率%d不一致", t_mppt_chg.us_in_pwr, us_chg_pwr);
                
                cQueue_GotoStep(p_task, STEP_FORWARD);
                break;
            }

            if (us_chg_pwr)
                bMppt_SetDevState(DS_WORK);
            else
                bMppt_SetDevState(DS_SHUT_DOWN);
            
            cQueue_GotoStep(p_task, STEP_NEXT);  /* 下一步 */
        }
        break;
        
        case 2:
            tMpptRx.usMaxInPwr = us_chg_pwr * 10;
            
            if (uPrint.tFlag.bMpptTask)
                sMyPrint("bMpptTask:----设置MPPT充电功率%dW完成----\r\n", us_chg_pwr);

            cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
            return;
            
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
            log_w("bMpptTask:设置MPPT充电功率任务等待超时,步骤%d", p_task->ucStep);
        
        if (tMppt.eDevState == DS_BOOTING)
        {
            bMppt_SetDevState(DS_SHUT_DOWN);
            c_mppt_cs_set_pwr(0);
        }
        
        cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
    }
    
    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, mpptTASK_SET_PWR_CYCLE_TIME);
    #endif  /* boardUSE_OS */
}

#endif  /* boardMPPT_EN */

