#ifndef LED_IFACE_H_
#define LED_IFACE_H_

#include "board_config.h"

#if(boardLED_EN)
#define 		ledTIMER                           		TIMER2
#define 		ledTIMER_RCU                       		RCU_TIMER2
#define 		ledTIMER_CH                        		TIMER_CH_2

#define 		ledPWR_SW_PWM_SET(x)                  	TIMER_CH2CV(ledTIMER) = ((uint32_t)x)
#define 		ledPWM_MAX_VALUE     					1000
#define 		ledPWM_PSC           					32

#define     	ledPWR_SW_RCU      						RCU_GPIOB
#define     	ledPWR_SW_PORT     						GPIOB
#define     	ledPWR_SW_PIN      						GPIO_PIN_0
#define     	ledPWR_SW_ON()     						ledPWR_SW_PWM_SET(1000)
#define     	ledPWR_SW_OFF()    						ledPWR_SW_PWM_SET(0)
//#define     	ledPWR_SW_ON()     						GPIO_BOP(ledPWR_SW_PORT) = ledPWR_SW_PIN
//#define     	ledPWR_SW_OFF()    						GPIO_BC(ledPWR_SW_PORT)  = ledPWR_SW_PIN

#define     	ledAC_SW_RCU      						RCU_GPIOB
#define     	ledAC_SW_PORT      						GPIOB
#define     	ledAC_SW_PIN      						GPIO_PIN_2
#define     	ledAC_SW_ON()      						GPIO_BOP(ledAC_SW_PORT) = ledAC_SW_PIN
#define     	ledAC_SW_OFF()     						GPIO_BC(ledAC_SW_PORT)  = ledAC_SW_PIN

#define     	ledUSB_SW_RCU      						RCU_GPIOB
#define     	ledUSB_SW_PORT     						GPIOB
#define     	ledUSB_SW_PIN      						GPIO_PIN_13
#define     	ledUSB_SW_ON()    						GPIO_BOP(ledUSB_SW_PORT) = ledUSB_SW_PIN
#define     	ledUSB_SW_OFF()    						GPIO_BC(ledUSB_SW_PORT)  = ledUSB_SW_PIN

#define     	ledLight_SW_RCU    						RCU_GPIOA
#define     	ledLight_SW_PORT   						GPIOA
#define     	ledLight_SW_PIN    						GPIO_PIN_12
#define     	ledLight_SW_ON()   						GPIO_BOP(ledLight_SW_PORT) = ledLight_SW_PIN
#define     	ledLight_SW_OFF()  						GPIO_BC(ledLight_SW_PORT)  = ledLight_SW_PIN

#define     	ledDC_SW_RCU      						RCU_GPIOB
#define     	ledDC_SW_PORT      						GPIOB
#define     	ledDC_SW_PIN      						GPIO_PIN_15
#define     	ledDC_SW_ON()     						GPIO_BOP(ledDC_SW_PORT) = ledDC_SW_PIN
#define     	ledDC_SW_OFF()     						GPIO_BC(ledDC_SW_PORT)  = ledDC_SW_PIN

void vLed_IfaceInit(void);

void vLed_IfaceDeInit(void);
	
#if(boardLOW_POWER)
void vLed_IoEnterLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardLED_EN
#endif  //LED_IFACE_H_
