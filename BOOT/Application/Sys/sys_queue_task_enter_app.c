/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application\Sys
 * File    : sys_queue_task_enter_app.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统跳转进入 APP 队列任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"
#include "Sys/sys_queue_task_update.h"
#include "Print/print_task.h"
#include "Print/print_iface.h"
#include "boot_info.h"

#if (1)
//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//


/***********************************************************************************************************************
 * 函数功能    : 进入 APP 队列任务执行函数
 * 说明(备注)  : 等待打印完成后尝试执行跳转，失败则等待重试并在超限后进入升级模式
 * 传入参数    : p_task: 队列任务控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_sys_queue_task_enter_app(Task_T *p_task)
{
    static s8 ret = 0;

    #if (boardUPDATE)
    static uint8_t s_uc_illegal_addr_cnt = 0;
    #endif  /* boardUPDATE */

    switch (p_task->ucStep)
    {
        case 0:
        {
            /* 等待 Print 打印完成,5S超时强制放行,防止串口发送异常导致BOOT静默挂死 */
            #if (boardPRINT_IFACE)
            if (bPrint_CheckSendFinish() == false)
            {
                if (bQueue_IsStepTimeout(p_task, (5000 / sysTASK_CYCLE_TIME)))
                    log_e("BOOT:等待打印完成超时,强制跳转");
                else
                    break;
            }
            #endif  /* boardPRINT_IFACE */

            #if (boardIC_TYPE == boardIC_STM32H7XX)
            ret = cQSPI_MemoryMapped();
            #else
            ret = 1;
            #endif  /* boardIC_TYPE == boardIC_STM32H7XX */

            if (ret > 0)
                cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            ret = cSys_JumpToApp();
            if (ret <= 0)
                cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        case 2:
        {
            /* 延时 1S */
            if (!bQueue_IsStepTimeout(p_task, (1000 / sysTASK_CYCLE_TIME)))
                break;

            #if (boardUPDATE)
            if (++s_uc_illegal_addr_cnt > 10)
            {
                #if (boardIC_TYPE == boardIC_STM32H7XX)
                cQSPI_QuitMemoryMapped();
                #endif  /* boardIC_TYPE == boardIC_STM32H7XX */

                s_uc_illegal_addr_cnt = 0;
                cBoot_CtrlUpdate(true, AS_ERASE);
                #if (boardCONSOLE_EN)
                cUpdate_ChSelect(CT_CONSOLE, PT_BAIKU);
                #elif (boardPRINT_IFACE)
                cUpdate_ChSelect(CT_PRINT, PT_XMODEM);
                #endif  /* (boardCONSOLE_EN) */
                cQueue_GotoStep(p_task, STEP_END);
                break;
            }

            #if (boardIC_TYPE == boardIC_STM32H7XX)
            ret = cQSPI_MemoryMapped();
            #else
            ret = 1;
            #endif  /* boardIC_TYPE == boardIC_STM32H7XX */

            log_e("BOOT跳转APP失败%d,错误代码%d!!!", s_uc_illegal_addr_cnt, ret);
            #endif  /* boardUPDATE */
            
            cQueue_GotoStep(p_task, STEP_FORWARD);
        }
        break;

        default:
        {
            cQueue_GotoStep(p_task, STEP_END);
        }
        break;
    }
}

#endif  /* 1 */
