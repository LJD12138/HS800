/*
 * This file is part of the EasyFlash Library.
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
 * Function: Portable interface for stm32f10x platform.
 * Created on: 2015-01-16
 */

#include <easyflash.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "board_config.h"

#if(boardEASY_FLASH)
#include "Print/print_task.h"
#include "boot_info.h"

#if(boardIC_TYPE == boardIC_GD32F30X)
#include "Flash/flash_gd32.h"
#elif(boardIC_TYPE == boardIC_STM32H7XX)
#if(boardUSE_SFUD)
#include "Flash/flash_iface.h"
#else
#include "Flash/flash_stm32.h"
#endif  //boardUSE_SFUD
#elif(boardIC_TYPE == boardIC_STM32G4XX)
#endif  //boardIC_TYPE

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t 	EasyFlashSemaphoreMutex = NULL;
#endif  //boardUSE_OS

char log_buf[256];

#endif  //boardEASY_FLASH


/**
 * Flash port for hardware initialize.
 * 当 flash 第一次初始化时会将默认的环境变量写入
 * @param default_env default ENV set for user
 * @param default_env_size default ENV size
 *
 * @return result
 */
EfErrCode ef_port_init(ef_env const **default_env, size_t *default_env_size) {
    EfErrCode result = EF_NO_ERR;
	
	#if(boardEASY_FLASH)
	#if(boardUSE_OS)
	/* 创建互斥信号量 */
	EasyFlashSemaphoreMutex = xSemaphoreCreateMutex();
	#endif  //boardUSE_OS
	
	if(cBoot_MemParamInit(tBootMemParamStr) <= 0)
		result = EF_ENV_INIT_FAILED;

    *default_env = default_env_set;
    *default_env_size = usBoot_GetMemParamSize();
	#endif  //boardEASY_FLASH
	
    return result;
}

/**
 * Read data from flash.
 * @note This operation's units is word.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, size_t size) {
    EfErrCode result = EF_NO_ERR;
	
	#if(boardEASY_FLASH)
	#if(boardIC_TYPE == boardIC_GD32F30X)
	vFlash_Gd32Read32Bit(addr, buf, size);
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	#if(boardUSE_SFUD)
//	if(cFlash_Read8BitData(addr, (u8*)buf, size) <= 0)
//		return EF_ENV_FULL;
//	if(sfud_read(flash, addr, size, (u8*)buf) != SFUD_SUCCESS)
//		return EF_ENV_FULL;
	#else
	
	#endif  //boardUSE_SFUD
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#endif  //boardIC_TYPE
	#endif  //boardEASY_FLASH
	
    return result;
}

/**
 * Erase data on flash.
 * @note This operation is irreversible.
 * @note This operation's units is different which on many chips.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
EfErrCode ef_port_erase(uint32_t addr, size_t size) {
    EfErrCode result = EF_NO_ERR;

    /* make sure the start address is a multiple of FLASH_ERASE_MIN_SIZE */
    EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);
	
	#if(boardEASY_FLASH)
	#if(boardIC_TYPE == boardIC_GD32F30X)
	if(bFlash_Gd32EraseSector(addr, addr + size) == false){
        result = EF_ERASE_ERR;
    }
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	#if(boardUSE_SFUD)
//	if(cFlash_EraseSector(addr, addr + size) <= 0)
//		return EF_ENV_FULL;
	
//	if(sfud_erase(flash,addr, addr + size) != SFUD_SUCCESS)
//		return EF_ENV_FULL;
	#else
	
	#endif  //boardUSE_SFUD
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#endif
	#endif  //boardEASY_FLASH
	
    return result;
}

/**
 * Write data to flash.
 * @note This operation's units is word.
 * @note This operation must after erase. @see flash_erase.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */
EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, size_t size) {
    EfErrCode result = EF_NO_ERR;
	
	__ALIGNED(4) u32 buff[size];
	memcpy(buff, buf, size);
	
	#if(boardEASY_FLASH)
	#if(boardIC_TYPE == boardIC_GD32F30X)
	if(bFlash_Gd32Write32Bit(addr, buf, size) == false){
        result = EF_WRITE_ERR;
    }
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	#if(boardUSE_SFUD)
//	if(cFlash_Write8BitData(addr, (u8*)buf, size) <= 0)
//		return EF_ENV_FULL;
//	if(sfud_write(flash, addr, size, (u8*)&buff) != SFUD_SUCCESS)
//		return EF_ENV_FULL;
	#else
	
	#endif  //boardUSE_SFUD
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#endif
	#endif  //boardEASY_FLASH
	
    return result;
}

/**
 * lock the ENV ram cache
 */
void ef_port_env_lock(void) {
	#if(boardEASY_FLASH)
	#if(boardUSE_OS)
	if(EasyFlashSemaphoreMutex == NULL)
		return;
	
	xSemaphoreTake(EasyFlashSemaphoreMutex, pdMS_TO_TICKS(portMAX_DELAY));
	#else
	__disable_irq();
	#endif  //boardUSE_OS
	#endif  //boardEASY_FLASH
}

/**
 * unlock the ENV ram cache
 */
void ef_port_env_unlock(void) {
	#if(boardEASY_FLASH)
	#if(boardUSE_OS)
	if(EasyFlashSemaphoreMutex == NULL)
		return;
	
	xSemaphoreGive(EasyFlashSemaphoreMutex); 
	#else
    __enable_irq();
	#endif  //boardUSE_OS
	#endif  //boardEASY_FLASH
}


/**
 * This function is print flash debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 *
 */
void ef_log_debug(const char *file, const long line, const char *format, ...) {

#ifdef PRINT_DEBUG
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[Flash](%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
    printf("%s", log_buf);
    va_end(args);

#endif

}

/**
 * This function is print flash routine info.
 * 打印普通日志信息
 * @param format output format
 * @param ... args
 */
void ef_log_info(const char *format, ...) 
{
	//局部数组过大,导致栈溢出
//	char log_buf[256];
	
	#if(boardEASY_FLASH)
    va_list args;
    va_start(args, format);

    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
	
    va_end(args);
	sMyPrint("%s", log_buf);
	#endif  //boardEASY_FLASH
}
/**
 * This function is print flash non-package info.
 * 该方法输出无固定格式的打印信息，为ef_print_env方法所用（如果不使用ef_print_env则可以忽略）。
 * 而ef_log_debug及ef_log_info可以输出带指定前缀及格式的打印日志信息。
 * @param format output format
 * @param ... args
 */
void ef_print(const char *format, ...) 
{
	#if(boardEASY_FLASH)
	va_list args;
    va_start(args, format);

    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
	
    va_end(args);
	sMyPrint("%s", log_buf);
	#endif  //boardEASY_FLASH
}
