#ifndef MD_BMS_IFACE_H_
#define MD_BMS_IFACE_H_

#include "board_config.h"

#if(boardBMS_IFACE && boardBMS_EN)

#include "main.h"
#include "gpio_init.h"


#if(boardBMS_IFACE == 1)
//RX
#define     	bmsUSART_GPIO_RX_RCU           			gpioUSART0_GPIO_RX_RCU
#define     	bmsUSART_GPIO_RX_GPIO          			gpioUSART0_GPIO_RX_PORT 
#define     	bmsUSART_GPIO_RX_PIN           			gpioUSART0_GPIO_RX_PIN
//TX
#define     	bmsUSART_GPIO_TX_RCU           			gpioUSART0_GPIO_TX_RCU
#define     	bmsUSART_GPIO_TX_GPIO          			gpioUSART0_GPIO_TX_PORT
#define     	bmsUSART_GPIO_TX_PIN           			gpioUSART0_GPIO_TX_PIN
//串口
#define     	bmsUSART_RCU           					RCU_USART0
#define     	bmsUSART               					USART0
#define     	bmsUSART_BAUD          					115200
#define     	bmsUSART_IRQ           					USART0_IRQn
#define     	bmsUSART_IRQ_HANDLER   					USART0_IRQHandler
//DMA
#if		(boardBMS_IFACE_DMA_EN)
#define     	bmsUSART_DMA                 			gpioUSART0_DMA
#define     	bmsUSART_DMA_RCU             			gpioUSART0_DMA_RCU
#define     	bmsUSART_DMA_RX_CH           			gpioUSART0_DMA_RX_CH
#define     	bmsUSART_DMA_TX_CH           			gpioUSART0_DMA_TX_CH
#define     	bmsUSART_DMA_TX_IRQ          			gpioUSART0_DMA_TX_IRQ
#define     	bmsUSART_DMA_TX_IRQ_HANDLER  			gpioUSART0_DMA_TX_IRQ_HANDLER
#endif  //bmsUSART_DMA_EN


#elif(boardBMS_IFACE == 2)
//RX
#define     	bmsUSART_GPIO_RX_RCU           			gpioUSART1_GPIO_RX_RCU
#define     	bmsUSART_GPIO_RX_GPIO          			gpioUSART1_GPIO_RX_PORT 
#define     	bmsUSART_GPIO_RX_PIN           			gpioUSART1_GPIO_RX_PIN
//TX
#define     	bmsUSART_GPIO_TX_RCU           			gpioUSART1_GPIO_TX_RCU
#define     	bmsUSART_GPIO_TX_GPIO          			gpioUSART1_GPIO_TX_PORT
#define     	bmsUSART_GPIO_TX_PIN           			gpioUSART1_GPIO_TX_PIN
//串口
#define     	bmsUSART_RCU           					RCU_USART1
#define     	bmsUSART               					USART1
#define     	bmsUSART_BAUD          					115200
#define     	bmsUSART_IRQ           					USART1_IRQn
#define     	bmsUSART_IRQ_HANDLER   					USART1_IRQHandler
//DMA
#if		(boardBMS_IFACE_DMA_EN)
#define     	bmsUSART_DMA                 			gpioUSART1_DMA
#define     	bmsUSART_DMA_RCU             			gpioUSART1_DMA_RCU
#define     	bmsUSART_DMA_RX_CH           			gpioUSART1_DMA_RX_CH
#define     	bmsUSART_DMA_TX_CH           			gpioUSART1_DMA_TX_CH
#define     	bmsUSART_DMA_TX_IRQ          			gpioUSART1_DMA_TX_IRQ
#define     	bmsUSART_DMA_TX_IRQ_HANDLER  			gpioUSART1_DMA_TX_IRQ_HANDLER
#endif  //bmsUSART_DMA_EN


#elif	(boardBMS_IFACE == 3)
//RX
#define     	bmsUSART_GPIO_RX_RCU           			gpioUSART2_GPIO_RX_RCU
#define     	bmsUSART_GPIO_RX_GPIO          			gpioUSART2_GPIO_RX_PORT
#define     	bmsUSART_GPIO_RX_PIN           			gpioUSART2_GPIO_RX_PIN
//TX
#define     	bmsUSART_GPIO_TX_RCU           			gpioUSART2_GPIO_TX_RCU
#define     	bmsUSART_GPIO_TX_GPIO          			gpioUSART2_GPIO_TX_PORT
#define     	bmsUSART_GPIO_TX_PIN           			gpioUSART2_GPIO_TX_PIN
//串口
#define     	bmsUSART_RCU           					RCU_USART2
#define     	bmsUSART               					USART2
#define     	bmsUSART_BAUD          					115200
#define     	bmsUSART_IRQ           					USART2_IRQn
#define     	bmsUSART_IRQ_HANDLER   					USART2_IRQHandler
//DMA
#if		(boardBMS_IFACE_DMA_EN)
#define     	bmsUSART_DMA                 			gpioUSART2_DMA
#define     	bmsUSART_DMA_RCU             			gpioUSART2_DMA_RCU
#define     	bmsUSART_DMA_RX_CH           			gpioUSART2_DMA_RX_CH
#define     	bmsUSART_DMA_TX_CH           			gpioUSART2_DMA_TX_CH
#define     	bmsUSART_DMA_TX_IRQ          			gpioUSART2_DMA_TX_IRQ
#define     	bmsUSART_DMA_TX_IRQ_HANDLER  			gpioUSART2_DMA_TX_IRQ_HANDLER
#endif  //bmsUSART_DMA_EN


