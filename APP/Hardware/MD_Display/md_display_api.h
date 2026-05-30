#ifndef MD_DISPLAY_API_H
#define MD_DISPLAY_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "board_config.h"
#include "MD_Display/md_display_iface.h"

#if (boardDISPLAY_EN)

#define DISP_HOR_RES dispTFT_WIDTH
#define DISP_VER_RES dispTFT_HEIGHT

void vDisp_Init(void);
void vDisp_ReqUiRefresh(void);
void vDisp_UiRefresh(void);
bool bDisp_IsReady(void);

#if (!LV_USE_ST7789)
void vDisp_FastDrawColor(u16 x, u16 y, u16 w, u16 h, u16 *color);
#endif // LV_USE_ST7789

#endif // boardDISPLAY_EN

#ifdef __cplusplus
}
#endif

#endif // MD_DISPLAY_API_H
