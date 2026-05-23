#ifndef LED_TASK_H_
#define LED_TASK_H_

#include "board_config.h"
#if(boardLED_EN)

void vLed_TaskInit(void);

#if(!boardUSE_OS)
void vLed_Task(void *pvParameters);
#endif  //boardUSE_OS

#endif  //boardLED_EN

#endif  //LED_TASK_H_

