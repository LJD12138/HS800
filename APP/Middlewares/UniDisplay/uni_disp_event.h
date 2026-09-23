/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_event.h
 * Date    : 2026-09-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 统一事件总线 - 静态环形队列对外接口
 * -------------------------------------------------------
 * 线程模型:
 * 1. bDisp_PostEvent: 任意任务上下文可调用 (临界区入队 + 任务通知唤醒, 不建议 ISR);
 * 2. bDisp_EventDequeue: 仅显示任务上下文调用 (唯一消费者, 无需保护);
 * 3. 外部任务禁止直接调用页面回调, 一律经事件队列中转。
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef UNI_DISP_EVENT_H
#define UNI_DISP_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if (1)

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//
/* 统一事件码 */
typedef enum
{
	DISP_EVT_NONE = 0,
	DISP_EVT_KEY_SHORT,		/* 按键短按 (param: KeyId_E) */
	DISP_EVT_KEY_LONG,		/* 按键长按 (param: KeyId_E) */
	DISP_EVT_FAULT_POPUP,	/* 故障弹出 (param: 错误码) */
	DISP_EVT_FAULT_CLEAR,	/* 故障全清 */
	DISP_EVT_REQ_PAGE,		/* 页面请求 (param: DispPageId_E) */
	DISP_EVT_MAX
}DispEvent_E;

/* 事件消息体 */
typedef struct
{
	DispEvent_E			eEvent;
	uint32_t			ulParam;
}DispEventMsg_T;

//****************************************************Extern********************************************************************//
/* 事件投递: 任意任务上下文调用, 入队成功返回 true, 队列满返回 false (调用方可记录日志) */
bool bDisp_PostEvent(DispEvent_E e_event, uint32_t ul_param);

/* 事件出队: 仅显示任务上下文调用, 队列空返回 false */
bool bDisp_EventDequeue(DispEventMsg_T *p_msg);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* UNI_DISP_EVENT_H */
