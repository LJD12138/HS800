/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
 * File    : md_bms_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS发送及调度任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_queue_task.h"
#include "MD_Bms/md_bms_prot_frame.h"

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  /* boardUPDATE */

//****************************************************Macros********************************************************************//
#if (boardUSE_OS)
#define			BMS_TASK_PRIO							3		/* 任务优先级(通信执行层) */
#define			BMS_TASK_SIZE							256		/* 任务堆栈大小 */
TaskHandle_t tBmsTaskHandler = NULL;
void         vBms_Task(void *pvParameters);
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Bms_T tBms;
static Task_T *s_tpTask = NULL;

//****************************************************Function Declaration******************************************************//
static bool b_bms_task_param_init(void);


/***********************************************************************************************************************
 * 函数功能    : 电池包任务参数初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
static bool b_bms_task_param_init(void)
{
    if (tpBmsTask == NULL)
        return false;

    memset(&tBms, 0, sizeof(tBms));
    lwrb_reset(&tpBmsTask->tQueueBuff);
    s_tpTask = tpBmsTask;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : BMS任务初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 1: 成功, 负数: 失败步骤码
 ************************************************************************************************************************/
/***********************************************************************************************************************
 * 函数功能    : 电池包任务初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, -1: 发送协议初始化失败, -2: 队列初始化失败, -3: 参数初始化失败, -4: 任务创建失败
 ************************************************************************************************************************/
s8 cBms_TaskInit(void)
{
    if (bBms_SendProtInit() == false)
        return -1;

    if (bBms_QueueInit() == false)
        return -2;

    if (b_bms_task_param_init() == false)
        return -3;

    #if (boardUSE_OS)
    if (xTaskCreate((TaskFunction_t )vBms_Task,
                    (const char*    )"BmsTask",
                    (uint16_t       )BMS_TASK_SIZE,
                    (void*          )NULL,
                    (UBaseType_t    )BMS_TASK_PRIO,
                    (TaskHandle_t*  )&tBmsTaskHandler) != pdPASS)
        return -4;
    vQueue_BindTaskHandler(tpBmsTask, tBmsTaskHandler);
    #endif  /* boardUSE_OS */

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 电池包任务主循环
 * 说明(备注)  : 无
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBms_Task(void *pvParameters)
{
    #if (boardUSE_OS)
    for (;;)
    #endif  /* boardUSE_OS */
    {
        if (s_tpTask == NULL)
        {
            b_bms_task_param_init();

            #if (boardUSE_OS)
            vTaskDelay(500);
            continue;
            #else
            return;
            #endif  /* boardUSE_OS */
        }

        vQueue_TaskPoll(s_tpTask, bmsTASK_CYCLE_TIME);
    }
}

#endif  /* boardBMS_EN */

