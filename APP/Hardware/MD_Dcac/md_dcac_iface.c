/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_iface.c
 * Date    : 2026-09-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器通信硬件接口与驱动实现(串口/DMA/RS485收发控制)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Dcac/md_dcac_iface.h"

#if (boardDCAC_IFACE && boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Print/print_task.h"

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#include "Sys/sys_task.h"
#endif  /* boardUPDATE */

#include "lwrb.h"

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#endif  /* boardMPPT_EN */

#if (boardDCAC_485_IFACE_EN)
#if (boardUSE_OS)
#include "timer_task.h"
#else
#include "systick.h"
#endif  /* boardUSE_OS */
#endif  /* boardDCAC_485_IFACE_EN */

//****************************************************Macros********************************************************************//
#define			dcacRX_DMA_BUFF_SIZE					256
#define			dcacTX_DMA_BUFF_SIZE					256		/* DMA 数组大小 */

// 使能RS485硬件发送完成(TC)中断换向补丁
#define     	boardRS485_HARDWARE_TC_PATCH_EN          1
//<i> 1:开启USART硬件TC中断自动切换RS485接收方向
//<i> 0:关闭硬件补丁，使用原有软件定时器延时模式
//-------------------------------------------------------------------

//****************************************************Parameter Initialization**************************************************//
/* 0:MPPT 1:DCAC */
__IO bool bDcacUseFlag = true;

static vu16 s_us_data_send_size = 0;
static vu16 s_us_data_send_cnt  = 0;

#if (boardDCAC_IFACE_DMA_EN)
static __ALIGNED(4) uint8_t s_uca_dcac_rx_dma_buff[dcacRX_DMA_BUFF_SIZE];
#endif  /* boardDCAC_IFACE_DMA_EN */
static __ALIGNED(4) uint8_t s_uca_dcac_tx_dma_buff[dcacTX_DMA_BUFF_SIZE];

//****************************************************Function Declaration******************************************************//
static void v_dcac_io_init(void);
static void v_dcac_usart_config(uint32_t ul_baud);
#if (boardDCAC_IFACE_DMA_EN)
static void v_dcac_dma_init(void);
#endif  /* boardDCAC_IFACE_DMA_EN */


