#ifndef FLASH_ALLOT_TABLE_H_
#define FLASH_ALLOT_TABLE_H_

#include "gd32f30x.h"
#include "board_config.h"

#define     	FLASH_PAGE_SIZE              			2048       //2K
#define     	KByte                        			(1024UL)

/*------------ IAP_AP分区表 -------------------------------------*/
//RAM大小 128K : 0x20000
#define     	SRAM_START              				(SRAM_BASE)                   // 协议栈占用 10K  0x20002A98
#define     	SRAM_END                				(SRAM_BASE + (128 * KByte))   // IAP SRAM结束地址 64KB


//GD32F303CBT6 	Flash:128K          					地址0x0800_0000--0x0801_FFFF
//GD32F303RCT6  Flash:256K          					地址0x0800_0000--0x0803_FFFF  @

#if(boardEASY_FLASH)
//BOOT数据：  	30K                     				地址范围：0x0800_0000--0x0800_77FF
#define     	flashBOOT_SIZE          				(26 * FLASH_PAGE_SIZE)
#define     	flashBOOT_START         				(FLASH_BASE)                                   
#define     	flashBOOT_END           				(flashBOOT_START + flashBOOT_SIZE - 1)              

//APP数据：   	60K                    					地址偏移：0x0800_8000--0x0803_EFFF = 0x0003_6FFF 
#define     	flashAPP_SIZE           				(80 * FLASH_PAGE_SIZE)
#define     	flashAPP_START          				(0x90000000)    
#define     	flashAPP_END            				(flashAPP_START + flashAPP_SIZE - 1)

//APP信息：   	4K                      				地址偏移：0x0803_F000--0x0803_FFFF   
#define     	flashAPP_INFO_SIZE      				(15 * FLASH_PAGE_SIZE)
#define     	flashAPP_INFO_SATRT     				(flashAPP_END + 1)
#define     	flashAPP_INFO_END		 				(flashAPP_INFO_SATRT + flashAPP_INFO_SIZE - 1)

//BOOT信息：  	和APP公用地址 
#define     	flashBOOT_INFO_SIZE     				flashAPP_INFO_SIZE
#define     	flashBOOT_INFO_START    				flashAPP_INFO_SATRT                            
#define     	flashBOOT_INFO_END      				flashAPP_INFO_END 

#else
//BOOT数据：  	30K                     				地址范围：0x0800_0000--0x0800_77FF
#define     	flashBOOT_SIZE          				(25 * FLASH_PAGE_SIZE)
#define     	flashBOOT_START         				(FLASH_BASE)                                   
#define     	flashBOOT_END           				(flashBOOT_START + flashBOOT_SIZE - 1)              

//BOOT信息：  	2K                      				地址偏移：0x0800_7800--0x0800_7FFF   
#define     	flashBOOT_INFO_SIZE     				(1 * FLASH_PAGE_SIZE)
#define     	flashBOOT_INFO_START    				(flashBOOT_END + 1)                            
#define     	flashBOOT_INFO_END      				(flashBOOT_INFO_START + flashBOOT_INFO_SIZE - 1)                            

//APP数据：   	60K                    					地址偏移：0x0800_8000--0x0803_EFFF = 0x0003_6FFF 
#define     	flashAPP_SIZE           				(80 * FLASH_PAGE_SIZE)
#define     	flashAPP_START          				(flashBOOT_INFO_END + 1)    
#define     	flashAPP_END            				(flashAPP_START + flashAPP_SIZE - 1) 

//APP信息：   	4K                      				地址偏移：0x0803_F000--0x0803_FFFF   
#define     	flashAPP_INFO_SIZE      				(15 * FLASH_PAGE_SIZE)
#define     	flashAPP_INFO_SATRT     				(flashAPP_END + 1)
#define     	flashAPP_INFO_END		 				(flashAPP_INFO_SATRT + flashAPP_INFO_SIZE - 1)
#endif


/*------------ IAP_AP分区表结束 -------------------------------------*/

#define     	flashBOOT_STACK_SIZE   					52

#endif







