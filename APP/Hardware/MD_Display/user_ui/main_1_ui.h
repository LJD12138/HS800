/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display\user_ui
 * File    : main_1_ui.h
 * Date    : 2026-05-28
 * Author  : LJD(291483914@qq.com)
 * Desc    : 主界面 UI 对象与接口定义
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef MAIN_1_UI_H
#define MAIN_1_UI_H


#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_data.h"
#include "MD_Display/user_ui/img_breath.h"
//****************************************************Macros********************************************************************//


//****************************************************Globals*******************************************************************

//****************************************************Extern********************************************************************//
void vDisp_Main1UiStart(void);
void vDisp_Main1Exit(void);
bool bDisp_Main1DataUpdate(const DispDataSnapshot_T *p_data);
void vDisp_SetAcWorkMode(ImgAnimMode_E eMode);
void vDisp_UpdateDevParam(const DispDataSnapshot_T *p_data);

#endif  //boardDISPLAY_EN

#ifdef __cplusplus
}
#endif

#endif  /* MAIN_1_UI_H */