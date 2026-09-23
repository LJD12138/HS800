/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Flash
 * File    : flash_gd32.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : GD32 片内 Flash 底层擦写驱动实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Flash/flash_gd32.h"
#include "Print/print_task.h"
#include "..\..\BOOT\Application\flash_allot_table.h"

#if (boardWDGT_EN)
#include "fwdgt.h"
#endif  /* boardWDGT_EN */


//****************************************************Macros********************************************************************//
#define			FMC_PAGE_SIZE							FLASH_PAGE_SIZE


/***********************************************************************************************************************
 * 函数功能    : Flash 擦除函数
 * 说明(备注)  : 按页擦除指定地址范围内的 Flash 扇区
 * 传入参数    : StartAddr: 起始地址, EndAddr: 结束地址
 * 输出参数    : 无
 * 返回值      : true: 擦除成功, false: 擦除失败
 ************************************************************************************************************************/
bool bFlash_Gd32EraseSector(uint32_t StartAddr, uint32_t EndAddr)
{
    uint32_t EraseCounter;
    uint8_t state = 0;

    /* 入参防御:地址回绕/越界或起始地址未页对齐时直接失败,防止无符号下溢导致误擦全片 */
    if (StartAddr > EndAddr || StartAddr < FLASH_BASE || EndAddr > flashAPP_INFO_END || (StartAddr & (FMC_PAGE_SIZE - 1)) != 0)
        return false;

    /* 计算需要擦除的页数与起始页号(向上取整,覆盖尾部不满一页的部分) */
    uint32_t PageNum = (EndAddr - StartAddr + FMC_PAGE_SIZE) / FMC_PAGE_SIZE;
    uint32_t num     = (StartAddr - FLASH_BASE) / FMC_PAGE_SIZE;
    
    if (uPrint.tFlag.bOperFlash == 1)
        sMyPrint("bOperFlash:Erase%d,address:0x%02X to 0x%02X \r\n", num, StartAddr, EndAddr);
        
    /* 解锁 Flash 控制器 */
    fmc_unlock();

    /* 清除所有挂起标志位 */
    fmc_flag_clear(FMC_FLAG_BANK0_END);
    fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
    fmc_flag_clear(FMC_FLAG_BANK0_PGERR);
    
    /* 逐页擦除 Flash */
    for (EraseCounter = 0; EraseCounter < PageNum; EraseCounter++)
    {
        #if (boardWDGT_EN)
        vFwdgt_Reload();
        #endif  /* boardWDGT_EN */

        if (fmc_page_erase(StartAddr + (FMC_PAGE_SIZE * EraseCounter)) == FMC_READY)
        {
            if (uPrint.tFlag.bOperFlash) 
                sMyPrint("bOperFlash:开始擦除第%d页\r\n", num + EraseCounter);			
        }
        else
        {
            state = 1;
            if (uPrint.tFlag.bOperFlash) 
                sMyPrint("bOperFlash:第%d页擦除失败\r\n", num + EraseCounter);	
        }
        fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    }

    /* 上锁 Flash 控制器 */
    fmc_lock();
    
    if (state)
        return false;
    else 
        return true;
}

