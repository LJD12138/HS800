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
#include <string.h>

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

//****************************************************Macros*******************************************************************//
#define TFT_SPI_TIMEOUT                    0x10000U

//****************************************************Parameter Initialization************************************************//
#if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
//DMA发送缓存
static u8 s_tft_dma_buf[DISP_TFT_WIDTH * 2];
#endif //boardDISP_SPI_MODE

//****************************************************Function Declaration****************************************************//
static void tft_delay_ms(vu16 ms);

#if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
static void tft_hw_spi_dma_init(void);
static void v_tft_spi_dma_send_byte(const u8 *data, u32 len);
static void tft_hw_spi_wait_idle(void);
#endif //boardDISP_SPI_MODE

static void tft_spi_send_byte(u8 data);

/***********************************************************************************************************************
 -----函数功能    TFT延时函数
 -----说明(备注)  毫秒级延时，支持FreeRTOS和裸机两种模式
 -----传入参数    ms:延时毫秒数
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void tft_delay_ms(vu16 ms)
{
    #if(boardUSE_OS)
    vTaskDelay(ms);
    #else
    while(ms--)
        tft_delay_us(1000);
    #endif  //boardUSE_OS
}

#if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
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
    rcu_periph_clock_enable(DISP_TFT_SPI_RCU);
    rcu_periph_clock_enable(RCU_DMA0);

    gpio_init(DISP_TFT_SCK_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, DISP_TFT_SCK_PIN);
    gpio_init(DISP_TFT_SDA_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, DISP_TFT_SDA_PIN);

    spi_i2s_deinit(DISP_TFT_SPI_PERIPH);
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.prescale = DISP_TFT_SPI_PRESCALE;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(DISP_TFT_SPI_PERIPH, &spi_init_struct);
    spi_nss_internal_high(DISP_TFT_SPI_PERIPH);
    spi_dma_enable(DISP_TFT_SPI_PERIPH, SPI_DMA_TRANSMIT);
    spi_enable(DISP_TFT_SPI_PERIPH);

    dma_deinit(DMA0, DMA_CH4);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.periph_addr = (uint32_t)(&SPI_DATA(DISP_TFT_SPI_PERIPH));
    dma_init_struct.memory_addr = (uint32_t)s_tft_dma_buf;
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.number = 1U;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init(DMA0, DMA_CH4, &dma_init_struct);
    dma_circulation_disable(DMA0, DMA_CH4);
    dma_memory_to_memory_disable(DMA0, DMA_CH4);
    dma_channel_disable(DMA0, DMA_CH4);
    dma_flag_clear(DMA0, DMA_CH4, DMA_FLAG_G);
}

/***********************************************************************************************************************
-----函数功能    SPI DMA发送数据
-----说明(备注)  使用DMA分块发送长数据
-----传入参数    data:数据指针  len:数据长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_spi_dma_send_byte(const u8 *data, u32 len)
{
    uint32_t timeout;
    u32 tx_len;
    u32 remaining = len;

    if((data == NULL) || (len == 0U))
        return;

    while(remaining > 0U)
    {
        tx_len = (remaining > sizeof(s_tft_dma_buf)) ? sizeof(s_tft_dma_buf) : remaining;
        memcpy(s_tft_dma_buf, data, tx_len);

        dma_channel_disable(DMA0, DMA_CH4);
        dma_memory_address_config(DMA0, DMA_CH4, (uint32_t)s_tft_dma_buf);
        dma_transfer_number_config(DMA0, DMA_CH4, tx_len);
        dma_flag_clear(DMA0, DMA_CH4, DMA_FLAG_G);
        dma_channel_enable(DMA0, DMA_CH4);

        timeout = TFT_SPI_TIMEOUT;
        while((RESET == dma_flag_get(DMA0, DMA_CH4, DMA_FLAG_FTF)) && (timeout > 0U))
        {
            timeout--;
        }

        dma_channel_disable(DMA0, DMA_CH4);
        dma_flag_clear(DMA0, DMA_CH4, DMA_FLAG_G);
        tft_hw_spi_wait_idle();

        if(timeout == 0U)
            return;

        data += tx_len;
        remaining -= tx_len;
    }
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
    while((SET == spi_i2s_flag_get(DISP_TFT_SPI_PERIPH, SPI_FLAG_TRANS)) && (timeout > 0U))
    {
        timeout--;
    }

    timeout = TFT_SPI_TIMEOUT;
    while((RESET == spi_i2s_flag_get(DISP_TFT_SPI_PERIPH, SPI_FLAG_TBE)) && (timeout > 0U))
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
static void tft_spi_send_byte(u8 data)
{
#if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_SW)
    for(u8 i = 0; i < 8; i++)
    {
        DISP_TFT_SCK_L();
        if(data & 0x80)
            DISP_TFT_SDA_H();
        else
            DISP_TFT_SDA_L();
        DISP_TFT_SCK_H();
        data <<= 1;
    }
#endif
}

/***********************************************************************************************************************
-----函数功能    显示接口硬件初始化
-----说明(备注)  完成TFT相关时钟、GPIO、SPI/DMA和复位时序配置
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_IfaceInit(void)
{
    rcu_periph_clock_enable(DISP_TFT_CS_RCU);
    rcu_periph_clock_enable(DISP_TFT_RES_RCU);
    rcu_periph_clock_enable(DISP_TFT_A0_RCU);
    rcu_periph_clock_enable(DISP_TFT_BL_RCU);
    rcu_periph_clock_enable(DISP_TFT_SCK_RCU);
    rcu_periph_clock_enable(DISP_TFT_SDA_RCU);

    gpio_init(DISP_TFT_CS_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, DISP_TFT_CS_PIN);
    gpio_init(DISP_TFT_RES_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, DISP_TFT_RES_PIN);
    gpio_init(DISP_TFT_A0_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, DISP_TFT_A0_PIN);
    gpio_init(DISP_TFT_BL_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, DISP_TFT_BL_PIN);

    #if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
    tft_hw_spi_dma_init();
    #else
    gpio_init(DISP_TFT_SCK_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, DISP_TFT_SCK_PIN);
    gpio_init(DISP_TFT_SDA_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, DISP_TFT_SDA_PIN);
    #endif

    DISP_TFT_CS_H();
    DISP_TFT_A0_H();
    DISP_TFT_BL_H();

    #if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_SW)
    DISP_TFT_SCK_H();
    DISP_TFT_SDA_H();
    #endif

    DISP_TFT_RES_L();
    tft_delay_ms(20);
    DISP_TFT_RES_H();
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

    #if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
    v_tft_spi_dma_send_byte(data, len);
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
        DISP_TFT_BL_H();
    else
        DISP_TFT_BL_L();
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
    DISP_TFT_A0_L();
    DISP_TFT_CS_L();
    
    #if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
    spi_i2s_data_transmit(DISP_TFT_SPI_PERIPH, cmd);
    while(RESET == spi_i2s_flag_get(DISP_TFT_SPI_PERIPH, SPI_FLAG_TBE));
    #else
    tft_spi_send_byte(cmd);
    #endif
    
    DISP_TFT_CS_H();
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
    DISP_TFT_A0_H();
    DISP_TFT_CS_L();
    
    #if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
    spi_i2s_data_transmit(DISP_TFT_SPI_PERIPH, data);
    while(RESET == spi_i2s_flag_get(DISP_TFT_SPI_PERIPH, SPI_FLAG_TBE));
    #else
    tft_spi_send_byte(data);
    #endif
    
    DISP_TFT_CS_H();
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
    DISP_TFT_A0_H();
    DISP_TFT_CS_L();
    
    #if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
    spi_i2s_data_transmit(DISP_TFT_SPI_PERIPH, (u8)(data >> 8));
    while(RESET == spi_i2s_flag_get(DISP_TFT_SPI_PERIPH, SPI_FLAG_TBE));
    spi_i2s_data_transmit(DISP_TFT_SPI_PERIPH, (u8)(data & 0xFF));
    while(RESET == spi_i2s_flag_get(DISP_TFT_SPI_PERIPH, SPI_FLAG_TBE));
    #else
    tft_spi_send_byte((u8)(data >> 8));
    tft_spi_send_byte((u8)(data & 0xFF));
    #endif
    
    DISP_TFT_CS_H();
}

/***********************************************************************************************************************
-----函数功能    写数据缓冲区到TFT
-----说明(备注)  使用SPI DMA或软件SPI批量发送数据
-----传入参数    data:数据指针  len:数据长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TftWriteBuffer(const u8 *data, u32 len)
{
    if((data == NULL) || (len == 0U))
        return;

    DISP_TFT_A0_H();
    DISP_TFT_CS_L();
    
    #if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
    v_tft_spi_dma_send_byte(data, len);
    #else
    for(u32 i = 0; i < len; i++)
        tft_spi_send_byte(data[i]);
    #endif
    
    DISP_TFT_CS_H();
}

/***********************************************************************************************************************
-----函数功能    设置显示窗口
-----说明(备注)  设置TFT的列地址和行地址
-----传入参数    x1:起始X坐标  y1:起始Y坐标  x2:结束X坐标  y2:结束Y坐标
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TftSetWindow(u16 x1, u16 y1, u16 x2, u16 y2)
{
    vDisp_TftWriteCommand(0x2A);    //列地址设置
    vDisp_TftWriteData16(x1);
    vDisp_TftWriteData16(x2);
    
    vDisp_TftWriteCommand(0x2B);    //行地址设置
    vDisp_TftWriteData16(y1);
    vDisp_TftWriteData16(y2);
    
    vDisp_TftWriteCommand(0x2C);    //开始写入显存
}

/***********************************************************************************************************************
-----函数功能    填充矩形区域
-----说明(备注)  用指定颜色填充矩形区域
-----传入参数    x:起始X坐标  y:起始Y坐标  w:宽度  h:高度  color:RGB565颜色
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TftFillRect(u16 x, u16 y, u16 w, u16 h, u16 color)
{
    u32 pixel_count = w * h;
    u8 color_h = (u8)(color >> 8);
    u8 color_l = (u8)(color & 0xFF);
    
    vDisp_TftSetWindow(x, y, x + w - 1, y + h - 1);
    
    DISP_TFT_A0_H();
    DISP_TFT_CS_L();
    
    #if(boardDISP_SPI_MODE == DISP_TFT_SPI_MODE_HW)
    //使用DMA发送颜色数据
    u32 remaining = pixel_count;
    u32 chunk_size;
    
    while(remaining > 0)
    {
        chunk_size = (remaining > (sizeof(s_tft_dma_buf) / 2)) ? (sizeof(s_tft_dma_buf) / 2) : remaining;
        
        for(u32 i = 0; i < chunk_size; i++)
        {
            s_tft_dma_buf[i * 2] = color_h;
            s_tft_dma_buf[i * 2 + 1] = color_l;
        }
        
        v_tft_spi_dma_send_byte(s_tft_dma_buf, chunk_size * 2);
        remaining -= chunk_size;
    }
    #else
    for(u32 i = 0; i < pixel_count; i++)
    {
        vDisp_TftWriteData8(color_h);
        vDisp_TftWriteData8(color_l);
    }
    #endif
    
    DISP_TFT_CS_H();
}

/***********************************************************************************************************************
-----函数功能    绘制RGB565位图
-----说明(备注)  在指定区域绘制RGB565格式的位图数据
-----传入参数    x1:起始X坐标  y1:起始Y坐标  x2:结束X坐标  y2:结束Y坐标  color_p:RGB565颜色数据指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TftDrawBitmapRgb565(u16 x1, u16 y1, u16 x2, u16 y2, const u16 *color_p)
{
    u32 pixel_count = (x2 - x1 + 1) * (y2 - y1 + 1);
    const u8 *data = (const u8 *)color_p;
    
    if((color_p == NULL) || (pixel_count == 0))
        return;
    
    vDisp_TftSetWindow(x1, y1, x2, y2);
    vDisp_TftWriteBuffer(data, pixel_count * 2);
}

#endif  //boardDISPLAY_EN