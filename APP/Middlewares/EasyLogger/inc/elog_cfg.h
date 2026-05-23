/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015-2016, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It is the configure head file for this library.
 * Created on: 2015-07-30
 */
 
 
#include "board_config.h"

#ifndef _ELOG_CFG_H_
#define _ELOG_CFG_H_
/*---------------------------------------------------------------------------*/
/* 启用日志输出 */
#if(boardEASY_LOGGER)
#define ELOG_OUTPUT_ENABLE
#endif

/* 设置静态输出日志级别。范围：从 ELOG_LVL_ASSERT 到 ELOG_LVL_VERBOSE */
#define ELOG_OUTPUT_LVL                         ELOG_LVL_VERBOSE
/* 启用断言检查 */
#define ELOG_ASSERT_ENABLE
/* 每行日志的缓冲区大小 */
#define ELOG_LINE_BUF_SIZE                      1024
/* 输出行号的最大长度 */
#define ELOG_LINE_NUM_MAX_LEN                   5
/* 输出过滤器的标签最大长度 */
#define ELOG_FILTER_TAG_MAX_LEN                 30
/* 输出过滤器的关键字最大长度 */
#define ELOG_FILTER_KW_MAX_LEN                  16
/* 输出过滤器的标签级别最大数量 */
#define ELOG_FILTER_TAG_LVL_MAX_NUM             5
/* 输出换行符定义 */
#define ELOG_NEWLINE_SIGN                       "\r\n"
/*---------------------------------------------------------------------------*/

/* 启用日志颜色 */
#define ELOG_COLOR_ENABLE
/* 如需自定义，可将某些级别的日志改为非默认颜色 */
#define ELOG_COLOR_ASSERT                       (F_MAGENTA B_NULL S_NORMAL)//断言(Assert)
#define ELOG_COLOR_ERROR                        (F_RED B_NULL S_NORMAL)//错误(Error)
#define ELOG_COLOR_WARN                         (F_YELLOW B_NULL S_NORMAL)//警告(Warn)
#define ELOG_COLOR_INFO                         (F_GREEN B_NULL S_NORMAL)//信息(Info)
#define ELOG_COLOR_DEBUG                        (F_GREEN B_NULL S_NORMAL)//调试(Debug)
#define ELOG_COLOR_VERBOSE                      (F_BLUE B_NULL S_NORMAL)//详细(Verbose)
/*---------------------------------------------------------------------------*/


/* 启用日志格式 */
/* 如果不希望输出这些信息，可将对应宏注释掉 */
#define ELOG_FMT_USING_FUNC     /* 输出函数名 */
#define ELOG_FMT_USING_DIR      /* 输出目录 */
#define ELOG_FMT_USING_LINE     /* 输出行号 */
/*---------------------------------------------------------------------------*/

//4.11 异步输出模式
//开启异步输出模式后，将会提升用户应用程序的执行效率。应用程序在进行日志输出时，无需等待日志彻底输出完成，即可直接返回。
//操作方法：开启、关闭ELOG_ASYNC_OUTPUT_ENABLE宏即可

//4.11.1 异步输出日志的最高级别
//日志低于或等于该级别时，才会通过异步输出。高于该级别的日志都将按照默认的同步方式输出。这样的好处是，提升了较高级别的日志输出的实时性。
//默认级别：ELOG_LVL_ASSERT ，不定义此宏，将会自动按照默认值设置
//操作方法：修改ELOG_ASYNC_OUTPUT_LVL宏对应值即可

//4.11.2 异步输出模式缓冲区大小
//默认大小：(ELOG_LINE_BUF_SIZE * 10) ，不定义此宏，将会自动按照默认值设置
//操作方法：修改ELOG_ASYNC_OUTPUT_BUF_SIZE宏对应值即可

//4.11.3 异步按行输出日志
//由于异步输出方式内部拥有缓冲区，所以直接输出缓冲区中积累的日志时，日志移植输出方法 (elog_port_output) 输出的日志
//将不会按照行日志（以换行符结尾的格式进行输出。这使得无法在移植输出方法中完成日志的分析处理。
//开启此功能后，将会最大限度保证移植输出方法每次输出的日志格式都为行日志。
//操作方法：开启、关闭ELOG_ASYNC_LINE_OUTPUT宏即可

//4.11.4 启用 pthread 库
//异步输出模式默认是使用 POSIX 的 pthread 库来实现，用户的平台如果支持 pthread ，则可以开启此宏。对于一些缺少 pthread 的支持平台，
//可以关闭此宏，参考 elog_async.c 中关于日志异步输出线程的实现方式，自己动手实现此功能。
//操作方法：开启、关闭ELOG_ASYNC_OUTPUT_USING_PTHREAD宏即可
                        
/* 启用异步输出模式 */
//#define ELOG_ASYNC_OUTPUT_ENABLE
/* 异步模式的最高输出级别，其他级别将同步输出 */
#define ELOG_ASYNC_OUTPUT_LVL                   ELOG_LVL_ASSERT
/* 异步输出模式的缓冲区大小 */
#define ELOG_ASYNC_OUTPUT_BUF_SIZE              (ELOG_LINE_BUF_SIZE * 10)
/* 异步输出的每条日志必须以换行符结尾 */
#define ELOG_ASYNC_LINE_OUTPUT
/* 异步输出模式使用 POSIX pthread 实现 */
#define ELOG_ASYNC_OUTPUT_USING_PTHREAD
/*---------------------------------------------------------------------------*/

//4.12 缓冲输出模式
//开启缓冲输出模式后，如果缓冲区不满，用户线程在进行日志输出时，无需等待日志彻底输出完成，
//即可直接返回。但当日志缓冲区满以后，将会占用用户线程，自动将缓冲区中的日志全部输出干净。
//同时用户也可以在非日志输出线程，通过定时等机制使用 void elog_flush(void) 将缓冲区中的日志输出干净。
//操作方法：开启、关闭ELOG_BUFF_OUTPUT_ENABLE宏即可

//4.12.1 缓冲输出模式缓冲区大小
//默认大小：(ELOG_LINE_BUF_SIZE * 10) ，不定义此宏，将会自动按照默认值设置
//操作方法：修改ELOG_BUF_OUTPUT_BUF_SIZE宏对应值即可

/* 启用缓冲输出模式 */
//#define ELOG_BUF_OUTPUT_ENABLE
/* 缓冲输出模式的缓冲区大小 */
#define ELOG_BUF_OUTPUT_BUF_SIZE                (ELOG_LINE_BUF_SIZE * 10)
/*---------------------------------------------------------------------------*/

#endif /* _ELOG_CFG_H_ */
