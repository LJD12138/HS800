/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Flash
 * File    : flash_iface.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Flash通用硬件接口实现(支持GD32内部Flash与SFUD/QSPI外部Flash)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Flash/flash_iface.h"
#include "Print/print_task.h"

#if (boardUSE_SFUD)
#include <sfud.h>
#elif (boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
#include "Flash/flash_gd32.h"
#elif (boardIC_TYPE == boardIC_STM32H7XX)
#include "Flash/flash_stm32.h"
#endif

//****************************************************Parameter Initialization**************************************************//
static Flash_T s_t_flash;

#if (boardUSE_SFUD)
const sfud_flash *flash = NULL; /* 获取设备结构体 */
#ifdef SFUD_USING_QSPI
const sfud_flash *qspi_flash = NULL; /* 获取设备结构体 */
#endif  /* SFUD_USING_QSPI */
#endif  /* boardUSE_SFUD */

//****************************************************Function Declaration******************************************************//


/***********************************************************************************************************************
 * 函数功能    : Flash接口初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true: 初始化成功, false: 初始化失败
 ************************************************************************************************************************/
bool bFlash_IfaceInit(void)
{
	#if (boardUSE_SFUD)
	if (sfud_init() == SFUD_SUCCESS)
	{
		/* IC1 Flash初始化 */
		flash = sfud_get_device_table() + 0;  /* 获取设备信息 */
		
		/* IC2 QSPI初始化 */
		#ifdef SFUD_USING_QSPI
		sfud_qspi_fast_read_enable(sfud_get_device(SFUD_W25Q_DEVICE_INDEX1), 4);
		qspi_flash = sfud_get_device(SFUD_W25Q_DEVICE_INDEX1);
		#endif  /* SFUD_USING_QSPI */
		return true;
	}
	else
		return false;
	#else
	return true;
	#endif  /* boardUSE_SFUD */
}

/***********************************************************************************************************************
 * 函数功能    : 写数据到Flash初始化
 * 说明(备注)  : 擦除整个APP区域并重置地址指针
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vFlash_WriteDataToFlashInit(void)
{
	if (uPrint.tFlag.bOperFlash == 1)
		sMyPrint("init flash \r\n ");
	
	s_t_flash.NextWriteAddr = FLASH_START_ADDR;  /* 初始化写地址 */
	s_t_flash.NextReadAddr  = FLASH_START_ADDR;  /* 初始化读地址 */
	
	s_t_flash.EraseSectorFinishNum = 0; /* 清除ELOG第一个扇区 */

	cFlash_EraseSector(FLASH_START_ADDR, FLASH_END_ADDR);
}

/***********************************************************************************************************************
 * 函数功能    : 依序写数据到Flash
 * 说明(备注)  : 支持地址自增并对溢出进行环形处理
 * 传入参数    : data: 数据指针, len: 字节长度
 * 输出参数    : none
 * 返回值      : true: 写入成功, false: 写入失败
 ************************************************************************************************************************/
bool bFlash_WriteDataToFlash(u8 *data, u32 len)
{
	s8 temp = 0;
	
	if (uPrint.tFlag.bOperFlash)
		sMyPrint("bOperFlash:开始地址0x%x\r\n", (u16)s_t_flash.NextWriteAddr);
	
	temp = cFlash_Write8BitData(s_t_flash.NextWriteAddr, data, len);
	if (temp > 0)
	{
		s_t_flash.NextWriteAddr += len;   /* 完成后指向下一个地址 */
		
		if (s_t_flash.NextWriteAddr > FLASH_END_ADDR)  /* 环形写入,到末地址后从头开始 */
			s_t_flash.NextWriteAddr = FLASH_START_ADDR;
		
		if (uPrint.tFlag.bOperFlash)
			sMyPrint("bOperFlash:写入成功,结束地址=0x%x,大小=%d.\r\n", s_t_flash.NextWriteAddr - 1, len);
		
		return true;
	} 
	else 
	{
		if (uPrint.tFlag.bOperFlash)
			sMyPrint("bOperFlash:写入失败.代码=%d\r\n", temp);
		
		return false;
	}
}

