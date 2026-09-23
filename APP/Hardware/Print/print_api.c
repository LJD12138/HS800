/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_api.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统格式化调试输出与日志打印接口实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_api.h"
#include "Print/print_iface.h"   
#include "Print/print_task.h"
#include "Sys/sys_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define printREAL_TIME_OUT
#define			printLOG_BUFF_SIZE						256

//****************************************************Parameter Initialization**************************************************//
static char log_buf[printLOG_BUFF_SIZE];

#if (boardUSE_OS)
static SemaphoreHandle_t MyPrintSemaphoreMutex = NULL;

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

//****************************************************Function Declaration******************************************************//
static s8 c_print_start_check(const char *str);

/***********************************************************************************************************************
 * 函数功能    : 打印前置条件检查
 * 说明(备注)  : 检查输入有效性、初始化状态与输出通道配置
 * 传入参数    : str: 待打印字符串指针
 * 输出参数    : 无
 * 返回值      : 1: 检查通过, 负数: 错误代码
 ************************************************************************************************************************/
static s8 c_print_start_check(const char *str)
{
    /* 如果输入为空或未完成打印初始化,返回错误 */
    if (str == NULL || tSysInfo.uInit.tFinish.bIF_Print == 0) 
        return -1;
    
    /* 如果没有开启输出,返回错误 */
    if ((boardPRINT_IFACE == 0) && (boardSEGGER == 0))
        return -2;
    
    #if (boardPRINT_IFACE)
    if (tPrintTxBuff.buff == NULL)
        return -3;
    #endif  /* boardPRINT_IFACE */
    
    #if (boardUSE_OS)
    if (MyPrintSemaphoreMutex == NULL)
        return -4;
    #endif  /* boardUSE_OS */
    
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : MyPrint 互斥量初始化
 * 说明(备注)  : 创建多任务打印互斥信号量
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vPrint_MyPrintParamInit(void)
{
    #if (boardUSE_OS)
    /* 创建互斥信号量 */
    MyPrintSemaphoreMutex = xSemaphoreCreateMutex();
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 格式化标准调试输出
 * 说明(备注)  : 支持可变参数，写入环形发送缓冲区并触发USART发送
 * 传入参数    : str: 格式化字符串, ...: 可变参数列表
 * 输出参数    : 无
 * 返回值      : 打印字符个数, 负数为错误代码
 ************************************************************************************************************************/
int sMyPrint(const char *str, ...)
{
    int len = 0;
    
    s8 c_ret = c_print_start_check(str);
    if (c_ret <= 0)
        return c_ret;
    
    #if (boardUSE_OS)
    if (xSemaphoreTake(MyPrintSemaphoreMutex, pdMS_TO_TICKS(delay_value)) == pdPASS)
    #endif  /* boardUSE_OS */
    {
        va_list args;
        va_start(args, str);
        
        len = vsnprintf(NULL, 0, str, args);
        if (len < 0)
        { 
            va_end(args);
            
            #if (boardUSE_OS)
            xSemaphoreGive(MyPrintSemaphoreMutex);
            #endif  /* boardUSE_OS */
            
            return -4; 
        }   
        if (len >= printLOG_BUFF_SIZE)
        { 
            va_end(args);
            
            #if (boardUSE_OS)
            xSemaphoreGive(MyPrintSemaphoreMutex);
            #endif  /* boardUSE_OS */
            
            return -5; 
        }  
        
        va_start(args, str);
        len = vsnprintf(log_buf, printLOG_BUFF_SIZE, str, args);
        va_end(args);
        
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, log_buf, len);
        #endif  /* boardPRINT_IFACE */
        
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
 * 函数功能    : 格式化输出错误日志 (红色)
 * 说明(备注)  : ANSI 红色高亮终端输出
 * 传入参数    : str: 格式化字符串, ...: 可变参数列表
 * 输出参数    : 无
 * 返回值      : 打印字符个数, 负数为错误代码
 ************************************************************************************************************************/
int sMyPrintErr(const char *str, ...)
{
    int len = 0;
    int len_str = 0;
    
    s8 c_ret = c_print_start_check(str);
    if (c_ret <= 0)
        return c_ret;
    
    #if (boardUSE_OS)
    if (xSemaphoreTake(MyPrintSemaphoreMutex, pdMS_TO_TICKS(delay_value)) == pdPASS)
    #endif  /* boardUSE_OS */
    {
        va_list args;
        va_start(args, str);
        
        len = vsnprintf(NULL, 0, str, args);
        if (len < 0)
        { 
            va_end(args);
            
            #if (boardUSE_OS)
            xSemaphoreGive(MyPrintSemaphoreMutex);
            #endif  /* boardUSE_OS */
            
            return -4; 
        }   
        if (len >= printLOG_BUFF_SIZE)
        { 
            va_end(args);
            
            #if (boardUSE_OS)
            xSemaphoreGive(MyPrintSemaphoreMutex);
            #endif  /* boardUSE_OS */
            
            return -5; 
        }  
        
        char err1[] = "\033[31;";
        len_str = strlen(err1);
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, err1, len_str);
        #endif  /* boardPRINT_IFACE */
        
        va_start(args, str);
        len = vsnprintf(log_buf, printLOG_BUFF_SIZE, str, args);
        va_end(args);
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, log_buf, len);
        #endif  /* boardPRINT_IFACE */
        len += len_str;
        
        char err2[] = "\033[0m \r\n";
        len_str = strlen(err2);
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, err2, len_str);
        #endif  /* boardPRINT_IFACE */
        len += len_str;
        
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
 * 函数功能    : 格式化输出警告日志 (黄色)
 * 说明(备注)  : ANSI 黄色高亮终端输出
 * 传入参数    : str: 格式化字符串, ...: 可变参数列表
 * 输出参数    : 无
 * 返回值      : 打印字符个数, 负数为错误代码
 ************************************************************************************************************************/
int sMyPrintWarn(const char *str, ...)
{
    int len = 0;
    int len_str = 0;
    
    s8 c_ret = c_print_start_check(str);
    if (c_ret <= 0)
        return c_ret;
    
    #if (boardUSE_OS)
    if (xSemaphoreTake(MyPrintSemaphoreMutex, pdMS_TO_TICKS(delay_value)) == pdPASS)
    #endif  /* boardUSE_OS */
    {
        va_list args;
        va_start(args, str);
        
        len = vsnprintf(NULL, 0, str, args);
        if (len < 0)
        { 
            va_end(args);
            
            #if (boardUSE_OS)
            xSemaphoreGive(MyPrintSemaphoreMutex);
            #endif  /* boardUSE_OS */
            
            return -4; 
        }   
        if (len >= printLOG_BUFF_SIZE)
        { 
            va_end(args);
            
            #if (boardUSE_OS)
            xSemaphoreGive(MyPrintSemaphoreMutex);
            #endif  /* boardUSE_OS */
            
            return -5; 
        }  
        
        char err1[] = "\033[33;";
        len_str = strlen(err1);
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, err1, len_str);
        #endif  /* boardPRINT_IFACE */
        
        va_start(args, str);
        len = vsnprintf(log_buf, printLOG_BUFF_SIZE, str, args);
        va_end(args);
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, log_buf, len);
        #endif  /* boardPRINT_IFACE */
        len += len_str;
        
        char err2[] = "\033[0m \r\n";
        len_str = strlen(err2);
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, err2, len_str);
        #endif  /* boardPRINT_IFACE */
        len += len_str;
        
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
 * 函数功能    : 格式化输出提示日志 (绿色)
 * 说明(备注)  : ANSI 绿色高亮终端输出
 * 传入参数    : str: 格式化字符串, ...: 可变参数列表
 * 输出参数    : 无
 * 返回值      : 打印字符个数, 负数为错误代码
 ************************************************************************************************************************/
int sMyPrintTips(const char *str, ...)
{
    int len = 0;
    int len_str = 0;
    
    s8 c_ret = c_print_start_check(str);
    if (c_ret <= 0)
        return c_ret;
    
    #if (boardUSE_OS)
    if (xSemaphoreTake(MyPrintSemaphoreMutex, pdMS_TO_TICKS(delay_value)) == pdPASS)
    #endif  /* boardUSE_OS */
    {
        va_list args;
        va_start(args, str);
        
        len = vsnprintf(NULL, 0, str, args);
        if (len < 0)
        { 
            va_end(args);
            
            #if (boardUSE_OS)
            xSemaphoreGive(MyPrintSemaphoreMutex);
            #endif  /* boardUSE_OS */
            
            return -4; 
        }   
        if (len >= printLOG_BUFF_SIZE)
        { 
            va_end(args);
            
            #if (boardUSE_OS)
            xSemaphoreGive(MyPrintSemaphoreMutex);
            #endif  /* boardUSE_OS */
            
            return -5; 
        }  
        
        char err1[] = "\033[32;";
        len_str = strlen(err1);
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, err1, len_str);
        #endif  /* boardPRINT_IFACE */
        
        va_start(args, str);
        len = vsnprintf(log_buf, printLOG_BUFF_SIZE, str, args);
        va_end(args);
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, log_buf, len);
        #endif  /* boardPRINT_IFACE */
        len += len_str;
        
        char err2[] = "\033[0m \r\n";
        len_str = strlen(err2);
        #if (boardPRINT_IFACE)
        lwrb_write(&tPrintTxBuff, err2, len_str);
        #endif  /* boardPRINT_IFACE */
        len += len_str;
        
        #if (boardUSE_OS)
        xSemaphoreGive(MyPrintSemaphoreMutex); 
        #endif  /* boardUSE_OS */
        
        #if (boardPRINT_IFACE)
        bPrint_SendDataToUsart();
        #endif  /* boardPRINT_IFACE */
    }

    return len;
}
