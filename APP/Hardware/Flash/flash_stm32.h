/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Flash
 * File    : flash_stm32.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : STM32内部Flash读写擦除底层驱动头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef FLASH_STM32_H_
#define FLASH_STM32_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"
#include "flash_allot_table.h"

#if (1)

//****************************************************Macros********************************************************************//
#if defined(STM32G474xx)
#define			bank_en									1
#else 
#define			bank_en									0
#endif

#if bank_en
#define			FLASH_BANK_DEMAR_LINE					(FLASH_BASE + FLASH_BANK_SIZE)
#define			FLASH_SEC_BANK_BEGIN					(0x08040000U)
#define			FLASH_USER_START_ADDR					(FLASH_BASE)
#define			FLASH_USER_END_ADDR						(FLASH_SEC_BANK_BEGIN + FLASH_BANK_SIZE - 1)
#else
#define			FLASH_USER_START_ADDR					(flashBOOT_INFO_START)
#define			FLASH_USER_END_ADDR						(flashBOOT_INFO_END)
#endif  /* bank_en */

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
#if defined(USE_HAL_DRIVER)
HAL_StatusTypeDef eFlash_Stm32EraseSector(uint32_t StarAddr, uint32_t EndAddr);
HAL_StatusTypeDef eFlash_Stm32Write64Bit(uint32_t WriteAddr, uint64_t *wData, uint32_t wNum);
HAL_StatusTypeDef eFlash_Stm32Read32Bit(uint32_t ReadAddr, uint32_t *rData, uint32_t rNum);
#endif  /* USE_HAL_DRIVER */

#endif  /* 1 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* FLASH_STM32_H_ */
