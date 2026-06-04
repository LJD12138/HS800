#ifndef GPIO_INIT_H_
#define GPIO_INIT_H_

#include "main.h"
#include "board_config.h"

#define     	KEY_POWER_RCU       					RCU_GPIOC
#define     	KEY_POWER_GPIO      					GPIOC
#define     	KEY_POWER_PIN       					GPIO_PIN_4

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
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	gpioUSART0_GPIO_RX_AF                   GPIO_AF_0
#define     	gpioUSART0_GPIO_TX_AF                   GPIO_AF_0
#endif //(boardIC_TYPE == boardIC_GD32F50X)
#else
//串口0重映射
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
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	gpioUSART1_GPIO_RX_AF                   GPIO_AF_0
#define     	gpioUSART1_GPIO_TX_AF                   GPIO_AF_0
#endif //(boardIC_TYPE == boardIC_GD32F50X)

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
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	gpioUSART2_GPIO_RX_AF                   GPIO_AF_1
#define     	gpioUSART2_GPIO_TX_AF                   GPIO_AF_1
#endif //(boardIC_TYPE == boardIC_GD32F50X)

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
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	gpioUART3_GPIO_RX_AF                   	GPIO_AF_1
#define     	gpioUART3_GPIO_TX_AF                   	GPIO_AF_1
#endif //(boardIC_TYPE == boardIC_GD32F50X)

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
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	gpioUART4_GPIO_RX_AF                   	GPIO_AF_1
#define     	gpioUART4_GPIO_TX_AF                   	GPIO_AF_1
#endif //(boardIC_TYPE == boardIC_GD32F50X)




__STATIC_INLINE bool KEY_POWER_IsPress(void)          
{    
    if((GPIO_ISTAT(KEY_POWER_GPIO)&(KEY_POWER_PIN)) == 0)//读取按键
        return true;
    else
        return false;
}

void vGPIO_Init(void);
bool bGPIO_BootJumpApp(void);
#if(boardLOW_POWER)
void vKey_EnterLowPower(void);
void vGPIO_EnterApp(void);
void vGPIO_ExitLowPower(void);
#endif  //boardLOW_POWER
#endif
