#ifndef MD_DISPLAY_IFACE_H_
#define MD_DISPLAY_IFACE_H_

#include "main.h"
#include "board_config.h"

#if(boardDISPLAY_EN)

// OLED SPI 引脚定义
//OLED SPI引脚映射, 高低电平宏直接写GPIO置位/复位寄存器
#define     	dispOLED_NSS_RCU              			RCU_GPIOB
#define     	dispOLED_NSS_PORT             			GPIOB
#define     	dispOLED_NSS_PIN              			GPIO_PIN_12
#define     	dispOLED_NSS_H()              			GPIO_BOP(dispOLED_NSS_PORT) = (uint32_t)dispOLED_NSS_PIN
#define     	dispOLED_NSS_L()              			GPIO_BC(dispOLED_NSS_PORT) = (uint32_t)dispOLED_NSS_PIN

#define     	dispOLED_SCK_RCU              			RCU_GPIOB
#define     	dispOLED_SCK_PORT             			GPIOB
#define     	dispOLED_SCK_PIN              			GPIO_PIN_13
#define     	dispOLED_SCK_H()              			GPIO_BOP(dispOLED_SCK_PORT) = (uint32_t)dispOLED_SCK_PIN
#define     	dispOLED_SCK_L()              			GPIO_BC(dispOLED_SCK_PORT) = (uint32_t)dispOLED_SCK_PIN

#define     	dispOLED_MOSI_RCU             			RCU_GPIOB
#define     	dispOLED_MOSI_PORT            			GPIOB
#define     	dispOLED_MOSI_PIN             			GPIO_PIN_15
#define     	dispOLED_MOSI_H()             			GPIO_BOP(dispOLED_MOSI_PORT) = (uint32_t)dispOLED_MOSI_PIN
#define     	dispOLED_MOSI_L()             			GPIO_BC(dispOLED_MOSI_PORT) = (uint32_t)dispOLED_MOSI_PIN

#define     	dispOLED_RES_RCU              			RCU_GPIOA
#define     	dispOLED_RES_PORT             			GPIOA
#define     	dispOLED_RES_PIN              			GPIO_PIN_11
#define     	dispOLED_RES_H()              			GPIO_BOP(dispOLED_RES_PORT) = (uint32_t)dispOLED_RES_PIN
#define     	dispOLED_RES_L()              			GPIO_BC(dispOLED_RES_PORT) = (uint32_t)dispOLED_RES_PIN

#define     	dispOLED_DC_RCU               			RCU_GPIOA
#define     	dispOLED_DC_PORT              			GPIOA
#define     	dispOLED_DC_PIN               			GPIO_PIN_12
#define     	dispOLED_DC_H()               			GPIO_BOP(dispOLED_DC_PORT) = (uint32_t)dispOLED_DC_PIN
#define     	dispOLED_DC_L()               			GPIO_BC(dispOLED_DC_PORT) = (uint32_t)dispOLED_DC_PIN


//硬件SPI DMA配置; boardDISP_SPI_MODE为0时使用软件模拟SPI
#if(boardDISP_SPI_MODE == 1)
#define     	dispOLED_SPI_PERIPH           			SPI1
#define     	dispOLED_SPI_RCU              			RCU_SPI1
#define     	dispOLED_SPI_DMA_PERIPH       			DMA0
#define     	dispOLED_SPI_DMA_RCU          			RCU_DMA0
#define     	dispOLED_SPI_DMA_TX_CH        			DMA_CH4
#define     	dispOLED_SPI_PRESCALE         			SPI_PSC_16
#endif  //boardDISP_SPI_MODE

void vDisp_IfaceInit(void);
void vDisp_SpiSendByte(const u8 *data, u16 len);
void vDisp_OledWriteByte(const u8 *data, u16 len, u8 mode);

#endif  // boardDISPLAY_EN

#endif  // MD_DISPLAY_IFACE_H_
