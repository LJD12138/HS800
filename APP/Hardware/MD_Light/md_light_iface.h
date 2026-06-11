#ifndef MD_LIGHT_IFACE_H_
#define MD_LIGHT_IFACE_H_
#include "board_config.h"

#if(boardLIGHT_EN)

//#define		//4Tab									//10Tab
#define 		lightPWM_MAX_VALUE                   	1000
#define 		lightPWM_PSC                         	32
#define 		lightPWM_SEMI_VALUE                  	200
#define 		lightPWM_FULL_VALUE                  	550

//???÷LED
#define 		lightPWM_GPIO_RCU                    	RCU_GPIOA
#define 		lightPWM_GPIO_PORT                  	GPIOA
#define 		lightPWM_PIN                         	GPIO_PIN_8

//#define 		lightPWM_EN_GPIO_RCU                 	RCU_GPIOA 
//#define 		lightPWM_EN_GPIO_PORT                	GPIOA
//#define 		lightPWM_EN_PIN                      	GPIO_PIN_15
//#define 		lightPWM_EN_ON()                     	GPIO_BOP(lightPWM_EN_GPIO_PORT)=lightPWM_EN_PIN;timer_enable(lightTIMER);
//#define 		lightPWM_EN_OFF()                    	GPIO_BC(lightPWM_EN_GPIO_PORT)=lightPWM_EN_PIN;timer_disable(lightTIMER);
#define 		lightPWM_EN_ON()                     	__NOP;
#define 		lightPWM_EN_OFF()                    	__NOP;

#define 		lightTIMER                           	TIMER0
#define 		lightTIMER_RCU                       	RCU_TIMER0
#define 		lightTIMER_CH                        	TIMER_CH_0
#if (boardIC_TYPE == boardIC_GD32F50X)
#define 		lightTIMER_AF                        	GPIO_AF_1
#endif  //boardIC_TYPE

#define 		lightPWM_SET(x)                      	TIMER_CH0CV(lightTIMER) = ((uint32_t)x)

void vLight_IfaceInit(void);
void vLight_IfaceDeInit(void);

#if(boardLOW_POWER)
void vLight_IoEnterLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardLIGHT_EN

#endif  //MD_LIGHT_IFACE_H_
