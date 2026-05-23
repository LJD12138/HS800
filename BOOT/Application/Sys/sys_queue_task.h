#ifndef SYS_QUEUE_FUNC_H_
#define SYS_QUEUE_FUNC_H_

#include "main.h"
#include "board_config.h"
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

#define     	sysTASK_CYCLE_TIME                		10  //任务时间

bool bSys_QueueInit(void);
s8 cSys_JumpToApp(void);

//队列任务
void v_sys_queue_task_init(Task_T *tp_task);
void v_sys_queue_task_enter_app(Task_T *tp_task);
void v_sys_queue_task_err(Task_T *tp_task);
void v_sys_queue_task_reset(Task_T *tp_task);

#if(boardDISPLAY_EN)
void v_sys_queue_task_disp(Task_T *tp_task);
#endif

#if(boardUPDATE)
void v_sys_queue_task_update(Task_T *tp_task);
#endif

#if(boardLOW_POWER)
void v_sys_queue_task_low_power(Task_T *tp_task);
#endif

#endif
