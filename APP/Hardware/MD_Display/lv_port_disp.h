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

#endif /*LV_PORT_DISP_TEMPL_H*/

#endif /*Disable/Enable content*/
