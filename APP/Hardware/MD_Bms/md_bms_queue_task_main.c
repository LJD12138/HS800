/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task_main.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 队列主任务函数实现
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
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

//****************************************************Macros********************************************************************//
#define			bmsTASK_PARAM_CYCLE_TIME				500

//****************************************************Function Declaration******************************************************//
static s8 c_bms_proc_rec_param(void);

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : BMS 队列主任务
 * 说明(备注)  : 无
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_main(Task_T *p_task)
{
    s8 c_result = 0;

    /* 非工作状态下,检查电池包是否开启中 */
    if (tSysInfo.eDevState == DS_SHUT_DOWN)
    {
        /* 处于非关闭状态,发送指令关闭 */
        if (tBms.eDevState >= DS_ERR)
            cBms_Switch(SO_KEY, ST_OFF, false);
    }
    
    /* 队列里面有任务 */
    if (lwrb_get_full(&p_task->tQueueBuff))  
    {
        cQueue_GotoStep(p_task, STEP_END);      /* 结束 */
        return;
    }
    
    switch (p_task->ucStep)
    {
        case 0:
        {
            /* 获取主机BMS参数 */
            c_result = c_bms_cs_get_param(bmsGET_PARAM_OBJ);
            if (c_result > 0)
                cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;
        
        case 1:
        {
            /* 处理接收数据 */
            c_bms_proc_rec_param();
            cBms_CheckPerm();

            if (bSys_LowVoltReqChg() == true)
                c_bms_cs_req_chg();
            
            cQueue_GotoStep(p_task, 0);         /* 重新轮询 */
        }
        break;
            
        default:
        {
            cQueue_GotoStep(p_task, STEP_END);  /* 结束 */
        }
        break;
    }
    
    /* 队列里面有任务 */
    if (lwrb_get_full(&p_task->tQueueBuff))  
    {
        cQueue_GotoStep(p_task, STEP_END);      /* 结束 */
        return;
    }
    
    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, bmsTASK_PARAM_CYCLE_TIME); /* 周期节拍；新任务投递立即唤醒抢占 */
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 处理 BMS 接收到的参数
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : s8: 1-正常处理完成, 0-系统未处于开机状态
 ************************************************************************************************************************/
__STATIC_INLINE s8 c_bms_proc_rec_param(void)
{
    if (tSysInfo.eDevState < DS_BOOTING)
        return 0;

    /* BMS状态不同步 */
    if (bSys_IsWorkState() == true && tBmsRx.tState.ucSysState < DS_BOOTING)
    {
        cBms_Switch(SO_KEY, ST_ON, true);

        if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
            log_w("bBmsTask:系统和BMS状态不一致 系统状态%d BMS状态%d \r\n", tSysInfo.eDevState, tBmsRx.tState.ucSysState);
    }
    else if (bSys_IsShutDownState() == true && tBmsRx.tState.ucSysState > DS_BOOTING)
    {
        cBms_Switch(SO_KEY, ST_ON, false);

        if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
            log_w("bBmsTask:系统和BMS状态不一致 系统状态%d BMS状态%d \r\n", tSysInfo.eDevState, tBmsRx.tState.ucSysState);
    }
    
    /*----------------------------------故障处理--------------------------------------*/
    static uint8_t s_uc_clear_err_cnt = 0;

    /* 充电过温检测 */
    static uint8_t s_uc_chg_temp_err_cnt = 0;
    if (tBms.sMaxTemp >= tAppMemParam.tBMS.cChgMaxTemp)
    {
        if (tBms.uErrCode.tCode.bSysChgOT == 0 && bBms_GetBmsChgState() == true)
        {
            s_uc_chg_temp_err_cnt++;
            if (s_uc_chg_temp_err_cnt >= 2)
            {
                s_uc_chg_temp_err_cnt = 0;
                s_uc_clear_err_cnt = 0;
                bBms_SetErrCode(BEC_SYS_CHG_OT, true);
            }
        }
    }
    else 
    {
        s_uc_chg_temp_err_cnt = 0;
        if (tBms.uErrCode.tCode.bSysChgOT == 1)
        {
            if (tBms.sMaxTemp <= (tAppMemParam.tBMS.cChgMaxTemp - 5))
            {
                s_uc_clear_err_cnt++;
                if (s_uc_clear_err_cnt >= 6)
                {
                    s_uc_clear_err_cnt = 0;
                    bBms_SetErrCode(BEC_SYS_CHG_OT, false);
                }
            }
        }
    }
    
    /* 放电过温检测 */
    static uint8_t s_uc_dischg_temp_err_cnt = 0;
    if (tBms.sMaxTemp >= tAppMemParam.tBMS.cDisChgMaxTemp)  
    {
        if (tBms.uErrCode.tCode.bSysDisChgOT == 0 && bBms_GetBmsChgState() == false)
        {
            s_uc_dischg_temp_err_cnt++;
            if (s_uc_dischg_temp_err_cnt)
            {
                s_uc_dischg_temp_err_cnt = 0;
                s_uc_clear_err_cnt = 0;
                bBms_SetErrCode(BEC_SYS_DISCHG_OT, true);
            }
        }
    }
    else 
    {
        s_uc_dischg_temp_err_cnt = 0;
        if (tBms.uErrCode.tCode.bSysDisChgOT == 1)
        {
            if (tBms.sMaxTemp <= (tAppMemParam.tBMS.cDisChgMaxTemp - 5))
            {
                s_uc_clear_err_cnt++;
                if (s_uc_clear_err_cnt >= 6)
                {
                    s_uc_clear_err_cnt = 0;
                    bBms_SetErrCode(BEC_SYS_DISCHG_OT, false);
                }
            }    
        }
    }
    
    /* 充电低温报警 */
    static uint8_t s_uc_low_temp_err_cnt = 0;
    if (tBms.sMinTemp <= tAppMemParam.tBMS.cChgMinTemp)
    {
        if (tBms.uErrCode.tCode.bSysChgUT == 0 && bBms_GetBmsChgState() == true)
        {
            s_uc_low_temp_err_cnt++;
            if (s_uc_low_temp_err_cnt >= 2)
            {
                s_uc_low_temp_err_cnt = 0;
                s_uc_clear_err_cnt = 0;
                bBms_SetErrCode(BEC_SYS_CHG_UT, true);
            }
        }
    }
    else if (tBms.sMinTemp > (tAppMemParam.tBMS.cChgMinTemp + 5))
    {
        s_uc_low_temp_err_cnt = 0;
        if (tBms.uErrCode.tCode.bSysChgUT == 1)
        {
            s_uc_clear_err_cnt++;
            if (s_uc_clear_err_cnt >= 6)
            {
                s_uc_clear_err_cnt = 0;
                bBms_SetErrCode(BEC_SYS_CHG_UT, false);
            }
        }
    }
    
    /* 放电低温报警 */
    static uint8_t s_uc_dischg_low_temp_err_cnt = 0;
    if (tBms.sMinTemp <= tAppMemParam.tBMS.cDisChgMinTemp)
    {
        if (tBms.uErrCode.tCode.bSysDisChgUT == 0 && bBms_GetBmsChgState() == false)
        {
            s_uc_dischg_low_temp_err_cnt++;
            if (s_uc_dischg_low_temp_err_cnt >= 2)
            {
                s_uc_dischg_low_temp_err_cnt = 0;
                s_uc_clear_err_cnt = 0;
                bBms_SetErrCode(BEC_SYS_DISCHG_UT, true);
            }
        }
    }
    else if (tBms.sMinTemp > (tAppMemParam.tBMS.cDisChgMinTemp + 5))
    {
        s_uc_dischg_low_temp_err_cnt = 0;
        if (tBms.uErrCode.tCode.bSysDisChgUT == 1)
        {
            s_uc_clear_err_cnt++;
            if (s_uc_clear_err_cnt >= 6)
            {
                s_uc_clear_err_cnt = 0;
                bBms_SetErrCode(BEC_SYS_DISCHG_UT, false);
            }
        }
    }
    
    return 1;
}

#endif  /* boardBMS_EN */
