/***********************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS803\APP\Hardware\MD_Display\user_ui
 * File    : main_1_ui.h
 * Date    : 2026-05-28 10:23:31
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
************************************************************************************************************************/
#ifndef MAIN_1_UI_H
#define MAIN_1_UI_H


#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================includes====================================*/
#include "board_config.h"

#if(boardDISPLAY_EN)
#include "MD_Display/user_ui/img_breath.h"
/* ==========================================macros======================================*/
// 设备类型枚举
typedef enum
{
	DEV_TYPE_AC_OUT = 0,// ACOUT设备
	DEV_TYPE_AC_IN,		// ACIN设备
	DEV_TYPE_PV,        // PV设备
	DEV_TYPE_LIGHT,     // Light设备
	DEV_TYPE_USB,       // USB设备
	DEV_TYPE_USB_A,     // USB A设备
	DEV_TYPE_USB_C1,    // USB C1设备
	DEV_TYPE_USB_C2,    // USB C2设备
	DEV_TYPE_DC,    	// DC设备
	DEV_TYPE_MAX
}DevType_E;

/* ==========================================globals=====================================*/


void vDisp_Main1UiStart(void);
void vDisp_Main1Exit(void);
bool bDisp_Main1DataUpdate(void);
void vDisp_SetDevStateIcon(DevType_E devType, bool bState);
void vDisp_SetAcWorkMode(ImgAnimMode_E eMode);

/* ==========================================types=======================================*/


/* ==========================================extern======================================*/


#endif  //boardDISPLAY_EN

#ifdef __cplusplus
}
#endif

#endif  //MAIN_1_UI_H