/***********************************************************************************************************************
 * 函数功能    : Flash扇区擦除函数
 * 说明(备注)  : 根据当前芯片类型分发至对应底层驱动
 * 传入参数    : star_addr: 开始地址, end_addr: 结束地址
 * 输出参数    : none
 * 返回值      : >0: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 cFlash_EraseSector(u32 star_addr, u32 end_addr)
{
	#if(boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
	if (bFlash_Gd32EraseSector(star_addr, end_addr) == false)
		return -1;
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
	#if (boardUSE_SFUD)
	if (flash == NULL)
		return -1;
	
	if (sfud_erase(flash, star_addr, end_addr) != SFUD_SUCCESS)
		return -2;
	#endif  /* boardUSE_SFUD */
	#endif  /* boardIC_TYPE */
	
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : Flash按8位字节写入函数
 * 说明(备注)  : 根据当前芯片类型分发至对应底层驱动
 * 传入参数    : start_addr: 开始地址, data: 数据指针, len: 字节长度
 * 输出参数    : none
 * 返回值      : 1: 写入成功, 0: 入参为空, 负数: 失败
 ************************************************************************************************************************/
s8 cFlash_Write8BitData(u32 start_addr, u8 *data, u32 len)
{
	if (data == NULL || len == 0)
		return 0;
	
	
	#if(boardIC_TYPE == boardIC_GD32F30X ||boardIC_TYPE == boardIC_GD32F50X)
	if (bFlash_Gd32Write32Bit(start_addr, (u32*)data, len) == false)
		return -1;
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
	#if (boardUSE_SFUD)
	if (flash == NULL)
		return -1;
	
	if (sfud_write(flash, start_addr, len, data) != SFUD_SUCCESS)
		return -2;
	#endif  /* boardUSE_SFUD */
	#endif  /* boardIC_TYPE */

	if (uPrint.tFlag.bOperFlash)
	{
		sMyPrint("\r\n 写入的数据 长度= %d ", len);
		for (int i = 0; i < len; i++)
			sMyPrint("%x ", data[i]);
		sMyPrint("\r\n");
	}
	
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : Flash按8位字节读取函数
 * 说明(备注)  : 根据当前芯片类型分发至对应底层驱动
 * 传入参数    : start_addr: 开始地址, data: 存放数据buff指针, len: 字节长度
 * 输出参数    : data
 * 返回值      : 1: 读取成功, 0: 入参为空, 负数: 失败
 ************************************************************************************************************************/
s8 cFlash_Read8BitData(u32 start_addr, u8 *data, u32 len)
{
	if (data == NULL || len == 0)
		return 0;
	
	#if(boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
	vFlash_Gd32Read8Bit(start_addr, data, len);
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
	#if (boardUSE_SFUD)
	if (flash == NULL)
		return -1;
	
	if (sfud_read(flash, start_addr, len, data) != SFUD_SUCCESS)
		return -1;
	#endif  /* boardUSE_SFUD */
	#endif  /* boardIC_TYPE */

	if (uPrint.tFlag.bOperFlash)
	{
		sMyPrint("\r\n读取的数据 长度= %d ", len);
		for (int i = 0; i < len; i++)
			sMyPrint("%x ", data[i]);
		sMyPrint("\r\n");
	}
	
	return 1;
}

#if (FLASH_DEBUG)
#include "systick.h"

void sfud_qspi_test_demo(void);
void sfud_spi_test_demo(void);

/***********************************************************************************************************************
 * 函数功能    : Flash读写测试函数
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vFlash_ReadWriteTest(void)
{
	sfud_spi_test_demo();
}

/***********************************************************************************************************************
 * 函数功能    : SFUD 标准 SPI Flash 读写测试示例
 * 说明(备注)  : 仅 FLASH_DEBUG 调试用; 擦除 flashBOOT_INFO 扇区后写入并读回校验 512 字节
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void sfud_spi_test_demo(void)
{
	s8 ret = 0;
	u16 size = 512;
	uint32_t addr = flashBOOT_INFO_START;
	__ALIGNED(4) u8 uc_w_buf[512] = {0};
	__ALIGNED(4) u8 uc_r_buf[512] = {0};
	
	const sfud_flash *sfud_dev = sfud_get_device(SFUD_W25Q_DEVICE_INDEX);
	
	/* prepare write data */
    for (int i = 0; i < size; i++)
    {
		if (i >= 256)
			uc_w_buf[i] = 512 - i - 1;
		else
			uc_w_buf[i] = i;
    }
    /* erase test */
    ret = cFlash_EraseSector(addr, flashBOOT_INFO_END);
    if (ret > 0)
        sMyPrint("擦除%s完成.地址0x%08X.大小%zu\r\n", sfud_dev->name, addr, size);
    else
    {
        sMyPrint("擦除%s失败.\r\n", sfud_dev->name);
        return;
    }
    /* write test */
    ret = cFlash_Write8BitData(addr, &uc_w_buf[addr], 200); addr += 200;
	ret = cFlash_Write8BitData(addr, &uc_w_buf[addr], 200); addr += 200;
	ret = cFlash_Write8BitData(addr, &uc_w_buf[addr], 112); addr = flashBOOT_INFO_START;
    if (ret > 0)
        sMyPrint("%s  Flash写入完成.地址0x%08X.大小%zu.\r\n", sfud_dev->name, addr, size);
    else
    {
        sMyPrint("%s  Flash写入失败.\r\n", sfud_dev->name);
        return;
    }
    /* read test */
    ret = cFlash_Read8BitData(addr, uc_r_buf, size);
    if (ret > 0)
    {
        sMyPrint("%s  Flash读取完成.地址0x%08X,大小%zu. :\r\n", sfud_dev->name, addr, size);
        sMyPrint("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
        for (int i = 0; i < size; i++)
        {
            if (i % 16 == 0)
                sMyPrint("[%08X] ", addr + i);
            sMyPrint("%02X ", uc_r_buf[i]);
            if (((i + 1) % 16 == 0) || i == size - 1)
            {
                sMyPrint("\r\n");
				vSys_MsDelay(2);
            }
        }
        sMyPrint("\r\n");
    }
    else
        sMyPrint("%s  Flash读取失败.\r\n", sfud_dev->name);
    
    /* data check */
    for (int i = 0; i < size; i++)
    {
        if (uc_r_buf[i] != uc_w_buf[i])
        {
            sMyPrint("数据比较失败.\r\n");
            return;
        }
    }
    sMyPrint("数据比较成功.\r\n");
}

#ifdef SFUD_USING_QSPI
/***********************************************************************************************************************
 * 函数功能    : SFUD QSPI Flash 读写测试示例
 * 说明(备注)  : 仅 FLASH_DEBUG 调试用; 擦除地址 0 处后写入并读回校验 512 字节
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void sfud_qspi_test_demo(void)
{
	u16 size = 512;
	uint32_t addr = 0;
	u8 uc_w_buf[512] = {0};
	u8 uc_r_buf[512] = {0};
	
    sfud_err ret = SFUD_SUCCESS;
    const sfud_flash *sfud_dev = sfud_get_device(SFUD_W25Q_DEVICE_INDEX1);

    /* prepare write data */
    for (int i = 0; i < size; i++)
    {
		if (i >= 256)
			uc_w_buf[i] = 512 - i - 1;
		else
			uc_w_buf[i] = i;
    }
    /* erase test */
    ret = sfud_erase(sfud_dev, addr, size);
    if (ret == SFUD_SUCCESS)
        sMyPrint("擦除%s完成.地址0x%08X.大小%zu\r\n", sfud_dev->name, addr, size);
    else
    {
        sMyPrint("擦除%s失败.\r\n", sfud_dev->name);
        return;
    }
    /* write test */
    ret = sfud_write(sfud_dev, addr, 200, &uc_w_buf[addr]); addr += 200;
	ret = sfud_write(sfud_dev, addr, 200, &uc_w_buf[addr]); addr += 200;
	ret = sfud_write(sfud_dev, addr, 112, &uc_w_buf[addr]); addr = 0;
    if (ret == SFUD_SUCCESS)
        sMyPrint("%s  Flash写入完成.地址0x%08X.大小%zu.\r\n", sfud_dev->name, addr, size);
    else
    {
        sMyPrint("%s  Flash写入失败.\r\n", sfud_dev->name);
        return;
    }
    /* read test */
    ret = sfud_read(sfud_dev, addr, size, uc_r_buf);
    if (ret == SFUD_SUCCESS)
    {
        sMyPrint("%s  Flash读取完成.地址0x%08X,大小%zu. :\r\n", sfud_dev->name, addr, size);
        sMyPrint("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
        for (int i = 0; i < size; i++)
        {
            if (i % 16 == 0)
                sMyPrint("[%08X] ", addr + i);
            sMyPrint("%02X ", uc_r_buf[i]);
            if (((i + 1) % 16 == 0) || i == size - 1)
            {
                sMyPrint("\r\n");
				vSys_MsDelay(2);
            }
        }
        sMyPrint("\r\n");
    }
    else
        sMyPrint("%s  Flash读取失败.\r\n", sfud_dev->name);
    
    /* data check */
    for (int i = 0; i < size; i++)
    {
        if (uc_r_buf[i] != uc_w_buf[i])
        {
            sMyPrint("数据比较失败.\r\n");
            return;
        }
    }
    sMyPrint("数据比较成功.\r\n");
}
#endif  /* SFUD_USING_QSPI */
#endif  /* FLASH_DEBUG */
