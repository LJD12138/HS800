#ifndef PRINT_USART_H_
#define PRINT_USART_H_

#include "board_config.h"

#if(boardPRINT_IFACE)

#include "main.h"
#include "gpio_init.h"

/*-------------------------------------------------------------------------------------*/
#if(boardPRINT_IFACE == 1)
//RX
#define     	printUSART_GPIO_RX_RCU          		gpioUSART0_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUSART0_GPIO_RX_PORT 
#define     	printUSART_GPIO_RX_PIN          		gpioUSART0_GPIO_RX_PIN
//TX
#define     	printUSART_GPIO_TX_RCU          		gpioUSART0_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUSART0_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUSART0_GPIO_TX_PIN
//串口
#if(!boardPRINT_IFACE_DMA_EN)
#define     	printUSART_IRQ_EN       				1
#endif

#define     	printUSART_RCU           				RCU_USART0
#define     	printUSART               				USART0
#define     	printUSART_BAUD          				115200
#define     	printUSART_IRQ           				USART0_IRQn
#define     	printUSART_IRQ_HANDLER   				USART0_IRQHandler
//DMA
#if(boardPRINT_IFACE_DMA_EN)
#define     	printUSART_DMA                 			gpioUSART0_DMA
#define     	printUSART_DMA_RCU             			gpioUSART0_DMA_RCU
#define     	printUSART_DMA_RX_CH           			gpioUSART0_DMA_RX_CH
#define     	printUSART_DMA_TX_CH           			gpioUSART0_DMA_TX_CH
#define     	printUSART_DMA_TX_IRQ          			gpioUSART0_DMA_TX_IRQ
#define     	printUSART_DMA_TX_IRQ_HANDLER  			gpioUSART0_DMA_TX_IRQ_HANDLER
#endif  //boardPRINT_IFACE_DMA_EN

#elif(boardPRINT_IFACE == 2)
//RX
#define     	printUSART_GPIO_RX_RCU          		gpioUSART1_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUSART1_GPIO_RX_PORT 
#define     	printUSART_GPIO_RX_PIN          		gpioUSART1_GPIO_RX_PIN
//TX
#define     	printUSART_GPIO_TX_RCU          		gpioUSART1_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUSART1_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUSART1_GPIO_TX_PIN
//串口
#define     	printUSART_IRQ_EN       				1
#define     	printUSART_RCU          				RCU_USART1
#define     	printUSART               				USART1
#define     	printUSART_BAUD         				115200
#define     	printUSART_IRQ          				USART1_IRQn
#define     	printUSART_IRQ_HANDLER           		USART1_IRQHandler
//DMA
#if(boardPRINT_IFACE_DMA_EN)
#define     	printUSART_DMA                 			gpioUSART1_DMA
#define     	printUSART_DMA_RCU             			gpioUSART1_DMA_RCU
#define     	printUSART_DMA_RX_CH          			gpioUSART1_DMA_RX_CH
#define     	printUSART_DMA_TX_CH          			gpioUSART1_DMA_TX_CH
#define     	printUSART_DMA_TX_IRQ         			gpioUSART1_DMA_TX_IRQ
#define     	printUSART_DMA_TX_IRQ_HANDLER  			gpioUSART1_DMA_TX_IRQ_HANDLER
#endif  //boardPRINT_IFACE_DMA_EN

#elif(boardPRINT_IFACE == 3)
//RX
#define     	printUSART_GPIO_RX_RCU          		gpioUSART2_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUSART2_GPIO_RX_PORT
#define     	printUSART_GPIO_RX_PIN          		gpioUSART2_GPIO_RX_PIN
//TX
#define     	printUSART_GPIO_TX_RCU          		gpioUSART2_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUSART2_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUSART2_GPIO_TX_PIN
//串口
#define     	printUSART_IRQ_EN       				1
#define     	printUSART_RCU          				RCU_USART2
#define     	printUSART               				USART2
#define     	printUSART_BAUD         				115200
#define     	printUSART_IRQ          				USART2_IRQn
#define     	printUSART_IRQ_HANDLER           		USART2_IRQHandler
//DMA
#if(boardPRINT_IFACE_DMA_EN)
#define     	printUSART_DMA                 			gpioUSART2_DMA
#define     	printUSART_DMA_RCU             			gpioUSART2_DMA_RCU
#define     	printUSART_DMA_RX_CH          			gpioUSART2_DMA_RX_CH
#define     	printUSART_DMA_TX_CH          			gpioUSART2_DMA_TX_CH
#define     	printUSART_DMA_TX_IRQ         			gpioUSART2_DMA_TX_IRQ
#define     	printUSART_DMA_TX_IRQ_HANDLER  			gpioUSART2_DMA_TX_IRQ_HANDLER
#endif  //boardPRINT_IFACE_DMA_EN

