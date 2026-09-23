/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Print
 * File    : print_api.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 打印输出与日志重定向API接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef PRINT_API_H_
#define PRINT_API_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"
#include <stdarg.h>

#if (boardEASY_LOGGER)
#include "elog.h"
#endif  /* boardEASY_LOGGER */

#if (boardSEGGER)
#include "SEGGER_RTT.h"
#include "SEGGER_SYSVIEW.h"
#endif  /* boardSEGGER */

//****************************************************Macros********************************************************************//
#define			printSEGGER								boardSEGGER

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
#if (boardEASY_LOGGER == 0)
extern int (*log_e)(const char *str, ...);
extern int (*log_w)(const char *str, ...);
extern int (*log_i)(const char *str, ...);
#endif  /* boardEASY_LOGGER == 0 */

void vPrint_MyPrintParamInit(void);

int sMyPrint(const char *str, ...);
int sMyPrintErr(const char *str, ...);
int sMyPrintWarn(const char *str, ...);
int sMyPrintTips(const char *str, ...);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PRINT_API_H_ */
