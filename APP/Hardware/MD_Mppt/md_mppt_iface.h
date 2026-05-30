#ifndef MD_MPPT_USART_H_
#define MD_MPPT_USART_H_

#include "board_config.h"

#if(boardMPPT_IFACE)

#define     	mpptGPIO_DC_EN_RCU       		    RCU_GPIOA
#define     	mpptGPIO_DC_EN_PORT      		    GPIOA
#define     	mpptGPIO_DC_EN_PIN       		    GPIO_PIN_5
#define     	mpptGPIO_DC_EN_ON()      		    GPIO_BOP(mpptGPIO_DC_EN_PORT) = mpptGPIO_DC_EN_PIN   //使能发送
#define     	mpptGPIO_DC_EN_OFF()     		    GPIO_BC(mpptGPIO_DC_EN_PORT)  = mpptGPIO_DC_EN_PIN   //使能接收

#define     	mpptGPIO_XT60_EN_RCU       		    RCU_GPIOB
#define     	mpptGPIO_XT60_EN_PORT      		    GPIOB
#define     	mpptGPIO_XT60_EN_PIN       		    GPIO_PIN_8
#define     	mpptGPIO_XT60_EN_ON()      		    GPIO_BOP(mpptGPIO_XT60_EN_PORT) = mpptGPIO_XT60_EN_PIN   //使能发送
#define     	mpptGPIO_XT60_EN_OFF()     		    GPIO_BC(mpptGPIO_XT60_EN_PORT)  = mpptGPIO_XT60_EN_PIN   //使能接收


void vMppt_IfaceInit(void);
void vMppt_IfaceDeInit(void);
bool bMppt_DataSendStart(u8* data,u16 len);

#if(boardMPPT_485_IFACE_EN)
void vMppt_485TransEnable(bool en);
#endif

#endif  //boardMPPT_IFACE

#endif //MD_MPPT_USART_H_

