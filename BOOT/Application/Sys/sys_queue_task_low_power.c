/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application\Sys
 * File    : sys_queue_task_low_power.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统低功耗队列任务与休眠管理实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"

#if (boardLOW_POWER)
#include "Print/print_task.h"
#include "rtc_wakeup.h"

#if (boardWDGT_EN)
#include "fwdgt.h"
#endif  /* boardWDGT_EN */

#if (1)
//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//
void v_enter_sleep(uint16_t time);


/***********************************************************************************************************************
 * 函数功能    : 系统低功耗队列任务执行函数
 * 说明(备注)  : 驱动低功耗流程步骤
 * 传入参数    : tp_task: 队列任务控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_sys_queue_task_low_power(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
        case 0:
        {
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;

        default:
        {
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 进入休眠模式
 * 说明(备注)  : 配置唤醒中断并进入低功耗待机
 * 传入参数    : time: 延时节拍
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_enter_sleep(uint16_t time)
{
    static uint16_t enter_sleep_cnt;

    if (++enter_sleep_cnt >= time)
    {
        enter_sleep_cnt = 0;

        bExti_SensorTriFlag = false;

        rtc_configuration(ALARM_TIME_INTERVAL);

        #if (boardWDGT_EN)
        vFwdgt_EnterLowPower();
        #endif  /* boardWDGT_EN */

        vPrint_EnterLowPower();
        vLcd_EnterLowPower();
        vAdc_IoEnterLowPower();
        vKey_EnterLowPower();

        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL = 0;

        system_reset_clock_8m_irc8m();
        pmu_to_sleepmode(WFI_CMD);

        SystemInit();
        vSys_TickConfig();
        cPrint_TaskInit();
        vGPIO_Init();
        vAdc_Init();
        vLcd_ExitLowPower();
        rtc_close();

        #if (boardWDGT_EN)
        vFwdgt_ExitLowPower();
        #endif  /* boardWDGT_EN */

        usBootRun1MsCnt = 0;
        uPrint.tFlag.bSysTask = 0;
        uPrint.tFlag.Sensor = 0;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 退出低功耗处理
 * 说明(备注)  : 恢复外设时钟与驱动
 * 传入参数    : ulExpectedIdleTime: 预计空闲时间
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void PostSleepProcessing(uint32_t ulExpectedIdleTime)
{
    SystemCoreClock = ((uint32_t)120000000);
    system_reset_clock_120m_irc8m();

    vGPIO_ExitLowPower();
    #if (boardPRINT_IFACE)
    vPrint_Init();
    #endif  /* boardPRINT_IFACE */
    vLcd_ExitLowPower();
    vBuz_ExitLowPower();
    vKey_ExitLowPower();
    bAdc_ExitLowPower();
    vCount_ExitLowPower();
    vDCAC_ExitLowPower();
    vLight_ExitLowPower();
    vUSB_ExitLowPower();
    tSysInfo.eWaitBootStep = WBS_WAIT;
}

#endif  /* 1 */
#endif  /* boardLOW_POWER */
