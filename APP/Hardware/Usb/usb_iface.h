#ifndef USB_IFACE_H_
#define USB_IFACE_H_

#include "board_config.h"

#if(boardUSB_EN)
#include "i2c.h"

#define     	usbIC1_SCL_RCU          				RCU_GPIOB
#define     	usbIC1_SCL_PORT     					GPIOB
#define     	usbIC1_SCL_PIN          				GPIO_PIN_6
#define     	usbIC1_SCL_OFF()        				GPIO_BC(usbIC1_SCL_PORT) = (uint32_t)usbIC1_SCL_PIN

#define     	usbIC1_SDA_RCU          				RCU_GPIOB
#define     	usbIC1_SDA_PORT     					GPIOB
#define     	usbIC1_SDA_PIN          				GPIO_PIN_7
#define     	usbIC1_SDA_OFF()        				GPIO_BC(usbIC1_SDA_PORT) = (uint32_t)usbIC1_SDA_PIN

// #define     	usbIC2_SCL_RCU          				RCU_GPIOC
// #define     	usbIC2_SCL_PORT     					GPIOC
// #define     	usbIC2_SCL_PIN          				GPIO_PIN_13
// #define     	usbIC2_SCL_OFF()        				GPIO_BC(usbIC2_SCL_PORT) = (uint32_t)usbIC2_SCL_PIN

// #define     	usbIC2_SDA_RCU          				RCU_GPIOC
// #define     	usbIC2_SDA_PORT     					GPIOC
// #define     	usbIC2_SDA_PIN          				GPIO_PIN_14
// #define     	usbIC2_SDA_OFF()        				GPIO_BC(usbIC2_SDA_PORT) = (uint32_t)usbIC2_SDA_PIN

#define     	usbPD_EN_RCU          					RCU_GPIOB
#define     	usbPD_EN_PORT     						GPIOB
#define     	usbPD_EN_PIN          					GPIO_PIN_3
#define     	usbPD_EN_ON()         					GPIO_BOP(usbPD_EN_PORT) = (uint32_t)usbPD_EN_PIN   //gpio_bit_set
#define     	usbPD_EN_OFF()        					GPIO_BC(usbPD_EN_PORT) = (uint32_t)usbPD_EN_PIN


// #define     	usbPOWER_EN_RCU       					RCU_GPIOB
// #define     	usbPOWER_EN_PORT      					GPIOB
// #define     	usbPOWER_EN_PIN       					GPIO_PIN_7
// #define     	usbPOWER_EN_ON()      					GPIO_BOP(usbPOWER_EN_PORT) = (uint32_t)usbPOWER_EN_PIN 
// #define     	usbPOWER_EN_OFF()     					GPIO_BC(usbPOWER_EN_PORT) = (uint32_t)usbPOWER_EN_PIN


extern I2cObj_T tUSB_IC1_I2C;
extern I2cObj_T tUSB_IC2_I2C;

void vUsb_IfaceInit(void);

#endif  //boardUSB_EN

#endif  //USB_IFACE_H_

