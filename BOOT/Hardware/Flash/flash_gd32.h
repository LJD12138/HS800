#ifndef FLASH_GD32_H_
#define FLASH_GD32_H_

#include "main.h"
 
bool bFlash_Gd32EraseSector(uint32_t StarAddr,uint32_t EndAddr);
bool bFlash_Gd32Write16Bit(uint32_t WriteAddr,uint16_t *wData,uint32_t wNum);
bool bFlash_Gd32Write32Bit(uint32_t WriteAddr,const uint32_t *wData,uint32_t wNum);
void vFlash_Gd32Read8Bit(uint32_t ReadAddr,uint8_t *rData,uint32_t rNum);
void vFlash_Gd32Read32Bit(uint32_t ReadAddr,uint32_t *rData,uint32_t rNum);
#endif
