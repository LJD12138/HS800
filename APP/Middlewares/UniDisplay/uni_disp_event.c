/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_event.c
 * Date    : 2026-09-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 统一事件总线 - 静态环形队列实现 (多生产者/单消费者)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "uni_disp_event.h"

#if (boardDISPLAY_EN)
#include "uni_disp_port.h"

//****************************************************Parameter Initialization**************************************************//
#define			dispEVT_QUEUE_DEPTH						(4U)	/* 事件队列深度 */

static DispEventMsg_T   S_atEvtQueue[dispEVT_QUEUE_DEPTH];
static volatile uint8_t S_ucEvtHead = 0;    /* 写指针: 仅生产者侧(临界区内)修改 */
static volatile uint8_t S_ucEvtTail = 0;    /* 读指针: 仅显示任务修改 */

/***********************************************************************************************************************
 * 函数功能    : 事件入队 (任意任务上下文)投递事件
 * 说明(备注)  : 多生产者并发入队, 头指针推进必须临界区保护; 队列满时丢弃并返回 false
 * 传入参数    : e_event: 事件码; ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : true: 入队成功; false: 事件码非法或队列满(事件被丢弃)
 ************************************************************************************************************************/
bool bDisp_PostEvent(DispEvent_E e_event, uint32_t ul_param)
{
    uint8_t uc_next;
    bool b_ok = false;

    if (e_event > DISP_EVT_NONE && e_event < DISP_EVT_MAX)
    {
        mainENTER_CRITICAL();
        uc_next = (uint8_t)((S_ucEvtHead + 1U) % dispEVT_QUEUE_DEPTH);
        if (uc_next != S_ucEvtTail)             /* 未满 */
        {
            S_atEvtQueue[S_ucEvtHead].eEvent  = e_event;
            S_atEvtQueue[S_ucEvtHead].ulParam = ul_param;
            S_ucEvtHead = uc_next;
            b_ok = true;
        }
        mainEXIT_CRITICAL();
    }

    if (b_ok)
        vDisp_PortWakeTask();                    /* 任务通知唤醒显示任务, 事件即时响应 */

    return b_ok;
}

/***********************************************************************************************************************
 * 函数功能    : 事件出队 (仅显示任务上下文调用)
 * 说明(备注)  : 唯一消费者, 无需临界区保护
 * 传入参数    : p_msg: 出队事件存放指针
 * 输出参数    : p_msg: 事件码与参数
 * 返回值      : true: 取到事件; false: 队列空
 ************************************************************************************************************************/
bool bDisp_EventDequeue(DispEventMsg_T *p_msg)
{
    if (p_msg == NULL || S_ucEvtTail == S_ucEvtHead)
        return false;                            /* 空 */

    p_msg->eEvent  = S_atEvtQueue[S_ucEvtTail].eEvent;
    p_msg->ulParam = S_atEvtQueue[S_ucEvtTail].ulParam;
    S_ucEvtTail = (uint8_t)((S_ucEvtTail + 1U) % dispEVT_QUEUE_DEPTH);
    return true;
}

#endif  /* boardDISPLAY_EN */
