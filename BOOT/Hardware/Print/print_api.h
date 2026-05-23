#ifndef PRINT_H_
#define PRINT_H_

#include "board_config.h"

#include "main.h"
#include <stdarg.h>

#if(boardEASY_LOGGER)
#include "elog.h"
#endif  //boardEASY_LOGGER

#if(boardSEGGER)
#include "SEGGER_RTT.h"
#include "SEGGER_SYSVIEW.h"
#endif  //boardSEGGER

#define 		printSEGGER    							boardSEGGER

void vPrint_MyPrintParamInit(void);

int sMyPrint(const char *str, ...);
int sMyPrintErr(const char* str, ...);
int sMyPrintWarn(const char* str, ...);
int sMyPrintTips(const char* str, ...);

#if(boardEASY_LOGGER == 0)
extern int (*log_e)(const char *str, ...);
extern int (*log_w)(const char *str, ...);
extern int (*log_i)(const char *str, ...);
#endif

#endif  //PRINT_H_


