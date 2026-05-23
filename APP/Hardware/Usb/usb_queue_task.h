#ifndef MD_MPPT_QUEUE_TASK_H_
#define MD_MPPT_QUEUE_TASK_H_

#include "board_config.h"

#if(boardUSB_EN)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif


#define       	usbTASK_CYCLE_TIME               		100  //任务时间


extern s32 us_usb_total_out_pwr;


bool bUsb_QueueInit(void);

//队列函数
void v_usb_queue_task_init(Task_T *tp_task);
void v_usb_queue_task_closing(Task_T *tp_task);
void v_usb_queue_task_booting(Task_T *tp_task);
void v_usb_queue_task_err(Task_T *tp_task);
void v_usb_queue_task_work(Task_T *tp_task);

#endif  //boardUSB_EN

#endif  //MD_MPPT_QUEUE_TASK_H_
