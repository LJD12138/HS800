/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
 * File    : md_bms_iface.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS通信物理接口驱动实现(串口/DMA/RS485收发控制)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_iface.h"

#if (boardBMS_IFACE && boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "lwrb.h"

#if (boardBMS_485_IFACE_EN)
#if (boardUSE_OS)
#include "timer_task.h"
#else
#include "systick.h"
#endif  /* boardUSE_OS */
#endif  /* boardBMS_485_IFACE_EN */

//****************************************************Macros********************************************************************//
#define			bmsRX_DMA_BUFF_SIZE						256
#define			bmsTX_DMA_BUFF_SIZE						256		/* DMA 数组大小 */

// 使能RS485硬件发送完成(TC)中断换向补丁
#define     	boardRS485_HARDWARE_TC_PATCH_EN          1
//<i> 1:开启USART硬件TC中断自动切换RS485接收方向
//<i> 0:关闭硬件补丁，使用原有软件定时器延时模式
//-------------------------------------------------------------------

//****************************************************Parameter Initialization**************************************************//
/* 0:MPPT 1:BMS */
__IO bool bBmsUseFlag = true;

static vu16 s_us_data_send_size = 0;
static vu16 s_us_data_send_cnt  = 0;

#if (boardBMS_IFACE_DMA_EN)
static __ALIGNED(4) uint8_t s_uca_bms_rx_dma_buff[bmsRX_DMA_BUFF_SIZE];
#endif  /* boardBMS_IFACE_DMA_EN */
static __ALIGNED(4) uint8_t s_uca_bms_tx_dma_buff[bmsTX_DMA_BUFF_SIZE];

//****************************************************Function Declaration******************************************************//
static void v_bms_io_init(void);
static void v_bms_usart_init(void);
#if (boardBMS_IFACE_DMA_EN)
static void v_bms_dma_init(void);
#endif  /* boardBMS_IFACE_DMA_EN */


/***********************************************************************************************************************
 * 函数功能    : 相关 IO 初始化
 * 说明(备注)  : 配置 USART TX/RX 复用引脚及 RS485 控制引脚
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_bms_io_init(void)
{
	rcu_periph_clock_enable(RCU_AF);

	/* enable COM GPIO clock */
	rcu_periph_clock_enable(bmsUSART_GPIO_TX_RCU);
    /* connect port to USARTx_Tx */
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_af_set(bmsUSART_GPIO_TX_PORT, bmsUSART_GPIO_TX_AF, bmsUSART_GPIO_TX_PIN);
    gpio_mode_set(bmsUSART_GPIO_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, bmsUSART_GPIO_TX_PIN);
    gpio_output_options_set(bmsUSART_GPIO_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, bmsUSART_GPIO_TX_PIN);
	#else
	gpio_init(bmsUSART_GPIO_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, bmsUSART_GPIO_TX_PIN);
	#endif  /* boardIC_TYPE */
	
	/* enable COM GPIO clock */
	rcu_periph_clock_enable(bmsUSART_GPIO_RX_RCU);
    /* connect port to USARTx_Rx */
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_af_set(bmsUSART_GPIO_RX_PORT, bmsUSART_GPIO_RX_AF, bmsUSART_GPIO_RX_PIN);
    gpio_mode_set(bmsUSART_GPIO_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, bmsUSART_GPIO_RX_PIN);
	gpio_output_options_set(bmsUSART_GPIO_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, bmsUSART_GPIO_RX_PIN);
	#else
	gpio_init(bmsUSART_GPIO_RX_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, bmsUSART_GPIO_RX_PIN);
	#endif  /* boardIC_TYPE */
	
	#if (boardBMS_485_IFACE_EN)
	rcu_periph_clock_enable(bmsGPIO_485_TX_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(bmsGPIO_485_TX_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, bmsGPIO_485_TX_EN_PIN);
	gpio_output_options_set(bmsGPIO_485_TX_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, bmsGPIO_485_TX_EN_PIN);
	#else
	gpio_init(bmsGPIO_485_TX_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, bmsGPIO_485_TX_EN_PIN);
	#endif  /* boardIC_TYPE */
	bmsGPIO_485_TX_EN_OFF();                                        /* 默认处于接收模式 */
	#endif  /* boardBMS_485_IFACE_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 串口配置
 * 说明(备注)  : 初始化串口时钟、波特率与中断
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_bms_usart_init(void)
{
	rcu_periph_clock_enable(bmsUSART_RCU);
	nvic_irq_enable(bmsUSART_IRQ, 2, 0);

	usart_deinit(bmsUSART);
	usart_baudrate_set(bmsUSART, bmsUSART_BAUD);
	usart_word_length_set(bmsUSART, USART_WL_8BIT);
	usart_stop_bit_set(bmsUSART, USART_STB_1BIT);
	usart_parity_config(bmsUSART, USART_PM_NONE);
	usart_hardware_flow_rts_config(bmsUSART, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(bmsUSART, USART_CTS_DISABLE);

	usart_receive_config(bmsUSART, USART_RECEIVE_ENABLE);
	usart_transmit_config(bmsUSART, USART_TRANSMIT_ENABLE);

	#if (!boardBMS_IFACE_DMA_EN)
	usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_RBNE);
	usart_interrupt_enable(bmsUSART, USART_INT_RBNE);

	usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_TBE);
	usart_interrupt_disable(bmsUSART, USART_INT_TBE);
	#endif  /* !boardBMS_IFACE_DMA_EN */

	usart_enable(bmsUSART);
}



