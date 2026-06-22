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

void vDisp_DrawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void vDisp_DrawText(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t scale);
void vDisp_DrawProgressCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t thickness, uint8_t progress, uint16_t active_color, uint16_t inactive_color, uint16_t bg_color);
void vDisp_DrawPillProgress(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t progress, uint16_t active_color, uint16_t inactive_color, uint16_t bg_color);

#endif // boardDISPLAY_EN

#ifdef __cplusplus
}
#endif

#endif // MD_DISPLAY_API_H
