/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application\Sys
 * File    : sys_queue_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统总任务队列调度分发实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (boardPRINT_IFACE)
#include "Print/print_iface.h"
#endif  /* boardPRINT_IFACE */

#if (boardBMS_EN)
#include "MD_Bms/md_bms_iface.h"
#endif  /* boardBMS_EN */

#if (boardADC_EN)
#include "Adc/adc_iface.h"
#endif  /* boardADC_EN */

#if (boardWDGT_EN)
#include "fwdgt.h"
#endif  /* boardWDGT_EN */

#if (boardLED_EN)
#include "Led/led_iface.h"
#endif  /* boardLED_EN */

#include "systick.h"
#include "boot_info.h"
#include "flash_allot_table.h"

#if (1)
//****************************************************Macros********************************************************************//
typedef void (*pAppFunction)(void);

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Task_T *tpSysTask = NULL;      /* 系统总任务控制块指针 */
static pAppFunction S_pApplication = NULL;  /* APP 入口函数指针(固定地址存储,不随栈切换失效) */

//****************************************************Function Declaration******************************************************//
static bool b_task_manage_func_cb(Task_T *p_task);
static void v_add_task_return_func_cb(Task_T *p_task, u8 num);

/***********************************************************************************************************************
 * 函数功能    : 系统任务队列初始化
 * 说明(备注)  : 初始化系统主任务队列对象并注册装载与返回回调函数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 成功, false 失败
 ************************************************************************************************************************/
bool bSys_QueueInit(void)
{
    if (cQueue_TaskInit(&tpSysTask, 8, 12, b_task_manage_func_cb, v_add_task_return_func_cb) <= 0)
    {
        if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
            log_e("bSysTask:tpSysTask任务对象初始化失败");

        return false;
    }
    else if (tpSysTask == NULL)
    {
        if (uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
            log_e("bSysTask:tpSysTask任务对象创建失败");

        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 任务管理装载回调函数
 * 说明(备注)  : 根据系统当前状态或队列消息分发装载下一任务处理函数
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 装载目标任务函数与参数
 * 返回值      : bool: true 成功, false 失败
 ************************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *p_task)
{
    TaskItem_T t_item;

    if (p_task == NULL)
        return false;

    vQueue_ResetTaskState(p_task);

    if (tSysInfo.uInit.tFinish.bIF_SysTask == 0)
    {
        p_task->ucID = STI_INIT;
        p_task->usInParam = 0;
    }
    else if (bQueue_PopTask(p_task, &t_item))
    {
        p_task->ucID = t_item.ucId;
        p_task->usInParam = t_item.usParam;
    }
    else
    {
        p_task->ucID = STI_NULL;
        p_task->usInParam = 0;
    }

    switch (p_task->ucID)
    {
        case STI_INIT:
        {
            p_task->vp_func = v_sys_queue_task_init;
        }
        break;

        case STI_ENTER_APP:
        {
            p_task->vp_func = v_sys_queue_task_enter_app;
        }
        break;

        case STI_ERR:
        {
            p_task->vp_func = v_sys_queue_task_err;
        }
        break;

        case STI_RESET:
        {
            p_task->vp_func = v_sys_queue_task_reset;
        }
        break;

        #if (boardUPDATE)
        case STI_UPDATE:
        {
            p_task->vp_func = v_sys_queue_task_update;
        }
        break;
        #endif  /* boardUPDATE */

        #if (boardDISPLAY_EN)
        case STI_DISPLAY:
        {
            p_task->vp_func = v_sys_queue_task_disp;
        }
        break;
        #endif  /* boardDISPLAY_EN */

        #if (boardLOW_POWER)
        case STI_LOW_POWER:
        {
            p_task->vp_func = v_sys_queue_task_low_power;
        }
        break;
        #endif  /* boardLOW_POWER */

        case STI_NULL:
        default:
        {
            p_task->vp_func = NULL;
            p_task->usInParam = 0;
        }
        break;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 队列事件及添加回调函数
 * 说明(备注)  : 处理任务入队等事件
 * 传入参数    : p_task: 任务控制块指针; num: 事件类型/回调编号
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_add_task_return_func_cb(Task_T *p_task, u8 num)
{
    (void)p_task;

    switch (num)
    {
        case QE_TASK_POSTED:
        {
            #if (boardUSE_OS)
            xTaskNotifyGive(tSysTaskHandler);
            #endif  /* boardUSE_OS */
        }
        break;

        default:
        {
        }
        break;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 跳转到 APP 应用程序
 * 说明(备注)  : 验证栈顶指针与复位向量合法性，反初始化外设与中断后执行跳转
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 成功不返回; 失败返回负错误码 (-1: 栈顶错误, -2: 处于擦除态, -3: 跳转异常, -4: 地址范围非法, -5: 标志落盘失败)
 ************************************************************************************************************************/
s8 cSys_JumpToApp(void)
{
    uint32_t JumpAddress = 0;

    /* APP 栈顶指针合法性检查 (SRAM 范围: 0x20000000 ~ 0x2001FFFF) */
    if (0x20000000 != ((*(__IO uint32_t *)flashAPP_START) & 0x2FFE0000))
        return -1;

    /* APP 复位中断入口地址合法性检查 (必须在 APP 代码区范围内且 Thumb 模式位为 1) */
    JumpAddress = *(__IO uint32_t *)(flashAPP_START + 4);
    if (JumpAddress < flashAPP_START || JumpAddress > flashAPP_END || (JumpAddress & 0x01) == 0)
        return -4;

    if (tBootMemParam.tParam.eAppState == AS_ERASE)
        return -2;

    #if (boardADC_EN)
    vAdc_DeInit();
    #endif  /* boardADC_EN */

    #if (boardLED_EN)
    vLed_IfaceDeInit();
    #endif  /* boardLED_EN */

    #if (boardLOW_POWER)
    vPrint_EnterLowPower();                                 /* 关闭串口 */
    vGPIO_EnterApp();                                       /* 关闭中断 */
    vAdc_IoEnterLowPower();                                 /* 关闭 AD */
    #endif  /* boardLOW_POWER */

    #if (boardBMS_EN)
    vBms_IfaceDeInit();
    #endif  /* boardBMS_EN */

    #if (boardPRINT_IFACE)
    /* 设置下次重启直接进 APP;落盘失败禁止带病跳转,由任务重试机制兜底 */
    if (cBoot_CtrlUpdate(false, AS_OK) <= 0)
        return -5;
    vPrint_IfaceDeInit();
    #endif  /* boardPRINT_IFACE */

    vSys_MsDelay(2);

    #if (boardWDGT_EN)
    vFwdgt_Reload();
    #endif  /* boardWDGT_EN */

    __disable_irq();                                        /* 关闭所有中断 */

    /* 关闭所有 NVIC 中断并清除所有挂起中断标志 */
    for (int i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    /* 关闭滴答定时器，复位到默认值 */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    S_pApplication = (pAppFunction)JumpAddress;

    __set_MSP(*(__IO uint32_t *)flashAPP_START);           //APP程序堆栈指针起始(用户代码区的第一个字用于存放栈顶地址)

    S_pApplication();                                      //跳转到Reset_Handler即APP

    return -3;
}

#endif  /* 1 */
