#ifndef MD_LIGHT_TASK_H_
#define MD_LIGHT_TASK_H_

#include "board_config.h"

#if(boardLIGHT_EN)

#define   		lightSIMPLE_MODE      					1   //0:简单模式   1:全功能

//工作模式
typedef enum 
{   
    LWM_OFF = 0,
    LWM_HALF,
    LWM_FULL,
	#if(lightSIMPLE_MODE)
	LWM_SOS,
	LWM_TWINKLE,
	#endif  //lightSIMPLE_MODE
}LightWorkMode_E;


typedef struct
{
    vu16              	usValue;
	vu16              	usLastValue;
	vu16				usPower;
    LightWorkMode_E  	eWordMode;
	DevState_E  		eDevState;
}Light_T;              
extern Light_T   		tLight;


void vLight_TaskInit(void);
bool bLight_Switch(SwitchType_E type);
void vLight_CircSelectMode(void); 

#if(boardLOW_POWER)
void vLight_EnterLowPower(void);
void vLight_ExitLowPower(void);
#endif

#endif  //boardLIGHT_EN

#endif  //MD_LIGHT_TASK_H_
