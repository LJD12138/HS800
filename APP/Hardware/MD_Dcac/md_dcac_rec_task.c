/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_rec_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器通信接收任务与状态检测实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Dcac/md_dcac_rec_task.h"
#include <stdbool.h>

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_rec_data_proc.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (boardUPDATE)
#include "proto_update.h"
#include "Sys/sys_queue_task_update.h"
#include "MD_Dcac/md_dcac_queue_task_update.h"
#endif  /* boardUPDATE */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			dcacREC_TASK_PRIO						3		/* 任务优先级(通信接收层:保障数据新鲜度) */
#define			dcacREC_TASK_SIZE						256		/* 任务堆栈(字) */
TaskHandle_t tDcacRecTaskHandle = NULL;							/* 任务句柄 */
void         vDcac_RecTask(void *pvParameters);					/* 任务函数 */
#endif  /* boardUSE_OS */


//****************************************************Parameter Initialization**************************************************//
DcacRx_T tDcacRx; 

//****************************************************Function Declaration******************************************************//
static uint8_t c_check_conn_state(void);
static void v_rec_task_param_init(void);


/***********************************************************************************************************************
 * 函数功能    : 复位接收参数 BUFF
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_rec_task_param_init(void)
{
    memset(&tDcacRx, 0, sizeof(tDcacRx));
    
    /* 重置响应缓存器 */
    if (tpDcacTask != NULL && tpDcacTask->tReplyBuff.buff != NULL)
        lwrb_reset(&tpDcacTask->tReplyBuff);
    
    /* 重置接收缓冲区 */
    if (tpDcacProtoRx != NULL)
    {
        cModbus_ResetRxBuff(tpDcacProtoRx);
        tpDcacProtoRx->usRecOverTimeCnt = 0;
        tpDcacProtoRx->usLostOverTimeCnt = 0;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 逆变接收任务初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, -1: 协议初始化失败, -2: 任务创建失败
 ************************************************************************************************************************/
s8 cDcac_RecTaskInit(void)
{
    /* 接口初始化 */
    #if (boardDCAC_EN)
    vDcac_IfaceInit();
    #endif  /* boardDCAC_EN */
    
    /* 接收协议初始化 */
    if (bDcac_RecProtInit() == false)
        return -1;
    
    /* 任务参数初始化 */
    v_rec_task_param_init();
    
    /* 数据解析任务初始化 */
    #if (boardUSE_OS)
    if (xTaskCreate((TaskFunction_t )vDcac_RecTask,        /* 任务函数 */
                    (const char*    )"DcacRecTask",        /* 任务名称 */
                    (uint16_t       )dcacREC_TASK_SIZE,    /* 任务堆栈大小 */
                    (void*          )NULL,                 /* 传递给任务函数的参数 */
                    (UBaseType_t    )dcacREC_TASK_PRIO,    /* 任务优先级 */
                    (TaskHandle_t*  )&tDcacRecTaskHandle) != pdPASS) /* 任务句柄 */
        return -2;
    #endif  /* boardUSE_OS */
                
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 逆变接收任务主循环
 * 说明(备注)  : 无
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_RecTask(void *pvParameters)
{
    s8 c_result = 0;
    
    #if (boardUSE_OS)
    for (;;)
    #endif  /* boardUSE_OS */
    {
        if (tpDcacProtoRx == NULL || tpDcacProtoTx == NULL)
        {
            bDcac_RecProtInit();
            
            #if (boardUSE_OS)
            vTaskDelay(500);
            continue;
            #else
            return;
            #endif  /* boardUSE_OS */
        }
        
        /*==========================================处理接收的数据===============================*/
        #if (boardUPDATE)
        if (tDcac.eDevState == DS_UPDATE_MODE || tpDcacTask->ucID == DTI_UPDATE)
        {
            c_result = cUpdate_ProtoCheck(&tpDcacProtoRx->tRxBuff);
            if (c_result != PT_MEGMEET)
                c_result = 0;
        }
        else
        #endif  /* boardUPDATE */
        {
            c_result = cModbus_ProtoCheck(tpDcacProtoRx);
        }

        /* 数据处理 */
        if (c_result > 0)
        {
            #if (boardUPDATE)
            if (tDcac.eDevState == DS_UPDATE_MODE || tpDcacTask->ucID == DTI_UPDATE)
                c_result = c_dcac_rec_proc_megmeet_proto(tpDcacMegmeetProtoRx);
            else
            #endif  /* boardUPDATE */
            {
                c_check_conn_state();
                c_result = c_dcac_rec_proc_data(tpDcacProtoRx, tpDcacProtoTx);
                vModbus_RecEnd(tpDcacProtoRx);
            }
            
            if (c_result <= 0)
            {
                if (uPrint.tFlag.bDcacRecTask || uPrint.tFlag.bImportant)
                    log_w("bDcacRecTask:装载的数据错误,代码%d", c_result);
            }
            #if (boardUPDATE)
            else if (tDcac.eDevState == DS_UPDATE_MODE || tpDcacTask->ucID == DTI_UPDATE)
            {
                #if (boardUSE_OS)
                xTaskNotifyGive(tDcacTaskHandler); /* 升级模式通知发送任务 */
                #endif  /* boardUSE_OS */
            }
            #endif  /* boardUPDATE */
            else
            {
                #if (boardUSE_OS)
                cModbus_NotifyAck(tpDcacProtoTx, c_result); /* 唤醒等待应答的发送任务 */
                #endif  /* boardUSE_OS */
            }
        }
        else 
        {
            if (c_result == 0)
            {
                #if (boardUSE_OS)
                if (lwrb_get_full(&tpDcacProtoRx->tRxBuff) == 0)
                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* 等待任务通知（退出时清零计数） */
                else
                    ulTaskNotifyTake(pdTRUE, 10);            /* 等待后续字节到达，立即唤醒继续拆帧 */
                #endif  /* boardUSE_OS */
            }
            else 
            {
                if (uPrint.tFlag.bDcacRecTask || uPrint.tFlag.bImportant)
                    log_w("bDcacRecTask:协议解析错误,代码%d", c_result);

                #if (boardUSE_OS)
                vTaskDelay(10);
                #endif  /* boardUSE_OS */
            }
        }
    }
}

/***********************************************************************************************************************
 * 函数功能    : 检测设备的连接状态
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : uint8_t: 0-没有错误
 ************************************************************************************************************************/
static uint8_t c_check_conn_state(void)    
{
    /* 丢失后第一次连接 */
    if (tDcac.eDevState == DS_LOST || tDcac.uErrCode.tCode.bSysDevLost == true)
    {
        bDcac_SetErrCode(DEC_SYS_DEV_LOST, false);
        
        #if (boardSYS_DATA_UPADATA)
        if (!BIT_GET(tSysInfo.Mod_Exist, OL_DCAC)) /* 第一次初始化 */
        {
            STAT_SET(tSysInfo.Mod_Exist, OL_DCAC);
            Sys_Update_Element(AT_SYS_MODEXIST_ADDR, NULL, tSysInfo.Mod_Exist, true);
        }
        #endif  /* boardSYS_DATA_UPADATA */
    }
    
    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 逆变通信定时器滴答处理
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_RecTickTimer(void)
{
    if (tpDcacProtoRx == NULL)
        return;
    
    /* 数据帧接收超时计算 */
    if (tpDcacProtoRx->usRecOverTimeCnt > 0)
    {    
        tpDcacProtoRx->usRecOverTimeCnt--;
    
        if (tpDcacProtoRx->usRecOverTimeCnt == 0)        
            cModbus_StepWaitOutTime(tpDcacProtoRx);
    }
    
    /* 逆变模块连接超时计算 */        
    if (tpDcacProtoRx->usLostOverTimeCnt > 0 && bSys_IsWorkState() == true)
    {    
        tpDcacProtoRx->usLostOverTimeCnt--;
    
        if (tpDcacProtoRx->usLostOverTimeCnt == 0)      /* 逆变器丢失 */    
        {
            v_rec_task_param_init();
            bDcac_SetErrCode(DEC_SYS_DEV_LOST, true);
        }
    }
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 进入低功耗
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_EnterLowPower(void)
{
    vTaskSuspend(tDcacRecTaskHandle);
    vTaskSuspend(tDcacTaskHandler);
    vDcac_IoEnterLowPower();
}

/***********************************************************************************************************************
 * 函数功能    : 退出低功耗
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_ExitLowPower(void)
{
    vDcac_IfaceInit();
    vTaskResume(tDcacRecTaskHandle);
    vTaskResume(tDcacTaskHandler);
}
#endif  /* boardLOW_POWER */

#endif  /* boardDCAC_EN */
