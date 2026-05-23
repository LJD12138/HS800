#ifndef MD_HM_TASK_H_
#define MD_HM_TASK_H_

#include "main.h"

#define   		fanSIMPLE_MODE      					1   //0:简单模式   1:全功能

//工作模式
typedef enum 
{   
    FWM_OFF = 0,
	FWM_GEAR_1,
	FWM_GEAR_2,
	FWM_GEAR_3,
	FWM_GEAR_FULL,
}FanWorkMode_E;

typedef struct
{
    vu16              	usValue;
    FanWorkMode_E    	eWordMode;
}HM_T;              
extern HM_T			tHM;


bool bHM_TaskInit(void);
FanWorkMode_E eFan_GetWorkMode(void);
void vFan_ForceOpenFan(bool en);

#if(boardLOW_POWER)
void vFan_EnterLowPower(void);
void vFan_ExitLowPower(void);
#endif

#endif  //MD_HM_TASK_H_