/***********************************************************************************************************************
 * 函数功能    : DMA 初始化
 * 说明(备注)  : 配置 USART 的 TX 和 RX DMA 通道
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
#if(boardBMS_IFACE_DMA_EN)
static void v_bms_dma_init(void)
{
	dma_parameter_struct dma_init_struct;

	nvic_irq_enable(bmsUSART_DMA_TX_IRQ, 2, 0);
	rcu_periph_clock_enable(bmsUSART_DMA_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	rcu_periph_clock_enable(RCU_DMAMUX);
	#endif /* boardIC_TYPE */
	
	/* initialize DMA channel(USART TX) */
	dma_deinit(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
	dma_struct_para_init(&dma_init_struct);

	#if (boardIC_TYPE == boardIC_GD32F50X)
	dma_init_struct.request = bmsUSART_DMA_TX_REQUEST;
	#endif	//boardIC_GD32F50X
    dma_init_struct.direction    = DMA_MEMORY_TO_PERIPHERAL;            /* 外设到内存 */              
    dma_init_struct.memory_addr  = (uint32_t)s_uca_bms_tx_dma_buff;       /* 设置内存接收基地址 */
    dma_init_struct.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;          /* 内存地址递增 */
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;               /* 8位内存数据 */
    dma_init_struct.number       = 0;  									/* Buff数组的大小 */
    dma_init_struct.periph_addr  = (uint32_t)(&USART_DATA(bmsUSART));  	/* 外设基地址,USART数据寄存器地址 */
    dma_init_struct.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;         /* 外设地址不递增 */
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;           /* 8位外设数据 */
    dma_init_struct.priority     = DMA_PRIORITY_ULTRA_HIGH;             /* 最高DMA通道优先级 */
    dma_init(bmsUSART_DMA, bmsUSART_DMA_TX_CH, &dma_init_struct);
     
	/* initialize DMA channel(USART RX) */
	dma_deinit(bmsUSART_DMA, bmsUSART_DMA_RX_CH);

	#if (boardIC_TYPE == boardIC_GD32F50X)
	dma_init_struct.request = bmsUSART_DMA_RX_REQUEST;
	#endif  //boardIC_GD32F50X
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.number = bmsRX_DMA_BUFF_SIZE;
    dma_init_struct.memory_addr = (uint32_t)s_uca_bms_rx_dma_buff;
    dma_init(bmsUSART_DMA, bmsUSART_DMA_RX_CH, &dma_init_struct);
	
	dma_circulation_disable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);			/* 关闭DMA_TX循环模式 */
	dma_memory_to_memory_disable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);		/* DMA内存到内存模式不开启 */
	dma_circulation_disable(bmsUSART_DMA, bmsUSART_DMA_RX_CH);			/* 关闭DMA_RX循环模式 */
    dma_memory_to_memory_disable(bmsUSART_DMA, bmsUSART_DMA_RX_CH);		/* DMA内存到内存模式不开启 */
	
	/* enable USART DMA for reception */
	#if (boardIC_TYPE == boardIC_GD32F30X)
	usart_dma_receive_config(bmsUSART, USART_RECEIVE_DMA_ENABLE);
    /* enable DMA0 channel4 transfer complete interrupt */
