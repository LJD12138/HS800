#ifndef FLASH_STM32_H_
#define FLASH_STM32_H_

#include "main.h"
#include "flash_allot_table.h"
 
/**
	*	@bref G4系列有3种存储密度，G474系列是Category 3，
	*				MCU在没有设置情况下，初次上电flash区域是单一bank，一页大小是4KB，
	*				即一次擦除最小单位为128bit；
	*
	*				如果将flash区域设置为双bank分区，则一页大小位2K，擦除最小单位为64bit；
	*				双bank涉及到分区读写，跨区读写，操作逻辑比较复杂。
	*
	*				下面是单bank 128bit读写，足够满足常规flash写入了。
	*/
#if defined(STM32G474xx)  //可以在此行用或指令( || )添加多个bank分区的MCU型号
 	/* 是否 bank 分区 */
	#define bank_en					1
#else 
	#define bank_en					0
#endif



#if	bank_en
	/**
		*	@bref BANK分区首尾地址，仅适用于G4系列 Category 3 存储密度，其他系列需要根据规格书修改；
		*				！！！默认开机启动，此分区交换是关闭的,需要开启
		*       ！！！默认开机启动，为sing bank，一页大小为4KB，需要可以通过软件设置
	*/
	/* bank 分区的分界线地址，用来划分bank1，bank2 */
	#define FLASH_BANK_DEMAR_LINE		(FLASH_BASE + FLASH_BANK_SIZE)
	
	/* bank 第二分区的起始地址,此起始地址需要根据规格书修改 */
	#define FLASH_SEC_BANK_BEGIN		(0x08040000U)

	#define FLASH_USER_START_ADDR		(FLASH_BASE)
	#define FLASH_USER_END_ADDR     (FLASH_SEC_BANK_BEGIN + FLASH_BANK_SIZE - 1)

#else
	/* 根据程序剩余flash空间进行设置，这里为了方便，给的整个flash区域，擦写需谨慎，会导致死机 */
	#define FLASH_USER_START_ADDR   (flashBOOT_INFO_START)					/* 用户flash区域起始地址 */
	#define FLASH_USER_END_ADDR     (flashBOOT_INFO_END)   /* 用户flash区域终止地址 */

#endif

HAL_StatusTypeDef eFlash_Stm32EraseSector(uint32_t StarAddr,uint32_t EndAddr);
HAL_StatusTypeDef eFlash_Stm32Write64Bit(uint32_t WriteAddr,uint64_t *wData,uint32_t wNum);
HAL_StatusTypeDef eFlash_Stm32Read32Bit(uint32_t ReadAddr,uint32_t *rData,uint32_t rNum);
#endif
