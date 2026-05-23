#ifndef MD_PRINT_QUEUE_TASK_H_
#define MD_PRINT_QUEUE_TASK_H_

#include "board_config.h"

#if(boardPRINT_IFACE)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

bool bPrint_QueueInit(void);

//¶ÓÁÐº¯Êý
void v_print_queue_task_main(Task_T *tp_task);

#endif  //boardPRINT_IFACE

#endif  //MD_PRINT_QUEUE_FUNC_H_
