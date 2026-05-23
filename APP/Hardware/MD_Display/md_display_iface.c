/*******************************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\2-User_Projects\3-SmartFishJar\1.software\SmartFishJar\APP\Hardware\MD_Display
 * File    : md_display_iface.c
 * Date    : 2026-04-29 11:11:29
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_iface.h"

#if(boardDISPLAY_EN)
#include <string.h>
#include "MD_Display/md_display_api.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

//****************************************************Macros*******************************************************************//



//****************************************************Parameter Initialization************************************************//
#if(boardDISP_SPI_MODE == 1)
//DMA发送缓存, 按OLED页宽分块发送
static u8 s_oled_dma_page_buf[OLED_WIDTH_PIXELS];
#endif //boardDISP_SPI_MODE


//****************************************************Function Declaration****************************************************//
static void oled_delay_ms(vu16 ms);

#if(boardDISP_SPI_MODE == 1)
static void oled_hw_spi_dma_init(void);
static void v_disp_spi_dma_send_byte(const u8 *data, u16 len);
static void oled_hw_spi_wait_idle(void);
#endif //boardDISP_SPI_MODE



/***********************************************************************************************************************
-----函数功能   OLED延时函数
-----传入参数   us
-----作者       LJD
-----日期       2026-04-29
************************************************************************************************************************/
static void oled_delay_ms(vu16 ms)
{
    #if(boardUSE_OS)
    vTaskDelay(ms);
    #else
    while(ms--)
        oled_delay_us(1000);
    #endif  //boardUSE_OS
}

