#ifndef SYS_QUEUE_TASK_ENG_H_
#define SYS_QUEUE_TASK_ENG_H_

#include "board_config.h"

#if(boardENG_MODE_EN)

#include "main.h"
#include "queue_task.h"

//π§≥Ã≤Ω÷Ë
typedef enum
{
	EMS_INIT = 0,
	EMS_SYS,
	EMS_LCD,
	EMS_BAT,
	EMS_DCAC,
	EMS_MPPT,
	EMS_USB,
	EMS_DC,
	EMS_ADC,
	EMS_LIGHT,
	EMS_SET,
	EMS_FINISH,
}EngModeStep_E;


typedef struct
{
	vu16 usEngModeCnt;
	vu8  ucEngModeItem;
	s8   cEngModeState;
}EngMode_T;

extern EngMode_T tEngMode;

void vEng_TaskFunc(Task_T *tp_task);
void vEng_RefreshEngModeTime(void);


#endif //boardENG_MODE_EN

#endif //SYS_QUEUE_TASK_ENG_H_
