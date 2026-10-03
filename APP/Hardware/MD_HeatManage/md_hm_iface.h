/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_HeatManage
 * File    : md_hm_iface.h
 * Date    : 2026-09-24
 * Author  : LJD(291483914@qq.com)
 * Desc    : 风扇与热管理硬件接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_HM_IFACE_H_
#define MD_HM_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"
#include "Buz/buz_iface.h"

//****************************************************Macros********************************************************************//
#define			fanPWM_MAX_VALUE						1000
#define			fanPWM_PSC								32
#define			fanPWM_SEMI_VALUE						200
#define			fanPWM_FULL_VALUE						550

/* 风扇 PWM 引脚与使能引脚 */
#define 		fanPWM_GPIO_RCU                    		RCU_GPIOA
#define 		fanPWM_GPIO_PORT                   		GPIOA
#define 		fanPWM_PIN                         		GPIO_PIN_15

#define 		fanPWM_EN_GPIO_RCU                 		RCU_GPIOA
#define 		fanPWM_EN_GPIO_PORT                		GPIOA
#define 		fanPWM_EN_PIN                      		GPIO_PIN_9
#define 		fanPWM_EN_ON()                     		GPIO_BOP(fanPWM_EN_GPIO_PORT) = (uint32_t)fanPWM_EN_PIN
#define 		fanPWM_EN_OFF()                    		GPIO_BC(fanPWM_EN_GPIO_PORT)  = (uint32_t)fanPWM_EN_PIN

#define 		fanTIMER                           		TIMER1
#define 		fanTIMER_RCU                       		RCU_TIMER1
#define 		fanTIMER_CH                        		TIMER_CH_0
#if (boardIC_TYPE == boardIC_GD32F50X)
#define 		fanTIMER_AF                        		GPIO_AF_1
#endif  /* boardIC_TYPE */

#define 		fanPWM_SET(x)                      		TIMER_CH0CV(fanTIMER) = ((uint32_t)x)

//****************************************************Globals*******************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
void vFan_IfaceInit(void);
void vFan_IfaceDeInit(void);

#if (boardLOW_POWER)
void vFan_IoEnterLowPower(void);
#endif  /* boardLOW_POWER */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_HM_IFACE_H_ */
