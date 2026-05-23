#ifndef MD_MPPT_USART_H_
#define MD_MPPT_USART_H_

#include "board_config.h"

#if(boardMPPT_IFACE)
#include "gpio_init.h"

#if(boardMPPT_IFACE == 1)
//RX
#define     	mpptUSART_GPIO_RX_RCU           		gpioUSART0_GPIO_RX_RCU
#define     	mpptUSART_GPIO_RX_PORT          		gpioUSART0_GPIO_RX_PORT 
#define     	mpptUSART_GPIO_RX_PIN           		gpioUSART0_GPIO_RX_PIN
//TX
#define     	mpptUSART_GPIO_TX_RCU           		gpioUSART0_GPIO_TX_RCU
#define     	mpptUSART_GPIO_TX_PORT          		gpioUSART0_GPIO_TX_PORT
#define     	mpptUSART_GPIO_TX_PIN           		gpioUSART0_GPIO_TX_PIN
//串口
#define     	mpptUSART_RCU           				RCU_USART0
#define     	mpptUSART               				USART0
#define     	mpptUSART_BAUD          				115200
#define     	mpptUSART_IRQ           				USART0_IRQn
#define     	mpptUSART_IRQ_HANDLER   				USART0_IRQHandler
//DMA
#if(boardMPPT_IFACE_DMA_EN)
#define     	mpptUSART_DMA                 			gpioUSART0_DMA
#define     	mpptUSART_DMA_RCU             			gpioUSART0_DMA_RCU
#define     	mpptUSART_DMA_RX_CH           			gpioUSART0_DMA_RX_CH
#define     	mpptUSART_DMA_TX_CH           			gpioUSART0_DMA_TX_CH
#define     	mpptUSART_DMA_TX_IRQ          			gpioUSART0_DMA_TX_IRQ
#define     	mpptUSART_DMA_TX_IRQ_HANDLER  			gpioUSART0_DMA_TX_IRQ_HANDLER
#endif  //mpptUSART_DMA_EN

#elif(boardMPPT_IFACE == 2)
//RX
#define     	mpptUSART_GPIO_RX_RCU           		gpioUSART1_GPIO_RX_RCU
#define     	mpptUSART_GPIO_RX_PORT          		gpioUSART1_GPIO_RX_PORT 
#define     	mpptUSART_GPIO_RX_PIN           		gpioUSART1_GPIO_RX_PIN
//TX
#define     	mpptUSART_GPIO_TX_RCU           		gpioUSART1_GPIO_TX_RCU
#define     	mpptUSART_GPIO_TX_PORT          		gpioUSART1_GPIO_TX_PORT
#define     	mpptUSART_GPIO_TX_PIN           		gpioUSART1_GPIO_TX_PIN
//串口
#define     	mpptUSART_RCU           				RCU_USART1
#define     	mpptUSART               				USART1
#define     	mpptUSART_BAUD          				115200
#define     	mpptUSART_IRQ           				USART1_IRQn
#define     	mpptUSART_IRQ_HANDLER   				USART1_IRQHandler
//DMA
#if(boardMPPT_IFACE_DMA_EN)
#define     	mpptUSART_DMA                 			gpioUSART1_DMA
#define     	mpptUSART_DMA_RCU             			gpioUSART1_DMA_RCU
#define     	mpptUSART_DMA_RX_CH           			gpioUSART1_DMA_RX_CH
#define     	mpptUSART_DMA_TX_CH           			gpioUSART1_DMA_TX_CH
#define     	mpptUSART_DMA_TX_IRQ          			gpioUSART1_DMA_TX_IRQ
#define     	mpptUSART_DMA_TX_IRQ_HANDLER  			gpioUSART1_DMA_TX_IRQ_HANDLER
#endif  //mpptUSART_DMA_EN

#elif(boardMPPT_IFACE == 3)
//RX
#define     	mpptUSART_GPIO_RX_RCU					gpioUSART2_GPIO_RX_RCU
#define     	mpptUSART_GPIO_RX_PORT					gpioUSART2_GPIO_RX_PORT
#define     	mpptUSART_GPIO_RX_PIN					gpioUSART2_GPIO_RX_PIN
//TX
#define     	mpptUSART_GPIO_TX_RCU					gpioUSART2_GPIO_TX_RCU
#define     	mpptUSART_GPIO_TX_PORT					gpioUSART2_GPIO_TX_PORT
#define     	mpptUSART_GPIO_TX_PIN					gpioUSART2_GPIO_TX_PIN
//串口
#define     	mpptUSART_RCU           				RCU_USART2
#define     	mpptUSART               				USART2
#define     	mpptUSART_BAUD          				115200
#define     	mpptUSART_IRQ           				USART2_IRQn
#define     	mpptUSART_IRQ_HANDLER   				USART2_IRQHandler
//DMA
#if(boardMPPT_IFACE_DMA_EN)
#define     	mpptUSART_DMA                 			gpioUSART2_DMA
#define     	mpptUSART_DMA_RCU             			gpioUSART2_DMA_RCU
#define     	mpptUSART_DMA_RX_CH           			gpioUSART2_DMA_RX_CH
#define     	mpptUSART_DMA_TX_CH           			gpioUSART2_DMA_TX_CH
#define     	mpptUSART_DMA_TX_IRQ          			gpioUSART2_DMA_TX_IRQ
#define     	mpptUSART_DMA_TX_IRQ_HANDLER  			gpioUSART2_DMA_TX_IRQ_HANDLER
#endif  //mpptUSART_DMA_EN

