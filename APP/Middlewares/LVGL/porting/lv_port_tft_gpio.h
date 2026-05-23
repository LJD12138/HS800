
#ifndef LV_PORT_TFT_GPIO_H_
#define LV_PORT_TFT_GPIO_H_

#include "board_config.h"

#if(boardDISPLAY)

#include "main.h"

//-------------------------------------触摸端口定义------------------------------------------------------------------ 
#define tftTOUCH_SPI_SELECT     2     //触摸SPI选择  0:关闭触摸   1:软件SPI     2:硬件SPI

//PD8   INT
#define			tftTOUCH_INT_GPIO_RCU()					__HAL_RCC_GPIOD_CLK_ENABLE()
#define			tftTOUCH_INT_PORT						GPIOD
#define			tftTOUCH_INT_PIN						GPIO_PIN_8
#define			tftREAD_INT_GPIO     					HAL_GPIO_ReadPin(tftTOUCH_INT_PORT, tftTOUCH_INT_PIN) 

//PB14  MISO
#define			tftTOUCH_MISO_GPIO_RCU()				__HAL_RCC_GPIOB_CLK_ENABLE()
#define			tftTOUCH_MISO_PORT						GPIOB
#define			tftTOUCH_MISO_PIN						GPIO_PIN_14
#define			tftREAD_MISO_GPIO    					HAL_GPIO_ReadPin(tftTOUCH_MISO_PORT, tftTOUCH_MISO_PIN) 	   
   					 
//PB15  MOSI
#define			tftTOUCH_MOSI_GPIO_RCU()				__HAL_RCC_GPIOB_CLK_ENABLE()
#define			tftTOUCH_MOSI_PORT						GPIOB
#define			tftTOUCH_MOSI_PIN						GPIO_PIN_15
#define			tftSET_MOSI_GPIO(x)  					HAL_GPIO_WritePin(tftTOUCH_MOSI_PORT, tftTOUCH_MOSI_PIN, (GPIO_PinState)x)

//PB13  SCLK
#define			tftTOUCH_SCLK_GPIO_RCU()				__HAL_RCC_GPIOB_CLK_ENABLE()
#define			tftTOUCH_SCLK_PORT						GPIOB
#define			tftTOUCH_SCLK_PIN						GPIO_PIN_13
#define			tftSET_SCLK_GPIO(x)  					HAL_GPIO_WritePin(tftTOUCH_SCLK_PORT, tftTOUCH_SCLK_PIN, (GPIO_PinState)x)

//PB12  CS
#define			tftTOUCH_CS_GPIO_RCU()					__HAL_RCC_GPIOB_CLK_ENABLE()
#define			tftTOUCH_CS_PORT						GPIOB
#define			tftTOUCH_CS_PIN							GPIO_PIN_12
#define			tftSET_TCS_GPIO(x)   					HAL_GPIO_WritePin(tftTOUCH_CS_PORT, tftTOUCH_CS_PIN, (GPIO_PinState)x)  

extern SPI_HandleTypeDef  hspi2;

//-------------------------------------显示端口定义------------------------------------------------------------------ 

#define			tftDISP_SPI_SELECT     					2     		   //触摸SPI选择  0:关闭显示   1:硬件SPI     2:硬件SPI+DMA

#define			LCD_RST_PORT   							GPIOB          //
#define			LCD_RST_PIN    							GPIO_PIN_11    //
#define			LCD_BL_PORT   	 						GPIOB          //
#define			LCD_BL_PIN     							GPIO_PIN_10    //
#define			LCD_DC_PORT    							GPIOE          //
#define			LCD_DC_PIN     							GPIO_PIN_15    //

#define			tftSET_RST_GPIO(n)  					HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, (GPIO_PinState)n)
#define			tftSET_BL_GPIO(n)   					HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN,  (GPIO_PinState)n)
#define			tftSET_DC_GPIO(n)   					HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN,  (GPIO_PinState)n)

extern SPI_HandleTypeDef  hspi4;
extern DMA_HandleTypeDef  hdma_spi4_tx;

void MX_SPI4_Init(void);
void MX_SPI4_DMA_Init(void);
void vTFT_TouchSpInit(void);
//-------------------------------------

#endif  //boardDISPLAY

#endif  //LV_PORT_TFT_GPIO_H_

