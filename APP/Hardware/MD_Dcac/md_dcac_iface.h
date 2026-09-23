/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_iface.h
 * Date    : 2026-09-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器通信硬件接口与驱动配置头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DCAC_IFACE_H_
#define MD_DCAC_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDCAC_IFACE && boardDCAC_EN)
#include "gpio_init.h"

//****************************************************Macros********************************************************************//
#if (boardDCAC_IFACE == 1)
/* RX */
#define     	dcacUSART_GPIO_RX_RCU           		gpioUSART0_GPIO_RX_RCU
#define     	dcacUSART_GPIO_RX_PORT          		gpioUSART0_GPIO_RX_PORT 
#define     	dcacUSART_GPIO_RX_PIN           		gpioUSART0_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_RX_AF            		gpioUSART0_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	dcacUSART_GPIO_TX_RCU           		gpioUSART0_GPIO_TX_RCU
#define     	dcacUSART_GPIO_TX_PORT          		gpioUSART0_GPIO_TX_PORT
#define     	dcacUSART_GPIO_TX_PIN           		gpioUSART0_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_TX_AF            		gpioUSART0_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	dcacUSART_RCU                   		RCU_USART0
#define     	dcacUSART                       		USART0
#define     	dcacUSART_BAUD                  		9600
#define     	dcacUSART_IRQ                   		USART0_IRQn
#define     	dcacUSART_IRQ_HANDLER           		USART0_IRQHandler
/* DMA */
#if (boardDCAC_IFACE_DMA_EN)
#define     	dcacUSART_DMA                   		gpioUSART0_DMA
#define     	dcacUSART_DMA_RCU               		gpioUSART0_DMA_RCU
#define     	dcacUSART_DMA_RX_CH             		gpioUSART0_DMA_RX_CH
#define     	dcacUSART_DMA_TX_CH             		gpioUSART0_DMA_TX_CH
#define     	dcacUSART_DMA_TX_IRQ            		gpioUSART0_DMA_TX_IRQ
#define     	dcacUSART_DMA_TX_IRQ_HANDLER    		gpioUSART0_DMA_TX_IRQ_HANDLER
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_DMA_TX_REQUEST        		DMA_REQUEST_USART0_TX
#define     	dcacUSART_DMA_RX_REQUEST        		DMA_REQUEST_USART0_RX
#endif  /* boardIC_TYPE */
#endif  /* boardDCAC_IFACE_DMA_EN */

#elif (boardDCAC_IFACE == 2)
/* RX */
#define     	dcacUSART_GPIO_RX_RCU           		gpioUSART1_GPIO_RX_RCU
#define     	dcacUSART_GPIO_RX_PORT          		gpioUSART1_GPIO_RX_PORT 
#define     	dcacUSART_GPIO_RX_PIN           		gpioUSART1_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_RX_AF            		gpioUSART1_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	dcacUSART_GPIO_TX_RCU           		gpioUSART1_GPIO_TX_RCU
#define     	dcacUSART_GPIO_TX_PORT          		gpioUSART1_GPIO_TX_PORT
#define     	dcacUSART_GPIO_TX_PIN           		gpioUSART1_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_TX_AF            		gpioUSART1_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	dcacUSART_RCU                   		RCU_USART1
#define     	dcacUSART                       		USART1
#define     	dcacUSART_BAUD                  		4800
#define     	dcacUSART_IRQ                   		USART1_IRQn
#define     	dcacUSART_IRQ_HANDLER           		USART1_IRQHandler
/* DMA */
#if (boardDCAC_IFACE_DMA_EN)
#define     	dcacUSART_DMA                   		gpioUSART1_DMA
#define     	dcacUSART_DMA_RCU               		gpioUSART1_DMA_RCU
#define     	dcacUSART_DMA_RX_CH             		gpioUSART1_DMA_RX_CH
#define     	dcacUSART_DMA_TX_CH             		gpioUSART1_DMA_TX_CH
#define     	dcacUSART_DMA_TX_IRQ            		gpioUSART1_DMA_TX_IRQ
#define     	dcacUSART_DMA_TX_IRQ_HANDLER    		gpioUSART1_DMA_TX_IRQ_HANDLER
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_DMA_TX_REQUEST        		DMA_REQUEST_USART1_TX
#define     	dcacUSART_DMA_RX_REQUEST        		DMA_REQUEST_USART1_RX
#endif  /* boardIC_TYPE */
#endif  /* boardDCAC_IFACE_DMA_EN */

