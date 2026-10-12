/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : lv_port_disp.h
 * Date    : 2026-06-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : LVGL显示端口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "board_config.h"

#if(boardDISPLAY_EN)
#include "lvgl.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  /* boardUSE_OS */

//****************************************************Globals*******************************************************************//
extern lv_display_t *tp_disp;

#if(boardUSE_OS)
extern SemaphoreHandle_t DispSemaphoreBinary;
extern SemaphoreHandle_t DispFlushDoneSemaphore;
#endif  /* boardUSE_OS */

//****************************************************Extern********************************************************************//
void lv_port_disp_init(void);

#endif  /* boardDISPLAY */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* LV_PORT_DISP_H */
