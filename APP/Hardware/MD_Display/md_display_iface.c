/***********************************************************************************************************************
 -----文件说明    TFT显示接口实现
 -----说明(备注)  TFT显示屏底层驱动接口，支持软件模拟SPI和硬件SPI+DMA两种传输模式
 -----文件版本    V1.0
 -----作者        HS800开发团队
 -----日期        2024
 ************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_iface.h"

#if(boardDISPLAY_EN)
#include "lv_port_disp.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

//****************************************************Macros*******************************************************************//
#define TFT_SPI_TIMEOUT                    0x10000U

//****************************************************Function Declaration****************************************************//
static void tft_delay_ms(vu16 ms);

#if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
static void tft_hw_spi_dma_init(void);
static void v_tft_spi_dma_send_bytes(const u8 *data, u32 len);
static void tft_hw_spi_wait_idle(void);
#endif //boardDISP_SPI_MODE

#if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_SW)
static void tft_spi_send_byte(u8 data);
#endif

/***********************************************************************************************************************
 -----函数功能    TFT延时函数
 -----说明(备注)  毫秒级延时，支持FreeRTOS和裸机两种模式
 -----传入参数    ms:延时毫秒数
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void tft_delay_ms(vu16 ms)
{
    while(ms--)
    {
        for(volatile u32 i = 0U; i < (SystemCoreClock / 8000000U); i++)
            __NOP();
    }
}

#if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
/***********************************************************************************************************************
 -----函数功能    SPI DMA接口初始化
 -----说明(备注)  配置SPI1和DMA作为TFT单向发送通道
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void tft_hw_spi_dma_init(void)
{
    spi_parameter_struct spi_init_struct;
    dma_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(dispTFT_SPI_RCU);
    rcu_periph_clock_enable(dispTFT_DMA_RCU);

    gpio_init(dispTFT_SCK_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, dispTFT_SCK_PIN);
    gpio_init(dispTFT_SDA_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, dispTFT_SDA_PIN);

    spi_i2s_deinit(dispTFT_SPI_PERIPH);
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.prescale = dispTFT_SPI_PRESCALE;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(dispTFT_SPI_PERIPH, &spi_init_struct);
    spi_nss_internal_high(dispTFT_SPI_PERIPH);
    spi_dma_enable(dispTFT_SPI_PERIPH, SPI_DMA_TRANSMIT);
    spi_enable(dispTFT_SPI_PERIPH);

    dma_deinit(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.periph_addr = (uint32_t)(&SPI_DATA(dispTFT_SPI_PERIPH));
    dma_init_struct.memory_addr = 0U;
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.number = 1U;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, &dma_init_struct);
    dma_circulation_disable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);
    dma_memory_to_memory_disable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);
    dma_channel_disable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);
    dma_flag_clear(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_FLAG_G);

    /* 不要全局开启 DMA 中断！ */
    // dma_interrupt_enable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_CHXCTL_FTFIE);
    nvic_irq_enable(dispTFT_DMA_TX_IRQ, 2, 0);
}


/***********************************************************************************************************************
-----传入参数    use_staging_buf:true-先拷贝到中转缓存再发  false-直接以源缓存作为DMA源
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_spi_dma_send_bytes(const u8 *data, u32 len)
{
    if((data == NULL) || (len == 0U))
        return;

    dma_channel_disable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);

    /* 明确关闭 DMA 完成中断，确保纯阻塞死等 */
    dma_interrupt_disable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_CHXCTL_FTFIE);

    dma_memory_address_config(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, (uint32_t)data);
    dma_transfer_number_config(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, len);
    dma_flag_clear(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_FLAG_G);

    dma_channel_enable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);

    /* 恢复阻塞等待：保证初始化指令和短参数完整发送 */
    uint32_t timeout = TFT_SPI_TIMEOUT;
    while((RESET == dma_flag_get(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_FLAG_FTF)) && (timeout > 0U))
        timeout--;

    dma_channel_disable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);
    dma_flag_clear(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_FLAG_G);
    
    /* 确保 SPI 移位完全结束 */
    tft_hw_spi_wait_idle(); 
}

