#ifndef FLASH_IFACE_H_
#define FLASH_IFACE_H_

#include "main.h"
#include "..\..\BOOT\Application\flash_allot_table.h"

#define  FLASH_DEBUG          0      //进入测试模式

#define  FALSH_START_ADDR     flashAPP_START  
#define  FALSH_END_ADDR       flashAPP_END    

typedef struct
{
	u32 NextWriteAddr;
	u32 NextReadAddr;
	u32 ReadSize;
	u32 EraseSectorFinishNum;
}Flash_T;

extern Flash_T  tFlash;

bool bFlash_IfaceInit(void);

s8 cFlash_EraseSector(u32 star_addr,u32 end_addr);
s8 cFlash_Write8BitData(u32 start_addr, u8* data, u32 len);
s8 cFlash_Read8BitData(u32 start_addr, u8* data, u32 len);

#if(FLASH_DEBUG)
void vFlash_ReadWriteTest(void);
#endif

#endif

