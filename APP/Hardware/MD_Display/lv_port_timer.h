#ifndef LV_PORT_TIMER_H
#define LV_PORT_TIMER_H

#include "board_config.h"

#if(boardDISPLAY_EN)

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "main.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* 定时器初始化 */
void vLV_TimerInit(void);

/* 获取当前时间戳（毫秒） */
uint32_t ulLV_GetTickMs(void);

/* 定时器中断回调 */
void vLV_TimerIrqCallback(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_TIMER_H*/

#endif /*Disable/Enable content*/