#ifndef MD_DCAC_QUEUE_TASK_H_
#define MD_DCAC_QUEUE_TASK_H_

#include "main.h"
#include "board_config.h"

#if(boardDCAC_EN)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

bool bDcac_QueueInit(void);

//队列函数
void v_dcac_queue_task_init(Task_T *tp_task);
void v_dcac_queue_task_main(Task_T *tp_task);
void v_dcac_queue_task_dcac_out(Task_T *tp_task);
void v_dcac_queue_task_dcac_in(Task_T *tp_task);
void v_dcac_queue_task_para_in(Task_T *tp_task);
void v_dcac_queue_task_err_proc(Task_T *tp_task);
void v_dcac_queue_task_update(Task_T *tp_task);

#endif  //boardDCAC_EN

#endif  //MD_DCAC_QUEUE_TASK_H_
