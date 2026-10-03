/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_api.h
 * Date    : 2026-09-22
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示对外 API 头文件：分辨率宏、初始化/页面加载/背光控制/UI 刷新，及绘制接口
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
void vDisp_LoadScreen(int screen_id);
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

#endif  /* MD_DISPLAY_API_H */
