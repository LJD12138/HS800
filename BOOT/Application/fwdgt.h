#ifndef FWDGT_H_
#define FWDGT_H_

#include "board_config.h"

#if(boardWDGT_EN)

typedef struct
{
    uint8_t ext_pin;
    uint8_t por;
    uint8_t sw;
    uint8_t fwdgt;
    uint8_t wwdgt;
    uint8_t low_power;
} reset_reason_t;
extern reset_reason_t g_reset_reason;

void vFwdgt_Init(void);
void vFwdgt_Reload(void);
void vFwdgt_EnterLowPower(void);
void vFwdgt_ExitLowPower(void);
void vFwdgt_PrintResetReason(void);

#endif  //boardWDGT_EN

#endif


