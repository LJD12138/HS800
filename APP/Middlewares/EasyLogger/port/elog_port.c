/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
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
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */
 
#include <elog.h>
#include <time.h>
#include "Print/print_task.h"
// #include "bsp_external_rtc.h"
#include "..\plugins\flash\elog_flash.h"
#include "easyflash.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"

/* 互斥信号量句柄 */
SemaphoreHandle_t eLogSemMutex = NULL;
#endif
/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

    /* add your code here */
	
	#if(boardUSE_OS)
    /* 创建互斥信号量 */
    eLogSemMutex = xSemaphoreCreateMutex();
	#endif
	
    return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {

    /* add your code here */

}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    
    /* output to terminal */
    sMyPrint("%.*s", size, log);

	#ifdef EF_USING_LOG
    /* output to flash */
	char char_temp[6] = {0};
	size_t cp_len = (size < sizeof(char_temp) - 1) ? size : sizeof(char_temp) - 1;
    memcpy(char_temp, log, cp_len);
    char_temp[cp_len] = '\0';        /* 确保字符串结束 */

	//只储存错误
	if(char_temp[0] == 0x1b &&
		char_temp[1] == 0x5b &&
		char_temp[2] == 0x33 &&
		char_temp[3] == 0x31 &&
		char_temp[4] == 0x3b)
	{
		if(ef_log_get_used_size() >= 800)
			elog_flash_clean();
		
		elog_flash_write(log, size);
	}
    #endif  //EF_USING_LOG
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
    
    /* add your code here */
	#if(boardUSE_OS)
	if(eLogSemMutex == NULL)
		return;
	
	xSemaphoreTake(eLogSemMutex, pdMS_TO_TICKS(100)); //获取
	#else
	__disable_irq();
	#endif
    
	
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
    
    /* add your code here */
	#if(boardUSE_OS)
	if(eLogSemMutex == NULL)
		return;
	
	xSemaphoreGive(eLogSemMutex); //释放互斥量
	#else
    __enable_irq();
	#endif
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {
    
    // static char buffer[80] = {0};
// 	struct tm time_info;
// 	time_info.tm_year = tExRtc.uReadTime.tTime.ucYear -1910; 	// 年份从1900年开始
// 	time_info.tm_mon = tExRtc.uReadTime.tTime.ucMonth - 1;		// 月份从0开始
// 	time_info.tm_mday = tExRtc.uReadTime.tTime.ucDay;          	// 日
// 	time_info.tm_hour = tExRtc.uReadTime.tTime.ucHour;			// 小时
// 	time_info.tm_min = tExRtc.uReadTime.tTime.ucMinute;         // 分钟
// 	time_info.tm_sec = tExRtc.uReadTime.tTime.ucSecond;         // 秒
// 	time_info.tm_isdst = -1;         // 让mktime()自动检测夏令时
// 	strftime(buffer, sizeof(buffer), "%y-%m-%d %H:%M:%S", &time_info);
// //	sMyPrint("%s \r\n", buffer);
	
    return 0;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    
    /* add your code here */
    return "pid:1008";
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    
    /* add your code here */
    return "tid:24";
}