#elif(boardPRINT_IFACE == 4)
//RX
#define     	printUSART_GPIO_RX_RCU          		gpioUART3_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUART3_GPIO_RX_PORT
#define     	printUSART_GPIO_RX_PIN          		gpioUART3_GPIO_RX_PIN
//TX
#define     	printUSART_GPIO_TX_RCU          		gpioUART3_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUART3_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUART3_GPIO_TX_PIN
//串口
#define     	printUSART_IRQ_EN       				1
#define     	printUSART_RCU          				RCU_UART3
#define     	printUSART               				UART3
#define     	printUSART_BAUD         				115200
#define     	printUSART_IRQ          				UART3_IRQn
#define     	printUSART_IRQ_HANDLER           		UART3_IRQHandler
//DMA
#if(boardPRINT_IFACE_DMA_EN)
#define     	printUSART_DMA                 			gpioUART3_DMA
#define     	printUSART_DMA_RCU             			gpioUART3_DMA_RCU
#define     	printUSART_DMA_RX_CH          			gpioUART3_DMA_RX_CH
#define     	printUSART_DMA_TX_CH          			gpioUART3_DMA_TX_CH
#define     	printUSART_DMA_TX_IRQ         			gpioUART3_DMA_TX_IRQ
#define     	printUSART_DMA_TX_IRQ_HANDLER  			gpioUART3_DMA_TX_IRQ_HANDLER
#endif  //boardPRINT_IFACE_DMA_EN

#elif(boardPRINT_IFACE == 5)
//RX
#define     	printUSART_GPIO_RX_RCU          		gpioUART4_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUART4_GPIO_RX_PORT
#define     	printUSART_GPIO_RX_PIN          		gpioUART4_GPIO_RX_PIN
//TX 
#define     	printUSART_GPIO_TX_RCU          		gpioUART4_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUART4_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUART4_GPIO_TX_PIN
//串口
#define     	printUSART_IRQ_EN       				1
#define     	printUSART_RCU          				RCU_UART4
#define     	printUSART               				UART4
#define     	printUSART_BAUD         				115200
#define     	printUSART_IRQ          				UART4_IRQn
#define     	printUSART_IRQ_HANDLER           		UART4_IRQHandler
#endif
/*-------------------------------------------------------------------------------------*/


#if(boardPRINT_485_IFACE_EN)
#define     	printGPIO_485_TX_EN_RCU       			RCU_GPIOA
#define     	printGPIO_485_TX_EN_PORT      			GPIOA
#define     	printGPIO_485_TX_EN_PIN       			GPIO_PIN_8
#define     	printGPIO_485_TX_EN_ON()      			GPIO_BOP(printGPIO_485_TX_EN_PORT) = (uint32_t)printGPIO_485_TX_EN_PIN   //使能发送
#define     	printGPIO_485_TX_EN_OFF()     			GPIO_BC(printGPIO_485_TX_EN_PORT) = (uint32_t)printGPIO_485_TX_EN_PIN   //使能接收
#endif //boardPRINT_485_IFACE_EN

#define     	printIFACE_EN_RCU       				RCU_GPIOC
#define     	printIFACE_EN_PORT      				GPIOC
#define     	printIFACE_EN_PIN       				GPIO_PIN_3
#define     	printIFACE_EN_ON()      				GPIO_BOP(printIFACE_EN_PORT) = (uint32_t)printIFACE_EN_PIN   //使能发送
#define     	printIFACE_EN_OFF()     				GPIO_BC(printIFACE_EN_PORT) = (uint32_t)printIFACE_EN_PIN   //使能接收


void vPrint_Init(void);//串口初始化
void vPrint_DeInit( void );//串口初始化
bool bPrint_DataSendStart(u16 len);
bool bPrint_CheckSendFinish(void);

#if(boardPRINT_485_IFACE_EN)
void vPrint_485TransEnable(bool en);
#endif //boardPRINT_485_IFACE_EN

#if(boardLOW_POWER)
void vPrint_EnterLowPower( void );
#endif  //boardLOW_POWER

#endif  //boardPRINT_IFACE

#endif  //PRINT_USART_H_


