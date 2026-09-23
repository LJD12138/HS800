/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_iface.h
 * Date    : 2026-09-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 打印与串口底层硬件接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef PRINT_IFACE_H_
#define PRINT_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardPRINT_IFACE)
#include "gpio_init.h"

//****************************************************Macros********************************************************************//
#if (boardPRINT_IFACE == 1)
/* RX */
#define     	printUSART_GPIO_RX_RCU          		gpioUSART0_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUSART0_GPIO_RX_PORT 
#define     	printUSART_GPIO_RX_PIN          		gpioUSART0_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_RX_AF           		gpioUSART0_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	printUSART_GPIO_TX_RCU          		gpioUSART0_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUSART0_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUSART0_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_TX_AF           		gpioUSART0_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	printUSART_RCU                  		RCU_USART0
#define     	printUSART                      		USART0
#define     	printUSART_BAUD                 		115200
#define     	printUSART_IRQ                  		USART0_IRQn
#define     	printUSART_IRQ_HANDLER          		USART0_IRQHandler
/* DMA */
#if (boardPRINT_IFACE_DMA_EN)
#define     	printUSART_DMA                  		gpioUSART0_DMA
#define     	printUSART_DMA_RCU              		gpioUSART0_DMA_RCU
#define     	printUSART_DMA_RX_CH            		gpioUSART0_DMA_RX_CH
#define     	printUSART_DMA_TX_CH            		gpioUSART0_DMA_TX_CH
#define     	printUSART_DMA_TX_IRQ           		gpioUSART0_DMA_TX_IRQ
#define     	printUSART_DMA_TX_IRQ_HANDLER   		gpioUSART0_DMA_TX_IRQ_HANDLER
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_DMA_TX_REQUEST       		DMA_REQUEST_USART0_TX
#define     	printUSART_DMA_RX_REQUEST       		DMA_REQUEST_USART0_RX
#endif  /* boardIC_TYPE */
#endif  /* boardPRINT_IFACE_DMA_EN */

#elif (boardPRINT_IFACE == 2)
/* RX */
#define     	printUSART_GPIO_RX_RCU          		gpioUSART1_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUSART1_GPIO_RX_PORT 
#define     	printUSART_GPIO_RX_PIN          		gpioUSART1_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_RX_AF           		gpioUSART1_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	printUSART_GPIO_TX_RCU          		gpioUSART1_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUSART1_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUSART1_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_TX_AF           		gpioUSART1_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	printUSART_RCU                  		RCU_USART1
#define     	printUSART                      		USART1
#define     	printUSART_BAUD                 		115200
#define     	printUSART_IRQ                  		USART1_IRQn
#define     	printUSART_IRQ_HANDLER          		USART1_IRQHandler
/* DMA */
#if (boardPRINT_IFACE_DMA_EN)
#define     	printUSART_DMA                  		gpioUSART1_DMA
#define     	printUSART_DMA_RCU              		gpioUSART1_DMA_RCU
#define     	printUSART_DMA_RX_CH            		gpioUSART1_DMA_RX_CH
#define     	printUSART_DMA_TX_CH            		gpioUSART1_DMA_TX_CH
#define     	printUSART_DMA_TX_IRQ           		gpioUSART1_DMA_TX_IRQ
#define     	printUSART_DMA_TX_IRQ_HANDLER   		gpioUSART1_DMA_TX_IRQ_HANDLER
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_DMA_TX_REQUEST       		DMA_REQUEST_USART1_TX
#define     	printUSART_DMA_RX_REQUEST       		DMA_REQUEST_USART1_RX
#endif  /* boardIC_TYPE */
#endif  /* boardPRINT_IFACE_DMA_EN */

#elif (boardPRINT_IFACE == 3)
/* RX */
#define     	printUSART_GPIO_RX_RCU          		gpioUSART2_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUSART2_GPIO_RX_PORT
#define     	printUSART_GPIO_RX_PIN          		gpioUSART2_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_RX_AF           		gpioUSART2_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	printUSART_GPIO_TX_RCU          		gpioUSART2_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUSART2_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUSART2_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_TX_AF           		gpioUSART2_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	printUSART_RCU                  		RCU_USART2
#define     	printUSART                      		USART2
#define     	printUSART_BAUD                 		115200
#define     	printUSART_IRQ                  		USART2_IRQn
#define     	printUSART_IRQ_HANDLER          		USART2_IRQHandler
/* DMA */
#if (boardPRINT_IFACE_DMA_EN)
#define     	printUSART_DMA                  		gpioUSART2_DMA
#define     	printUSART_DMA_RCU              		gpioUSART2_DMA_RCU
#define     	printUSART_DMA_RX_CH            		gpioUSART2_DMA_RX_CH
#define     	printUSART_DMA_TX_CH            		gpioUSART2_DMA_TX_CH
#define     	printUSART_DMA_TX_IRQ           		gpioUSART2_DMA_TX_IRQ
#define     	printUSART_DMA_TX_IRQ_HANDLER   		gpioUSART2_DMA_TX_IRQ_HANDLER
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_DMA_TX_REQUEST       		DMA_REQUEST_USART2_TX
#define     	printUSART_DMA_RX_REQUEST       		DMA_REQUEST_USART2_RX
#endif  /* boardIC_TYPE */
#endif  /* boardPRINT_IFACE_DMA_EN */

