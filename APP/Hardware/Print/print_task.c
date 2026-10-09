/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统调试输出与上位机通信任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_task.h"
#include <stdbool.h>

DebugPrint_U uPrint;

#if (boardPRINT_IFACE)
#include "Print/print_prot_frame.h"
#include "Print/print_iface.h"
#include "Print/print_queue_task.h"
#include "Sys/sys_task.h"

#if (boardUSB_EN)
#include "Usb/usb_task.h"
#endif  /* boardUSB_EN */

#if (boardWDGT_EN)
#include "fwdgt.h"
#endif  /* boardWDGT_EN */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			printTASK_PRIO							1		/* 任务优先级(通信接收层) */
#define			printTASK_SIZE							384		/* 任务堆栈(字) */
TaskHandle_t tPrintTaskHandler = NULL;							/* 任务句柄 */
void         vPrint_Task(void *pvParameters);					/* 任务函数 */
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			printTX_BUFF_SIZE						384		/* 打印发送缓冲区大小 */

//****************************************************Parameter Initialization**************************************************//
Print_T tPrint;

lwrb_t tPrintTxBuff;
__ALIGNED(4) static u8 uca_print_tx_buff[printTX_BUFF_SIZE];

static Task_T *p_task = NULL;

#if (boardUSE_OS)
SemaphoreHandle_t PrintSemaphoreBinary = NULL;
#endif  /* boardUSE_OS */

//****************************************************Function Declaration******************************************************//
static bool b_print_task_param_init(void);

/***********************************************************************************************************************
 * 函数功能    : 任务参数初始化
 * 说明(备注)  : 初始化互斥量、环形缓冲区与上位机状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
static bool b_print_task_param_init(void)
{
    if (tpPrintTask == NULL)
        return false;

    #if (boardUSE_OS)
    PrintSemaphoreBinary = xSemaphoreCreateBinary(); 
    xSemaphoreGive(PrintSemaphoreBinary);
    #endif  /* boardUSE_OS */
    
    lwrb_reset(&tPrintTxBuff);
    
    p_task = tpPrintTask;
    
    vPrint_MyPrintParamInit();
    
    tSysInfo.uInit.tFinish.bIF_Print = 1;
    tPrint.eDevState = DS_SHUT_DOWN;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 打印任务初始化
 * 说明(备注)  : 初始化接收发送协议、队列与OS任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, 负数: 失败步骤码
 ************************************************************************************************************************/
s8 cPrint_TaskInit(void)
{
    /* 接收协议初始化 */
    if (bPrint_RecProtInit() == false)
        return -1;
    
    /* 发送协议初始化 */
    if (bPrint_SendProtInit() == false)
        return -2;
    
    /* 任务队列初始化 */
    if (bPrint_QueueInit() == false)
        return -3;
    
    /* 调试缓存器初始化 */
    lwrb_init(&tPrintTxBuff, uca_print_tx_buff, printTX_BUFF_SIZE);  

    /* 任务参数初始化 */
    if (b_print_task_param_init() == false)
        return -4;
    
    #if (boardUSE_OS)
    if (xTaskCreate((TaskFunction_t )vPrint_Task,
                    (const char*    )"PrintTask",
                    (uint16_t       )printTASK_SIZE,
                    (void*          )NULL,
                    (UBaseType_t    )printTASK_PRIO,
                    (TaskHandle_t*  )&tPrintTaskHandler) != pdPASS)
        return -5;

    vQueue_BindTaskHandler(tpPrintTask, tPrintTaskHandler);
    #endif  /* boardUSE_OS */

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 打印任务主循环
 * 说明(备注)  : 轮询队列任务，管理看门狗喂狗与USB接口使能
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vPrint_Task(void *pvParameters)
{
    #if (boardUSE_OS)
    for (;;)
    #endif  /* boardUSE_OS */
    { 
        #if (boardWDGT_EN && boardPRINT_IFACE)
        vFwdgt_Reload();
        #endif  /* boardWDGT_EN && boardPRINT_IFACE */
        
        if (p_task == NULL)
        {
            if (p_task == NULL)
                b_print_task_param_init();
            
            #if (boardUSE_OS)
            vTaskDelay(500);
            continue;
            #else
            return;
            #endif  /* boardUSE_OS */
        }

        #if (boardUSB_EN)
        if (tUsb.eDevState == DS_SHUT_DOWN || tSysInfo.eDevState == DS_UPDATE_MODE)
            printIFACE_EN_ON();
        else
            printIFACE_EN_OFF();
        #endif  /* boardUSB_EN */

        #if (boardPRINT_IFACE == 7)
        vUsbCdc_Tick();
        #endif  /* boardPRINT_IFACE == 7 */
        
        vQueue_TaskPoll(p_task, printTASK_CYCLE_TIME);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 触发串口发送缓存数据
 * 说明(备注)  : 从环形缓冲区提取数据并通过串口发送
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功开始发送, false: 忙或无数据
 ************************************************************************************************************************/
bool bPrint_SendDataToUsart(void)
{
    static u16 us_data_len;
    bool flag = false;
    
    if (tpPrintProtoTx == NULL)
        return false;
    
    #if (boardUSE_OS)
    if (PrintSemaphoreBinary == NULL)
        return false;
    #endif  /* boardUSE_OS */
    
    if (bPrint_CheckSendFinish() == false)
        return false;
    
    #if (boardUSE_OS)
    if (xSemaphoreTake(PrintSemaphoreBinary, (TickType_t)0) == pdPASS) 
    #endif  /* boardUSE_OS */
    {
        us_data_len = lwrb_get_full(&tPrintTxBuff);
        
        #if (boardPRINT_IFACE)
        if (us_data_len)
            flag = bPrint_DataSendStart(us_data_len);   
        #endif  /* boardPRINT_IFACE */
        
        #if (boardUSE_OS)
        xSemaphoreGive(PrintSemaphoreBinary);
        #endif  /* boardUSE_OS */
    }
    return flag;
}

/***********************************************************************************************************************
 * 函数功能    : 接收超时与连接检测定时器
 * 说明(备注)  : 定时计算数据帧接收超时并复位接收缓冲区
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vPrint_RecTickTimer(void)
{
    if (tpPrintProtoRx == NULL)
        return;
    
    /* 数据帧接收超时计算 */
    if (tpPrintProtoRx->usRecOverTimeCnt > 0)
    {    
        tpPrintProtoRx->usRecOverTimeCnt--;
        if (tpPrintProtoRx->usRecOverTimeCnt == 0)        
            cBaiku_ResetRxBuff(tpPrintProtoRx);
    }
    
    /* 模块连接超时计算 */
    if (tpPrintProtoRx->usLostOverTimeCnt > 0)
    {    
        tpPrintProtoRx->usLostOverTimeCnt--;
        if (tpPrintProtoRx->usLostOverTimeCnt == 0)
            cBaiku_ResetRxBuff(tpPrintProtoRx);
    }
}

#endif  /* boardPRINT_IFACE */
