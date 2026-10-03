/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Usb
 * File    : print_usb_iface.h
 * Date    : 2026-09-30
 * Author  : LJD(291483914@qq.com)
 * Desc    : CherryUSB CDC-ACM 虚拟串口硬件驱动与通信接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef PRINT_USB_IFACE_H
#define PRINT_USB_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardUSB_EN || boardPRINT_IFACE == 7)

//****************************************************Macros********************************************************************//
#define			usbCDC_IN_EP							0x81U	/* CDC Bulk IN 端点 */
#define			usbCDC_OUT_EP							0x02U	/* CDC Bulk OUT 端点 */
#define			usbCDC_INT_EP							0x83U	/* CDC Interrupt IN 端点 */

#define			usbCDC_RX_BUF_SIZE						512U	/* 接收单包最大缓冲区大小 */
#define			usbCDC_TX_BUF_SIZE						512U	/* 发送缓冲区大小 */

//****************************************************Types*********************************************************************//
typedef void (*pfnUsbCdcRxCallback_T)(const uint8_t *p_data, uint16_t us_len);

//****************************************************Globals*******************************************************************//

void     vUsbCdc_Init(void);
void     vUsbCdc_DeInit(void);
bool     bUsbCdc_IsConfigured(void);
bool     bUsbCdc_IsTxBusy(void);
bool     bUsbCdc_Send(const uint8_t *p_data, uint16_t us_len);
void     vUsbCdc_RegisterRxCallback(pfnUsbCdcRxCallback_T pfn_callback);
void     vUsbCdc_Tick(void);

#endif  /* boardUSB_EN || boardPRINT_IFACE == 7 */

#ifdef __cplusplus
}
#endif

#endif  /* PRINT_USB_IFACE_H */
