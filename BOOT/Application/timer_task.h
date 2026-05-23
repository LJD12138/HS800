#ifndef TIMER_TASK_H_
#define TIMER_TASK_H_
#include "main.h"

void vTimer_Task(void);

#if(boardLOW_POWER)
void vCount_EnterLowPower(void);
void vCount_ExitLowPower(void);
#endif
#endif