/***********************************************************************************************************************
-----函数功能   SPI DMA接口初始化
-----作者       LJD
-----日期       2026-04-29
************************************************************************************************************************/
#if(boardDISP_SPI_MODE == 1)
/***********************************************************************************************************************
-----函数功能    SPI DMA接口初始化
-----说明(备注)  配置SPI1和DMA0 CH4作为OLED单向发送通道
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void oled_hw_spi_dma_init(void)
{
    spi_parameter_struct spi_init_struct;
    dma_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(dispOLED_SPI_RCU);
    rcu_periph_clock_enable(dispOLED_SPI_DMA_RCU);

    gpio_init(dispOLED_SCK_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, dispOLED_SCK_PIN);
    gpio_init(dispOLED_MOSI_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, dispOLED_MOSI_PIN);

    spi_i2s_deinit(dispOLED_SPI_PERIPH);
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.trans_mode = SPI_TRANSMODE_BDTRANSMIT;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.prescale = dispOLED_SPI_PRESCALE;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(dispOLED_SPI_PERIPH, &spi_init_struct);
    spi_nss_internal_high(dispOLED_SPI_PERIPH);
    spi_dma_enable(dispOLED_SPI_PERIPH, SPI_DMA_TRANSMIT);
    spi_enable(dispOLED_SPI_PERIPH);

    dma_deinit(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.periph_addr = (uint32_t)(&SPI_DATA(dispOLED_SPI_PERIPH));
    dma_init_struct.memory_addr = (uint32_t)s_oled_dma_page_buf;
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.number = 1U;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH, &dma_init_struct);
    dma_circulation_disable(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH);
    dma_memory_to_memory_disable(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH);
    dma_channel_disable(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH);
    dma_flag_clear(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH, DMA_FLAG_G);
}

/***********************************************************************************************************************
-----函数功能    SPI DMA发送数据
-----说明(备注)  使用固定页缓存分块发送长数据, 每块发送完成后再继续
-----传入参数    data:数据指针  len:数据长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_spi_dma_send_byte(const u8 *data, u16 len)
{
    uint32_t timeout;
    u16 tx_len;

    if((data == NULL) || (len == 0U))
        return;

    while(len > 0U)
    {
        tx_len = (len > OLED_WIDTH_PIXELS) ? OLED_WIDTH_PIXELS : len;
        memcpy(s_oled_dma_page_buf, data, tx_len);

        dma_channel_disable(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH);
        dma_memory_address_config(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH, (uint32_t)s_oled_dma_page_buf);
        dma_transfer_number_config(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH, tx_len);
        dma_flag_clear(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH, DMA_FLAG_G);
        dma_channel_enable(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH);

        timeout = OLED_SPI_TIMEOUT;
        while((RESET == dma_flag_get(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH, DMA_FLAG_FTF)) && (timeout > 0U))
        {
            timeout--;
        }

        dma_channel_disable(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH);
        dma_flag_clear(dispOLED_SPI_DMA_PERIPH, dispOLED_SPI_DMA_TX_CH, DMA_FLAG_G);
        oled_hw_spi_wait_idle();

        if(timeout == 0U)
            return;

        data += tx_len;
        len = (u16)(len - tx_len);
    }
}

/***********************************************************************************************************************
-----函数功能    等待SPI发送空闲
-----说明(备注)  等待SPI移位和发送缓存完成, 避免过早切换片选或DC引脚
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void oled_hw_spi_wait_idle(void)
{
    uint32_t timeout;

    timeout = OLED_SPI_TIMEOUT;
    while((SET == spi_i2s_flag_get(dispOLED_SPI_PERIPH, SPI_FLAG_TRANS)) && (timeout > 0U))
    {
        timeout--;
    }

    timeout = OLED_SPI_TIMEOUT;
    while((RESET == spi_i2s_flag_get(dispOLED_SPI_PERIPH, SPI_FLAG_TBE)) && (timeout > 0U))
    {
        timeout--;
    }
}
#endif







/***********************************************************************************************************************
-----函数功能    显示接口硬件初始化
-----说明(备注)  完成OLED相关时钟、GPIO、SPI/DMA和复位时序配置
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_IfaceInit(void)
{
    rcu_periph_clock_enable(dispOLED_NSS_RCU);
    rcu_periph_clock_enable(dispOLED_RES_RCU);
    rcu_periph_clock_enable(dispOLED_DC_RCU);
    rcu_periph_clock_enable(dispOLED_SCK_RCU);
    rcu_periph_clock_enable(dispOLED_MOSI_RCU);

    gpio_init(dispOLED_NSS_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispOLED_NSS_PIN);
    gpio_init(dispOLED_RES_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispOLED_RES_PIN);
    gpio_init(dispOLED_DC_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispOLED_DC_PIN);

    #if(boardDISP_SPI_MODE == 1)
    oled_hw_spi_dma_init();
    #else
    gpio_init(dispOLED_SCK_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispOLED_SCK_PIN);
    gpio_init(dispOLED_MOSI_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispOLED_MOSI_PIN);
    #endif

    dispOLED_NSS_H();
    dispOLED_DC_H();

    #if(boardDISP_SPI_MODE == 0)
    dispOLED_SCK_H();
    dispOLED_MOSI_H();
    #endif

    dispOLED_RES_L();
    oled_delay_ms(20);
    dispOLED_RES_H();
    oled_delay_ms(200);
}

/***********************************************************************************************************************
-----函数功能    SPI发送数据
-----说明(备注)  根据配置选择硬件DMA SPI或软件模拟SPI发送原始字节流
-----传入参数    data:数据指针  len:数据长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_SpiSendByte(const u8 *data, u16 len)
{
    if((data == NULL) || (len == 0U))
        return;

#if(boardDISP_SPI_MODE == 1)
    v_disp_spi_dma_send_byte(data, len);
    #else
    u8 temp = 0;
    for(int x = 0; x < len; x++)
    {
        temp = *data;
        for(int i = 0; i < 8U; i++)
        {
            dispOLED_SCK_L();
            if((temp & 0x80U) != 0U)
                dispOLED_MOSI_H();
            else
                dispOLED_MOSI_L();

            dispOLED_SCK_H();
            temp <<= 1;
        }
        data++;
    }
    #endif  //boardDISP_SPI_MODE
}

/***********************************************************************************************************************
-----函数功能    向OLED写入数据
-----说明(备注)  根据命令/数据模式设置DC和片选, 完成一次OLED写入
-----传入参数    data:数据指针  len:数据长度  mode:命令/数据模式
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_OledWriteByte(const u8 *data, u16 len, u8 mode)
{
    if((data == NULL) || (len == 0U))
        return;

    if(mode == OLED_DATA)
        dispOLED_DC_H();
    else
        dispOLED_DC_L();

    dispOLED_NSS_L();

    vDisp_SpiSendByte(data, len);

    dispOLED_NSS_H();
    dispOLED_DC_H();
}

#endif  //boardDISPLAY_EN
