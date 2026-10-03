/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Key
 * File    : key_iface.h
 * Date    : 2026-09-24
 * Author  : LJD(291483914@qq.com)
 * Desc    : 按键硬件驱动底层接口定义头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef KEY_IFACE_H
#define KEY_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardKEY_EN)

//****************************************************Types*********************************************************************//
/* 按键ID枚举: 枚举序与硬件配置表严格一致 */
typedef enum
{
	keyPOWER = 0,		/* 电源按键 */

	#if (boardDCAC_EN)
	keyAC,				/* AC逆变按键 */
	#endif  /* boardDCAC_EN */

	#if (boardLIGHT_EN)
	keyLIGHT,			/* 照明按键 */
	#endif  /* boardLIGHT_EN */

	#if (boardUSB_EN)
	keyUSB,				/* USB按键 */
	#endif  /* boardUSB_EN */

	#if (boardDC_EN)
	keyDC,				/* DC输出按键 */
	#endif  /* boardDC_EN */

	keyNUM				/* 按键总数 */
}KeyId_E;

//****************************************************Extern********************************************************************//
void vKey_IfaceInit(void);
bool bKey_IsPressById(KeyId_E e_id);

#if (boardLOW_POWER)
void vKey_IoEnterLowPower(void);
void vKey_IoExitLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardKEY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* KEY_IFACE_H */