#elif	(boardBMS_IFACE == 4)
//RX
#define     	bmsUSART_GPIO_RX_RCU           			gpioUART3_GPIO_RX_RCU
#define     	bmsUSART_GPIO_RX_GPIO          			gpioUART3_GPIO_RX_PORT
#define     	bmsUSART_GPIO_RX_PIN           			gpioUART3_GPIO_RX_PIN
//TX
#define     	bmsUSART_GPIO_TX_RCU           			gpioUART3_GPIO_TX_RCU
#define     	bmsUSART_GPIO_TX_GPIO          			gpioUART3_GPIO_TX_PORT
#define     	bmsUSART_GPIO_TX_PIN           			gpioUART3_GPIO_TX_PIN
//串口
#define     	bmsUSART_RCU           					RCU_UART3
#define     	bmsUSART               					UART3
#define     	bmsUSART_BAUD          					115200
#define     	bmsUSART_IRQ           					UART3_IRQn
#define     	bmsUSART_IRQ_HANDLER   					UART3_IRQHandler
//DMA
#if		(boardBMS_IFACE_DMA_EN)
#define     	bmsUSART_DMA                 			gpioUART3_DMA
#define     	bmsUSART_DMA_RCU             			gpioUART3_DMA_RCU
#define     	bmsUSART_DMA_RX_CH           			gpioUART3_DMA_RX_CH
#define     	bmsUSART_DMA_TX_CH           			gpioUART3_DMA_TX_CH
#define     	bmsUSART_DMA_TX_IRQ          			gpioUART3_DMA_TX_IRQ
#define     	bmsUSART_DMA_TX_IRQ_HANDLER  			gpioUART3_DMA_TX_IRQ_HANDLER
#endif  //bmsUSART_DMA_EN


#elif	(boardBMS_IFACE == 5)
//RX
#define     	bmsUSART_GPIO_RX_RCU          			gpioUART4_GPIO_RX_RCU
#define     	bmsUSART_GPIO_RX_GPIO					gpioUART4_GPIO_RX_PORT
#define     	bmsUSART_GPIO_RX_PIN          			gpioUART4_GPIO_RX_PIN
//TX
#define     	bmsUSART_GPIO_TX_RCU          			gpioUART4_GPIO_TX_RCU
#define     	bmsUSART_GPIO_TX_GPIO         			gpioUART4_GPIO_TX_PORT
#define     	bmsUSART_GPIO_TX_PIN          			gpioUART4_GPIO_TX_PIN
//串口
#define     	bmsUSART_RCU          					RCU_UART4
#define     	bmsUSART              					UART4
#define     	bmsUSART_BAUD         					115200
#define     	bmsUSART_IRQ          					UART4_IRQn
#define     	bmsUSART_IRQ_HANDLER  					UART4_IRQHandler
#endif


#if		(boardBMS_485_IFACE_EN)
#define     	bmsGPIO_485_TX_EN_RCU      				RCU_GPIOB
#define     	bmsGPIO_485_TX_EN_GPIO     				GPIOB
#define     	bmsGPIO_485_TX_EN_PIN      				GPIO_PIN_2
#define     	bmsGPIO_485_TX_EN_ON()     				GPIO_BOP(bmsGPIO_485_TX_EN_GPIO) = bmsGPIO_485_TX_EN_PIN   //使能发送
#define     	bmsGPIO_485_TX_EN_OFF()    				GPIO_BC(bmsGPIO_485_TX_EN_GPIO)  = bmsGPIO_485_TX_EN_PIN   //使能接收
#define     	bmsGPIO_485_TX_EN_SATTE()               gpio_output_bit_get(bmsGPIO_485_TX_EN_GPIO,bmsGPIO_485_TX_EN_PIN)
#endif 	//boardBMS_485_IFACE_EN

extern __IO bool bBmsUseFlag;

void vBms_IfaceInit(void);
void vBms_IfaceDeInit(void);
bool bBms_DataSendStart(u8* data, u16 len);

#if(boardBMS_485_IFACE_EN)
void vBms_485TransEnable(bool en);
#endif

#endif  //boardBMS_IFACE && boardBMS_EN

#endif  //MD_BMS_IFACE_H_