#elif (boardDCAC_IFACE == 3)
/* RX */
#define     	dcacUSART_GPIO_RX_RCU           		gpioUSART2_GPIO_RX_RCU
#define     	dcacUSART_GPIO_RX_PORT          		gpioUSART2_GPIO_RX_PORT
#define     	dcacUSART_GPIO_RX_PIN           		gpioUSART2_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_RX_AF            		gpioUSART2_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	dcacUSART_GPIO_TX_RCU           		gpioUSART2_GPIO_TX_RCU
#define     	dcacUSART_GPIO_TX_PORT          		gpioUSART2_GPIO_TX_PORT
#define     	dcacUSART_GPIO_TX_PIN           		gpioUSART2_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_TX_AF            		gpioUSART2_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	dcacUSART_RCU                   		RCU_USART2
#define     	dcacUSART                       		USART2
#define     	dcacUSART_BAUD                  		9600
#define     	dcacUSART_IRQ                   		USART2_IRQn
#define     	dcacUSART_IRQ_HANDLER           		USART2_IRQHandler
/* DMA */
#if (boardDCAC_IFACE_DMA_EN)
#define     	dcacUSART_DMA                   		gpioUSART2_DMA
#define     	dcacUSART_DMA_RCU               		gpioUSART2_DMA_RCU
#define     	dcacUSART_DMA_RX_CH             		gpioUSART2_DMA_RX_CH
#define     	dcacUSART_DMA_TX_CH             		gpioUSART2_DMA_TX_CH
#define     	dcacUSART_DMA_TX_IRQ            		gpioUSART2_DMA_TX_IRQ
#define     	dcacUSART_DMA_TX_IRQ_HANDLER    		gpioUSART2_DMA_TX_IRQ_HANDLER
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_DMA_TX_REQUEST        		DMA_REQUEST_USART2_TX
#define     	dcacUSART_DMA_RX_REQUEST        		DMA_REQUEST_USART2_RX
#endif  /* boardIC_TYPE */
#endif  /* boardDCAC_IFACE_DMA_EN */

#elif (boardDCAC_IFACE == 4)
/* RX */
#define     	dcacUSART_GPIO_RX_RCU           		gpioUART3_GPIO_RX_RCU
#define     	dcacUSART_GPIO_RX_PORT          		gpioUART3_GPIO_RX_PORT
#define     	dcacUSART_GPIO_RX_PIN           		gpioUART3_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_RX_AF            		gpioUART3_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	dcacUSART_GPIO_TX_RCU           		gpioUART3_GPIO_TX_RCU
#define     	dcacUSART_GPIO_TX_PORT          		gpioUART3_GPIO_TX_PORT
#define     	dcacUSART_GPIO_TX_PIN           		gpioUART3_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_TX_AF            		gpioUART3_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	dcacUSART_RCU                   		RCU_UART3
#define     	dcacUSART                       		UART3
#define     	dcacUSART_BAUD                  		9600
#define     	dcacUSART_IRQ                   		UART3_IRQn
#define     	dcacUSART_IRQ_HANDLER           		UART3_IRQHandler
/* DMA */
#if (boardDCAC_IFACE_DMA_EN)
#define     	dcacUSART_DMA                   		gpioUART3_DMA
#define     	dcacUSART_DMA_RCU               		gpioUART3_DMA_RCU
#define     	dcacUSART_DMA_RX_CH             		gpioUART3_DMA_RX_CH
#define     	dcacUSART_DMA_TX_CH             		gpioUART3_DMA_TX_CH
#define     	dcacUSART_DMA_TX_IRQ            		gpioUART3_DMA_TX_IRQ
#define     	dcacUSART_DMA_TX_IRQ_HANDLER    		gpioUART3_DMA_TX_IRQ_HANDLER
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_DMA_TX_REQUEST        		DMA_REQUEST_UART3_TX
#define     	dcacUSART_DMA_RX_REQUEST        		DMA_REQUEST_UART3_RX
#endif  /* boardIC_TYPE */
#endif  /* boardDCAC_IFACE_DMA_EN */

