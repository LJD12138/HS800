#ifndef MD_DISPLAY_QUEUE_TASK_H_
#define MD_DISPLAY_QUEUE_TASK_H_


#include "board_config.h"

#if(boardDISPLAY_EN)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

#define     dispTASK_CYCLE_TIME                 100  //任务时间

extern Task_T *tpDispTask;          //队列任务对象

bool bDisp_QueueInit(void);

//队列任务
void v_disp_queue_task_init(Task_T *tp_task);
void v_disp_queue_task_closing(Task_T *tp_task);
void v_disp_queue_task_shut_down(Task_T *tp_task);
void v_disp_queue_task_booting(Task_T *tp_task);
void v_disp_queue_task_work(Task_T *tp_task);
void v_disp_queue_task_err(Task_T *tp_task);

#if(boardUPDATA)
void v_disp_queue_task_updata(Task_T *tp_task);
void vDisp_RenderUpgradePage(void);
#endif  //boardUPDATA

#if(boardENG_MODE_EN)
void v_disp_queue_task_eng(Task_T *tp_task);
#endif  //boardENG_MODE_EN

#endif  //boardDISPLAY_EN

#endif  //MD_DISPLAY_QUEUE_TASK_H_
