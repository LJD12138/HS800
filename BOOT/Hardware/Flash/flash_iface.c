#include "Flash/flash_iface.h"
#include "Print/print_task.h"

#if(boardUSE_SFUD)
#include <sfud.h>
#elif(boardIC_TYPE == boardIC_GD32F30X)
#include "Flash/flash_gd32.h"
#elif(boardIC_TYPE == boardIC_STM32H7XX)
#include "Flash/flash_stm32.h"
#endif

//------------------------------------变量常量定义------------------------------------
Flash_T  tFlash;

#if(boardUSE_SFUD)
const sfud_flash *flash = NULL; //获取设备结构体
#ifdef SFUD_USING_QSPI
const sfud_flash *qspi_flash = NULL; //获取设备结构体
#endif  //SFUD_USING_QSPI
#endif  //boardUSE_SFUD

/*****************************************************************************************************************
-----函数功能    Flash接口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
bool bFlash_IfaceInit(void)
{
	#if(boardUSE_SFUD)
	if(sfud_init() == SFUD_SUCCESS)
	{
		//IC1 Flash初始化
		flash = sfud_get_device_table() + 0;  //获取设备信息
		
		//IC2 QSPI初始化
		#ifdef SFUD_USING_QSPI
		sfud_qspi_fast_read_enable(sfud_get_device(SFUD_W25Q_DEVICE_INDEX1), 4);
		qspi_flash = sfud_get_device(SFUD_W25Q_DEVICE_INDEX1);
		#endif
		return true;
	}
	else
		return false;
	#else
	return true;
	#endif  //boardUSE_SFUD
}


/*****************************************************************************************************************
-----函数功能    写数据到Flash初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vFlash_WriteDataToFlashInit(void)
{
	if(uPrint.tFlag.bOperFlash == 1)
		sMyPrint("init flash \r\n ");
	
	tFlash.NextWriteAddr = FALSH_START_ADDR;  //初始化写地址
	tFlash.NextReadAddr = FALSH_START_ADDR;   //初始化读地址
	
	tFlash.EraseSectorFinishNum = 0 ; //清除ELOG第一个扇区

	cFlash_EraseSector(FALSH_START_ADDR, FALSH_END_ADDR);
}


/*****************************************************************************************************************
-----函数功能    依序写数据到Flash
-----说明(备注)  使用这个回进入错误状态,原因未知
-----传入参数    data:数据指针    len:字节长度
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
bool bFlash_WriteDataToFlash(u8* data,u32 len)
{
	s8 temp = 0;
	
	if(uPrint.tFlag.bOperFlash)
		sMyPrint("bOperFlash:开始地址0x%x\r\n",(u16) tFlash.NextWriteAddr);
	
	temp = cFlash_Write8BitData(tFlash.NextWriteAddr, data, len);
	if (temp > 0)   //最多写入0.255K
	{
		tFlash.NextWriteAddr += len ;   //完成后指向下一个地址
		
		if(tFlash.NextWriteAddr > FALSH_END_ADDR)  //环形写入,到末地址后从头开始
			tFlash.NextWriteAddr = FALSH_START_ADDR;
		
		if(uPrint.tFlag.bOperFlash)
			sMyPrint("bOperFlash:写入成功,结束地址=0x%x,大小=%d.\r\n",tFlash.NextWriteAddr-1,len);
		
		return true;;
	} 
	else 
	{
		if(uPrint.tFlag.bOperFlash)
			sMyPrint("bOperFlash:写入失败.代码=%d\r\n",temp);
		
		return false;
	}
}


/*****************************************************************************************************************
-----函数功能    flash 擦除函数
-----说明(备注)  none
-----传入参数    start_addr:开始地址    end_addr:结束地址
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
s8 cFlash_EraseSector(u32 star_addr,u32 end_addr)
{
	#if(boardIC_TYPE == boardIC_GD32F30X)
	if(bFlash_Gd32EraseSector(star_addr, end_addr) == false)
		return -1;
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	#if(boardUSE_SFUD)
	if(flash == NULL)
		return -1;
	
	if(sfud_erase(flash,star_addr, end_addr) != SFUD_SUCCESS)
		return -2;
	#else
	
	#endif  //boardUSE_SFUD
	#endif  //boardIC_TYPE
	
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    flash 写入函数,按8位写入
-----说明(备注)  none
-----传入参数    start_addr:开始地址    data:数据    len:字节长度
-----输出参数    none
-----返回值      0:写入成功    反之失败
*****************************************************************************************************************/
s8 cFlash_Write8BitData(u32 start_addr, u8* data, u32 len)
{
	if(data == NULL || len == 0)
		return 0;
	
	
	#if(boardIC_TYPE == boardIC_GD32F30X)
	if(bFlash_Gd32Write32Bit(start_addr,(u32*)data,len) == false)
		return -1;
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	#if(boardUSE_SFUD)
	if(flash == NULL)
		return -1;
	
	if(sfud_write(flash, start_addr, len, data) != SFUD_SUCCESS)
		return -2;
	#else
//	#error "错误,当前STMH7 不带内部Flash操作";
	#endif  //boardUSE_SFUD
	#endif  //boardIC_TYPE
	

	if(uPrint.tFlag.bOperFlash)
	{
		sMyPrint("\r\n 写入的数据 长度= %d ",len);
		for(int i = 0; i < len; i++)
		{
			sMyPrint("%x ",data[i]);
		}
		sMyPrint("\r\n");
	}
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    flash 读取函数,按8位读取
-----说明(备注)  none
-----传入参数    start_addr:开始地址    data:存放数据buff的指针   len:字节长度
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
s8 cFlash_Read8BitData(u32 start_addr, u8* data, u32 len)
{
	if(data == NULL || len == 0)
		return 0;
	
	#if(boardIC_TYPE == boardIC_GD32F30X)
	vFlash_Gd32Read8Bit(start_addr, data, len);
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	#if(boardUSE_SFUD)
	if(flash == NULL)
		return -1;
	
	if(sfud_read(flash, start_addr, len, data) != SFUD_SUCCESS)
		return -1;
	#else
	
	#endif  //boardUSE_SFUD
	#endif  //boardIC_TYPE
	

	
	if(uPrint.tFlag.bOperFlash)
	{
		sMyPrint("\r\n读取的数据 长度= %d ",len);
		for(int i = 0; i < len; i++)
		{
			sMyPrint("%x ",data[i]);
		}
		sMyPrint("\r\n");
	}
	
	return 1;
}

#if(FLASH_DEBUG)
#include "systick.h"
/*****************************************************************************************************************
-----函数功能    flash测试Demo
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void sfud_qspi_test_demo(void);
void sfud_spi_test_demo(void);

void vFlash_ReadWriteTest(void)
{
	sfud_spi_test_demo();
//	sfud_qspi_test_demo();
}

void sfud_spi_test_demo(void)
{
	s8 ret = 0;
	u16 size = 512;
	uint32_t addr = flashBOOT_INFO_START;
	__ALIGNED(4) u8 uc_w_buf[512] = {0};
	__ALIGNED(4) u8 uc_r_buf[512] = {0};
	
	const sfud_flash *flash = sfud_get_device(SFUD_W25Q_DEVICE_INDEX);
	
	/* prepare write data */
    for (int i = 0; i < size; i++)
    {
		if(i >= 256)
			uc_w_buf[i] = 512 - i - 1;
		else
			uc_w_buf[i] = i;
    }
    /* erase test */
    ret = cFlash_EraseSector(addr, flashBOOT_INFO_END);
    if (ret > 0)
        sMyPrint("擦除%s完成.地址0x%08X.大小%zu\r\n", flash->name, addr, size);
    else
    {
        sMyPrint("擦除%s失败.\r\n", flash->name);
        return;
    }
    /* write test */
    ret = cFlash_Write8BitData(addr, &uc_w_buf[addr], 200); addr +=200;
	ret = cFlash_Write8BitData(addr, &uc_w_buf[addr], 200); addr +=200;
	ret = cFlash_Write8BitData(addr, &uc_w_buf[addr], 112); addr = flashBOOT_INFO_START;
    if (ret > 0)
        sMyPrint("%s  Flash写入完成.地址0x%08X.大小%zu.\r\n", flash->name, addr, size);
    else
    {
        sMyPrint("%s  Flash写入失败.\r\n", flash->name);
        return;
    }
    /* read test */
    ret = cFlash_Read8BitData(addr, uc_r_buf, size);
    if (ret > 0)
    {
        sMyPrint("%s  Flash读取完成.地址0x%08X,大小%zu. :\r\n", flash->name, addr, size);
        sMyPrint("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
        for (int i = 0; i < size; i++)
        {
            if (i % 16 == 0)
            {
                sMyPrint("[%08X] ", addr + i);
            }
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
        sMyPrint("%s  Flash读取失败.\r\n", flash->name);
    
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
void sfud_qspi_test_demo(void)
{
	u16 size = 512;
	uint32_t addr = 0;
	u8 uc_w_buf[512] = {0};
	u8 uc_r_buf[512] = {0};
	
    sfud_err ret = SFUD_SUCCESS;
    const sfud_flash *flash = sfud_get_device(SFUD_W25Q_DEVICE_INDEX1);

    /* prepare write data */
    for (int i = 0; i < size; i++)
    {
		if(i >= 256)
			uc_w_buf[i] = 512 - i - 1;
		else
			uc_w_buf[i] = i;
    }
    /* erase test */
    ret = sfud_erase(flash, addr, size);
    if (ret == SFUD_SUCCESS)
        sMyPrint("擦除%s完成.地址0x%08X.大小%zu\r\n", flash->name, addr, size);
    else
    {
        sMyPrint("擦除%s失败.\r\n", flash->name);
        return;
    }
    /* write test */
    ret = sfud_write(flash, addr, 200, &uc_w_buf[addr]); addr +=200;
	ret = sfud_write(flash, addr, 200, &uc_w_buf[addr]); addr +=200;
	ret = sfud_write(flash, addr, 112, &uc_w_buf[addr]); addr = 0;
    if (ret == SFUD_SUCCESS)
        sMyPrint("%s  Flash写入完成.地址0x%08X.大小%zu.\r\n", flash->name, addr, size);
    else
    {
        sMyPrint("%s  Flash写入失败.\r\n", flash->name);
        return;
    }
    /* read test */
    ret = sfud_read(flash, addr, size, uc_r_buf);
    if (ret == SFUD_SUCCESS)
    {
        sMyPrint("%s  Flash读取完成.地址0x%08X,大小%zu. :\r\n", flash->name, addr, size);
        sMyPrint("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
        for (int i = 0; i < size; i++)
        {
            if (i % 16 == 0)
            {
                sMyPrint("[%08X] ", addr + i);
            }
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
        sMyPrint("%s  Flash读取失败.\r\n", flash->name);
    
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
#endif  //SFUD_USING_QSPI
#endif  //FLASH_DEBUG