//    dma_interrupt_enable(bmsUSART_DMA, bmsUSART_DMA_RX_CH, DMA_INT_FTF);
	#elif (boardIC_TYPE == boardIC_GD32F50X)
	usart_dma_receive_config(bmsUSART, USART_DENR_ENABLE);
	#endif  /* boardIC_TYPE */
	dma_channel_enable(bmsUSART_DMA, bmsUSART_DMA_RX_CH);

	/* enable USART DMA for transmission */
	#if (boardIC_TYPE == boardIC_GD32F30X)
    usart_dma_transmit_config(bmsUSART,USART_TRANSMIT_DMA_ENABLE);
	#elif (boardIC_TYPE == boardIC_GD32F50X)
	usart_dma_transmit_config(bmsUSART, USART_DENT_ENABLE);
	#endif  /* boardIC_TYPE */
	dma_interrupt_enable(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_INT_FTF);
	dma_channel_disable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);

	/* 串口空闲中断 */
	usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_IDLE);
	usart_interrupt_enable(bmsUSART, USART_INT_IDLE);
}
#endif  /* boardBMS_IFACE_DMA_EN */

/***********************************************************************************************************************
 * 函数功能    : 串口初始化
 * 说明(备注)  : 初始化 IO、串口与 DMA
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBms_IfaceInit(void)
{
	v_bms_io_init();
	v_bms_usart_init();
	#if (boardBMS_IFACE_DMA_EN)
	v_bms_dma_init();
	#endif  /* boardBMS_IFACE_DMA_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 串口去初始化
 * 说明(备注)  : 复位串口及 DMA 通道
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBms_IfaceDeInit(void)
{
	usart_deinit(bmsUSART);

	#if (boardBMS_IFACE_DMA_EN)
	dma_deinit(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
	dma_deinit(bmsUSART_DMA, bmsUSART_DMA_RX_CH);
	#endif  /* boardBMS_IFACE_DMA_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 串口发送数据启动
 * 说明(备注)  : 支持 DMA 与中断方式发送报文
 * 传入参数    : p_data: 发送数据地址, us_len: 数据长度
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bBms_DataSendStart(uint8_t *p_data, uint16_t us_len)
{
	if (p_data == NULL || us_len == 0)
		return false;

	if (us_len > bmsTX_DMA_BUFF_SIZE)
		return false;    /* 超长拒绝,严禁截断发送残帧 */

	#if (boardBMS_IFACE_DMA_EN)
	/* 上一帧DMA尚未发完(含FTF中断未及时处理),禁止重入破坏在途数据 */
	if (DMA_CHCTL(bmsUSART_DMA, bmsUSART_DMA_TX_CH) & DMA_CHXCTL_CHEN)
		return false;
	#endif  /* boardBMS_IFACE_DMA_EN */

	#if (boardBMS_485_IFACE_EN)
	vBms_485TransEnable(true);
	#endif  /* boardBMS_485_IFACE_EN */

	memcpy(s_uca_bms_tx_dma_buff, p_data, us_len);

	#if (boardBMS_IFACE_DMA_EN)
	dma_flag_clear(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_FLAG_FTF);
	dma_memory_address_config(bmsUSART_DMA, bmsUSART_DMA_TX_CH, (uint32_t)s_uca_bms_tx_dma_buff);
	dma_transfer_number_config(bmsUSART_DMA, bmsUSART_DMA_TX_CH, us_len);
	dma_channel_enable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
	return true;
	#else
	if (s_us_data_send_cnt)
		return false;

	s_us_data_send_size = us_len;
	s_us_data_send_cnt  = 0;
	usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_TBE);
	usart_interrupt_enable(bmsUSART, USART_INT_TBE);
	return true;
	#endif  /* boardBMS_IFACE_DMA_EN */
}

