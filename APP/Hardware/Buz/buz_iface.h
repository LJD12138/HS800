#ifndef BUZ_IFACE_H
#define BUZ_IFACE_H

#include "main.h"
#include "board_config.h"

#define    		buzPWM_GPIO_RCU     					RCU_GPIOA
#define    		buzPWM_GPIO_PORT    					GPIOA
#define    		buzPWM_GPIO_PIN     					GPIO_PIN_11

#define    		buzTIMER         						TIMER0
#define    		buzTIMER_RCU    						RCU_TIMER0
#define    		buzTIMER_CH     						TIMER_CH_3
#define    		buzTIMER_PWM_SET(x)    					TIMER_CH3CV(buzTIMER) = ((uint32_t)x)

void vBuz_Init(void);

#if(boardLOW_POWER)
void vBuz_IoEnterLowPower(void);
#endif

#endif  //BUZ_IFACE_H
