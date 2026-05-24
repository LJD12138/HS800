/***********************************************************************************************************************
 -----文件说明    LVGL显示端口头文件
 -----说明(备注)  LVGL显示驱动接口层头文件，声明显示初始化和刷新控制函数
 -----文件版本    V1.0
 -----作者        HS800开发团队
 -----日期        2024
 ************************************************************************************************************************/

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H

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

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/


void lv_port_disp_init(void);
void disp_enable_update(u8 index);
void disp_disable_update(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_DISP_TEMPL_H*/

#endif /*Disable/Enable content*/
