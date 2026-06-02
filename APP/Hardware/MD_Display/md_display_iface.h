#ifndef MD_DISPLAY_IFACE_H_
#define MD_DISPLAY_IFACE_H_

#include "main.h"
#include "board_config.h"

#if(boardDISPLAY_EN)

#define         dispTFT_SPI_MODE_SW                     0U
#define         dispTFT_SPI_MODE_HW                     1U
#ifndef         boardDISP_SPI_MODE
#define         boardDISP_SPI_MODE                     dispTFT_SPI_MODE_HW
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
#define         dispTFT_DMA_PERIPH                      DMA0
#define         dispTFT_DMA_CH                          DMA_CH4
#define         dispTFT_DMA_RCU                         RCU_DMA0
#define     	dispTFT_DMA_TX_IRQ          			DMA0_Channel4_IRQn
#define     	dispTFT_DMA_TX_IRQ_HANDLER  			DMA0_Channel4_IRQHandler
#endif

void vDisp_IfaceInit(void);
void vDisp_SpiSendByte(const u8 *data, u16 len);
void vDisp_TftSetBacklight(bool on);
void vDisp_TftWriteCommand(u8 cmd);
void vDisp_TftWriteData8(u8 data);
void vDisp_TftWriteData16(u16 data);
void vDisp_TftWriteBuffer(const u8 *data, u32 len);
void vDisp_TftWriteColorAsync(const u8 *data, u32 len);

#endif  /*boardDISPLAY_EN*/
#endif  //MD_DISPLAY_IFACE_H_
