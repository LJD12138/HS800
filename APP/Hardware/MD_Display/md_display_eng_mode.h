#ifndef MD_DISP_ENG_MODE_H
#define MD_DISP_ENG_MODE_H

#include "board_config.h"

#if(boardENG_MODE_EN && boardDISPLAY_EN)

typedef enum
{
	LTS_HIGH = 0,
	LTS_LOW,
	LTS_OFF_TIME,
	LTS_NULL,
}DispTypeSet_E;


void vDisp_EnginModeDis(void);
void bDisp_SetLightness(void);
void vDisp_MemParamSet(bool add);
bool bDisp_ExitSetLightness(void);
void vDisp_TypeSelect(void);

#endif  //boardENG_MODE_EN && boardDISPLAY_EN

#endif //MD_DISP_ENG_MODE_H




