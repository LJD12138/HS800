/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\TaskQueue
 * File    : queue_task.c
 * Date    : 2026-09-12
 * Author  : LJD(291483914@qq.com)
 * Desc    : 队列任务管理模块
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "queue_task.h"
#include "check.h"

#if (boardHEALTH_MONITOR_EN)
#include "queue_telemetry.h"
#endif  /* boardHEALTH_MONITOR_EN */

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

#if (1)
//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//
static bool b_is_task_in_queue(const Task_T *p_task, const u8 *p_target_item);
static bool b_lwrb_write_front(lwrb_t *buff, const void *data, lwrb_sz_t len);

/***********************************************************************************************************************
 * 函数功能    : 队列任务动态初始化
 * 说明(备注)  : 分配控制块与柔性缓冲区，并初始化任务队列与回复缓存器
 * 传入参数    : task: 任务控制块二级指针; task_queue_size: 任务队列深度（可容纳任务项数量，1~100）;
 *               reply_buff_size: 回复/载荷缓冲区字节数; mfunc: 任务管理/调度函数指针; rFunc: 事件通知回调函数指针
 * 输出参数    : *task: 指向分配的任务控制块
 * 返回值      : 1: 成功; -1: 参数错误; -2: 内存分配失败
 ************************************************************************************************************************/
s8 cQueue_TaskInit(Task_T** task,
                   u16 task_queue_size,
                   u16 reply_buff_size,
                   bpTaskManageFunc mfunc,
                   vpAddTaskReturnFunc rFunc)
{
    s8 c_result = 1;
    
    if (task == NULL || mfunc == NULL || task_queue_size == 0 || task_queue_size > QUEUE_TASK_MAX_SIZE)
        return -1;
    
    /* 任务项占 sizeof(TaskItem_T) 字节，lwrb 需多消耗 1 字节区分空满；4 字节向上对齐确保 tReplyBuff 自然对齐 */
    size_t queue_bytes = (size_t)task_queue_size * sizeof(TaskItem_T) + 1;
    size_t queue_aligned_bytes = (queue_bytes + 3) & ~((size_t)3);
    size_t total_size = sizeof(Task_T) + queue_aligned_bytes + reply_buff_size;
    
    #if(boardUSE_OS)
    *task = (Task_T *)pvPortMalloc(total_size);
    #else
    *task = (Task_T *)malloc(total_size);
    #endif  /* boardUSE_OS */
    
    if (*task != NULL)
    {
        memset(*task, 0, total_size);
        
        (*task)->bp_task_manage_func = mfunc;
        (*task)->vp_return_func = rFunc;
        
        #if(boardUSE_OS)
        (*task)->tTaskHandler = NULL;
        #endif  /* boardUSE_OS */
        
        /* 初始化任务队列缓存器（使用柔性数组的前部分） */
        lwrb_init(&(*task)->tQueueBuff, &(*task)->uac_buff[0], queue_aligned_bytes);
        lwrb_reset(&(*task)->tQueueBuff);
        
        /* 初始化回复缓存器（使用柔性数组的后部分） */
        if (reply_buff_size > 0)
        {
            lwrb_init(&(*task)->tReplyBuff, &(*task)->uac_buff[queue_aligned_bytes], reply_buff_size);
            lwrb_reset(&(*task)->tReplyBuff);
        }

        #if (boardHEALTH_MONITOR_EN)
        vQueue_TelAttach(*task);        /* 队列初始化成功,自动旁挂遥测槽 */
        #endif  //boardHEALTH_MONITOR_EN
    }
    else
        c_result = -2;
    
    return c_result;
}

/***********************************************************************************************************************
 * 函数功能    : 队列任务静态初始化
 * 说明(备注)  : 使用用户提供的静态内存初始化任务控制块与缓冲区
 * 传入参数    : task: 任务控制块指针; buff: 静态缓冲区首地址; task_queue_size: 任务队列深度（可容纳任务项数量，1~100）;
 *               reply_buff_size: 回复/载荷缓冲区字节数; mfunc: 任务管理/调度函数指针; rFunc: 事件通知回调函数指针
 * 输出参数    : task: 初始化完成的任务控制块
 * 返回值      : 1: 成功; -1: 参数错误
 ************************************************************************************************************************/
