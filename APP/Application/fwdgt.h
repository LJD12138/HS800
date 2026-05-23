#ifndef FWDGT_H_
#define FWDGT_H_

#include "board_config.h"

#if(boardWDGT_EN)

#include "main.h"

void vFwdgt_Init(void);
void vFwdgt_Reload(void);
void vFwdgt_EnterLowPower(void);
void vFwdgt_ExitLowPower(void);

#endif  //boardWDGT_EN

#endif


