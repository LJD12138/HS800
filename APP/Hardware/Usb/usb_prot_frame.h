/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_prot_frame.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB 快充芯片 (SW3516) 协议解析头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef USB_PROT_FRAME_H_
#define USB_PROT_FRAME_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardUSB_EN)
#include "i2c.h"

//****************************************************Extern********************************************************************//
s8 c_usb_cs_get_ic_param(const I2cObj_T *p_i2c_obj);
void vUSB_ControlPorts(bool b_open);

#endif  /* boardUSB_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* USB_PROT_FRAME_H_ */
