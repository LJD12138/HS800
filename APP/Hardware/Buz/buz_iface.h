#ifndef BUZ_IFACE_H
#define BUZ_IFACE_H

#include "main.h"
#include "board_config.h"

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

void vBuz_Init(void);

#if(boardLOW_POWER)
void vBuz_IoEnterLowPower(void);
#endif

#endif  //BUZ_IFACE_H
