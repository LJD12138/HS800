/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Print
 * File    : print_api.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 打印输出与日志重定向API接口实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_api.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  /* boardUSE_OS */

#include <stddef.h>
#include <string.h>
#include <stdio.h>

//****************************************************Macros********************************************************************//
/* 开启实时Print输出,则有部分打印会丢失,反之会因为等待输出而影响输出任务的实时性.建议开启此定义 */
#define printREAL_TIME_OUT
#define			printLOG_BUFF_SIZE						256

//****************************************************Parameter Initialization**************************************************//
static char log_buf[printLOG_BUFF_SIZE];

#if (boardUSE_OS)
/* 创建信号量句柄 */
SemaphoreHandle_t MyPrintSemaphoreMutex = NULL;

#ifdef printREAL_TIME_OUT
const int delay_value = 0;
#else
const int delay_value = 100;
#endif  /* printREAL_TIME_OUT */
#endif  /* boardUSE_OS */

#if (boardEASY_LOGGER == 0)
int (*log_e)(const char *fmt, ...) = sMyPrintErr;
int (*log_w)(const char *fmt, ...) = sMyPrintWarn;
int (*log_i)(const char *fmt, ...) = sMyPrintTips;
#endif  /* boardEASY_LOGGER == 0 */


/***********************************************************************************************************************
 * 函数功能    : 打印前置条件检查
 * 说明(备注)  : none
 * 传入参数    : str: 待格式化字符串指针
 * 输出参数    : none
 * 返回值      : 1: 正常, 负数: 错误码
 ************************************************************************************************************************/
s8 c_print_start_check(const char *str)
{
	/* 如果输入为空,返回错误 */
	if (str == NULL || tSysInfo.uInit.tFinish.bIF_Print == 0) 
		return -1;
	
	/* 如果没有开启输出,返回错误 */
	if ((boardPRINT_IFACE == 0) && (boardSEGGER == 0))
		return -2;
	
	#if (boardPRINT_IFACE)
	if (tPrintTxBuff.buff == NULL)
		return -3;
	
	/* 开启传输就关闭打印 */
	if (tUpdate.eChType == CT_PRINT && tpSysTask->ucID == STI_UPDATE) 
		return -5;
	#endif  /* boardPRINT_IFACE */
	
	#if (boardUSE_OS)
	if (MyPrintSemaphoreMutex == NULL)
		return -4;
	#endif  /* boardUSE_OS */
	
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 内部格式化并输出统一处理函数
 * 说明(备注)  : none
 * 传入参数    : prefix: 前缀（如 ANSI 颜色码）, fmt: 格式化字符串, args: 参数列表, suffix: 后缀
 * 输出参数    : none
 * 返回值      : 打印字符总长度, 负数为错误码
 ************************************************************************************************************************/
static int s_print_format_and_send(const char *prefix, const char *fmt, va_list args, const char *suffix)
{
	int len = 0;
	s8 c_ret = c_print_start_check(fmt);
	if (c_ret <= 0)
		return c_ret;

	#if (boardUSE_OS)
	if (xSemaphoreTake(MyPrintSemaphoreMutex, pdMS_TO_TICKS(delay_value)) == pdPASS)
	#endif  /* boardUSE_OS */
	{
		if (prefix != NULL)
		{
			int prefix_len = (int)strlen(prefix);
			#if (boardPRINT_IFACE)
			lwrb_write(&tPrintTxBuff, prefix, prefix_len);
			#endif  /* boardPRINT_IFACE */
			len += prefix_len;
		}

		int body_len = vsnprintf(log_buf, sizeof(log_buf), fmt, args);
		if (body_len > 0)
		{
			if (body_len >= (int)sizeof(log_buf))
				body_len = (int)sizeof(log_buf) - 1;
			#if (boardPRINT_IFACE)
			lwrb_write(&tPrintTxBuff, log_buf, body_len);
			#endif  /* boardPRINT_IFACE */
			len += body_len;
		}

		if (suffix != NULL)
		{
			int suffix_len = (int)strlen(suffix);
			#if (boardPRINT_IFACE)
			lwrb_write(&tPrintTxBuff, suffix, suffix_len);
			#endif  /* boardPRINT_IFACE */
			len += suffix_len;
		}

		#if (boardUSE_OS)
		xSemaphoreGive(MyPrintSemaphoreMutex);
		#endif  /* boardUSE_OS */

		#if (boardPRINT_IFACE)
		bPrint_SendDataToUsart();
		#endif  /* boardPRINT_IFACE */
	}

	return len;
}

/***********************************************************************************************************************
 * 函数功能    : MyPrint函数参数初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vPrint_MyPrintParamInit(void)
{
	#if (boardUSE_OS)
	/* 创建互斥信号量 */
	MyPrintSemaphoreMutex = xSemaphoreCreateMutex();
	#endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 封装的普通输出函数
 * 说明(备注)  : none
 * 传入参数    : str: 格式化字符串及不定参数
 * 输出参数    : none
 * 返回值      : 打印字符数, 负数为错误
 ************************************************************************************************************************/
int sMyPrint(const char *str, ...)
{
	va_list args;
	va_start(args, str);
	int len = s_print_format_and_send(NULL, str, args, NULL);
	va_end(args);
	return len;
}

/***********************************************************************************************************************
 * 函数功能    : 封装的输出错误函数（红色）
 * 说明(备注)  : none
 * 传入参数    : str: 格式化字符串及不定参数
 * 输出参数    : none
 * 返回值      : 打印字符数, 负数为错误
 ************************************************************************************************************************/
int sMyPrintErr(const char *str, ...)
{
	va_list args;
	va_start(args, str);
	int len = s_print_format_and_send("\033[31;1m", str, args, "\033[0m\r\n");
	va_end(args);
	return len;
}

/***********************************************************************************************************************
 * 函数功能    : 封装的输出警告函数（黄色）
 * 说明(备注)  : none
 * 传入参数    : str: 格式化字符串及不定参数
 * 输出参数    : none
 * 返回值      : 打印字符数, 负数为错误
 ************************************************************************************************************************/
int sMyPrintWarn(const char *str, ...)
{
	va_list args;
	va_start(args, str);
	int len = s_print_format_and_send("\033[33;1m", str, args, "\033[0m\r\n");
	va_end(args);
	return len;
}

/***********************************************************************************************************************
 * 函数功能    : 封装的输出提示函数（绿色）
 * 说明(备注)  : none
 * 传入参数    : str: 格式化字符串及不定参数
 * 输出参数    : none
 * 返回值      : 打印字符数, 负数为错误
 ************************************************************************************************************************/
int sMyPrintTips(const char *str, ...)
{
	va_list args;
	va_start(args, str);
	int len = s_print_format_and_send("\033[32;1m", str, args, "\033[0m\r\n");
	va_end(args);
	return len;
}
