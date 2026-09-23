/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application
 * File    : systick.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统滴答定时器(SysTick)配置与毫秒级延时/定时标志管理头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef SYS_TICK_H_
#define SYS_TICK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (1)
//****************************************************Macros********************************************************************//

//****************************************************Globals*******************************************************************//
#if (boardBMS_485_IFACE_EN)
extern __IO bool bSysTick_BmsSendFinish;
#endif  /* boardBMS_485_IFACE_EN */

#if (boardPRINT_485_IFACE_EN)
extern __IO bool bSysTick_PrintSendFinish;
#endif  /* boardPRINT_485_IFACE_EN */

extern __IO bool bSystick_10MsFlag;
extern __IO bool bSystick_100MsFlag;

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
void vSys_TickConfig(void);
void vSys_MsDelay(uint32_t cnt);
void vSys_Tick(void);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* SYS_TICK_H_ */
