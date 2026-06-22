#ifndef FLASH_ALLOT_TABLE_H_
#define FLASH_ALLOT_TABLE_H_

#include "board_config.h"

#define     	FLASH_PAGE_SIZE              			2048       //2K
#define     	KByte                        			(1024UL)

/*------------ IAP_AP分区表 -------------------------------------*/
//GD32F303CBT6 	Flash:128K          					地址0x0800_0000--0x0801_FFFF
//GD32F303RCT6  Flash:256K          					地址0x0800_0000--0x0803_FFFF
//GD32F303RGT6  Flash:1024K          					地址0x0800_0000--0x080F_FFFF
//GD32F502RG    Flash:1024K                             地址0x0800_0000--0x080F_FFFF

//RAM大小 128K : 0x20000
#define     	SRAM_START              				(SRAM_BASE)                   // 协议栈占用 10K  0x20002A98
#define     	SRAM_END                				(SRAM_BASE + (128 * KByte))   // IAP SRAM结束地址 128KB


#if(boardEASY_FLASH)
/* BOOT数据：100K，地址范围：0x0800_0000 -- 0x0801_8FFF */
#define     	flashBOOT_SIZE          				(50 * FLASH_PAGE_SIZE)
#define     	flashBOOT_START         				(FLASH_BASE)                                   
#define     	flashBOOT_END           				(flashBOOT_START + flashBOOT_SIZE - 1)              

/* APP数据：800K，地址范围：0x0801_9000 -- 0x080E_0FFF */
#define     	flashAPP_SIZE           				(400 * FLASH_PAGE_SIZE)
#define     	flashAPP_START          				(flashBOOT_END + 1)    
#define     	flashAPP_END            				(flashAPP_START + flashAPP_SIZE - 1)

/* APP信息：30K，地址范围：0x080E_1800 -- 0x080E_8FFF */
#define     	flashAPP_INFO_SIZE      				(15 * FLASH_PAGE_SIZE)
#define     	flashAPP_INFO_SATRT     				(flashAPP_END + 1 + FLASH_PAGE_SIZE)
#define     	flashAPP_INFO_END		 				(flashAPP_INFO_SATRT + flashAPP_INFO_SIZE - 1)

/* BOOT信息：30K，与APP公用，地址范围：0x080E_1800 -- 0x080E_8FFF */
#define     	flashBOOT_INFO_SIZE     				flashAPP_INFO_SIZE
#define     	flashBOOT_INFO_START    				flashAPP_INFO_SATRT                            
#define     	flashBOOT_INFO_END      				flashAPP_INFO_END 

#else
/* BOOT数据：50K，地址范围：0x0800_0000 -- 0x0800_C7FF */
#define     	flashBOOT_SIZE          				(25 * FLASH_PAGE_SIZE)
#define     	flashBOOT_START         				(FLASH_BASE)                                   
#define     	flashBOOT_END           				(flashBOOT_START + flashBOOT_SIZE - 1)              

/* BOOT信息：2K，地址范围：0x0800_C800 -- 0x0800_CFFF */
#define     	flashBOOT_INFO_SIZE     				(1 * FLASH_PAGE_SIZE)
#define     	flashBOOT_INFO_START    				(flashBOOT_END + 1)                            
#define     	flashBOOT_INFO_END      				(flashBOOT_INFO_START + flashBOOT_INFO_SIZE - 1)                            

/* APP数据：160K，地址范围：0x0800_D000 -- 0x0803_4FFF */
#define     	flashAPP_SIZE           				(80 * FLASH_PAGE_SIZE)
#define     	flashAPP_START          				(flashBOOT_INFO_END + 1)    
#define     	flashAPP_END            				(flashAPP_START + flashAPP_SIZE - 1) 

/* APP信息：30K，地址范围：0x0803_5000 -- 0x0803_C7FF */
#define     	flashAPP_INFO_SIZE      				(15 * FLASH_PAGE_SIZE)
#define     	flashAPP_INFO_SATRT     				(flashAPP_END + 1)
#define     	flashAPP_INFO_END		 				(flashAPP_INFO_SATRT + flashAPP_INFO_SIZE - 1)
#endif


/*------------ IAP_AP分区表结束 -------------------------------------*/

#define     	flashBOOT_STACK_SIZE   					100

#endif







