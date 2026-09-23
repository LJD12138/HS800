/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Dc
 * File    : dc_iface.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : DC 底层硬件接口驱动头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef DC_IFACE_H_
#define DC_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDC_EN)

//****************************************************Macros********************************************************************//
#define			dcPOWER_EN_RCU							RCU_GPIOA
#define			dcPOWER_EN_PORT							GPIOA
#define			dcPOWER_EN_PIN							GPIO_PIN_4
#define			dcPOWER_EN_ON()							GPIO_BOP(dcPOWER_EN_PORT) = (uint32_t)dcPOWER_EN_PIN
#define			dcPOWER_EN_OFF()						GPIO_BC(dcPOWER_EN_PORT)  = (uint32_t)dcPOWER_EN_PIN

//****************************************************Extern********************************************************************//
void vDc_IfaceInit(void);

#endif  /* boardDC_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* DC_IFACE_H_ */