/***********************************************************************************************************************
 * 函数功能    : 逆变串口相关 IO 初始化
 * 说明(备注)  : 配置 USART TX/RX 复用引脚、逆变电源引脚及 RS485 控制引脚
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_dcac_io_init(void)
{
	rcu_periph_clock_enable(RCU_AF);

	/* enable COM GPIO clock & TX */
	rcu_periph_clock_enable(dcacUSART_GPIO_TX_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_af_set(dcacUSART_GPIO_TX_PORT, dcacUSART_GPIO_TX_AF, dcacUSART_GPIO_TX_PIN);
	gpio_mode_set(dcacUSART_GPIO_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, dcacUSART_GPIO_TX_PIN);
	gpio_output_options_set(dcacUSART_GPIO_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, dcacUSART_GPIO_TX_PIN);
	#else
	gpio_init(dcacUSART_GPIO_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, dcacUSART_GPIO_TX_PIN);
	#endif  /* boardIC_TYPE */

	/* enable COM GPIO clock & RX */
	rcu_periph_clock_enable(dcacUSART_GPIO_RX_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_af_set(dcacUSART_GPIO_RX_PORT, dcacUSART_GPIO_RX_AF, dcacUSART_GPIO_RX_PIN);
	gpio_mode_set(dcacUSART_GPIO_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, dcacUSART_GPIO_RX_PIN);
	gpio_output_options_set(dcacUSART_GPIO_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, dcacUSART_GPIO_RX_PIN);
	#else
	gpio_init(dcacUSART_GPIO_RX_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, dcacUSART_GPIO_RX_PIN);
	#endif  /* boardIC_TYPE */

	/* 逆变电源使能 */
	rcu_periph_clock_enable(dcacPOWER_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(dcacPOWER_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, dcacPOWER_EN_PIN);
	gpio_output_options_set(dcacPOWER_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, dcacPOWER_EN_PIN);
	#else
	gpio_init(dcacPOWER_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, dcacPOWER_EN_PIN);
	#endif  /* boardIC_TYPE */
	dcacPOWER_EN_ON();

	#if (boardDCAC_485_IFACE_EN)
	rcu_periph_clock_enable(dcacGPIO_485_TX_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(dcacGPIO_485_TX_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, dcacGPIO_485_TX_EN_PIN);
	gpio_output_options_set(dcacGPIO_485_TX_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, dcacGPIO_485_TX_EN_PIN);
	#else
	gpio_init(dcacGPIO_485_TX_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, dcacGPIO_485_TX_EN_PIN);
	#endif  /* boardIC_TYPE */
	dcacGPIO_485_TX_EN_OFF();	/* 默认处于接收模式 */
	#endif  /* boardDCAC_485_IFACE_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 逆变串口配置
 * 说明(备注)  : 初始化串口时钟、波特率与中断
 * 传入参数    : ul_baud: 波特率
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_dcac_usart_config(uint32_t ul_baud)
{
	rcu_periph_clock_enable(dcacUSART_RCU);
	nvic_irq_enable(dcacUSART_IRQ, 2, 0);

	usart_deinit(dcacUSART);
	usart_baudrate_set(dcacUSART, ul_baud);
	usart_word_length_set(dcacUSART, USART_WL_8BIT);
	usart_stop_bit_set(dcacUSART, USART_STB_1BIT);
	usart_parity_config(dcacUSART, USART_PM_NONE);
	usart_hardware_flow_rts_config(dcacUSART, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(dcacUSART, USART_CTS_DISABLE);

	usart_receive_config(dcacUSART, USART_RECEIVE_ENABLE);
	usart_transmit_config(dcacUSART, USART_TRANSMIT_ENABLE);

	#if (!boardDCAC_IFACE_DMA_EN)
	usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_RBNE);
	usart_interrupt_enable(dcacUSART, USART_INT_RBNE);

	usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_TBE);
	usart_interrupt_disable(dcacUSART, USART_INT_TBE);
	#endif  /* !boardDCAC_IFACE_DMA_EN */

	usart_enable(dcacUSART);
}

/***********************************************************************************************************************
 * 函数功能    : 逆变通信 DMA 初始化
 * 说明(备注)  : 配置 USART 的 TX 和 RX DMA 通道
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
#if (boardDCAC_IFACE_DMA_EN)
static void v_dcac_dma_init(void)
{
	dma_parameter_struct dma_init_struct;

	nvic_irq_enable(dcacUSART_DMA_TX_IRQ, 2, 0);
	rcu_periph_clock_enable(dcacUSART_DMA_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	rcu_periph_clock_enable(RCU_DMAMUX);
	#endif  /* boardIC_TYPE */

	/* initialize DMA channel(USART TX) */
	dma_deinit(dcacUSART_DMA, dcacUSART_DMA_TX_CH);
	dma_struct_para_init(&dma_init_struct);

	#if (boardIC_TYPE == boardIC_GD32F50X)
	dma_init_struct.request = dcacUSART_DMA_TX_REQUEST;
	#endif  /* boardIC_TYPE */
	dma_init_struct.direction    = DMA_MEMORY_TO_PERIPHERAL;            /* 内存到外设 */
	dma_init_struct.memory_addr  = (uint32_t)s_uca_dcac_tx_dma_buff;   /* 设置内存发送基地址 */
	dma_init_struct.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;          /* 内存地址递增 */
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;               /* 8位内存数据 */
	dma_init_struct.number       = 0;                                   /* Buff数组的大小 */
	dma_init_struct.periph_addr  = (uint32_t)(&USART_DATA(dcacUSART)); /* 外设基地址,USART数据寄存器地址 */
	dma_init_struct.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;         /* 外设地址不递增 */
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;           /* 8位外设数据 */
	dma_init_struct.priority     = DMA_PRIORITY_ULTRA_HIGH;             /* 最高DMA通道优先级 */
	dma_init(dcacUSART_DMA, dcacUSART_DMA_TX_CH, &dma_init_struct);

	/* initialize DMA channel(USART RX) */
	dma_deinit(dcacUSART_DMA, dcacUSART_DMA_RX_CH);

	#if (boardIC_TYPE == boardIC_GD32F50X)
	dma_init_struct.request = dcacUSART_DMA_RX_REQUEST;
	#endif  /* boardIC_TYPE */
	dma_init_struct.direction   = DMA_PERIPHERAL_TO_MEMORY;
	dma_init_struct.number      = dcacRX_DMA_BUFF_SIZE;
	dma_init_struct.memory_addr = (uint32_t)s_uca_dcac_rx_dma_buff;
	dma_init(dcacUSART_DMA, dcacUSART_DMA_RX_CH, &dma_init_struct);

	dma_circulation_disable(dcacUSART_DMA, dcacUSART_DMA_TX_CH);      /* 关闭DMA_TX循环模式 */
	dma_memory_to_memory_disable(dcacUSART_DMA, dcacUSART_DMA_TX_CH); /* DMA内存到内存模式不开启 */
	dma_circulation_disable(dcacUSART_DMA, dcacUSART_DMA_RX_CH);      /* 关闭DMA_RX循环模式 */
	dma_memory_to_memory_disable(dcacUSART_DMA, dcacUSART_DMA_RX_CH); /* DMA内存到内存模式不开启 */

	/* enable USART DMA for reception */
	#if (boardIC_TYPE == boardIC_GD32F30X)
	usart_dma_receive_config(dcacUSART, USART_RECEIVE_DMA_ENABLE);
	#elif (boardIC_TYPE == boardIC_GD32F50X)
	usart_dma_receive_config(dcacUSART, USART_DENR_ENABLE);
	#endif  /* boardIC_TYPE */
	dma_channel_enable(dcacUSART_DMA, dcacUSART_DMA_RX_CH);

	/* enable USART DMA for transmission */
	#if (boardIC_TYPE == boardIC_GD32F30X)
	usart_dma_transmit_config(dcacUSART, USART_TRANSMIT_DMA_ENABLE);
	#elif (boardIC_TYPE == boardIC_GD32F50X)
	usart_dma_transmit_config(dcacUSART, USART_DENT_ENABLE);
	#endif  /* boardIC_TYPE */
	dma_interrupt_enable(dcacUSART_DMA, dcacUSART_DMA_TX_CH, DMA_INT_FTF);
	dma_channel_disable(dcacUSART_DMA, dcacUSART_DMA_TX_CH);

	/* 串口空闲中断 */
	usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_IDLE);
	usart_interrupt_enable(dcacUSART, USART_INT_IDLE);
}
#endif  /* boardDCAC_IFACE_DMA_EN */

/***********************************************************************************************************************
 * 函数功能    : 逆变串口初始化
 * 说明(备注)  : 初始化 IO、串口与 DMA
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_IfaceInit(void)
{
	v_dcac_io_init();
	v_dcac_usart_config(dcacUSART_BAUD);
	#if (boardDCAC_IFACE_DMA_EN)
	v_dcac_dma_init();
	#endif  /* boardDCAC_IFACE_DMA_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 串口重置
 * 说明(备注)  : 复位串口及 DMA 通道
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_IfaceDeInit(void)
{
	usart_deinit(dcacUSART);

	#if (boardDCAC_IFACE_DMA_EN)
	dma_deinit(dcacUSART_DMA, dcacUSART_DMA_TX_CH);
	dma_deinit(dcacUSART_DMA, dcacUSART_DMA_RX_CH);
	#endif  /* boardDCAC_IFACE_DMA_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 设置串口波特率
 * 说明(备注)  : 重设串口波特率并重新初始化
 * 传入参数    : ul_baud: 要设置的波特率值
 * 输出参数    : 无
 * 返回值      : true: 设置成功, false: 失败
 ************************************************************************************************************************/
bool bDcac_IfaceSetBaud(uint32_t ul_baud)
{
	if (ul_baud == 0)
		return false;

	#if (boardDCAC_485_IFACE_EN)
	vDcac_485TransEnable(false);
	#endif  /* boardDCAC_485_IFACE_EN */

	s_us_data_send_cnt  = 0;
	s_us_data_send_size = 0;

	v_dcac_usart_config(ul_baud);
	#if (boardDCAC_IFACE_DMA_EN)
	v_dcac_dma_init();
	#endif  /* boardDCAC_IFACE_DMA_EN */

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 串口发送数据启动
 * 说明(备注)  : 支持 DMA 与中断方式发送报文
 * 传入参数    : p_data: 发送数据地址, us_len: 数据长度
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bDcac_DataSendStart(uint8_t *p_data, uint16_t us_len)
{
	if (p_data == NULL || us_len == 0)
		return false;

	if (us_len > dcacTX_DMA_BUFF_SIZE)
		return false;    /* 超长拒绝,严禁截断发送残帧 */

	#if (boardDCAC_IFACE_DMA_EN)
	/* 上一帧DMA尚未发完(含FTF中断未及时处理),禁止重入破坏在途数据 */
	if (DMA_CHCTL(dcacUSART_DMA, dcacUSART_DMA_TX_CH) & DMA_CHXCTL_CHEN)
		return false;
	#endif  /* boardDCAC_IFACE_DMA_EN */

	#if (boardDCAC_485_IFACE_EN)
	vDcac_485TransEnable(true);
	#endif  /* boardDCAC_485_IFACE_EN */

	memcpy(s_uca_dcac_tx_dma_buff, p_data, us_len);

	#if (boardDCAC_IFACE_DMA_EN)
	dma_flag_clear(dcacUSART_DMA, dcacUSART_DMA_TX_CH, DMA_FLAG_FTF);
	dma_memory_address_config(dcacUSART_DMA, dcacUSART_DMA_TX_CH, (uint32_t)s_uca_dcac_tx_dma_buff);
	dma_transfer_number_config(dcacUSART_DMA, dcacUSART_DMA_TX_CH, us_len);
	dma_channel_enable(dcacUSART_DMA, dcacUSART_DMA_TX_CH);
	return true;
	#else
	if (s_us_data_send_cnt)
		return false;

	s_us_data_send_size = us_len;
	s_us_data_send_cnt  = 0;
	usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_TBE);
	usart_interrupt_enable(dcacUSART, USART_INT_TBE);
	return true;
	#endif  /* boardDCAC_IFACE_DMA_EN */
}

/***********************************************************************************************************************
 * 函数功能    : RS485 发送使能切换
 * 说明(备注)  : 控制 RS485 收发方向
 * 传入参数    : b_en: true-发送模式, false-接收模式
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
#if (boardDCAC_485_IFACE_EN)
void vDcac_485TransEnable(bool b_en)
{
	if (b_en)
		dcacGPIO_485_TX_EN_ON();
	else
	{
		dcacGPIO_485_TX_EN_OFF();
		#if (!boardUSE_OS)
		bSysTick_DcacSendFinish = false;
		#endif  /* !boardUSE_OS */
	}
}
#endif  /* boardDCAC_485_IFACE_EN */

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 进入低功耗模式
 * 说明(备注)  : 关闭串口中断与时钟，引脚配置为模拟输入
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_IoEnterLowPower(void)
{
	rcu_periph_clock_enable(dcacUSART_GPIO_TX_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(dcacUSART_GPIO_TX_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, dcacUSART_GPIO_TX_PIN);
	#else
	gpio_init(dcacUSART_GPIO_TX_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, dcacUSART_GPIO_TX_PIN);
	#endif  /* boardIC_TYPE */

	rcu_periph_clock_enable(dcacUSART_GPIO_RX_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(dcacUSART_GPIO_RX_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, dcacUSART_GPIO_RX_PIN);
	#else
	gpio_init(dcacUSART_GPIO_RX_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, dcacUSART_GPIO_RX_PIN);
	#endif  /* boardIC_TYPE */

	rcu_periph_clock_enable(dcacPOWER_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(dcacPOWER_EN_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, dcacPOWER_EN_PIN);
	#else
	gpio_init(dcacPOWER_EN_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, dcacPOWER_EN_PIN);
	#endif  /* boardIC_TYPE */

	rcu_periph_clock_disable(dcacUSART_GPIO_RX_RCU);
	rcu_periph_clock_disable(dcacUSART_GPIO_TX_RCU);
	rcu_periph_clock_disable(dcacUSART_RCU);
	usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_RBNE);
	usart_interrupt_disable(dcacUSART, USART_INT_RBNE);
	usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_TBE);
	usart_interrupt_disable(dcacUSART, USART_INT_TBE);
	usart_disable(dcacUSART);
}
#endif  /* boardLOW_POWER */

#if (boardDCAC_IFACE_DMA_EN)
/***********************************************************************************************************************
 * 函数功能    : DMA 发送完成中断服务函数
 * 说明(备注)  : 发送完成后关闭通道并切换接收模式
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void dcacUSART_DMA_TX_IRQ_HANDLER(void)
{
	if (dma_interrupt_flag_get(dcacUSART_DMA, dcacUSART_DMA_TX_CH, DMA_INT_FLAG_FTF)) 
	{
		#if (boardIC_TYPE == boardIC_GD32F30X)
		dma_interrupt_flag_clear(dcacUSART_DMA, dcacUSART_DMA_TX_CH, DMA_INT_FLAG_G);
		#elif (boardIC_TYPE == boardIC_GD32F50X)
		dma_interrupt_flag_clear(dcacUSART_DMA, dcacUSART_DMA_TX_CH, DMA_INT_FLAG_GIF);
		#endif  /* boardIC_TYPE */

		/* 关闭DMA发送 */
		dma_channel_disable(dcacUSART_DMA, dcacUSART_DMA_TX_CH);
		/* 发送完成 */
		s_us_data_send_size = 0;
		
		/* 使用TC中断来关闭RS485收发 */
		#if (boardRS485_HARDWARE_TC_PATCH_EN && boardDCAC_485_IFACE_EN)
		usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_TC);
		usart_interrupt_enable(dcacUSART, USART_INT_TC);
		#else
		/* 使用延时来关闭RS485收发 */
		#if (boardDCAC_485_IFACE_EN)
		#if (boardUSE_OS)
		xTimerResetFromISR(tDcacRxEnTimer, 0);
		#else
		bSysTick_DcacSendFinish = true;
		#endif  /* boardUSE_OS */
		#endif  /* boardDCAC_485_IFACE_EN */
		#endif  /* boardRS485_HARDWARE_TC_PATCH_EN */
	}
}

