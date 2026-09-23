/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_rec_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 串口接收与环形缓冲管理任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Mppt/md_mppt_rec_task.h"

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_rec_data_proc.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//
#if (boardUSE_OS)
#define			mpptREC_TASK_PRIO						3		/* 任务优先级(通信接收层:保障数据新鲜度) */
#define			mpptREC_TASK_SIZE						192		/* 任务堆栈大小 */
TaskHandle_t tMpptRecTaskHandle;
void         vMppt_RecTask(void *pvParameters);
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
MpptRx_T tMpptRx;
vs16     sMpptMaxTemp = 0;

//****************************************************Function Declaration******************************************************//
static void v_rec_task_param_init(void);
static s8   c_check_conn_state(void);

/***********************************************************************************************************************
 * 函数功能    : 复位接收参数BUFF
 * 说明(备注)  : 清零接收运行结构体与回复队列
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_rec_task_param_init(void)
{
    memset(&tMpptRx, 0, sizeof(tMpptRx));
    vQueue_ResetReply(tpMpptTask);
    cModbus_ResetRxBuff(tpMpptProtoRx);
}

/***********************************************************************************************************************
 * 函数功能    : MPPT接收任务初始化
 * 说明(备注)  : 初始化Modbus接收协议、参数与OS接收任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, -1: 协议初始化失败, -2: 任务创建失败
 ************************************************************************************************************************/
s8 cMppt_RecTaskInit(void)
{
    /* 接收协议初始化 */
    if (bMppt_RecProtInit() == false)
        return -1;
    
    /* 任务参数初始化 */
    v_rec_task_param_init();

    #if (boardUSE_OS)
    if (xTaskCreate((TaskFunction_t )vMppt_RecTask,
                    (const char*    )"MpptRecTask",
                    (uint16_t       )mpptREC_TASK_SIZE,
                    (void*          )NULL,
                    (UBaseType_t    )mpptREC_TASK_PRIO,
                    (TaskHandle_t*  )&tMpptRecTaskHandle) != pdPASS)
        return -2;
    #endif  /* boardUSE_OS */
                
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : MPPT接收任务
 * 说明(备注)  : 监听串口接收帧并调用协议解包函数
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vMppt_RecTask(void *pvParameters)
{
    s8 c_result = 0;
    
    #if (boardUSE_OS)
    for (;;)
    #endif  /* boardUSE_OS */
    { 
        if (tpMpptProtoRx == NULL || tpMpptProtoTx == NULL)
        {
            bMppt_RecProtInit();
            
            #if (boardUSE_OS)
            vTaskDelay(500);
            continue;
            #else
            return;
            #endif  /* boardUSE_OS */
        }

        /* 处理接收的数据 */
        c_result = cModbus_ProtoCheck(tpMpptProtoRx);
        if (c_result > 0)
        {
            c_check_conn_state();
            c_result = c_mppt_rec_proc_data(tpMpptProtoRx, tpMpptProtoTx);
            vModbus_RecEnd(tpMpptProtoRx);
            if (c_result <= 0)
            {
                if (uPrint.tFlag.bMpptRecTask || uPrint.tFlag.bImportant)
                    log_w("bMpptRecTask:装载的数据错误,代码%d", c_result);
            }
            else
            {
                /* 通知发送任务 */
                #if (boardUSE_OS)
                xTaskNotifyGive(tMpptTaskHandler);
                #endif  /* boardUSE_OS */
            }
        }
        else 
        {
            if (c_result == 0)
            {
                #if (boardUSE_OS)
                if (lwrb_get_full(&tpMpptProtoRx->tRxBuff) == 0)
                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  /* 等待任务通知（退出时清零计数） */
                else
                    ulTaskNotifyTake(pdTRUE, 10);  /* 等待后续字节到达，立即唤醒继续拆帧 */
                #endif  /* boardUSE_OS */
            }
            else 
            {
                if (uPrint.tFlag.bMpptRecTask || uPrint.tFlag.bImportant)
                    log_w("bMpptRecTask:协议解析错误,代码%d", c_result);
                
                #if (boardUSE_OS)
                vTaskDelay(10);
                #endif  /* boardUSE_OS */
            }
        }
    }
}

/***********************************************************************************************************************
 * 函数功能    : 检测设备的连接状态
 * 说明(备注)  : 丢失后首次重连更新系统在线状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 操作成功
 ************************************************************************************************************************/
static s8 c_check_conn_state(void)  
{
    /* 丢失后第一次连接 */
    if (tMppt.eDevState == DS_LOST || tMppt.uErrCode.tCode.bDevLost == true)
    {
        bMppt_SetErrCode(MEC_SYS_DEV_LOST, false);
        
        #if (boardSYS_DATA_UPADATA)
        if (!BIT_GET(tSysInfo.Mod_Exist, OL_MPPT))  /* 第一次初始化 */
        {
            STAT_SET(tSysInfo.Mod_Exist, OL_MPPT);
            Sys_Update_Element(AT_SYS_MODEXIST_ADDR, NULL, tSysInfo.Mod_Exist, true);
        }
        #endif  /* boardSYS_DATA_UPADATA */
    }
    
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 串口接收节拍定时检测
 * 说明(备注)  : 计算数据帧超时与MPPT设备断联掉线超时
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vMppt_RecTickTimer(void)
{
    if (tpMpptProtoRx == NULL)
        return;
    
    /* 数据帧接收超时计算 */
    if (tpMpptProtoRx->usRecOverTimeCnt > 0)
    {    
        tpMpptProtoRx->usRecOverTimeCnt--;
        if (tpMpptProtoRx->usRecOverTimeCnt == 0)        
            cModbus_StepWaitOutTime(tpMpptProtoRx);
    }
    
    /* MPPT模块连接超时计算 */
    if (tpMpptProtoRx->usLostOverTimeCnt > 0 && bSys_IsWorkState() == true)
    {    
        tpMpptProtoRx->usLostOverTimeCnt--;
        if (tpMpptProtoRx->usLostOverTimeCnt == 0)  /* MPPT丢失 */
        {
            v_rec_task_param_init();
            bMppt_SetErrCode(MEC_SYS_DEV_LOST, true);
        }
    }
}

#endif  /* boardMPPT_EN */