/***********************************************************************************************************************
 * 函数功能    : RS485 发送使能切换
 * 说明(备注)  : 控制 RS485 收发方向
 * 传入参数    : b_en: true-发送模式, false-接收模式
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
#if(boardBMS_485_IFACE_EN)
void vBms_485TransEnable(bool b_en)
{
	if (b_en)
		bmsGPIO_485_TX_EN_ON();
	else
	{
		bmsGPIO_485_TX_EN_OFF();
		#if (!boardUSE_OS)
		bSysTick_BmsSendFinish = false;
		#endif  /* !boardUSE_OS */
	}
}
#endif  /* boardBMS_485_IFACE_EN */

#if (boardBMS_IFACE_DMA_EN)
/***********************************************************************************************************************
 * 函数功能    : DMA 发送完成中断服务函数
 * 说明(备注)  : 发送完成后关闭通道并切换接收模式
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void bmsUSART_DMA_TX_IRQ_HANDLER(void)
{
    if (dma_interrupt_flag_get(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_INT_FLAG_FTF)) 
	{
		#if (boardIC_TYPE == boardIC_GD32F30X)
        dma_interrupt_flag_clear(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_INT_FLAG_G);
		#elif (boardIC_TYPE == boardIC_GD32F50X)
		dma_interrupt_flag_clear(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_INT_FLAG_GIF);
		#endif  //boardIC_GD32F30X

		/* 关闭DMA发送 */
	    dma_channel_disable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
		/* 发送完成 */
		s_us_data_send_size = 0;
		
		/* 使用TC中断来关闭RS485收发 */
		#if (boardRS485_HARDWARE_TC_PATCH_EN && boardBMS_485_IFACE_EN)
		usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_TC);
		usart_interrupt_enable(bmsUSART, USART_INT_TC);
		#else
		/* 使用延时来关闭RS485收发 */
		#if (boardBMS_485_IFACE_EN)
		#if (boardUSE_OS)
		xTimerResetFromISR(tBmsRxEnTimer, 0);
		#else
		bSysTick_BmsSendFinish = true;
		#endif  /* boardUSE_OS */
		#endif  /* boardBMS_485_IFACE_EN */
		#endif  /* boardRS485_HARDWARE_TC_PATCH_EN */
    }
}