static u16 s_us_read_buff_len = 0;
/***********************************************************************************************************************
 * 函数功能    : 串口空闲/接收中断服务函数
 * 说明(备注)  : 处理空闲帧接收并按目标对象分流转存至环形缓冲区，通知处理任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void dcacUSART_IRQ_HANDLER(void)
{
	#if (boardUSE_OS)
	BaseType_t x_woken = pdFALSE;	/* 高优先级任务唤醒标记 */
	#endif  /* boardUSE_OS */

	/* 使用TC中断来关闭RS485收发 */
	#if (boardRS485_HARDWARE_TC_PATCH_EN && boardDCAC_485_IFACE_EN)
	if (RESET != usart_interrupt_flag_get(dcacUSART, USART_INT_FLAG_TC))
	{
		usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_TC);
		usart_interrupt_disable(dcacUSART, USART_INT_TC);
		dcacGPIO_485_TX_EN_OFF();  /* 最后一个停止位完全发出，纳秒级瞬间切回接收模式 */
	}
	#endif  /* boardRS485_HARDWARE_TC_PATCH_EN */

	if (RESET != usart_interrupt_flag_get(dcacUSART, USART_INT_FLAG_IDLE)) 
	{
		/* 清除中断 */
		usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_IDLE);
		/* 清除空闲标志位 */
		usart_data_receive(dcacUSART);
		/* 关闭DMA传输 */
		dma_channel_disable(dcacUSART_DMA, dcacUSART_DMA_RX_CH); 
		
		/* 获取接收到的数据长度，单位：字节 */
		s_us_read_buff_len = dcacRX_DMA_BUFF_SIZE - dma_transfer_number_get(dcacUSART_DMA, dcacUSART_DMA_RX_CH);

		/* 转存数据到待处理数据缓冲区 */
		if (bDcacUseFlag == true)
		{
			if (s_us_read_buff_len && tpDcacProtoRx != NULL)
				lwrb_write(&tpDcacProtoRx->tRxBuff, s_uca_dcac_rx_dma_buff, s_us_read_buff_len);

			#if (boardUSE_OS)
			vTaskNotifyGiveFromISR(tDcacRecTaskHandle, &x_woken);
			#endif  /* boardUSE_OS */
		}
		else
		{
			#if (boardMPPT_EN)
			if (s_us_read_buff_len && tpMpptProtoRx != NULL)
				lwrb_write(&tpMpptProtoRx->tRxBuff, s_uca_dcac_rx_dma_buff, s_us_read_buff_len);

			#if (boardUSE_OS)
			vTaskNotifyGiveFromISR(tMpptRecTaskHandle, &x_woken);
			#endif  /* boardUSE_OS */
			#endif  /* boardMPPT_EN */
		}

		/* 重新设置DMA传输 */
		dma_transfer_number_config(dcacUSART_DMA, dcacUSART_DMA_RX_CH, dcacRX_DMA_BUFF_SIZE);
		dma_channel_enable(dcacUSART_DMA, dcacUSART_DMA_RX_CH);    
	}

	#if (boardUSE_OS)
	portYIELD_FROM_ISR(x_woken);	/* 立即切换到被唤醒的接收任务，不再等待下一个 tick */
	#endif  /* boardUSE_OS */
}
#else
/***********************************************************************************************************************
 * 函数功能    : 串口中断服务函数(非DMA方式)
 * 说明(备注)  : 字节收发及空闲帧中断处理
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void dcacUSART_IRQ_HANDLER(void)
{
	#if (boardUSE_OS)
	BaseType_t x_woken = pdFALSE;	/* 高优先级任务唤醒标记 */
	#endif  /* boardUSE_OS */

	if (RESET != usart_interrupt_flag_get(dcacUSART, USART_INT_FLAG_RBNE))
	{
		uint8_t uc_data = (uint8_t)USART_DATA(dcacUSART);

		if (bDcacUseFlag == true)
		{
			if (tpDcacProtoRx != NULL)
				lwrb_write(&tpDcacProtoRx->tRxBuff, &uc_data, 1);

			#if (boardUSE_OS)
			vTaskNotifyGiveFromISR(tDcacRecTaskHandle, &x_woken);
			#endif  /* boardUSE_OS */
		}
		else
		{
			#if (boardMPPT_EN)
			if (tpMpptProtoRx != NULL)
				lwrb_write(&tpMpptProtoRx->tRxBuff, &uc_data, 1);

			#if (boardUSE_OS)
			vTaskNotifyGiveFromISR(tMpptRecTaskHandle, &x_woken);
			#endif  /* boardUSE_OS */
			#endif  /* boardMPPT_EN */
		}

		usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_RBNE); /* 清除串口接收中断 */
	}

	if (RESET != usart_interrupt_flag_get(dcacUSART, USART_INT_FLAG_TBE))
	{
		usart_interrupt_flag_clear(dcacUSART, USART_INT_FLAG_TBE);
		
		if (s_us_data_send_cnt < s_us_data_send_size)
		{    
			USART_DATA(dcacUSART) = s_uca_dcac_tx_dma_buff[s_us_data_send_cnt];
			s_us_data_send_cnt++;
		}
		else
		{
			usart_interrupt_disable(dcacUSART, USART_INT_TBE);
			s_us_data_send_cnt  = 0;
			s_us_data_send_size = 0;
			
			#if (boardDCAC_485_IFACE_EN)
			#if (boardUSE_OS)
			xTimerResetFromISR(tDcacRxEnTimer, 0);
			#else
			bSysTick_DcacSendFinish = true;
			#endif  /* boardUSE_OS */
			#endif  /* boardDCAC_485_IFACE_EN */
		}               
	}    

	#if (boardUSE_OS)
	portYIELD_FROM_ISR(x_woken);	/* 立即切换到被唤醒的接收任务，不再等待下一个 tick */
	#endif  /* boardUSE_OS */
}
#endif  /* boardDCAC_IFACE_DMA_EN */

#endif  /* (boardDCAC_IFACE && boardDCAC_EN) */
