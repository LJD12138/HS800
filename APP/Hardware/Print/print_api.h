/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_api.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统格式化调试输出与日志打印接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
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

//****************************************************Extern********************************************************************//
void vPrint_MyPrintParamInit(void);

int sMyPrint(const char *str, ...);
int sMyPrintErr(const char *str, ...);
int sMyPrintWarn(const char *str, ...);
int sMyPrintTips(const char *str, ...);

#if (boardEASY_LOGGER == 0)
extern int (*log_e)(const char *str, ...);
extern int (*log_w)(const char *str, ...);
extern int (*log_i)(const char *str, ...);
#endif  /* boardEASY_LOGGER == 0 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PRINT_API_H_ */
