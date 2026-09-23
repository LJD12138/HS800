/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Light
 * File    : md_light_iface.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 照明灯硬件接口与定时器PWM定义头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_LIGHT_IFACE_H_
#define MD_LIGHT_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardLIGHT_EN)

//****************************************************Macros********************************************************************//
#define			lightPWM_MAX_VALUE						1000
#define			lightPWM_PSC							32
#define			lightPWM_SEMI_VALUE						200
#define			lightPWM_FULL_VALUE						550

/* 照明LED引脚与外设定义 */
#define			lightPWM_GPIO_RCU						RCU_GPIOA
#define			lightPWM_GPIO_PORT						GPIOA
#define 		lightPWM_PIN                         	GPIO_PIN_8

//#define 		lightPWM_EN_GPIO_RCU                 	RCU_GPIOA 
//#define 		lightPWM_EN_GPIO_PORT                	GPIOA
//#define 		lightPWM_EN_PIN                      	GPIO_PIN_15
//#define 		lightPWM_EN_ON()                     	GPIO_BOP(lightPWM_EN_GPIO_PORT)=lightPWM_EN_PIN;timer_enable(lightTIMER);
//#define 		lightPWM_EN_OFF()                    	GPIO_BC(lightPWM_EN_GPIO_PORT)=lightPWM_EN_PIN;timer_disable(lightTIMER);
#define			lightPWM_EN_ON()						__NOP;
#define			lightPWM_EN_OFF()						__NOP;

#define 		lightTIMER                           	TIMER0
#define 		lightTIMER_RCU                       	RCU_TIMER0
#define 		lightTIMER_CH                        	TIMER_CH_0
#if (boardIC_TYPE == boardIC_GD32F50X)
#define 		lightTIMER_AF                        	GPIO_AF_1
#endif  //boardIC_TYPE

#define 		lightPWM_SET(x)                      	TIMER_CH0CV(lightTIMER) = ((uint32_t)x)

//****************************************************Extern********************************************************************//
void vLight_IfaceInit(void);
void vLight_IfaceDeInit(void);

#if (boardLOW_POWER)
void vLight_IoEnterLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardLIGHT_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_LIGHT_IFACE_H_ */
