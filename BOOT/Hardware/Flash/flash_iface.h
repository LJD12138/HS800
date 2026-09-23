/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Flash
 * File    : flash_iface.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Flash通用硬件操作接口定义头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef FLASH_IFACE_H_
#define FLASH_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"
#include "flash_allot_table.h"

//****************************************************Macros********************************************************************//
#define			FLASH_DEBUG								0		/* 进入测试模式 */

#define			FLASH_START_ADDR						flashAPP_START
#define			FLASH_END_ADDR							flashAPP_END
#define			FALSH_START_ADDR						FLASH_START_ADDR
#define			FALSH_END_ADDR							FLASH_END_ADDR

//****************************************************Types*********************************************************************//
typedef struct
{
	u32					NextWriteAddr;
	u32					NextReadAddr;
	u32					ReadSize;
	u32					EraseSectorFinishNum;
}Flash_T;

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
bool bFlash_IfaceInit(void);
void vFlash_WriteDataToFlashInit(void);
bool bFlash_WriteDataToFlash(u8 *data, u32 len);
s8   cFlash_EraseSector(u32 star_addr, u32 end_addr);
s8   cFlash_Write8BitData(u32 start_addr, u8 *data, u32 len);
s8   cFlash_Read8BitData(u32 start_addr, u8 *data, u32 len);

#if (FLASH_DEBUG)
void vFlash_ReadWriteTest(void);
#endif  /* FLASH_DEBUG */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* FLASH_IFACE_H_ */
