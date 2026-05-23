#ifndef USB_PROT_FRAME_H_
#define USB_PROT_FRAME_H_

#include "board_config.h"

#if(boardUSB_EN)
#include "main.h"
#include "i2c.h"
// #include "Modbus/modbus_proto.h"


// extern			ModbusProtoTx_t 						*tpUsbProtoTx;
// extern 			ModbusProtoRx_t 						*tpUsbProtoRx;

s8 c_usb_cs_get_ic_param(const I2cObj_T *p_i2c_obj);
s8 c_usb_set_pwr_cs(u16 pwr);

bool bUsb_SendProtInit(void);
bool bUsb_RecProtInit(void);

#endif  //boardUSB_EN

#endif  //USB_PROT_FRAME_H_
