#ifndef GPIO_INIT_H_
#define GPIO_INIT_H_

#include "main.h"
#include "board_config.h"

//#define       gpioVCC21_EN_RCU        				RCU_GPIOB
//#define       gpioVCC21_EN_GPIO          				GPIOB
//#define       gpioVCC21_EN_PIN           				GPIO_PIN_2
//#define       gpioVCC21_EN_ON()         				GPIO_BOP(gpioVCC21_EN_GPIO) = (uint32_t)gpioVCC21_EN_PIN   
//#define       gpioVCC21_EN_OFF()         				GPIO_BC(gpioVCC21_EN_GPIO) = (uint32_t)gpioVCC21_EN_PIN
//#define       gpioVCC21_EN_IsON()        				(GPIO_ISTAT(gpioVCC21_EN_GPIO) & (gpioVCC21_EN_PIN))

#define     	gpioASSIST_OPEN_RCU               		RCU_GPIOC
#define     	gpioASSIST_OPEN_PORT              		GPIOC
#define     	gpioASSIST_OPEN_PIN               		GPIO_PIN_0
#define     	gpioASSIST_OPEN_ON()              		GPIO_BOP(gpioASSIST_OPEN_PORT) = (uint32_t)gpioASSIST_OPEN_PIN  
#define     	gpioASSIST_OPEN_OFF()             		GPIO_BC(gpioASSIST_OPEN_PORT) = (uint32_t)gpioASSIST_OPEN_PIN
#define     	gpioASSIST_GET_STATE()             		gpio_output_bit_get(gpioASSIST_OPEN_PORT, gpioASSIST_OPEN_PIN)

//************************************USART0***************************
#define     	gpioUSART0_REMAP_EN               		0
#if(!gpioUSART0_REMAP_EN)
#define     	gpioUSART0_GPIO_RX_RCU                 	RCU_GPIOA
#define     	gpioUSART0_GPIO_RX_PORT                	GPIOA
#define     	gpioUSART0_GPIO_RX_PIN                 	GPIO_PIN_10
#define     	gpioUSART0_GPIO_TX_RCU                 	RCU_GPIOA
#define     	gpioUSART0_GPIO_TX_PORT                	GPIOA
#define     	gpioUSART0_GPIO_TX_PIN                 	GPIO_PIN_9
#else
//´®¿Ú0ÖØÓ³Éä
#define     	gpioUSART0_GPIO_RX_RCU                 	RCU_GPIOB
#define     	gpioUSART0_GPIO_RX_PORT                	GPIOB
#define     	gpioUSART0_GPIO_RX_PIN                 	GPIO_PIN_7
#define     	gpioUSART0_GPIO_TX_RCU                 	RCU_GPIOB
#define     	gpioUSART0_GPIO_TX_PORT                	GPIOB
#define     	gpioUSART0_GPIO_TX_PIN                 	GPIO_PIN_6
#endif  //gpioUSART0_REMAP_EN
//DMA 
#define     	gpioUSART0_DMA                    		DMA0
#define     	gpioUSART0_DMA_RCU                		RCU_DMA0
#define     	gpioUSART0_DMA_RX_CH              		DMA_CH4
#define     	gpioUSART0_DMA_TX_CH              		DMA_CH3
#define     	gpioUSART0_DMA_TX_IRQ             		DMA0_Channel3_IRQn
#define     	gpioUSART0_DMA_TX_IRQ_HANDLER     		DMA0_Channel3_IRQHandler

//************************************USART1***************************
#define     	gpioUSART1_GPIO_RX_RCU                 	RCU_GPIOA
#define     	gpioUSART1_GPIO_RX_PORT                	GPIOA
#define     	gpioUSART1_GPIO_RX_PIN                 	GPIO_PIN_3
#define     	gpioUSART1_GPIO_TX_RCU                 	RCU_GPIOA
#define     	gpioUSART1_GPIO_TX_PORT                	GPIOA
#define     	gpioUSART1_GPIO_TX_PIN                 	GPIO_PIN_2

//DMA 
#define     	gpioUSART1_DMA                    		DMA0
#define     	gpioUSART1_DMA_RCU                		RCU_DMA0
#define     	gpioUSART1_DMA_RX_CH              		DMA_CH5
#define     	gpioUSART1_DMA_TX_CH              		DMA_CH6
#define     	gpioUSART1_DMA_TX_IRQ             		DMA0_Channel6_IRQn
#define     	gpioUSART1_DMA_TX_IRQ_HANDLER     		DMA0_Channel6_IRQHandler

//************************************USART2***************************
#define     	gpioUSART2_GPIO_RX_RCU                 	RCU_GPIOB
#define     	gpioUSART2_GPIO_RX_PORT                	GPIOB
#define     	gpioUSART2_GPIO_RX_PIN                 	GPIO_PIN_11
#define     	gpioUSART2_GPIO_TX_RCU                 	RCU_GPIOB
#define     	gpioUSART2_GPIO_TX_PORT                	GPIOB
#define     	gpioUSART2_GPIO_TX_PIN                 	GPIO_PIN_10

//DMA 
#define     	gpioUSART2_DMA                    		DMA0
#define     	gpioUSART2_DMA_RCU                		RCU_DMA0
#define     	gpioUSART2_DMA_RX_CH              		DMA_CH2
#define     	gpioUSART2_DMA_TX_CH              		DMA_CH1
#define     	gpioUSART2_DMA_TX_IRQ             		DMA0_Channel1_IRQn
#define     	gpioUSART2_DMA_TX_IRQ_HANDLER     		DMA0_Channel1_IRQHandler

//************************************UART3***************************
#define     	gpioUART3_GPIO_RX_RCU                  	RCU_GPIOC
#define     	gpioUART3_GPIO_RX_PORT                 	GPIOC
#define     	gpioUART3_GPIO_RX_PIN                  	GPIO_PIN_11
#define     	gpioUART3_GPIO_TX_RCU                  	RCU_GPIOC
#define     	gpioUART3_GPIO_TX_PORT                 	GPIOC
#define     	gpioUART3_GPIO_TX_PIN                  	GPIO_PIN_10

//DMA 
#define     	gpioUART3_DMA                     		DMA1
#define     	gpioUART3_DMA_RCU                 		RCU_DMA1
#define     	gpioUART3_DMA_RX_CH               		DMA_CH2
#define     	gpioUART3_DMA_TX_CH               		DMA_CH4
#define     	gpioUART3_DMA_TX_IRQ              		DMA1_Channel3_Channel4_IRQn
#define     	gpioUART3_DMA_TX_IRQ_HANDLER      		DMA1_Channel3_4_IRQHandler

//************************************UART4***************************
#define     	gpioUART4_GPIO_RX_RCU                  	RCU_GPIOD
#define     	gpioUART4_GPIO_RX_PORT                 	GPIOD
#define     	gpioUART4_GPIO_RX_PIN                  	GPIO_PIN_2
#define     	gpioUART4_GPIO_TX_RCU                  	RCU_GPIOC
#define     	gpioUART4_GPIO_TX_PORT                 	GPIOC
#define     	gpioUART4_GPIO_TX_PIN                  	GPIO_PIN_12


void vGPIO_Init(void);
//bool vGPIO_V21PowerSwitch(bool en);
bool vGPIO_AssistBmsOpen(bool en);
//bool vGPIO_CtrlV21Power(void);

#if(boardLOW_POWER)
void vGPIO_EnterLowPower(void);
void vGPIO_ExitLowPower(void);
#endif

#endif