#elif (boardDCAC_IFACE == 5)
/* RX */
#define     	dcacUSART_GPIO_RX_RCU           		gpioUART4_GPIO_RX_RCU
#define     	dcacUSART_GPIO_RX_PORT          		gpioUART4_GPIO_RX_PORT
#define     	dcacUSART_GPIO_RX_PIN           		gpioUART4_GPIO_RX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_RX_AF            		gpioUART4_GPIO_RX_AF
#endif  /* boardIC_TYPE */
/* TX */
#define     	dcacUSART_GPIO_TX_RCU           		gpioUART4_GPIO_TX_RCU
#define     	dcacUSART_GPIO_TX_PORT          		gpioUART4_GPIO_TX_PORT
#define     	dcacUSART_GPIO_TX_PIN           		gpioUART4_GPIO_TX_PIN
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	dcacUSART_GPIO_TX_AF            		gpioUART4_GPIO_TX_AF
#endif  /* boardIC_TYPE */
/* 串口 */
#define     	dcacUSART_RCU                   		RCU_UART4
#define     	dcacUSART                       		UART4
#define     	dcacUSART_BAUD                  		9600
#define     	dcacUSART_IRQ                   		UART4_IRQn
#define     	dcacUSART_IRQ_HANDLER           		UART4_IRQHandler
#endif  /* boardDCAC_IFACE == 1 */

#define     	dcacPOWER_EN_RCU                		RCU_GPIOB
#define     	dcacPOWER_EN_GPIO               		GPIOB
#define     	dcacPOWER_EN_PIN                		GPIO_PIN_1
#define     	dcacPOWER_EN_ON()               		GPIO_BOP(dcacPOWER_EN_GPIO) = dcacPOWER_EN_PIN
#define     	dcacPOWER_EN_OFF()              		GPIO_BC(dcacPOWER_EN_GPIO)  = dcacPOWER_EN_PIN
#define     	dcacPOWER_EN_STATE()            		gpio_output_bit_get(dcacPOWER_EN_GPIO, dcacPOWER_EN_PIN)

#if (boardDCAC_485_IFACE_EN)
#define     	dcacGPIO_485_TX_EN_RCU          		RCU_GPIOB
#define     	dcacGPIO_485_TX_EN_PORT         		GPIOB
#define     	dcacGPIO_485_TX_EN_PIN          		GPIO_PIN_2
#define			dcacGPIO_485_TX_EN_ON()         		GPIO_BOP(dcacGPIO_485_TX_EN_PORT) = dcacGPIO_485_TX_EN_PIN	/* 使能发送 */
#define			dcacGPIO_485_TX_EN_OFF()        		GPIO_BC(dcacGPIO_485_TX_EN_PORT)  = dcacGPIO_485_TX_EN_PIN	/* 使能接收 */
#define     	dcacGPIO_485_TX_EN_STATE()      		gpio_output_bit_get(dcacGPIO_485_TX_EN_PORT, dcacGPIO_485_TX_EN_PIN)
#endif  /* boardDCAC_485_IFACE_EN */

//****************************************************Globals*******************************************************************//
extern __IO bool bDcacUseFlag;

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
void vDcac_IfaceInit(void);
void vDcac_IfaceDeInit(void);
bool bDcac_IfaceSetBaud(uint32_t ul_baud);
bool bDcac_DataSendStart(uint8_t *p_data, uint16_t us_len);

#if (boardDCAC_485_IFACE_EN)
void vDcac_485TransEnable(bool b_en);
#endif  /* boardDCAC_485_IFACE_EN */

#if (boardLOW_POWER)
void vDcac_IoEnterLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardDCAC_IFACE && boardDCAC_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DCAC_IFACE_H_ */
