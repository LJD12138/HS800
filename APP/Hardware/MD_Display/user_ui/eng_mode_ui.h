/*******************************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS800\APP\Hardware\MD_Display\user_ui
 * File    : eng_mode_ui.h
 * Date    : 2026-06-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : 工程模式LVGL UI - 主菜单/参数查看/记忆参数设置/系统设置
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef ENG_MODE_UI_H
#define ENG_MODE_UI_H


#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================includes====================================*/
#include "board_config.h"

#if(boardENG_MODE_EN && boardDISPLAY_EN)

/* ==========================================macros======================================*/

/* ==========================================types=======================================*/

/* 工程模式页面枚举 */
typedef enum
{
    ENG_PAGE_MAIN_MENU = 0,     /* 主菜单 */
    ENG_PAGE_PARAM_VIEW,        /* 参数查看 */
    ENG_PAGE_PARAM_SET,         /* 记忆参数设置 */
    ENG_PAGE_SYS_SET,           /* 系统设置 */
    ENG_PAGE_CONFIRM,           /* 确认对话框 */
    ENG_PAGE_MAX
}EngModePage_E;

/* ==========================================globals=====================================*/


/* ==========================================extern======================================*/

void vEngMode_UiCreate(void);
void vEngMode_UiDelete(void);
void vEngMode_UiTick(void);
bool bEngMode_IsExitReq(void);

void vEngMode_KeyUp(void);
void vEngMode_KeyDown(void);
void vEngMode_KeyEnter(void);
void vEngMode_KeyBack(void);
void vEngMode_KeyLeft(void);
void vEngMode_KeyRight(void);

#endif  /* boardENG_MODE_EN && boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* ENG_MODE_UI_H */