/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_iface.h
 * Date    : 2026-09-24
 * Author  : LJD(291483914@qq.com)
 * Desc    : TFT 显示底层接口头文件：SPI 模式选择、引脚定义与屏幕尺寸等硬件抽象宏
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DISPLAY_IFACE_H_
#define MD_DISPLAY_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDISPLAY_EN)

//****************************************************Macros********************************************************************//
#define         dispTFT_SPI_MODE_SW                     0U
#define         dispTFT_SPI_MODE_HW                     1U
#ifndef         boardDISP_SPI_MODE
#define			boardDISP_SPI_MODE						dispTFT_SPI_MODE_HW
#endif

#define         dispTFT_WIDTH                           320U
#define         dispTFT_HEIGHT                          240U

/* LCD最大传输字节数 */
#define         dispTFT_BUF_SIZE                        (dispTFT_WIDTH * 2)

#define         dispTFT_CS_RCU                          RCU_GPIOC
#define         dispTFT_CS_PORT                         GPIOC
#define         dispTFT_CS_PIN                          GPIO_PIN_8
#define         dispTFT_CS_H()                          (GPIO_BOP(dispTFT_CS_PORT) = (uint32_t)dispTFT_CS_PIN)
#define         dispTFT_CS_L()                          (GPIO_BC(dispTFT_CS_PORT) = (uint32_t)dispTFT_CS_PIN)

#define         dispTFT_RES_RCU                         RCU_GPIOC
#define         dispTFT_RES_PORT                        GPIOC
#define         dispTFT_RES_PIN                         GPIO_PIN_7
#define         dispTFT_RES_H()                         (GPIO_BOP(dispTFT_RES_PORT) = (uint32_t)dispTFT_RES_PIN)
#define         dispTFT_RES_L()                         (GPIO_BC(dispTFT_RES_PORT) = (uint32_t)dispTFT_RES_PIN)

#define         dispTFT_BL_RCU                          RCU_GPIOC
#define         dispTFT_BL_PORT                         GPIOC
#define         dispTFT_BL_PIN                          GPIO_PIN_6
#define         dispTFT_BL_H()                          (GPIO_BOP(dispTFT_BL_PORT) = (uint32_t)dispTFT_BL_PIN)
#define         dispTFT_BL_L()                          (GPIO_BC(dispTFT_BL_PORT) = (uint32_t)dispTFT_BL_PIN)

#define         dispTFT_SDA_RCU                         RCU_GPIOB
#define         dispTFT_SDA_PORT                        GPIOB
#define         dispTFT_SDA_PIN                         GPIO_PIN_15
#define         dispTFT_SDA_H()                         (GPIO_BOP(dispTFT_SDA_PORT) = (uint32_t)dispTFT_SDA_PIN)
#define         dispTFT_SDA_L()                         (GPIO_BC(dispTFT_SDA_PORT) = (uint32_t)dispTFT_SDA_PIN)

#define         dispTFT_SCK_RCU                         RCU_GPIOB
#define         dispTFT_SCK_PORT                        GPIOB
#define         dispTFT_SCK_PIN                         GPIO_PIN_13
#define         dispTFT_SCK_H()                         (GPIO_BOP(dispTFT_SCK_PORT) = (uint32_t)dispTFT_SCK_PIN)
#define         dispTFT_SCK_L()                         (GPIO_BC(dispTFT_SCK_PORT) = (uint32_t)dispTFT_SCK_PIN)

#define         dispTFT_A0_RCU                          RCU_GPIOB
#define         dispTFT_A0_PORT                         GPIOB
#define         dispTFT_A0_PIN                          GPIO_PIN_14
#define         dispTFT_A0_H()                          (GPIO_BOP(dispTFT_A0_PORT) = (uint32_t)dispTFT_A0_PIN)
#define         dispTFT_A0_L()                          (GPIO_BC(dispTFT_A0_PORT) = (uint32_t)dispTFT_A0_PIN)

#if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
#define         dispTFT_SPI_PERIPH                      SPI1
#define         dispTFT_SPI_RCU                         RCU_SPI1
#define         dispTFT_SPI_PRESCALE                    SPI_PSC_2
#if (boardIC_TYPE == boardIC_GD32F50X)
#define         dispTFT_SPI_SCK_AF                      GPIO_AF_1
#define         dispTFT_SPI_SDA_AF                      GPIO_AF_0
#define         dispTFT_SPI_DMA_REQUEST                 DMA_REQUEST_SPI1_TX
#endif  /* boardIC_TYPE */

#define         dispTFT_DMA_PERIPH                      DMA1
#define         dispTFT_DMA_CH                          DMA_CH0
#define         dispTFT_DMA_RCU                         RCU_DMA1
#define     	dispTFT_DMA_TX_IRQ          			DMA1_Channel0_IRQn
#define     	dispTFT_DMA_TX_IRQ_HANDLER  			DMA1_Channel0_IRQHandler
#endif  /* boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW */

//****************************************************Globals*******************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
void vDisp_IfaceInit(void);
void vDisp_SpiSendByte(const u8 *data, u16 len);
void vDisp_SetBacklight(bool on);
void vDisp_WriteCommand(u8 cmd);
void vDisp_WriteData8(u8 data);
void vDisp_WriteData16(u16 data);
void vDisp_WriteBuffer(const u8 *data, u32 len);
bool bDisp_WriteColorAsync(const u8 *data, u32 len);

#endif  /* boardDISPLAY_EN */
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DISPLAY_IFACE_H_ */
