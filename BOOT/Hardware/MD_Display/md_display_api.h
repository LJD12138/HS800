/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Display
 * File    : md_display_api.h
 * Date    : 2026-09-22
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示对外 API 头文件：分辨率宏、初始化与 UI 刷新接口，及矩形填充/文本/进度圆环/分段圆环绘制接口
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DISPLAY_API_H
#define MD_DISPLAY_API_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "board_config.h"
#include "MD_Display/md_display_iface.h"

#if (boardDISPLAY_EN)

//****************************************************Macros********************************************************************//
#define			DISP_HOR_RES							dispTFT_WIDTH
#define			DISP_VER_RES							dispTFT_HEIGHT

//****************************************************Extern********************************************************************//
void vDisp_Init(void);
void vDisp_ReqUiRefresh(void);
void vDisp_UiRefresh(void);
bool bDisp_IsReady(void);

void vDisp_DrawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void vDisp_DrawText(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t scale);
void vDisp_DrawProgressCircle(uint16_t x0, uint16_t y0, uint16_t r, uint8_t thickness, uint8_t progress, uint16_t active_color, uint16_t inactive_color, uint16_t bg_color);
void vDisp_DrawPillProgress(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t progress, uint16_t active_color, uint16_t inactive_color, uint16_t bg_color);
void vDisp_DrawSegmentedRing(uint16_t cx, uint16_t cy, uint16_t r, uint8_t thickness, uint8_t lit_segs, uint8_t total_segs, uint8_t gap_angle, uint16_t active_color, uint16_t head_color, uint16_t inactive_color, uint16_t bg_color);

#endif // boardDISPLAY_EN

#ifdef __cplusplus
}
#endif

#endif  /* MD_DISPLAY_API_H */