/***********************************************************************************************************************
-----函数功能    等待SPI发送空闲
-----说明(备注)  等待SPI移位和发送缓存完成, 避免过早切换片选或DC引脚
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void tft_hw_spi_wait_idle(void)
{
    uint32_t timeout;

    timeout = TFT_SPI_TIMEOUT;
    while((SET == spi_i2s_flag_get(dispTFT_SPI_PERIPH, SPI_FLAG_TRANS)) && (timeout > 0U))
    {
        timeout--;
    }

    timeout = TFT_SPI_TIMEOUT;
    while((RESET == spi_i2s_flag_get(dispTFT_SPI_PERIPH, SPI_FLAG_TBE)) && (timeout > 0U))
    {
        timeout--;
    }
}
#endif

/***********************************************************************************************************************
-----函数功能    软件SPI发送单字节
-----说明(备注)  使用GPIO模拟SPI时序
-----传入参数    data:要发送的字节
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_SW)
static void tft_spi_send_byte(u8 data)
{
    for(u8 i = 0; i < 8; i++)
    {
        dispTFT_SCK_L();
        if(data & 0x80)
            dispTFT_SDA_H();
        else
            dispTFT_SDA_L();
        dispTFT_SCK_H();
        data <<= 1;
    }
}
#endif















/***********************************************************************************************************************
-----函数功能    显示接口硬件初始化
-----说明(备注)  完成TFT相关时钟、GPIO、SPI/DMA和复位时序配置
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_IfaceInit(void)
{
    rcu_periph_clock_enable(dispTFT_CS_RCU);
    rcu_periph_clock_enable(dispTFT_RES_RCU);
    rcu_periph_clock_enable(dispTFT_A0_RCU);
    rcu_periph_clock_enable(dispTFT_BL_RCU);
    rcu_periph_clock_enable(dispTFT_SCK_RCU);
    rcu_periph_clock_enable(dispTFT_SDA_RCU);

    gpio_init(dispTFT_CS_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispTFT_CS_PIN);
    gpio_init(dispTFT_RES_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispTFT_RES_PIN);
    gpio_init(dispTFT_A0_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispTFT_A0_PIN);
    gpio_init(dispTFT_BL_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispTFT_BL_PIN);

    #if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
    tft_hw_spi_dma_init();
    #else
    gpio_init(dispTFT_SCK_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispTFT_SCK_PIN);
    gpio_init(dispTFT_SDA_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dispTFT_SDA_PIN);
    #endif

    dispTFT_CS_H();
    dispTFT_A0_H();
    dispTFT_BL_L();

    #if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_SW)
    dispTFT_SCK_H();
    dispTFT_SDA_H();
    #endif

    dispTFT_RES_L();
    tft_delay_ms(20);
    dispTFT_RES_H();
    tft_delay_ms(120);
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
    
    #if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
    v_tft_spi_dma_send_bytes(data, len);
    #else
    for(u16 x = 0; x < len; x++)
    {
        tft_spi_send_byte(*data);
        data++;
    }
    #endif  //boardDISP_SPI_MODE
}

/***********************************************************************************************************************
-----函数功能    设置背光
-----说明(备注)  控制TFT背光开关
-----传入参数    on: true开启背光, false关闭背光
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TftSetBacklight(bool on)
{
    if(on)
        dispTFT_BL_H();
    else
        dispTFT_BL_L();
}

/***********************************************************************************************************************
-----函数功能    写命令到TFT
-----说明(备注)  向TFT写入单个命令字节
-----传入参数    cmd:命令字节
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TftWriteCommand(u8 cmd)
{
    dispTFT_A0_L();
    dispTFT_CS_L();
    
    #if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
    spi_i2s_data_transmit(dispTFT_SPI_PERIPH, cmd);
    while(RESET == spi_i2s_flag_get(dispTFT_SPI_PERIPH, SPI_FLAG_TBE));
    tft_hw_spi_wait_idle();
    #else
    tft_spi_send_byte(cmd);
    #endif
    
    dispTFT_CS_H();
}

/***********************************************************************************************************************
-----函数功能    写8位数据到TFT
-----说明(备注)  向TFT写入单个8位数据
-----传入参数    data:8位数据
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TftWriteData8(u8 data)
{
    dispTFT_A0_H();
    dispTFT_CS_L();
    
    #if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
    spi_i2s_data_transmit(dispTFT_SPI_PERIPH, data);
    while(RESET == spi_i2s_flag_get(dispTFT_SPI_PERIPH, SPI_FLAG_TBE));
    tft_hw_spi_wait_idle();
    #else
    tft_spi_send_byte(data);
    #endif
    
    dispTFT_CS_H();
}

/***********************************************************************************************************************
-----函数功能    写16位数据到TFT
-----说明(备注)  向TFT写入单个16位数据
-----传入参数    data:16位数据
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TftWriteData16(u16 data)
{
    dispTFT_A0_H();
    dispTFT_CS_L();
    
    #if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
    spi_i2s_data_transmit(dispTFT_SPI_PERIPH, (u8)(data >> 8));
    while(RESET == spi_i2s_flag_get(dispTFT_SPI_PERIPH, SPI_FLAG_TBE));
    spi_i2s_data_transmit(dispTFT_SPI_PERIPH, (u8)(data & 0xFF));
    while(RESET == spi_i2s_flag_get(dispTFT_SPI_PERIPH, SPI_FLAG_TBE));
    tft_hw_spi_wait_idle();
    #else
    tft_spi_send_byte((u8)(data >> 8));
    tft_spi_send_byte((u8)(data & 0xFF));
    #endif
    
    dispTFT_CS_H();
}

/***********************************************************************************************************************
-----函数功能    写数据缓冲区到TFT
-----说明(备注)  发送命令参数等通用字节流
-----传入参数    data:数据指针  len:数据长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TftWriteBuffer(const u8 *data, u32 len)
{
    if((data == NULL) || (len == 0U))
        return;

    dispTFT_A0_H();
    dispTFT_CS_L();
    
    #if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
    v_tft_spi_dma_send_bytes(data, len);
    #else
    {
        const u8 *byte_data = (const u8 *)data;
        u32 byte_len = len;

        for(u32 i = 0U; i < byte_len; i++)
            tft_spi_send_byte(byte_data[i]);
    }
    #endif
    
    dispTFT_CS_H();
}

/***********************************************************************************************************************
-----函数功能    异步发送颜色数据 (专供 LVGL 刷屏使用)
-----说明(备注)  开启 DMA 后立刻返回，片选拉高交由 DMA 中断处理
************************************************************************************************************************/
void vDisp_TftWriteColorAsync(const u8 *data, u32 len)
{
    if((data == NULL) || (len == 0U))
        return;

    dispTFT_A0_H();
    dispTFT_CS_L();
    
    #if(boardDISP_SPI_MODE == dispTFT_SPI_MODE_HW)
    dma_channel_disable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);
    dma_memory_address_config(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, (uint32_t)data);
    dma_transfer_number_config(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, len);
    dma_flag_clear(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_FLAG_G);

    /* 专属异步通道，动态开启 DMA 中断！ */
    dma_interrupt_enable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_CHXCTL_FTFIE);
    
    dma_channel_enable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);
    #endif
}

