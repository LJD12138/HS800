#ifndef SYS_QUEUE_FUNC_H_
#define SYS_QUEUE_FUNC_H_

#include "main.h"
#include "board_config.h"
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif


#define     	sysTASK_CYCLE_TIME                		100  //任务时间


bool bSys_QueueInit(void);

//队列任务
void v_sys_queue_task_init(Task_T *tp_task);
void v_sys_queue_task_closing(Task_T *tp_task);
void v_sys_queue_task_shut_down(Task_T *tp_task);
void v_sys_queue_task_booting(Task_T *tp_task);
void v_sys_queue_task_work(Task_T *tp_task);
void v_sys_queue_task_err(Task_T *tp_task);
void v_sys_queue_task_reset(Task_T *tp_task);

#if(boardUPDATE)
void v_sys_queue_task_update(Task_T *tp_task);
#endif  //boardUPDATE

#if(boardENG_MODE_EN)
void v_sys_queue_task_eng(Task_T *tp_task);
#endif  //boardENG_MODE_EN
#endif  //SYS_QUEUE_FUNC_H_
