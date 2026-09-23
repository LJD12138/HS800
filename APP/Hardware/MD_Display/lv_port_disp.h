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

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H

//****************************************************Includes******************************************************************//
#include "board_config.h"

#if(boardDISPLAY_EN)

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "main.h"
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

/* FreeRTOS 相关头文件：用于创建/使用二值信号量保护显示访问 */
#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  //boardUSE_OS

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
extern lv_display_t *disp;

#if(boardUSE_OS)
extern SemaphoreHandle_t DispSemaphoreBinary;
extern SemaphoreHandle_t DispFlushDoneSemaphore;
#endif  //boardUSE_OS

void lv_port_disp_init(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif  /* boardDISPLAY_EN */

#endif  /* LV_PORT_DISP_TEMPL_H */
