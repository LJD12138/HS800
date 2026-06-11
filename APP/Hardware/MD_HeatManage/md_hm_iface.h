#ifndef MD_HM_IFACE_H_
#define MD_HM_IFACE_H_
#include "main.h"

#define 		fanPWM_MAX_VALUE     					1000
#define 		fanPWM_PSC           					32
#define 		fanPWM_SEMI_VALUE    					200
#define 		fanPWM_FULL_VALUE    					550

//·???
#define 		fanPWM_GPIO_RCU                    		RCU_GPIOA
#define 		fanPWM_GPIO_PORT                   		GPIOA
#define 		fanPWM_PIN                         		GPIO_PIN_15

#define 		fanPWM_EN_GPIO_RCU                 		RCU_GPIOC
#define 		fanPWM_EN_GPIO_PORT                		GPIOC
#define 		fanPWM_EN_PIN                      		GPIO_PIN_10
#define 		fanPWM_EN_ON()                     		GPIO_BOP(fanPWM_EN_GPIO_PORT)=fanPWM_EN_PIN
#define 		fanPWM_EN_OFF()                    		GPIO_BC(fanPWM_EN_GPIO_PORT)=fanPWM_EN_PIN
//#define 		fanPWM_EN_ON()                     		GPIO_BOP(fanPWM_EN_GPIO_PORT)=fanPWM_EN_PIN;timer_enable(fanTIMER)
//#define 		fanPWM_EN_OFF()                    		GPIO_BC(fanPWM_EN_GPIO_PORT)=fanPWM_EN_PIN;timer_disable(fanTIMER)
//#define 		fanPWM_EN_ON()                     		__NOP;
//#define 		fanPWM_EN_OFF()                    		__NOP;

#define 		fanTIMER                           		TIMER1
#define 		fanTIMER_RCU                       		RCU_TIMER1
#define 		fanTIMER_CH                        		TIMER_CH_0
// #define 		fanLED_TIMER_CH                    		TIMER_CH_2
#if (boardIC_TYPE == boardIC_GD32F50X)
#define 		fanTIMER_AF                        		GPIO_AF_1
#endif  //boardIC_TYPE

#define 		fanPWM_SET(x)                      		TIMER_CH0CV(fanTIMER) = ((uint32_t)x)
// #define 		fanLED_PWM_SET(x)                  		TIMER_CH2CV(fanTIMER) = ((uint32_t)x)

void vFan_IfaceInit(void);
void vFan_IfaceDeInit(void);

#if(boardLOW_POWER)
void vFan_IoEnterLowPower(void);
#endif

#endif  //MD_HM_IFACE_H_