#elif (boardPRINT_IFACE == 4)
/* RX */
#define     	printUSART_GPIO_RX_RCU          		gpioUART3_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUART3_GPIO_RX_PORT
#define     	printUSART_GPIO_RX_PIN          		gpioUART3_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_RX_AF           		gpioUART3_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	printUSART_GPIO_TX_RCU          		gpioUART3_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUART3_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUART3_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_TX_AF           		gpioUART3_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	printUSART_RCU                  		RCU_UART3
#define     	printUSART                      		UART3
#define     	printUSART_BAUD                 		115200
#define     	printUSART_IRQ                  		UART3_IRQn
#define     	printUSART_IRQ_HANDLER          		UART3_IRQHandler
/* DMA */
#if (boardPRINT_IFACE_DMA_EN)
#define     	printUSART_DMA                  		gpioUART3_DMA
#define     	printUSART_DMA_RCU              		gpioUART3_DMA_RCU
#define     	printUSART_DMA_RX_CH            		gpioUART3_DMA_RX_CH
#define     	printUSART_DMA_TX_CH            		gpioUART3_DMA_TX_CH
#define     	printUSART_DMA_TX_IRQ           		gpioUART3_DMA_TX_IRQ
#define     	printUSART_DMA_TX_IRQ_HANDLER   		gpioUART3_DMA_TX_IRQ_HANDLER
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_DMA_TX_REQUEST       		DMA_REQUEST_UART3_TX
#define     	printUSART_DMA_RX_REQUEST       		DMA_REQUEST_UART3_RX
#endif  /* boardIC_TYPE */
#endif  /* boardPRINT_IFACE_DMA_EN */

#elif (boardPRINT_IFACE == 5)
/* RX */
#define     	printUSART_GPIO_RX_RCU          		gpioUART4_GPIO_RX_RCU
#define     	printUSART_GPIO_RX_PORT         		gpioUART4_GPIO_RX_PORT
#define     	printUSART_GPIO_RX_PIN          		gpioUART4_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_RX_AF           		gpioUART4_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	printUSART_GPIO_TX_RCU          		gpioUART4_GPIO_TX_RCU
#define     	printUSART_GPIO_TX_PORT         		gpioUART4_GPIO_TX_PORT
#define     	printUSART_GPIO_TX_PIN          		gpioUART4_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	printUSART_GPIO_TX_AF           		gpioUART4_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	printUSART_RCU                  		RCU_UART4
#define     	printUSART                      		UART4
#define     	printUSART_BAUD                 		115200
#define     	printUSART_IRQ                  		UART4_IRQn
#define     	printUSART_IRQ_HANDLER          		UART4_IRQHandler
#endif  /* boardPRINT_IFACE == 1 */

#if (boardPRINT_485_IFACE_EN)
#define     	printGPIO_485_TX_EN_RCU         		RCU_GPIOA
#define     	printGPIO_485_TX_EN_PORT        		GPIOA
#define     	printGPIO_485_TX_EN_PIN         		GPIO_PIN_8
#define			printGPIO_485_TX_EN_ON()        		GPIO_BOP(printGPIO_485_TX_EN_PORT) = (uint32_t)printGPIO_485_TX_EN_PIN	/* 使能发送 */
#define			printGPIO_485_TX_EN_OFF()       		GPIO_BC(printGPIO_485_TX_EN_PORT)  = (uint32_t)printGPIO_485_TX_EN_PIN	/* 使能接收 */
#define     	printGPIO_485_TX_EN_STATE()     		gpio_output_bit_get(printGPIO_485_TX_EN_PORT, printGPIO_485_TX_EN_PIN)
#endif	/* boardPRINT_485_IFACE_EN */

#define     	printIFACE_EN_RCU               		RCU_GPIOC
#define     	printIFACE_EN_PORT              		GPIOC
#define     	printIFACE_EN_PIN               		GPIO_PIN_3
#define			printIFACE_EN_ON()              		GPIO_BOP(printIFACE_EN_PORT) = (uint32_t)printIFACE_EN_PIN	/* 使能接口 */
#define			printIFACE_EN_OFF()             		GPIO_BC(printIFACE_EN_PORT)  = (uint32_t)printIFACE_EN_PIN	/* 禁用接口 */

//****************************************************Globals*******************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
void vPrint_Init(void);
void vPrint_DeInit(void);
#define vPrint_IfaceInit   vPrint_Init
#define vPrint_IfaceDeInit vPrint_DeInit

bool bPrint_DataSendStart(uint16_t us_len);
bool bPrint_CheckSendFinish(void);

#if (boardPRINT_485_IFACE_EN)
void vPrint_485TransEnable(bool b_en);
#endif  /* boardPRINT_485_IFACE_EN */

#if (boardLOW_POWER)
void vPrint_EnterLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardPRINT_IFACE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PRINT_IFACE_H_ */
