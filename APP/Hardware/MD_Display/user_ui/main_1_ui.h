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
#include "MD_Display/user_ui/img_breath.h"
//****************************************************Macros********************************************************************//
// 设备类型枚举
typedef enum
{
	DEV_TYPE_AC_OUT = 0,	// ACOUT设备
	DEV_TYPE_AC_IN,			// ACIN设备
	DEV_TYPE_PV,			// PV设备
	DEV_TYPE_LIGHT,			// Light设备
	DEV_TYPE_USB,			// USB设备
	DEV_TYPE_USB_A,			// USB A设备
	DEV_TYPE_USB_C1,		// USB C1设备
	DEV_TYPE_USB_C2,		// USB C2设备
	DEV_TYPE_DC,			// DC设备
	DEV_TYPE_MAX
}DevType_E;

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
void vDisp_Main1UiStart(void);
void vDisp_Main1Exit(void);
bool bDisp_Main1DataUpdate(void);
void vDisp_SetDevStateIcon(DevType_E devType, u8 ucState);
void vDisp_SetAcWorkMode(ImgAnimMode_E eMode);
void vDisp_UpdateDevParam(void);

#endif  //boardDISPLAY_EN

#ifdef __cplusplus
}
#endif

#endif  /* MAIN_1_UI_H */