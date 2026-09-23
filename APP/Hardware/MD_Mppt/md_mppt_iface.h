/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_iface.h
 * Date    : 2026-09-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 串口底层硬件接口与控制定义头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_MPPT_IFACE_H_
#define MD_MPPT_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardMPPT_IFACE)

//****************************************************Macros********************************************************************//
#define			mpptGPIO_DC_EN_RCU              		RCU_GPIOA
#define			mpptGPIO_DC_EN_PORT             		GPIOA
#define			mpptGPIO_DC_EN_PIN              		GPIO_PIN_5
#define			mpptGPIO_DC_EN_ON()             		GPIO_BOP(mpptGPIO_DC_EN_PORT) = mpptGPIO_DC_EN_PIN	/* 使能输出 */
#define			mpptGPIO_DC_EN_OFF()            		GPIO_BC(mpptGPIO_DC_EN_PORT)  = mpptGPIO_DC_EN_PIN	/* 关闭输出 */

#define			mpptGPIO_XT60_EN_RCU            		RCU_GPIOB
#define			mpptGPIO_XT60_EN_PORT           		GPIOB
#define			mpptGPIO_XT60_EN_PIN            		GPIO_PIN_8
#define			mpptGPIO_XT60_EN_ON()           		GPIO_BOP(mpptGPIO_XT60_EN_PORT) = mpptGPIO_XT60_EN_PIN	/* 使能XT60输出 */
#define			mpptGPIO_XT60_EN_OFF()          		GPIO_BC(mpptGPIO_XT60_EN_PORT)  = mpptGPIO_XT60_EN_PIN	/* 关闭XT60输出 */

//****************************************************Globals*******************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
void vMppt_IfaceInit(void);
void vMppt_IfaceDeInit(void);
bool bMppt_DataSendStart(uint8_t *p_data, uint16_t us_len);

#if (boardMPPT_485_IFACE_EN)
void vMppt_485TransEnable(bool b_en);
#endif  /* boardMPPT_485_IFACE_EN */

#endif  /* boardMPPT_IFACE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_MPPT_IFACE_H_ */
