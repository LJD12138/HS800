/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\TaskQueue
 * File    : queue_task.h
 * Date    : 2026-09-12
 * Author  : LJD(291483914@qq.com)
 * Desc    : 队列任务管理模块头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef QUEUE_TASK_H
#define QUEUE_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"
#include "lwrb.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

#if (1)
//****************************************************Macros********************************************************************//
#define			QUEUE_TASK_MAX_SIZE						100		/* 最大队列任务深度 */

//****************************************************Types*********************************************************************//
typedef struct Task_T Task_T;

/* 任务调度管理函数指针 */
typedef bool (*bpTaskManageFunc)(Task_T *tp_task);

/* 任务执行函数指针 */
typedef void (*vpFunc)(Task_T *tp_task);

/* 任务队列事件及添加回调函数指针 */
typedef void (*vpAddTaskReturnFunc)(Task_T *tp_task, u8 num);

/* 任务项数据结构（紧凑存储于环形队列中） */
#pragma pack(push, 1)
typedef struct
{
	u8					ucId;				/* 任务 ID */
	u16					usParam;			/* 任务参数 */
}TaskItem_T;
#pragma pack(pop)

/* 队列事件定义（完全兼容原有回调数值） */
typedef enum
{
    QE_TASK_CLEARED     = 0,                /* 队列清空事件 */
    QE_TASK_PREEMPT     = 1,                /* 抢占模式触发事件 */
    QE_TASK_POSTED      = 2,                /* 任务入队成功（触发 OS 调度唤醒） */
    QE_STEP_JUMP        = 10,               /* 步骤指定跳转事件 */
    QE_STEP_FORWARD     = STEP_FORWARD,     /* 0xFD 步骤回退 */
    QE_STEP_NEXT        = STEP_NEXT,        /* 0xFE 步骤前进 */
    QE_STEP_END         = STEP_END,         /* 0xFF 步骤结束/全部完成 */
} QueueEvent_E;

/* 任务控制块结构体 */
struct Task_T
{
	vu8					ucID;					/* 当前任务ID */
	vu8					ucStep;					/* 当前步骤 */
	vu16				usInParam;				/* 函数参数 */
	vu16				usStepWaitCnt;			/* 步骤等待次数 */
	vu16				usStepRepeatCnt;		/* 步骤重复次数 */
	vu16				usTaskWaitCnt;			/* 任务等待次数 */
	bool				bNowRun;				/* 立刻执行 */
	u8					ucPad;					/* 填充1字节，确保后续4字节成员和结构体自然对齐 */
	bpTaskManageFunc	bp_task_manage_func;	/* 任务调度函数 */
	vpFunc				vp_func;				/* 任务函数 */
	vpAddTaskReturnFunc	vp_return_func;			/* 事件回调函数 */
	#if (boardUSE_OS)
	TaskHandle_t		tTaskHandler;			/* 绑定的 FreeRTOS 任务句柄（用于中断或异步唤醒） */
	#endif  /* boardUSE_OS */
	lwrb_t				tQueueBuff;				/* 任务队列缓存器 */
	lwrb_t				tReplyBuff;				/* 回复缓存器 */
	u8					uac_buff[];				/* 柔性数组缓冲区（包含任务队列和回复缓存器） */
};

/* 队列遥测结构体(旁挂于 Task_T,与主控制块分离;
 * 由 queue_telemetry.c 内部注册表托管,队列初始化时自动挂载) */
#if (boardHEALTH_MONITOR_EN)
typedef struct
{
	vu16				usPeakItems;		/* 遥测: 历史最大排队任务数 */
	vu16				usFullEvtCnt;		/* 遥测: 队列满(-4)丢任务事件计数 */
	vu16				usCorruptEvtCnt;	/* 遥测: 队列损坏自愈(-3)复位事件计数 */
}TaskTelemetry_T;

/* 获取任务队列的遥测块指针(未注册/未开启时返回 NULL) */
TaskTelemetry_T *tpQueue_GetTelemetry(const Task_T *task);
#endif  //boardHEALTH_MONITOR_EN

/***********************************************************************************************************************
 * 函数功能    : 步骤等待超时判定
 * 说明(备注)  : 每次调用自动累加等待计数器，达到门限后清零计数器并返回超时
 * 传入参数    : task: 任务控制块指针; timeout_ticks: 超时门限（周期次数）
 * 输出参数    : 无
 * 返回值      : true: 已超时; false: 正在等待
 ************************************************************************************************************************/