s8 cQueue_TaskInitStatic(Task_T* task,
                         u8* buff,
                         u16 task_queue_size,
                         u16 reply_buff_size,
                         bpTaskManageFunc mfunc,
                         vpAddTaskReturnFunc rFunc)
{
    if (task == NULL || buff == NULL || mfunc == NULL || 
        task_queue_size == 0 || task_queue_size > QUEUE_TASK_MAX_SIZE)
    {
        return -1;
    }
    
    size_t queue_bytes = (size_t)task_queue_size * sizeof(TaskItem_T) + 1;
    size_t queue_aligned_bytes = (queue_bytes + 3) & ~((size_t)3);
    
    memset(task, 0, sizeof(Task_T));
    task->bp_task_manage_func = mfunc;
    task->vp_return_func = rFunc;
    
    #if(boardUSE_OS)
    task->tTaskHandler = NULL;
    #endif  /* boardUSE_OS */
    
    lwrb_init(&task->tQueueBuff, &buff[0], queue_aligned_bytes);
    lwrb_reset(&task->tQueueBuff);
    
    if (reply_buff_size > 0)
    {
        lwrb_init(&task->tReplyBuff, &buff[queue_aligned_bytes], reply_buff_size);
        lwrb_reset(&task->tReplyBuff);
    }

    #if (boardHEALTH_MONITOR_EN)
    vQueue_TelAttach(task);             /* 队列初始化成功,自动旁挂遥测槽 */
    #endif  //boardHEALTH_MONITOR_EN

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 让任务执行到指定步骤
 * 说明(备注)  : 支持单步前进、回退、跳转与结束；到达 STEP_END 时置位 bNowRun 并返回 1
 * 传入参数    : task: 任务控制块指针
 *               toStep: 目标步骤枚举或具体步骤号
 * 输出参数    : none
 * 返回值      : 1: 步骤结束，触发新调度; 0: 普通步骤切换; -1: 参数错误
 ************************************************************************************************************************/
s8 cQueue_GotoStep(Task_T* task, u8 toStep)
{
    s8 c_result = 0;
    u8 uc_cb_event = 0;
    bool b_has_cb = false;
    bool b_finish_cb = false;
    
    if (task == NULL)
        return -1;
    
    mainENTER_CRITICAL();
    
    task->usStepWaitCnt = 0;
    task->usStepRepeatCnt = 0;
    
    switch (toStep)
    {
        case STEP_FORWARD:
        {
            uc_cb_event = QE_STEP_FORWARD;
            b_has_cb = true;
            if (task->ucStep > 0)
                task->ucStep--;
        }
        break;
        
        case STEP_NEXT:
        {
            uc_cb_event = QE_STEP_NEXT;
            b_has_cb = true;
            task->ucStep++;
        }
        break;
        
        case STEP_END:
        {
            task->ucStep = STEP_END;
        }
        break;
        
        default:
        {
            uc_cb_event = QE_STEP_JUMP;
            b_has_cb = true;
            task->ucStep = toStep;
        }
        break;
    }
    
    if (task->ucStep >= STEP_END)
    {
        b_finish_cb = true;
        task->bNowRun = true;
        c_result = 1;
    }
    
   mainEXIT_CRITICAL();
    
    /* 退出临界区后执行用户回调，防止长时间关中断 */
    if (task->vp_return_func != NULL)
    {
        if (b_has_cb)
            task->vp_return_func(task, uc_cb_event);
        if (b_finish_cb)
            task->vp_return_func(task, QE_STEP_END);
    }
    
    return c_result;
}

/***********************************************************************************************************************
 * 函数功能    : 添加队列任务
 * 说明(备注)  : 往任务队列中添加数据；添加后根据 now_run 或空闲状态自动就绪
 * 传入参数    : task: 任务控制块指针
 *               task_id: 任务 ID
 *               in_param: 任务参数
 *               now_run: true: 清空现有队列立刻抢占运行; false: 排队正常入队
 * 输出参数    : none
 * 返回值      : 2: 抢占执行成功; 1: 正常入队成功; 0: 重复忽略; -2: 指针为空; -3: 队列损坏重置; -4: 队列已满
 ************************************************************************************************************************/
s8 cQueue_AddQueueTask(Task_T* task, u8 task_id, u16 in_param, bool now_run)
{
    u8 uca_write_array[sizeof(TaskItem_T)];
    bool b_call_cb_cleared = false;
    bool b_call_cb_preempt = false;
    #if (boardHEALTH_MONITOR_EN)
    TaskTelemetry_T *tp_tel = NULL;
    #endif  //boardHEALTH_MONITOR_EN

    if (task == NULL)
        return -2;

    #if (boardHEALTH_MONITOR_EN)
    tp_tel = tpQueue_GetTelemetry(task);
    #endif  //boardHEALTH_MONITOR_EN

    /* 4.2 保持原样：若当前运行中的任务与待添加任务相同，则忽略不处理 */
    if (task->ucID == task_id && task->usInParam == in_param)
        return 0;
    
    uca_write_array[0] = task_id;
    memcpy(&uca_write_array[1], (u8*)&in_param, 2);
    
    mainENTER_CRITICAL();
    
    /* 立刻运行模式：重置排队缓冲区 */
    if (now_run)
    {
        lwrb_reset(&task->tQueueBuff);
        b_call_cb_cleared = true;
    }
    else
    {
        u16 us_len = (u16)lwrb_get_full(&task->tQueueBuff);
        if (us_len != 0)
        {
            /* 任务队列长度异常（非任务项整数倍） */
            if (us_len % sizeof(TaskItem_T) != 0)
            {
                lwrb_reset(&task->tQueueBuff);

                #if (boardHEALTH_MONITOR_EN)
                if (tp_tel != NULL)
                    tp_tel->usCorruptEvtCnt++;
                #endif  //boardHEALTH_MONITOR_EN

                mainEXIT_CRITICAL();

                return -3;
            }

            /* 查重：队列中已存在相同任务则不重复写入 */
            if (b_is_task_in_queue(task, uca_write_array))
            {
                mainEXIT_CRITICAL();

                return 0;
            }
        }

        /* 检查队列剩余空间是否足够容纳一个完整任务项 */
        if (lwrb_get_free(&task->tQueueBuff) < sizeof(TaskItem_T))
        {
            #if (boardHEALTH_MONITOR_EN)
            if (tp_tel != NULL)
                tp_tel->usFullEvtCnt++;
            #endif  //boardHEALTH_MONITOR_EN
            
            mainEXIT_CRITICAL();

            return -4;  /* 队列已满 */
        }
    }

    /* 写入队列并校验写入字节数 */
    if (lwrb_write(&task->tQueueBuff, uca_write_array, sizeof(TaskItem_T)) != sizeof(TaskItem_T))
    {
        #if (boardHEALTH_MONITOR_EN)
        if (tp_tel != NULL)
            tp_tel->usFullEvtCnt++;
        #endif  //boardHEALTH_MONITOR_EN

        mainEXIT_CRITICAL();

        return -4;
    }

    #if (boardHEALTH_MONITOR_EN)
    vQueue_UpdatePeak(task);
    #endif  //boardHEALTH_MONITOR_EN
    
    if (now_run || task->ucID == 0)
    {
        b_call_cb_preempt = true;
        task->bNowRun = true;
    }
    
    mainEXIT_CRITICAL();
    
    /* 退出临界区后执行回调通知 */
    if (task->vp_return_func != NULL)
    {
        if (b_call_cb_cleared)
            task->vp_return_func(task, QE_TASK_CLEARED);
        if (b_call_cb_preempt)
            task->vp_return_func(task, QE_TASK_PREEMPT);

        task->vp_return_func(task, QE_TASK_POSTED);
    }
    
    return now_run ? 2 : 1;
}

/***********************************************************************************************************************
 * 函数功能    : 添加紧急插队任务
 * 说明(备注)  : 将任务直接插入队首（Push Front），优先执行且不丢失已有排队任务
 * 传入参数    : task: 任务控制块指针
 *               task_id: 任务 ID
 *               in_param: 任务参数
 * 输出参数    : none
 * 返回值      : 1: 成功; 0: 重复忽略; -2: 指针为空; -3: 队列损坏重置; -4: 队列已满
 ************************************************************************************************************************/
s8 cQueue_AddQueueTaskUrgent(Task_T* task, u8 task_id, u16 in_param)
{
    u8 uca_write_array[sizeof(TaskItem_T)];
    #if (boardHEALTH_MONITOR_EN)
    TaskTelemetry_T *tp_tel = NULL;
    #endif  //boardHEALTH_MONITOR_EN

    if (task == NULL)
        return -2;

    #if (boardHEALTH_MONITOR_EN)
    tp_tel = tpQueue_GetTelemetry(task);
    #endif  //boardHEALTH_MONITOR_EN
    
    if (task->ucID == task_id && task->usInParam == in_param)
        return 0;

    
    uca_write_array[0] = task_id;
    memcpy(&uca_write_array[1], (u8*)&in_param, 2);
    
    mainENTER_CRITICAL();
    
    u16 us_len = (u16)lwrb_get_full(&task->tQueueBuff);
    if (us_len != 0)
    {
        if (us_len % sizeof(TaskItem_T) != 0)
        {
            lwrb_reset(&task->tQueueBuff);
            #if (boardHEALTH_MONITOR_EN)
            if (tp_tel != NULL)
                tp_tel->usCorruptEvtCnt++;
            #endif  //boardHEALTH_MONITOR_EN

            mainEXIT_CRITICAL();

            return -3;
        }

        if (b_is_task_in_queue(task, uca_write_array))
        {
            mainEXIT_CRITICAL();
            
            return 0;
        }
    }

    if (!b_lwrb_write_front(&task->tQueueBuff, uca_write_array, sizeof(TaskItem_T)))
    {
        #if (boardHEALTH_MONITOR_EN)
        if (tp_tel != NULL)
            tp_tel->usFullEvtCnt++;
        #endif  //boardHEALTH_MONITOR_EN

        mainEXIT_CRITICAL();
        
        return -4;
    }

    #if (boardHEALTH_MONITOR_EN)
    vQueue_UpdatePeak(task);
    #endif  //boardHEALTH_MONITOR_EN
    
    if (task->ucID == 0)
        task->bNowRun = true;
    
    mainEXIT_CRITICAL();
    
    if (task->vp_return_func != NULL)
        task->vp_return_func(task, QE_TASK_POSTED);
    
    return 1;
}

#if(boardUSE_OS)
/***********************************************************************************************************************
 * 函数功能    : 绑定 FreeRTOS 任务句柄
 * 说明(备注)  : 用于在中断入队 (FromISR) 时自动唤醒关联的任务
 * 传入参数    : task: 任务控制块指针
 *               handle: FreeRTOS 任务句柄
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vQueue_BindTaskHandler(Task_T *task, TaskHandle_t handle)
{
    if (task != NULL)
        task->tTaskHandler = handle;
}

/***********************************************************************************************************************
 * 函数功能    : 中断中添加队列任务
 * 说明(备注)  : 在 ISR 中断上下文中安全入队，支持临界区保护；若已绑定任务句柄则自动唤醒
 * 传入参数    : task: 任务控制块指针; task_id: 任务 ID; in_param: 任务参数;
 *               now_run: true: 清空现有队列立刻抢占运行; false: 排队正常入队; pxHigherPriorityTaskWoken: 指向高优先级任务唤醒标记
 * 输出参数    : pxHigherPriorityTaskWoken: 是否需要执行上下文切换
 * 返回值      : 2: 抢占成功; 1: 正常入队成功; 0: 重复忽略; -2: 指针为空; -3: 队列损坏重置; -4: 队列已满
 ************************************************************************************************************************/
s8 cQueue_AddQueueTaskFromISR(Task_T* task, u8 task_id, u16 in_param, bool now_run, BaseType_t *pxHigherPriorityTaskWoken)
{
    u8 uca_write_array[sizeof(TaskItem_T)];
    UBaseType_t ux_saved_status;
    #if (boardHEALTH_MONITOR_EN)
    TaskTelemetry_T *tp_tel = NULL;
    #endif  //boardHEALTH_MONITOR_EN

    if (task == NULL)
        return -2;

    #if (boardHEALTH_MONITOR_EN)
    tp_tel = tpQueue_GetTelemetry(task);
    #endif  //boardHEALTH_MONITOR_EN
    
    if (task->ucID == task_id && task->usInParam == in_param)
        return 0;
    
    uca_write_array[0] = task_id;
    memcpy(&uca_write_array[1], (u8*)&in_param, 2);
    
    ux_saved_status = taskENTER_CRITICAL_FROM_ISR();
    
    if (now_run)
        lwrb_reset(&task->tQueueBuff);
    else
    {
        u16 us_len = (u16)lwrb_get_full(&task->tQueueBuff);
        if (us_len != 0)
        {
            if (us_len % sizeof(TaskItem_T) != 0)
            {
                lwrb_reset(&task->tQueueBuff);
                #if (boardHEALTH_MONITOR_EN)
                if (tp_tel != NULL)
                    tp_tel->usCorruptEvtCnt++;
                #endif  //boardHEALTH_MONITOR_EN
                
                taskEXIT_CRITICAL_FROM_ISR(ux_saved_status);
                
                return -3;
            }

            if (b_is_task_in_queue(task, uca_write_array))
            {
                taskEXIT_CRITICAL_FROM_ISR(ux_saved_status);
                return 0;
            }
        }

        if (lwrb_get_free(&task->tQueueBuff) < sizeof(TaskItem_T))
        {
            #if (boardHEALTH_MONITOR_EN)
            if (tp_tel != NULL)
                tp_tel->usFullEvtCnt++;
            #endif  //boardHEALTH_MONITOR_EN

            taskEXIT_CRITICAL_FROM_ISR(ux_saved_status);

            return -4;
        }
    }

    if (lwrb_write(&task->tQueueBuff, uca_write_array, sizeof(TaskItem_T)) != sizeof(TaskItem_T))
    {
        #if (boardHEALTH_MONITOR_EN)
        if (tp_tel != NULL)
            tp_tel->usFullEvtCnt++;
        #endif  //boardHEALTH_MONITOR_EN

        taskEXIT_CRITICAL_FROM_ISR(ux_saved_status);

        return -4;
    }

    #if (boardHEALTH_MONITOR_EN)
    vQueue_UpdatePeak(task);
    #endif  //boardHEALTH_MONITOR_EN
    
    if (now_run || task->ucID == 0)
        task->bNowRun = true;
    
    taskEXIT_CRITICAL_FROM_ISR(ux_saved_status);
    
    /* 中断环境下自动触发任务通知唤醒 */
    if (task->tTaskHandler != NULL)
        vTaskNotifyGiveFromISR(task->tTaskHandler, pxHigherPriorityTaskWoken);
    
    return now_run ? 2 : 1;
}
#endif  // boardUSE_OS

/***********************************************************************************************************************
 * 函数功能    : 从队列弹出下一个任务项
 * 说明(备注)  : 原子操作；完整读取一个 TaskItem_T 结构
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : item: 弹出的任务项
 * 返回值      : true: 成功弹出任务; false: 队列为空或异常
 ************************************************************************************************************************/
bool bQueue_PopTask(Task_T* task, TaskItem_T* item)
{
    if (task == NULL || item == NULL)
        return false;
    
    mainENTER_CRITICAL();
    
    u16 us_len = (u16)lwrb_get_full(&task->tQueueBuff);
    if (us_len != 0 && us_len < sizeof(TaskItem_T))
    {
        /* 队列残段(1~2字节,非任务项整数倍):复位自愈,防止假死 */
        lwrb_reset(&task->tQueueBuff);

        #if (boardHEALTH_MONITOR_EN)
        TaskTelemetry_T *tp_tel = tpQueue_GetTelemetry(task);
        if (tp_tel != NULL)
            tp_tel->usCorruptEvtCnt++;
        #endif  //boardHEALTH_MONITOR_EN

        mainEXIT_CRITICAL();

        return false;
    }

    if (us_len % sizeof(TaskItem_T) != 0)
    {
        lwrb_reset(&task->tQueueBuff);

        #if (boardHEALTH_MONITOR_EN)
        TaskTelemetry_T *tp_tel = tpQueue_GetTelemetry(task);
        if (tp_tel != NULL)
            tp_tel->usCorruptEvtCnt++;
        #endif  //boardHEALTH_MONITOR_EN

        mainEXIT_CRITICAL();

        return false;
    }
    
    u8 uca_read_temp[sizeof(TaskItem_T)];
    if (lwrb_read(&task->tQueueBuff, uca_read_temp, sizeof(TaskItem_T)) != sizeof(TaskItem_T))
    {
        mainEXIT_CRITICAL();

        return false;
    }
    
    item->ucId = uca_read_temp[0];
    memcpy(&item->usParam, &uca_read_temp[1], 2);
    
    mainEXIT_CRITICAL();
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 重置任务队列
 * 说明(备注)  : 线程安全清空任务队列环形缓冲区
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : none
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
bool bQueue_Reset(Task_T* task)
{
    if (task == NULL)
        return false;
    
    mainENTER_CRITICAL();
    lwrb_reset(&task->tQueueBuff);
    mainEXIT_CRITICAL();
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 检查指定 ID 的任务是否已经在队列中等待执行
 * 说明(备注)  : 仅按任务 ID 匹配排队项
 * 传入参数    : task: 任务控制块指针
 *               task_id: 待查询的任务 ID
 * 输出参数    : none
 * 返回值      : true: 已在排队; false: 未在排队
 ************************************************************************************************************************/
bool bQueue_IsTaskPending(const Task_T *task, uint8_t task_id)
{
    if (task == NULL)
        return false;
    
    mainENTER_CRITICAL();
    
    u16 us_len = (u16)lwrb_get_full(&task->tQueueBuff);
    u8 uca_item[sizeof(TaskItem_T)];
    bool b_pending = false;
    
    for (u16 us_offset = 0; us_offset + sizeof(TaskItem_T) <= us_len; us_offset += sizeof(TaskItem_T))
    {
        if (lwrb_peek(&task->tQueueBuff, us_offset, uca_item, sizeof(TaskItem_T)) == sizeof(TaskItem_T))
        {
            if (uca_item[0] == task_id)
            {
                b_pending = true;
                break;
            }
        }
    }
    
    mainEXIT_CRITICAL();
    
    return b_pending;
}

/***********************************************************************************************************************
 * 函数功能    : 向回复/载荷缓冲区写入数据
 * 说明(备注)  : 线程安全；写入前自动清空旧载荷（覆盖写模式）
 * 传入参数    : task: 任务控制块指针
 *               data: 待写入数据源
 *               len: 待写入数据长度
 * 输出参数    : none
 * 返回值      : true: 写入成功; false: 失败（指针空/空间不足）
 ************************************************************************************************************************/
bool bQueue_WriteReply(Task_T *task, const void *data, uint16_t len)
{
    if (task == NULL || data == NULL || len == 0 || task->tReplyBuff.buff == NULL)
        return false;
    
    mainENTER_CRITICAL();
    lwrb_reset(&task->tReplyBuff);
    bool b_ret = (lwrb_write(&task->tReplyBuff, data, len) == len);
    mainEXIT_CRITICAL();
    
    return b_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 从回复/载荷缓冲区读取数据
 * 说明(备注)  : 线程安全；读出后数据从缓冲区中移除
 * 传入参数    : task: 任务控制块指针
 *               max_len: 目标输出缓冲区最大容量
 * 输出参数    : out_data: 读出的数据存储区
 * 返回值      : 实际读取的字节数
 ************************************************************************************************************************/
uint16_t usQueue_ReadReply(Task_T *task, void *out_data, uint16_t max_len)
{
    if (task == NULL || out_data == NULL || max_len == 0 || task->tReplyBuff.buff == NULL)
        return 0;
    
    mainENTER_CRITICAL();
    uint16_t us_read = (uint16_t)lwrb_read(&task->tReplyBuff, out_data, max_len);
    mainEXIT_CRITICAL();
    
    return us_read;
}

/***********************************************************************************************************************
 * 函数功能    : 窥视回复/载荷缓冲区数据（不移除数据）
 * 说明(备注)  : 线程安全；支持带偏移量窥视
 * 传入参数    : task: 任务控制块指针
 *               offset: 偏移字节
 *               len: 需要读取的字节数
 * 输出参数    : out_data: 读出的数据存储区
 * 返回值      : 实际窥视到的字节数
 ************************************************************************************************************************/
uint16_t usQueue_PeekReply(Task_T *task, uint16_t offset, void *out_data, uint16_t len)
{
    if (task == NULL || out_data == NULL || len == 0 || task->tReplyBuff.buff == NULL)
        return 0;
    
    mainENTER_CRITICAL();
    uint16_t us_peek = (uint16_t)lwrb_peek(&task->tReplyBuff, offset, out_data, len);
    mainEXIT_CRITICAL();
    
    return us_peek;
}

/***********************************************************************************************************************
 * 函数功能    : 清空回复/载荷缓冲区
 * 说明(备注)  : 线程安全复位 tReplyBuff
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vQueue_ResetReply(Task_T *task)
{
    if (task != NULL && task->tReplyBuff.buff != NULL)
    {
        mainENTER_CRITICAL();
        lwrb_reset(&task->tReplyBuff);
        mainEXIT_CRITICAL();
    }
}

/***********************************************************************************************************************
 * 函数功能    : 标准任务状态机轮询驱动函数
 * 说明(备注)  : 在各个模块的 FreeRTOS 任务主循环中调用；自动处理执行、通知等待与新任务调度
 * 传入参数    : task: 任务控制块指针
 *               cycle_time_ms: 队列空闲时的轮询/超时等待周期 (ms)
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vQueue_TaskPoll(Task_T *task, uint32_t cycle_time_ms)
{
    if (task == NULL)
        return;
    
    if (task->vp_func != NULL && task->bNowRun == false)
        task->vp_func(task);

    else if (task->vp_func == NULL || task->bNowRun == true)
    {
        #if (boardUSE_OS)
        if (lwrb_get_full(&task->tQueueBuff) == 0)
            ulTaskNotifyTake(pdTRUE, cycle_time_ms);
        #endif  /* boardUSE_OS */
        
        if (task->bp_task_manage_func != NULL)
            task->bp_task_manage_func(task);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 检查队列中是否存在相同任务
 * 说明(备注)  : 仅在临界区内调用；按 TaskItem_T 步长对齐比对，杜绝跨字节虚假匹配
 * 传入参数    : p_task: 任务控制块指针
 *               p_target_item: 待比对的 3 字节任务数据
 * 输出参数    : none
 * 返回值      : true: 存在重复任务; false: 不存在
 ************************************************************************************************************************/
static bool b_is_task_in_queue(const Task_T *p_task, const u8 *p_target_item)
{
    u16 us_len = (u16)lwrb_get_full(&p_task->tQueueBuff);
    u8 uca_item[sizeof(TaskItem_T)];
    
    for (u16 us_offset = 0; us_offset + sizeof(TaskItem_T) <= us_len; us_offset += sizeof(TaskItem_T))
    {
        if (lwrb_peek(&p_task->tQueueBuff, us_offset, uca_item, sizeof(TaskItem_T)) == sizeof(TaskItem_T))
        {
            if (memcmp(uca_item, p_target_item, sizeof(TaskItem_T)) == 0)
                return true;
        }
    }
    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 向环形队列头部逆向写入数据（插队）
 * 说明(备注)  : 仅在临界区内调用
 * 传入参数    : buff: lwrb 句柄
 *               data: 待写入数据
 *               len: 字节数
 * 输出参数    : none
 * 返回值      : true: 成功; false: 失败/空间不足
 ************************************************************************************************************************/
static bool b_lwrb_write_front(lwrb_t *buff, const void *data, lwrb_sz_t len)
{
    if (buff == NULL || buff->buff == NULL || data == NULL || len == 0 || lwrb_get_free(buff) < len)
        return false;
    
    lwrb_sz_t r_ptr = buff->r_ptr;
    const uint8_t *src = (const uint8_t *)data;
    
    for (lwrb_sz_t i = 0; i < len; i++)
    {
        if (r_ptr == 0)
            r_ptr = buff->size - 1;
        else
            r_ptr--;
        buff->buff[r_ptr] = src[len - 1 - i];
    }
    
    buff->r_ptr = r_ptr;
    return true;
}

#endif  /* 1 */

