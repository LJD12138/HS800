#include "flash_stm32.h"
#include "print.h"



/*****************************************************************************************************************
-----函数功能    获取给定地址在flash的页数
-----说明(备注)  none
-----传入参数    Addr:地址
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
static uint32_t ul_stm32_get_page(uint32_t Addr)
{
	uint32_t page = 0;

	if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
	{
		page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE; /* Bank 1 */
	}
	else
	{
		page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE; /* Bank 2 */
	}

	return page;
}

/*****************************************************************************************************************
-----函数功能    获取地址所在 bank 分区
-----说明(备注)  none
-----传入参数    Addr:地址
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
static uint32_t ul_stm32_get_bank(uint32_t Addr)
{
	return FLASH_BANK_1;
}

/*****************************************************************************************************************
-----函数功能    flash擦除函数
-----说明(备注)  注意G4系列，单 bank 分区一次擦写最小单位为4KB，双 bank 分区一次擦写最小单位为2KB
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
HAL_StatusTypeDef eFlash_Stm32EraseSector(uint32_t StartAddr,uint32_t EndAddr)
{
	/* 用于擦除的结构体 */
	static FLASH_EraseInitTypeDef EraseInitStruct = {0};
	/* 函数操作状态位 */
	HAL_StatusTypeDef erase_status = HAL_OK;
	
	uint32_t FirstPage = 0;
	uint32_t PageError = 0;
	uint32_t BankNumber = 0;
	uint32_t NbOfPages = 0;
	
	/* 解锁flash，启用flash控制寄存器 */
	HAL_FLASH_Unlock();

	/* 获取要写入的地址所在页 */
	FirstPage = ul_stm32_get_page(StartAddr);
	
	/* 获取要从第一页开始擦除的总页数 */
	NbOfPages = ul_stm32_get_page(EndAddr) - FirstPage + 1;
	
	/* Get the bank */
	BankNumber = ul_stm32_get_bank(StartAddr);

	/* 填充擦除结构体*/
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks       = BankNumber;
	EraseInitStruct.Page        = FirstPage;
	EraseInitStruct.NbPages     = NbOfPages;

	/* 按结构体进行擦除 */
	erase_status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
	if(erase_status != HAL_OK)
	{
		if(uPrint.tFlag.bOperFlash)
			sMyPrintWarn("bOperFlash:擦除失败");
	}
	
	/* 不管是否操作成功，均上锁flash，防止误操作 */
	HAL_FLASH_Lock();
	
	return erase_status;
	
}


/*****************************************************************************************************************
-----函数功能    flash 写入函数,按64位写入
-----说明(备注)  none
-----传入参数    Addr:开始地址    Data:数据    Num:64Bit长度
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
HAL_StatusTypeDef eFlash_Stm32Write64Bit(uint32_t WriteAddr,uint64_t *wData,uint32_t wNum)
{
	/* 函数操作状态位 */
	HAL_StatusTypeDef write_status = HAL_OK;
	/* 写入函数的开始地址 */
	uint32_t wStartAddr = WriteAddr;
	/* 写入函数的结束地址 */
	uint32_t wEndAddr = WriteAddr + wNum * 8;

	/* 解锁flash，启用flash控制寄存器 */
	HAL_FLASH_Unlock();
	
	/* 清除原始设置上的 OPTVERR 标志位设置 */
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
	
	/* 开始写入 */
	while(wStartAddr < wEndAddr)
	{
		write_status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, wStartAddr, *wData);
		if (write_status == HAL_OK)
		{
			wData++;
			wStartAddr = wStartAddr + 8;
		}
		else
		{
			/* 不管是否操作成功，均上锁flash，防止误操作 */
			HAL_FLASH_Lock();
			
			if(uPrint.tFlag.bOperFlash)
				sMyPrintWarn("bOperFlash:写入失败");
			
			return write_status;
		}
	}

	/* 不管是否操作成功，均上锁flash，防止误操作 */
	HAL_FLASH_Lock();
	
	if(uPrint.tFlag.bOperFlash)
		sMyPrint("bOperFlash:写入成功\r\n");
	
	return write_status;
}

/*****************************************************************************************************************
-----函数功能    flash 读取函数,按32位读取
-----说明(备注)  none
-----传入参数    ReadAddr:开始地址    rData:数据    rNum:32Bit长度
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
HAL_StatusTypeDef eFlash_Stm32Read32Bit(uint32_t ReadAddr, uint32_t *rData, uint32_t rNum)
{
	/* 函数操作状态位 */
	HAL_StatusTypeDef read_status = HAL_OK;
	/* 读取函数的开始地址 */
	uint32_t rStartAddr = ReadAddr;
	/* 读取函数的结束地址 */
	uint32_t rEndAddr = ReadAddr + rNum * 4;
	
	/* 开始读取 */
	while(rStartAddr < rEndAddr)
	{
		*rData = *(__IO uint32_t *)rStartAddr;

			rData++;
			rStartAddr = rStartAddr + 4;
	}
	
	if(uPrint.tFlag.bOperFlash)
		sMyPrint("读取成功\r\n");
	
	return read_status;
}

