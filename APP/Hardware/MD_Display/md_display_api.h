#ifndef MD_DISPLAY_API_H
#define MD_DISPLAY_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "board_config.h"
#include "MD_Display/md_display_iface.h"

#if(boardDISPLAY_EN)

#define DISP_HOR_RES                        DISP_TFT_WIDTH
#define DISP_VER_RES                        DISP_TFT_HEIGHT


void vLV_TimerInit(void);
uint32_t ulLV_GetTickMs(void);
void vLV_TimerIrqCallback(void);

void vDisp_TftInitController(void);
void vDisp_Init(void);
void vDisp_Refresh(void);
void vDisp_SetPower(bool on);
void vDisp_SetContrast(u8 value);
bool bDisp_IsReady(void);

#endif

#ifdef __cplusplus
}
#endif

#endif