/***********************************************************************************************************************
-----函数功能    DMA0 通道4中断处理函数
-----说明(备注)  处理DMA传输完成的中断
-----日期        2026-05-29
************************************************************************************************************************/
void DMA0_Channel4_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(dma_interrupt_flag_get(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_INT_FLAG_FTF))
    {
        /* 触发后立刻关闭中断使能，用完即弃，不干扰别人 */
        dma_interrupt_disable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_CHXCTL_FTFIE);

        /* 必须明确清除 FTF 标志位，只清 G 标志可能不够 */
        dma_interrupt_flag_clear(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_INT_FLAG_G);
        dma_interrupt_flag_clear(dispTFT_DMA_PERIPH, dispTFT_DMA_CH, DMA_INT_FLAG_FTF);

        // 1. 关闭 DMA
        dma_channel_disable(dispTFT_DMA_PERIPH, dispTFT_DMA_CH);

        // 2. 致命修复：死等 SPI 移位寄存器把最后几个 bit 发送完毕
        while(RESET == spi_i2s_flag_get(dispTFT_SPI_PERIPH, SPI_FLAG_TBE));
        while(SET == spi_i2s_flag_get(dispTFT_SPI_PERIPH, SPI_FLAG_TRANS));

        // 3. 数据真正发完后，安全拉高片选
        dispTFT_CS_H();

        // 4. 释放总线信号量
        if(DispSemaphoreBinary != NULL) {
            xSemaphoreGiveFromISR(DispSemaphoreBinary, &xHigherPriorityTaskWoken);
        }

        // 5. 通知 LVGL 可以开始渲染下一块缓冲了
        if(disp) {
            lv_display_flush_ready(disp);
        }

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

#endif  //boardDISPLAY_EN