/***********************************************************************************************************************
 * 函数功能    : Flash 写入函数 (按16位写入)
 * 说明(备注)  : 半字编程，奇数字节时高字节补0xFF填充
 * 传入参数    : WriteAddr: 写入起始地址, wData: 数据源指针, wNum: 字节长度
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bFlash_Gd32Write16Bit(uint32_t WriteAddr, uint16_t *wData, uint32_t wNum)
{
    /* 入参防御:空指针或写地址非半字对齐时直接失败 */
    if (wData == NULL || (WriteAddr & 0x1U) != 0)
        return false;

    if (uPrint.tFlag.bOperFlash) 
        sMyPrint("开始写入\r\n");
    
    /* 解锁 Flash 控制器 */
    fmc_unlock();

    /* 编程前清除所有标志位 */
    fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);

    /* 逐半字写入 */
    for (uint32_t i = 0; i < (wNum / 2); i++)
    {
        if (fmc_halfword_program(WriteAddr, *wData) != FMC_READY)
        {
            fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
            fmc_lock();
            return false;
        }
        WriteAddr += 2;
        wData++;
    }
    
    /* 数据为单数,补上最后一个字节 (高字节填充0xFF保持Flash擦除态，不破坏后续字节) */
    if (wNum % 2 != 0)
    {
        uint16_t data_temp = ((uint16_t)(*(const uint8_t*)wData)) | 0xFF00;
        if (fmc_halfword_program(WriteAddr, data_temp) != FMC_READY)
        {
            fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
            fmc_lock();
            return false;
        }
    }
        
    fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    /* 编程完成后上锁 Flash */
    fmc_lock();
    
    if (uPrint.tFlag.bOperFlash)
        sMyPrint("写入完成\r\n");
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : Flash 写入函数 (按32位写入)
 * 说明(备注)  : 全字编程，不足4字节时尾部填充0xFF保持Flash擦除态
 * 传入参数    : WriteAddr: 写入起始地址, wData: 数据源指针, wNum: 字节长度
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bFlash_Gd32Write32Bit(uint32_t WriteAddr, const uint32_t *wData, uint32_t wNum)
{
    /* 入参防御:空指针或写地址非全字对齐时直接失败 */
    if (wData == NULL || (WriteAddr & 0x3U) != 0)
        return false;

    if (uPrint.tFlag.bOperFlash) 
        sMyPrint("\r\n start write FMC \n");
    
    /* 解锁 Flash 控制器 */
    fmc_unlock();

    /* 编程前清除所有标志位 */
    fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);

    /* 逐全字写入 */
    for (uint32_t i = 0; i < (wNum / 4); i++)
    {
        if (fmc_word_program(WriteAddr, *wData) != FMC_READY)
        {
            fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
            fmc_lock();
            return false;
        }
        WriteAddr += 4;
        wData++;
    }
    
    /* 数据不足4字节时,组装尾部字,未使用的字节填充0xFF保持Flash擦除态 */
    uint32_t len = wNum % 4;
    if (len != 0)
    {
        uint32_t data_temp = 0xFFFFFFFF;
        memcpy(&data_temp, wData, len);
        if (fmc_word_program(WriteAddr, data_temp) != FMC_READY)
        {
            fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
            fmc_lock();
            return false;
        }
    }
        
    fmc_flag_clear(FMC_FLAG_BANK0_END | FMC_FLAG_BANK0_WPERR | FMC_FLAG_BANK0_PGERR);
    /* 编程完成后上锁 Flash */
    fmc_lock();
    
    if (uPrint.tFlag.bOperFlash) 
    {
        sMyPrint("\r\nWrite complete!\n");
        sMyPrint("\r\n");
    }
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : Flash 读取函数 (按8位读取)
 * 说明(备注)  : 字节指针逐字节寻址读取
 * 传入参数    : ReadAddr: 起始地址, rData: 接收缓冲区, rNum: 字节长度
 * 输出参数    : rData: 接收数据缓冲区
 * 返回值      : 无
 ************************************************************************************************************************/
void vFlash_Gd32Read8Bit(uint32_t ReadAddr, uint8_t *rData, uint32_t rNum)
{
    uint32_t i;

    if (uPrint.tFlag.bOperFlash)
        sMyPrint("从地址0x%02X开始读取\r\n", ReadAddr);

    for (i = 0; i < rNum; i++)
    {
        rData[i] = *(__IO uint8_t*)ReadAddr;
        if (uPrint.tFlag.bOperFlash) 
            sMyPrint("0x%x  ", rData[i]);
        ReadAddr++;
    }

    if (uPrint.tFlag.bOperFlash)
        sMyPrint("读取完成\r\n");
}

/***********************************************************************************************************************
 * 函数功能    : Flash 读取函数 (按32位整字读取)
 * 说明(备注)  : 按 32 位整字读取并在尾部补足余数字节，解决非 4 字节倍数尾部截断问题
 * 传入参数    : ReadAddr: 开始地址, rData: 目标缓存, rNum: 字节长度
 * 输出参数    : rData: 目标缓存
 * 返回值      : 无
 ************************************************************************************************************************/
void vFlash_Gd32Read32Bit(uint32_t ReadAddr, uint32_t *rData, uint32_t rNum)
{
    uint32_t i;
    uint32_t words     = rNum / 4;
    uint32_t remainder = rNum % 4;

    if (uPrint.tFlag.bOperFlash)
        sMyPrint("\r\nRead data from 0x%02X, len=%d\r\n", ReadAddr, rNum);
    
    for (i = 0; i < words; i++)
    {
        rData[i] = *(__IO uint32_t*)ReadAddr;
        
        if (uPrint.tFlag.bOperFlash)
            sMyPrint("0x%x  ", rData[i]);
        
        ReadAddr += 4;
    }

    if (remainder != 0)
    {
        uint32_t last_word = *(__IO uint32_t*)ReadAddr;
        uint8_t *p_dst     = (uint8_t *)&rData[words];
        memcpy(p_dst, &last_word, remainder);

        if (uPrint.tFlag.bOperFlash)
            sMyPrint("0x%x (tail %d) ", last_word, remainder);
    }
    
    if (uPrint.tFlag.bOperFlash)
        sMyPrint("\r\nRead end\r\n");
}