#elif(boardMPPT_IFACE == 4)
//RX
#define     	mpptUSART_GPIO_RX_RCU           		gpioUART3_GPIO_RX_RCU
#define     	mpptUSART_GPIO_RX_PORT          		gpioUART3_GPIO_RX_PORT
#define     	mpptUSART_GPIO_RX_PIN           		gpioUART3_GPIO_RX_PIN
//TX
#define     	mpptUSART_GPIO_TX_RCU           		gpioUART3_GPIO_TX_RCU
#define     	mpptUSART_GPIO_TX_PORT          		gpioUART3_GPIO_TX_PORT
#define     	mpptUSART_GPIO_TX_PIN           		gpioUART3_GPIO_TX_PIN
//串口
#define     	mpptUSART_RCU           				RCU_UART3
#define     	mpptUSART               				UART3
#define     	mpptUSART_BAUD          				115200
#define     	mpptUSART_IRQ           				UART3_IRQn
#define     	mpptUSART_IRQ_HANDLER   				UART3_IRQHandler
//DMA
#if(boardMPPT_IFACE_DMA_EN)
#define     	mpptUSART_DMA                 			gpioUART3_DMA
#define     	mpptUSART_DMA_RCU             			gpioUART3_DMA_RCU
#define     	mpptUSART_DMA_RX_CH           			gpioUART3_DMA_RX_CH
#define     	mpptUSART_DMA_TX_CH           			gpioUART3_DMA_TX_CH
#define     	mpptUSART_DMA_TX_IRQ          			gpioUART3_DMA_TX_IRQ
#define     	mpptUSART_DMA_TX_IRQ_HANDLER  			gpioUART3_DMA_TX_IRQ_HANDLER
#endif  //mpptUSART_DMA_EN

#elif(boardMPPT_IFACE == 5)
//RX
#define     	mpptUSART_GPIO_RX_RCU           		gpioUART4_GPIO_RX_RCU
#define     	mpptUSART_GPIO_RX_PORT          		gpioUART4_GPIO_RX_PORT
#define     	mpptUSART_GPIO_RX_PIN           		gpioUART4_GPIO_RX_PIN
//TX
#define     	mpptUSART_GPIO_TX_RCU           		gpioUART4_GPIO_TX_RCU
#define     	mpptUSART_GPIO_TX_PORT          		gpioUART4_GPIO_TX_PORT
#define     	mpptUSART_GPIO_TX_PIN           		gpioUART4_GPIO_TX_PIN
//串口
#define     	mpptUSART_RCU           				RCU_UART4
#define     	mpptUSART               				UART4
#define     	mpptUSART_BAUD          				115200
#define     	mpptUSART_IRQ           				UART4_IRQn
#define     	mpptUSART_IRQ_HANDLER   				UART4_IRQHandler

#endif

#if(boardMPPT_485_IFACE_EN)
#define     	mpptGPIO_485_TX_EN_RCU       		RCU_GPIOB
#define     	mpptGPIO_485_TX_EN_PORT      		GPIOB
#define     	mpptGPIO_485_TX_EN_PIN       		GPIO_PIN_2
#define     	mpptGPIO_485_TX_EN_ON()      		GPIO_BOP(mpptGPIO_485_TX_EN_PORT) = mpptGPIO_485_TX_EN_PIN   //使能发送
#define     	mpptGPIO_485_TX_EN_OFF()     		GPIO_BC(mpptGPIO_485_TX_EN_PORT)  = mpptGPIO_485_TX_EN_PIN   //使能接收
#endif //boardMPPT_485_IFACE_EN



void vMppt_IfaceInit(void);
void vMppt_IfaceDeInit(void);
bool bMppt_DataSendStart(u8* data,u16 len);

#if(boardMPPT_485_IFACE_EN)
void vMppt_485TransEnable(bool en);
#endif

#endif  //boardMPPT_IFACE

#endif //MD_MPPT_USART_H_

