#include "Flash/flash_gd32.h"
#include "Print/print_task.h"
#include "flash_allot_table.h"

#define FMC_PAGE_SIZE           FLASH_PAGE_SIZE


/*****************************************************************************************************************
-----函数功能    flash擦除函数
-----说明(备注)  注意G4系列，单 bank 分区一次擦写最小单位为4KB，双 bank 分区一次擦写最小单位为2KB
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
bool bFlash_Gd32EraseSector(uint32_t StartAddr,uint32_t EndAddr)
{
	uint32_t EraseCounter;
	uint8_t state = 0;
	/* calculate the number of page to be programmed/erased */
	uint32_t PageNum = (EndAddr - StartAddr + 1) / FMC_PAGE_SIZE; //一共要擦除多少页
	uint32_t num = (StartAddr - FLASH_BASE) / FMC_PAGE_SIZE ;      //第几页开始
	
	if(uPrint.tFlag.bOperFlash == 1)
		sMyPrint("bOperFlash:Erase%d,address:0x%02X to 0x%02X \r\n", num, StartAddr, EndAddr );
		
    /* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
    fmc_flag_clear(FMC_FLAG_BANK0_END);
    fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
    fmc_flag_clear(FMC_FLAG_BANK0_PGERR);
    
    /* erase the flash pages */
    for(EraseCounter = 0; EraseCounter < PageNum; EraseCounter++)
    {
 		if(fmc_page_erase(StartAddr + (FMC_PAGE_SIZE * EraseCounter)) == FMC_READY )
		{
			if( uPrint.tFlag.bOperFlash ) 
				sMyPrint("bOperFlash:开始擦除第%d页\r\n", num+EraseCounter );			
		}
		else
		{
			state = 1;
			if( uPrint.tFlag.bOperFlash ) 
				sMyPrint("bOperFlash:第%d页擦除失败\r\n", num+EraseCounter );	
		}
		fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    }

    /* lock the main FMC after the erase operation */
    fmc_lock();
	
	if(state)
		return false;
	else 
		return true;
	
}


/*****************************************************************************************************************
-----函数功能    flash 写入函数,按16位写入
-----说明(备注)  none
-----传入参数    Addr:开始地址    Data:数据    Num:8Bit长度
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
bool bFlash_Gd32Write16Bit(uint32_t WriteAddr,uint16_t *wData,uint32_t wNum)
{
	if( uPrint.tFlag.bOperFlash ) 
		sMyPrint("开始写入\r\n");
	
    /* unlock the flash program/erase controller */
    fmc_unlock();
    /* program flash */
	for(int i = 0 ; i < (wNum/2) ;i++)
	{
		if(fmc_halfword_program(WriteAddr, *wData) != FMC_READY)
		{
			return false;
		}
		WriteAddr = WriteAddr + 2;
		wData++;
	}
    
	//数据为单数,补上最后一个字节
	if(wNum % 2 != 0)
	{
		uint16_t data_temp = *wData & 0x00FF;  //取低位
		if(fmc_halfword_program(WriteAddr, data_temp) != FMC_READY)
		{
			return false;
		}
	}
		
	
    fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    /* lock the main FMC after the program operation */
    fmc_lock();
	
    if( uPrint.tFlag.bOperFlash ) 
	{
		sMyPrint("写入完成\r\n");
	}
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    flash 写入函数,按32位写入
-----说明(备注)  none
-----传入参数    Addr:开始地址    Data:数据    Num:8Bit长度
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
bool bFlash_Gd32Write32Bit(uint32_t WriteAddr,const uint32_t *wData,uint32_t wNum)
{
	if( uPrint.tFlag.bOperFlash ) 
		sMyPrint("\r\n start write FMC \n");
	
    /* unlock the flash program/erase controller */
    fmc_unlock();
    /* program flash */
	for(int i = 0 ; i < (wNum/4) ;i++)
	{
		if(fmc_word_program(WriteAddr, *wData) != FMC_READY)
		{
			return false;
		}
		WriteAddr = WriteAddr + 4;
		wData++;
	}
    
	//数据为单数,补上最后字节
	int len = wNum % 4;
	if(len != 0)
	{
		uint16_t data_temp = *wData & (0xFFFFFFFF>>(4-len));  //取低位
		if(fmc_word_program(WriteAddr, data_temp) != FMC_READY)
		{
			return false;
		}
	}
		
	
    fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    /* lock the main FMC after the program operation */
    fmc_lock();
	
    if( uPrint.tFlag.bOperFlash ) 
	{
		sMyPrint("\r\nWrite complete!\n");
		sMyPrint("\r\n");
	}
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    flash 读取函数,按8位读取
-----说明(备注)  none
-----传入参数    ReadAddr:开始地址    rData:数据    rNum:8Bit长度
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vFlash_Gd32Read8Bit(uint32_t ReadAddr,uint8_t *rData,uint32_t rNum)
{
	uint8_t i;
    if( uPrint.tFlag.bOperFlash )
	{		
		sMyPrint("从地址0x%02X开始读取\r\n", ReadAddr);
	}
    for(i=0; i<rNum; i++){
        rData[i] = *(__IO uint8_t*)ReadAddr;
        if( uPrint.tFlag.bOperFlash ) 
			sMyPrint("0x%x  ", rData[i]);
        ReadAddr++;
    }
    if( uPrint.tFlag.bOperFlash ) 
	{
		sMyPrint("读取完成\r\n");
	}
}

/*****************************************************************************************************************
-----函数功能    flash 读取函数,按8位读取
-----说明(备注)  none
-----传入参数    ReadAddr:开始地址    rData:数据    rNum:8Bit长度
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vFlash_Gd32Read32Bit(uint32_t ReadAddr,uint32_t *rData,uint32_t rNum)
{
	uint8_t i;
    if( uPrint.tFlag.bOperFlash )
	{		
		sMyPrint("\r\nRead data from 0x%02X\n", ReadAddr);
		sMyPrint("\r\n");
	}
	
    for(i=0; i<(rNum/4); i++)
	{
        rData[i] = *(__IO uint32_t*)ReadAddr;
		
        if( uPrint.tFlag.bOperFlash ) 
			sMyPrint("0x%x  ", rData[i]);
		
        ReadAddr = ReadAddr + 4;
    }
	
    if( uPrint.tFlag.bOperFlash ) 
	{
		sMyPrint("\r\nRead end\n");
		sMyPrint("\r\n");
	}
}

