/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Led
 * File    : led_iface.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 指示灯底层 GPIO 与 PWM 配置接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef LED_IFACE_H_
#define LED_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardLED_EN)

//****************************************************Macros********************************************************************//
#define 		ledTIMER                           		TIMER3
#define 		ledTIMER_RCU                       		RCU_TIMER3
#define 		ledTIMER_CH                        		TIMER_CH_3
#if (boardIC_TYPE == boardIC_GD32F50X)
#define 		ledTIMER_AF                        		GPIO_AF_2
#endif  //boardIC_TYPE

#define 		ledPWR_SW_PWM_SET(x)                  	TIMER_CH3CV(ledTIMER) = ((uint32_t)x)
#define 		ledPWM_MAX_VALUE     					1000
#define 		ledPWM_PSC           					32

#define     	ledPWR_SW_RCU      						RCU_GPIOB
#define     	ledPWR_SW_PORT     						GPIOB
#define     	ledPWR_SW_PIN      						GPIO_PIN_9
#define     	ledPWR_SW_ON()     						ledPWR_SW_PWM_SET(1000)
#define     	ledPWR_SW_OFF()    						ledPWR_SW_PWM_SET(0)

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//

//****************************************************Extern********************************************************************//
void vLed_IfaceInit(void);
void vLed_IfaceDeInit(void);

#if (boardLOW_POWER)
void vLed_IoEnterLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardLED_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* LED_IFACE_H_ */

