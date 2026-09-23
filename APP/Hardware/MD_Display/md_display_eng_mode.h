/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_eng_mode.h
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示工程模式头文件 (TFT+LVGL适配)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DISPLAY_ENG_MODE_H_
#define MD_DISPLAY_ENG_MODE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "board_config.h"

#if (boardENG_MODE_EN && boardDISPLAY_EN)
#include "main.h"

//****************************************************Types*********************************************************************//
typedef enum
{
	LTS_HIGH = 0,		/* 高亮度设置 */
	LTS_LOW,			/* 低亮度设置 */
	LTS_OFF_TIME,		/* 息屏时间设置 */
	LTS_NULL,			/* 无效项/枚举结束 */
}DispTypeSet_E;

//****************************************************Extern********************************************************************//
void vDisp_EnginModeDis(void);
void vDisp_MemParamSet(bool add);
void vDisp_TypeSelect(void);

#endif  /* boardENG_MODE_EN && boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DISPLAY_ENG_MODE_H_ */
