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

#if (boardDISPLAY_EN)

//****************************************************Macros********************************************************************//
#ifndef			dispEVT_QUEUE_DEPTH
#define			dispEVT_QUEUE_DEPTH					(4U)		/* 事件队列深度 (可用容量 = 深度 - 1; 项目 board 头可覆盖) */
#endif

//****************************************************Types*********************************************************************//
/* 统一事件码 */
typedef enum
{
	DISP_EVT_NONE = 0,
	DISP_EVT_KEY_SHORT,		/* 按键短按 (param: KeyId_E) */
	DISP_EVT_KEY_LONG,		/* 按键长按 (param: KeyId_E) */
	DISP_EVT_REQ_PAGE,		/* 页面请求 (param: DispPageId_E; 调试后门: 显式页切换, 生产导航由场景调度接管) */
	DISP_EVT_BACK,			/* 返回请求 (param: 无; 框架默认处理: 页面栈非空弹出返回, 空栈无行为; 页面可自行消费实现兜底) */
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

/* 事件溢出丢弃计数 (任意任务上下文): 队满丢弃累计值, 调试用 (单调递增, 不复位) */
uint32_t ulDisp_GetEventDropCnt(void);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* UNI_DISP_EVENT_H */