__STATIC_INLINE bool bQueue_IsStepTimeout(Task_T *task, uint16_t timeout_ticks)
{
    if (task == NULL)
        return false;
		
    task->usStepWaitCnt++;
    if (task->usStepWaitCnt >= timeout_ticks)
    {
        task->usStepWaitCnt = 0;
        return true;
    }
    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 步骤重试次数检查
 * 说明(备注)  : 每次调用自动递增重试计数器，达到上限后返回 true
 * 传入参数    : task: 任务控制块指针; max_retry: 最大重试次数
 * 输出参数    : 无
 * 返回值      : true: 重试超限; false: 仍可重试
 ************************************************************************************************************************/
__STATIC_INLINE bool bQueue_IsStepRetryOver(Task_T *task, uint16_t max_retry)
{
    if (task == NULL)
        return true;

    task->usStepRepeatCnt++;
    return (task->usStepRepeatCnt >= max_retry);
}

/***********************************************************************************************************************
 * 函数功能    : 重置步骤等待与重试计数器
 * 说明(备注)  : 重置单步骤内的等待周期计数与重试次数计数
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void vQueue_ResetStepCounters(Task_T *task)
{
    if (task != NULL)
    {
        task->usStepWaitCnt   = 0;
        task->usStepRepeatCnt = 0;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 重置任务装载状态与步骤计数器
 * 说明(备注)  : 供各任务管理回调装载新任务前统一复位运行标志、步骤索引、任务等待计数及步骤等待/重试计数器
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void vQueue_ResetTaskState(Task_T *task)
{
    if (task != NULL)
    {
        task->bNowRun         = false;
        task->ucStep          = 0;
        task->usTaskWaitCnt   = 0;
        task->usStepWaitCnt   = 0;
        task->usStepRepeatCnt = 0;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 获取当前排队的任务数量
 * 说明(备注)  : 线程安全返回当前未处理的任务项数
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : none
 * 返回值      : 排队任务数
 ************************************************************************************************************************/
__STATIC_INLINE uint16_t usQueue_GetTaskCount(const Task_T *task)
{
    if (task == NULL)
        return 0;

    return (uint16_t)(lwrb_get_full(&task->tQueueBuff) / sizeof(TaskItem_T));
}

/***********************************************************************************************************************
 * 函数功能    : 检查队列是否已满
 * 说明(备注)  : 剩余空间不足容纳 1 个任务项时返回 true
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : none
 * 返回值      : true: 已满; false: 未满
 ************************************************************************************************************************/
__STATIC_INLINE bool bQueue_IsQueueFull(const Task_T *task)
{
    if (task == NULL)
        return true;

    return (lwrb_get_free(&task->tQueueBuff) < sizeof(TaskItem_T));
}

/***********************************************************************************************************************
 * 函数功能    : 检查队列是否为空
 * 说明(备注)  : 无排队任务时返回 true
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : none
 * 返回值      : true: 空; false: 非空
 ************************************************************************************************************************/
__STATIC_INLINE bool bQueue_IsQueueEmpty(const Task_T *task)
{
    if (task == NULL)
        return true;

    return (lwrb_get_full(&task->tQueueBuff) == 0);
}

/***********************************************************************************************************************
 * 函数功能    : 获取回复/载荷缓冲区当前有效数据长度
 * 说明(备注)  : 线程安全
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : none
 * 返回值      : 缓冲区中已有数据字节数
 ************************************************************************************************************************/
__STATIC_INLINE uint16_t usQueue_GetReplyLen(const Task_T *task)
{
    if (task == NULL || task->tReplyBuff.buff == NULL)
        return 0;

    return (uint16_t)lwrb_get_full(&task->tReplyBuff);
}

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
s8   cQueue_TaskInit(Task_T** task,
                     u16 task_queue_size,
                     u16 reply_buff_size,
                     bpTaskManageFunc mfunc,
                     vpAddTaskReturnFunc aFunc);

s8   cQueue_TaskInitStatic(Task_T* task,
                           u8* buff,
                           u16 task_queue_size,
                           u16 reply_buff_size,
                           bpTaskManageFunc mfunc,
                           vpAddTaskReturnFunc aFunc);
s8   cQueue_GotoStep(Task_T* task, u8 toStep);
s8   cQueue_AddQueueTask(Task_T* task, u8 task_id, u16 in_param, bool now_run);
s8   cQueue_AddQueueTaskUrgent(Task_T* task, u8 task_id, u16 in_param);

#if (boardUSE_OS)
void vQueue_BindTaskHandler(Task_T *task, TaskHandle_t handle);
s8   cQueue_AddQueueTaskFromISR(Task_T* task, u8 task_id, u16 in_param, bool now_run, BaseType_t *pxHigherPriorityTaskWoken);
#endif

bool bQueue_PopTask(Task_T* task, TaskItem_T* item);
bool bQueue_Reset(Task_T* task);

bool bQueue_IsTaskPending(const Task_T *task, uint8_t task_id);

/* 回复/载荷缓冲区（tReplyBuff）线程安全封装接口 */
bool     bQueue_WriteReply(Task_T *task, const void *data, uint16_t len);
uint16_t usQueue_ReadReply(Task_T *task, void *out_data, uint16_t max_len);
uint16_t usQueue_PeekReply(Task_T *task, uint16_t offset, void *out_data, uint16_t len);
void     vQueue_ResetReply(Task_T *task);

/* 主任务标准化运行引擎 */
void     vQueue_TaskPoll(Task_T *task, uint32_t cycle_time_ms);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* QUEUE_TASK_H */