static u16 us_read_buff_len = 0;
/***********************************************************************************************************************
 * 函数功能    : 串口接收完成中断
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void bmsUSART_IRQ_HANDLER(void)
{
	#if (boardUSE_OS)
	BaseType_t x_woken = pdFALSE;	/* 高优先级任务唤醒标记 */
	#endif  /* boardUSE_OS */

	/* 使用TC中断来关闭RS485收发 */
	#if (boardRS485_HARDWARE_TC_PATCH_EN && boardBMS_485_IFACE_EN)
	if (RESET != usart_interrupt_flag_get(bmsUSART, USART_INT_FLAG_TC))
	{
		usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_TC);
		usart_interrupt_disable(bmsUSART, USART_INT_TC);
		bmsGPIO_485_TX_EN_OFF();  /* 最后一个停止位完全发出，纳秒级瞬间切回接收模式 */
	}
	#endif  /* boardRS485_HARDWARE_TC_PATCH_EN */

    if (RESET != usart_interrupt_flag_get(bmsUSART, USART_INT_FLAG_IDLE)) 
	{
		/* 清除中断 */
        usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_IDLE);
		/* 清除空闲标志位 */
		usart_data_receive(bmsUSART);
		/* 关闭DMA传输 */
		dma_channel_disable(bmsUSART_DMA, bmsUSART_DMA_RX_CH); 
		
		/* 获取接收到的数据长度，单位：字节 */
		us_read_buff_len = bmsRX_DMA_BUFF_SIZE - dma_transfer_number_get(bmsUSART_DMA, bmsUSART_DMA_RX_CH);

		/* 转存数据到待处理数据缓冲区 */
		if (bBmsUseFlag == true)
		{
			if (us_read_buff_len && tpBmsProtoRx != NULL)
				lwrb_write(&tpBmsProtoRx->tRxBuff, s_uca_bms_rx_dma_buff, us_read_buff_len);

			#if (boardUSE_OS)
			vTaskNotifyGiveFromISR(tBmsRecTaskHandle, &x_woken);
			#endif  /* boardUSE_OS */
		}

		/* 重新设置DMA传输 */
		dma_transfer_number_config(bmsUSART_DMA, bmsUSART_DMA_RX_CH, bmsRX_DMA_BUFF_SIZE);
		dma_channel_enable(bmsUSART_DMA, bmsUSART_DMA_RX_CH);    
    }

	#if (boardUSE_OS)
	portYIELD_FROM_ISR(x_woken);	/* 立即切换到被唤醒的接收任务，不再等待下一个 tick */
	#endif  /* boardUSE_OS */
}
#else
/***********************************************************************************************************************
 * 函数功能    : 串口中断函数
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void bmsUSART_IRQ_HANDLER(void)
{
	#if (boardUSE_OS)
	BaseType_t x_woken = pdFALSE;	/* 高优先级任务唤醒标记 */
	#endif  /* boardUSE_OS */

    if (RESET != usart_interrupt_flag_get(bmsUSART, USART_INT_FLAG_RBNE))
    {
		if (tpBmsProtoRx != NULL)
		{
			u8 ucData = (u8)USART_DATA(bmsUSART);
			lwrb_write(&tpBmsProtoRx->tRxBuff, &ucData, 1);
		}

        usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_RBNE); /* 清除串口接收中断 */

		#if (boardUSE_OS)
        vTaskNotifyGiveFromISR(tBmsRecTaskHandle, &x_woken);
		#endif  /* boardUSE_OS */
    }

    if (RESET != usart_interrupt_flag_get(bmsUSART, USART_INT_FLAG_TBE))
    {
        usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_TBE);
        
        if (s_us_data_send_cnt < s_us_data_send_size)
        {    
            USART_DATA(bmsUSART) = s_uca_bms_tx_dma_buff[s_us_data_send_cnt];
			s_us_data_send_cnt++;
        }
        else
        {
            usart_interrupt_disable(bmsUSART, USART_INT_TBE);
            s_us_data_send_cnt = 0;
            s_us_data_send_size = 0;
			
			#if (boardBMS_485_IFACE_EN)
			#if (boardUSE_OS)
			xTimerResetFromISR(tBmsRxEnTimer, 0);
			#else
			bSysTick_BmsSendFinish = true;
			#endif  /* boardUSE_OS */
			#endif  /* boardBMS_485_IFACE_EN */
        }               
    }    

	#if (boardUSE_OS)
	portYIELD_FROM_ISR(x_woken);	/* 立即切换到被唤醒的接收任务，不再等待下一个 tick */
	#endif  /* boardUSE_OS */
}
#endif  /* boardBMS_IFACE_DMA_EN */

#endif  /* (boardBMS_IFACE && boardBMS_EN) */

