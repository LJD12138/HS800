/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Buz
 * File    : buz_iface.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 蜂鸣器底层硬件驱动接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef BUZ_IFACE_H_
#define BUZ_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardBUZ_EN)

//****************************************************Macros********************************************************************//
#define    		buzPWM_GPIO_RCU     					RCU_GPIOC
#define    		buzPWM_GPIO_PORT    					GPIOC
#define    		buzPWM_GPIO_PIN     					GPIO_PIN_9

#define    		buzTIMER         						TIMER7
#define    		buzTIMER_RCU    						RCU_TIMER7
#define    		buzTIMER_CH     						TIMER_CH_3
#if (boardIC_TYPE == boardIC_GD32F50X)
#define 		buzTIMER_AF                        		GPIO_AF_2
#endif  //boardIC_TYPE

#define    		buzTIMER_PWM_SET(x)    					TIMER_CH3CV(buzTIMER) = ((uint32_t)x)

//****************************************************Extern********************************************************************//
void vBuz_Init(void);

#if (boardLOW_POWER)
void vBuz_IoEnterLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardBUZ_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* BUZ_IFACE_H_ */
