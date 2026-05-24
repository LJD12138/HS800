#ifndef MD_DISPLAY_IFACE_H_
#define MD_DISPLAY_IFACE_H_

#include "main.h"
#include "board_config.h"

#if(boardDISPLAY_EN)

#define DISP_TFT_SPI_MODE_SW               0U
#define DISP_TFT_SPI_MODE_HW               1U
#ifndef boardDISP_SPI_MODE
#define boardDISP_SPI_MODE                 DISP_TFT_SPI_MODE_HW
#endif

#define DISP_TFT_WIDTH                     240U
#define DISP_TFT_HEIGHT                    320U

#define DISP_TFT_CS_RCU                    RCU_GPIOC
#define DISP_TFT_CS_PORT                   GPIOC
#define DISP_TFT_CS_PIN                    GPIO_PIN_8
#define DISP_TFT_CS_H()                    (GPIO_BOP(DISP_TFT_CS_PORT) = (uint32_t)DISP_TFT_CS_PIN)
#define DISP_TFT_CS_L()                    (GPIO_BC(DISP_TFT_CS_PORT) = (uint32_t)DISP_TFT_CS_PIN)

#define DISP_TFT_RES_RCU                   RCU_GPIOC
#define DISP_TFT_RES_PORT                  GPIOC
#define DISP_TFT_RES_PIN                   GPIO_PIN_7
#define DISP_TFT_RES_H()                   (GPIO_BOP(DISP_TFT_RES_PORT) = (uint32_t)DISP_TFT_RES_PIN)
#define DISP_TFT_RES_L()                   (GPIO_BC(DISP_TFT_RES_PORT) = (uint32_t)DISP_TFT_RES_PIN)

#define DISP_TFT_BL_RCU                    RCU_GPIOC
#define DISP_TFT_BL_PORT                   GPIOC
#define DISP_TFT_BL_PIN                    GPIO_PIN_6
#define DISP_TFT_BL_H()                    (GPIO_BOP(DISP_TFT_BL_PORT) = (uint32_t)DISP_TFT_BL_PIN)
#define DISP_TFT_BL_L()                    (GPIO_BC(DISP_TFT_BL_PORT) = (uint32_t)DISP_TFT_BL_PIN)

#define DISP_TFT_SDA_RCU                   RCU_GPIOB
#define DISP_TFT_SDA_PORT                  GPIOB
#define DISP_TFT_SDA_PIN                   GPIO_PIN_15
#define DISP_TFT_SDA_H()                   (GPIO_BOP(DISP_TFT_SDA_PORT) = (uint32_t)DISP_TFT_SDA_PIN)
#define DISP_TFT_SDA_L()                   (GPIO_BC(DISP_TFT_SDA_PORT) = (uint32_t)DISP_TFT_SDA_PIN)

#define DISP_TFT_SCK_RCU                   RCU_GPIOB
#define DISP_TFT_SCK_PORT                  GPIOB
#define DISP_TFT_SCK_PIN                   GPIO_PIN_13
#define DISP_TFT_SCK_H()                   (GPIO_BOP(DISP_TFT_SCK_PORT) = (uint32_t)DISP_TFT_SCK_PIN)
#define DISP_TFT_SCK_L()                   (GPIO_BC(DISP_TFT_SCK_PORT) = (uint32_t)DISP_TFT_SCK_PIN)

#define DISP_TFT_A0_RCU                    RCU_GPIOB
#define DISP_TFT_A0_PORT                   GPIOB
#define DISP_TFT_A0_PIN                    GPIO_PIN_14
#define DISP_TFT_A0_H()                    (GPIO_BOP(DISP_TFT_A0_PORT) = (uint32_t)DISP_TFT_A0_PIN)
#define DISP_TFT_A0_L()                    (GPIO_BC(DISP_TFT_A0_PORT) = (uint32_t)DISP_TFT_A0_PIN)

#if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
#define DISP_TFT_SPI_PERIPH                SPI1
#define DISP_TFT_SPI_RCU                   RCU_SPI1
#define DISP_TFT_SPI_PRESCALE              SPI_PSC_8
#endif

void vDisp_IfaceInit(void);
void vDisp_SpiSendByte(const u8 *data, u16 len);
void vDisp_TftSetBacklight(bool on);
void vDisp_TftWriteCommand(u8 cmd);
void vDisp_TftWriteData8(u8 data);
void vDisp_TftWriteData16(u16 data);
void vDisp_TftWriteBuffer(const u8 *data, u32 len);
void vDisp_TftSetWindow(u16 x1, u16 y1, u16 x2, u16 y2);
void vDisp_TftFillRect(u16 x, u16 y, u16 w, u16 h, u16 color);
void vDisp_TftDrawBitmapRgb565(u16 x1, u16 y1, u16 x2, u16 y2, const u16 *color_p);

#endif

#endif
