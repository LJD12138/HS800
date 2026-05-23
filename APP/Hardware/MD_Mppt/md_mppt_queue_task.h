#ifndef MD_MPPT_QUEUE_TASK_H_
#define MD_MPPT_QUEUE_TASK_H_

#include "main.h"
#include "board_config.h"
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif


#define       	mpptTASK_CYCLE_TIME               		1000  //任务时间


bool bMppt_QueueInit(void);

//队列函数
void v_mppt_queue_task_init(Task_T *tp_task);
void v_mppt_queue_task_main(Task_T *tp_task);
void v_mppt_queue_task_set_chg_pwr(Task_T *tp_task);
void v_mppt_queue_task_err_process(Task_T *tp_task);

#endif  //MD_MPPT_QUEUE_TASK_H_
