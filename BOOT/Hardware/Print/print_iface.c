/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Print
 * File    : print_iface.c
 * Date    : 2026-09-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 打印与串口底层硬件接口驱动实现(串口/DMA/RS485收发控制)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_iface.h"

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "Sys/sys_task.h"

#include "lwrb.h"

#if (boardPRINT_485_IFACE_EN)
#if (boardUSE_OS)
#include "timer_task.h"
#else
#include "systick.h"
#endif  /* boardUSE_OS */
#endif  /* boardPRINT_485_IFACE_EN */

//****************************************************Macros********************************************************************//
#define			printRX_DMA_BUFF_SIZE					256
#define			printTX_DMA_BUFF_SIZE					256		/* DMA 数组大小 */

// 使能RS485硬件发送完成(TC)中断换向补丁
#define     	boardRS485_HARDWARE_TC_PATCH_EN          1
//<i> 1:开启USART硬件TC中断自动切换RS485接收方向
//<i> 0:关闭硬件补丁，使用原有软件定时器延时模式
//-------------------------------------------------------------------

//****************************************************Parameter Initialization**************************************************//
static vu16 s_us_data_send_size = 0;
static vu16 s_us_data_send_cnt  = 0;

#if (boardPRINT_IFACE_DMA_EN)
static __ALIGNED(4) uint8_t s_uca_print_rx_dma_buff[printRX_DMA_BUFF_SIZE];
#endif  /* boardPRINT_IFACE_DMA_EN */
static __ALIGNED(4) uint8_t s_uca_print_tx_dma_buff[printTX_DMA_BUFF_SIZE];

//****************************************************Function Declaration******************************************************//
#if (boardPRINT_IFACE != 7)
static void v_print_gpio_init(void);
static void v_print_usart_init(void);
#if (boardPRINT_IFACE_DMA_EN)
static void v_print_dma_init(void);
#endif  /* boardPRINT_IFACE_DMA_EN */
#endif  /* boardPRINT_IFACE != 7 */

#if (boardCM_BACKTRACE)
#if defined(__CC_ARM)
#pragma import(__use_no_semihosting)             
/* 标准库需要的支持函数 */
struct __FILE 
{ 
	int					handle;
};
 
FILE __stdout;       
/* 定义_sys_exit()以避免使用半主机模式 */
/***********************************************************************************************************************
 * 函数功能    : 重定义 _sys_exit 以避免使用半主机模式
 * 说明(备注)  : AC5(Keil) 编译分支下的空实现，仅用于屏蔽半主机模式
 * 传入参数    : x: 退出码（未使用）
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void _sys_exit(int x) 
{ 
	x = x; 
} 
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
/* ARMCLANG: 使用链接器选项 --specs=nosys.specs 替代半主机模式 */
/***********************************************************************************************************************
 * 函数功能    : 重定义 _sys_exit 以避免使用半主机模式
 * 说明(备注)  : AC6(ARMCLANG) 编译分支下的空实现，配合 --specs=nosys.specs 屏蔽半主机模式
 * 传入参数    : x: 退出码（未使用）
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__attribute__((used)) void _sys_exit(int x) 
{ 
	(void)x; 
}
#endif  /* __CC_ARM / __ARMCC_VERSION */

/***********************************************************************************************************************
 * 函数功能    : 重定义 fputc 函数
 * 说明(备注)  : 重定向字符输出至调试串口
 * 传入参数    : ch: 待发送字符, f: 文件句柄指针
 * 输出参数    : 无
 * 返回值      : 写入字符
 ************************************************************************************************************************/
int fputc(int ch, FILE *f)
{
#if (boardPRINT_IFACE == 7)
	uint8_t uc_c = (uint8_t)ch;
	bUsbCdc_Send(&uc_c, 1);
	return ch;
#else
	#if (boardPRINT_485_IFACE_EN)
	vPrint_485TransEnable(true);
	#endif  /* boardPRINT_485_IFACE_EN */

	#if (boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
	usart_data_transmit(printUSART, (uint8_t)ch);
	while (RESET == usart_flag_get(printUSART, USART_FLAG_TBE));
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
	HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, 0xFFFF);
	#endif  /* boardIC_TYPE */

	#if (boardPRINT_485_IFACE_EN)
	vPrint_485TransEnable(false);
	#endif  /* boardPRINT_485_IFACE_EN */
	return ch;
#endif  /* boardPRINT_IFACE == 7 */
}
#endif  /* boardCM_BACKTRACE */

#if (boardPRINT_IFACE != 7)
/***********************************************************************************************************************
 * 函数功能    : 相关 IO 初始化
 * 说明(备注)  : 配置 USART TX/RX 复用引脚及 485/接口控制引脚
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_print_gpio_init(void)
{
	rcu_periph_clock_enable(RCU_AF);
	#if (gpioUSART0_REMAP_EN && boardIC_TYPE != boardIC_GD32F50X)
	gpio_pin_remap_config(GPIO_USART0_REMAP, ENABLE);
	#endif  /* gpioUSART0_REMAP_EN */
	
	/* TX */
	rcu_periph_clock_enable(printUSART_GPIO_TX_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_af_set(printUSART_GPIO_TX_PORT, printUSART_GPIO_TX_AF, printUSART_GPIO_TX_PIN);
	gpio_mode_set(printUSART_GPIO_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, printUSART_GPIO_TX_PIN);
	gpio_output_options_set(printUSART_GPIO_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, printUSART_GPIO_TX_PIN);
	#else
	gpio_init(printUSART_GPIO_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, printUSART_GPIO_TX_PIN);
	#endif  /* boardIC_TYPE */
	
	/* RX */
	rcu_periph_clock_enable(printUSART_GPIO_RX_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_af_set(printUSART_GPIO_RX_PORT, printUSART_GPIO_RX_AF, printUSART_GPIO_RX_PIN);
	gpio_mode_set(printUSART_GPIO_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, printUSART_GPIO_RX_PIN);
	gpio_output_options_set(printUSART_GPIO_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, printUSART_GPIO_RX_PIN);
	#else
	gpio_init(printUSART_GPIO_RX_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, printUSART_GPIO_RX_PIN);
	#endif  /* boardIC_TYPE */

	#if (boardPRINT_485_IFACE_EN)
	rcu_periph_clock_enable(printGPIO_485_TX_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(printGPIO_485_TX_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, printGPIO_485_TX_EN_PIN);
	gpio_output_options_set(printGPIO_485_TX_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, printGPIO_485_TX_EN_PIN);
	#else
	gpio_init(printGPIO_485_TX_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, printGPIO_485_TX_EN_PIN);
	#endif  /* boardIC_TYPE */
	printGPIO_485_TX_EN_OFF();	/* 默认处于接收模式 */
	#endif  /* boardPRINT_485_IFACE_EN */
	
	/* 接口使能 */
	rcu_periph_clock_enable(printIFACE_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(printIFACE_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, printIFACE_EN_PIN);
	gpio_output_options_set(printIFACE_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, printIFACE_EN_PIN);
	#else
	gpio_init(printIFACE_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, printIFACE_EN_PIN);
	#endif  /* boardIC_TYPE */
	printIFACE_EN_ON();	/* 默认使能 */
}

/***********************************************************************************************************************
 * 函数功能    : 串口配置
 * 说明(备注)  : 初始化串口时钟、波特率与中断
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_print_usart_init(void)
{
	rcu_periph_clock_enable(printUSART_RCU);
	nvic_irq_enable(printUSART_IRQ, 2, 0);

	usart_deinit(printUSART);
	usart_baudrate_set(printUSART, printUSART_BAUD);
	usart_word_length_set(printUSART, USART_WL_8BIT);
	usart_stop_bit_set(printUSART, USART_STB_1BIT);
	usart_parity_config(printUSART, USART_PM_NONE);
	usart_hardware_flow_rts_config(printUSART, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(printUSART, USART_CTS_DISABLE);
	
	usart_receive_config(printUSART, USART_RECEIVE_ENABLE);
	usart_transmit_config(printUSART, USART_TRANSMIT_ENABLE);
	
	#if (!boardPRINT_IFACE_DMA_EN)
	usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_RBNE);
	usart_interrupt_enable(printUSART, USART_INT_RBNE); 
	
	usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TBE);
	usart_interrupt_disable(printUSART, USART_INT_TBE); 	
	#endif  /* !boardPRINT_IFACE_DMA_EN */

	usart_enable(printUSART);
}

/***********************************************************************************************************************
 * 函数功能    : DMA 初始化
 * 说明(备注)  : 配置 USART 的 TX 和 RX DMA 通道
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
#if (boardPRINT_IFACE_DMA_EN)
static void v_print_dma_init(void)
{
	dma_parameter_struct dma_init_struct;

	nvic_irq_enable(printUSART_DMA_TX_IRQ, 2, 0);
	rcu_periph_clock_enable(printUSART_DMA_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	rcu_periph_clock_enable(RCU_DMAMUX);
	#endif  /* boardIC_TYPE */
	
	/* initialize DMA channel(USART TX) */
	dma_deinit(printUSART_DMA, printUSART_DMA_TX_CH);
	dma_struct_para_init(&dma_init_struct);
	
	#if (boardIC_TYPE == boardIC_GD32F50X)
	dma_init_struct.request = printUSART_DMA_TX_REQUEST;
	#endif  /* boardIC_TYPE */
	dma_init_struct.direction    = DMA_MEMORY_TO_PERIPHERAL;            /* 内存到外设 */              
	dma_init_struct.memory_addr  = (uint32_t)s_uca_print_tx_dma_buff;   /* 设置内存发送基地址 */
	dma_init_struct.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;          /* 内存地址递增 */
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;               /* 8位内存数据 */
	dma_init_struct.number       = 0;                                   /* Buff数组的大小 */
	dma_init_struct.periph_addr  = (uint32_t)(&USART_DATA(printUSART)); /* 外设基地址,USART数据寄存器地址 */
	dma_init_struct.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;         /* 外设地址不递增 */
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;           /* 8位外设数据 */
	dma_init_struct.priority     = DMA_PRIORITY_ULTRA_HIGH;             /* 最高DMA通道优先级 */
	dma_init(printUSART_DMA, printUSART_DMA_TX_CH, &dma_init_struct);
	
	/* initialize DMA channel(USART RX) */
	dma_deinit(printUSART_DMA, printUSART_DMA_RX_CH);

	#if (boardIC_TYPE == boardIC_GD32F50X)
	dma_init_struct.request = printUSART_DMA_RX_REQUEST;
	#endif  /* boardIC_TYPE */
	dma_init_struct.direction   = DMA_PERIPHERAL_TO_MEMORY;
	dma_init_struct.number      = printRX_DMA_BUFF_SIZE;
	dma_init_struct.memory_addr = (uint32_t)s_uca_print_rx_dma_buff;
	dma_init(printUSART_DMA, printUSART_DMA_RX_CH, &dma_init_struct);
	
	dma_circulation_disable(printUSART_DMA, printUSART_DMA_TX_CH);      /* 关闭DMA_TX循环模式 */
	dma_memory_to_memory_disable(printUSART_DMA, printUSART_DMA_TX_CH); /* DMA内存到内存模式不开启 */
	dma_circulation_disable(printUSART_DMA, printUSART_DMA_RX_CH);      /* 关闭DMA_RX循环模式 */
	dma_memory_to_memory_disable(printUSART_DMA, printUSART_DMA_RX_CH); /* DMA内存到内存模式不开启 */
	
	/* enable USART DMA for reception */
	#if (boardIC_TYPE == boardIC_GD32F30X)
	usart_dma_receive_config(printUSART, USART_RECEIVE_DMA_ENABLE);
	#elif (boardIC_TYPE == boardIC_GD32F50X)
	usart_dma_receive_config(printUSART, USART_DENR_ENABLE);
	#endif  /* boardIC_TYPE */
	dma_channel_enable(printUSART_DMA, printUSART_DMA_RX_CH);
	
	/* enable USART DMA for transmission */
	#if (boardIC_TYPE == boardIC_GD32F30X)
	usart_dma_transmit_config(printUSART, USART_TRANSMIT_DMA_ENABLE);
	#elif (boardIC_TYPE == boardIC_GD32F50X)
	usart_dma_transmit_config(printUSART, USART_DENT_ENABLE);
	#endif  /* boardIC_TYPE */
	dma_interrupt_enable(printUSART_DMA, printUSART_DMA_TX_CH, DMA_INT_FTF);
	dma_channel_disable(printUSART_DMA, printUSART_DMA_TX_CH);
	
	/* 串口空闲中断 */
	usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_IDLE);
	usart_interrupt_enable(printUSART, USART_INT_IDLE); 
}
#endif  /* boardPRINT_IFACE_DMA_EN */
#endif  /* boardPRINT_IFACE != 7 */

/***********************************************************************************************************************
 * 函数功能    : 串口初始化
 * 说明(备注)  : 初始化 IO、串口与 DMA，或初始化 USB CDC
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vPrint_IfaceInit(void)
{
#if (boardPRINT_IFACE == 7)
	vUsbCdc_Init();
#else
	v_print_gpio_init();
	v_print_usart_init();
	#if (boardPRINT_IFACE_DMA_EN)
	v_print_dma_init();
	#endif  /* boardPRINT_IFACE_DMA_EN */
#endif  /* boardPRINT_IFACE == 7 */
}

/***********************************************************************************************************************
 * 函数功能    : 串口去初始化
 * 说明(备注)  : 复位串口及 DMA 通道
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vPrint_IfaceDeInit(void)
{
	#if (boardPRINT_IFACE == 7)
	vUsbCdc_DeInit();
	#else
	usart_deinit(printUSART);
	
	#if (boardPRINT_IFACE_DMA_EN)
	dma_deinit(printUSART_DMA, printUSART_DMA_TX_CH);
	dma_deinit(printUSART_DMA, printUSART_DMA_RX_CH);
	#endif  /* boardPRINT_IFACE_DMA_EN */
	#endif  /* boardPRINT_IFACE == 7 */
}

/***********************************************************************************************************************
 * 函数功能    : 串口发送数据启动
 * 说明(备注)  : 从环形缓冲区读取数据并通过 DMA、中断或 USB CDC 发送
 * 传入参数    : us_len: 期望发送数据长度
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bPrint_DataSendStart(uint16_t us_len)
{
	if (us_len == 0)
		return false;

	#if (boardPRINT_IFACE == 7)
	if (!bUsbCdc_IsConfigured() || bUsbCdc_IsTxBusy())
		return false;

	if (us_len > printTX_DMA_BUFF_SIZE)
		us_len = printTX_DMA_BUFF_SIZE;

	s_us_data_send_size = lwrb_peek(&tPrintTxBuff, 0, s_uca_print_tx_dma_buff, us_len);
	if (s_us_data_send_size == 0)
		return false;

	if (!bUsbCdc_Send(s_uca_print_tx_dma_buff, s_us_data_send_size))
	{
		s_us_data_send_size = 0;
		return false;
	}
	lwrb_skip(&tPrintTxBuff, s_us_data_send_size);
	s_us_data_send_size = 0;
	return true;
	#else
	if (us_len > printTX_DMA_BUFF_SIZE)
		us_len = printTX_DMA_BUFF_SIZE;

	#if (boardPRINT_IFACE_DMA_EN)
	/* 上一帧DMA尚未发完(含FTF中断未及时处理),禁止重入破坏在途数据 */
	if (DMA_CHCTL(printUSART_DMA, printUSART_DMA_TX_CH) & DMA_CHXCTL_CHEN)
		return false;
	#else
	if (s_us_data_send_cnt)
		return false;
	#endif  /* boardPRINT_IFACE_DMA_EN */

	#if (boardPRINT_485_IFACE_EN)
	vPrint_485TransEnable(true);
	#endif  /* boardPRINT_485_IFACE_EN */

	/* 取出数据 */
	s_us_data_send_size = lwrb_read(&tPrintTxBuff, s_uca_print_tx_dma_buff, us_len);
	if (s_us_data_send_size == 0)
	{
		#if (boardPRINT_485_IFACE_EN)
		vPrint_485TransEnable(false);
		#endif  /* boardPRINT_485_IFACE_EN */
		return false;
	}

	#if (boardPRINT_IFACE_DMA_EN)
	dma_flag_clear(printUSART_DMA, printUSART_DMA_TX_CH, DMA_FLAG_FTF); 
	dma_memory_address_config(printUSART_DMA, printUSART_DMA_TX_CH, (uint32_t)s_uca_print_tx_dma_buff);
	dma_transfer_number_config(printUSART_DMA, printUSART_DMA_TX_CH, s_us_data_send_size);
	dma_channel_enable(printUSART_DMA, printUSART_DMA_TX_CH);
	return true;
	#else
	s_us_data_send_cnt = 0; 
	usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TBE);
	usart_interrupt_enable(printUSART, USART_INT_TBE);           
	return true;
	#endif  /* boardPRINT_IFACE_DMA_EN */
	#endif  /* boardPRINT_IFACE == 7 */
}

/***********************************************************************************************************************
 * 函数功能    : RS485 发送使能切换
 * 说明(备注)  : 控制 RS485 收发方向
 * 传入参数    : b_en: true-发送模式, false-接收模式
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
#if (boardPRINT_485_IFACE_EN)
void vPrint_485TransEnable(bool b_en)
{
	if (b_en)
		printGPIO_485_TX_EN_ON();
	else
	{
		printGPIO_485_TX_EN_OFF();
		#if (!boardUSE_OS)
		bSysTick_PrintSendFinish = false;
		#endif  /* !boardUSE_OS */
	}
}
#endif  /* boardPRINT_485_IFACE_EN */

/***********************************************************************************************************************
 * 函数功能    : 检查发送完成状态
 * 说明(备注)  : 查询当前在途发送是否已经结束
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 发送完成, false: 发送中  
 ************************************************************************************************************************/
bool bPrint_CheckSendFinish(void)
{
#if (boardPRINT_IFACE == 7)
	return !bUsbCdc_IsTxBusy();
#else
	if (s_us_data_send_size)
		return false;
	else 
		return true;
#endif  /* boardPRINT_IFACE == 7 */
}

/***********************************************************************************************************************
 * 函数功能    : 进入低功耗模式
 * 说明(备注)  : 关闭串口中断与时钟，引脚配置为模拟输入；或去初始化 USB
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 * **********************************************************************************************************************/
#if (boardLOW_POWER)
void vPrint_EnterLowPower(void)
{
#if (boardPRINT_IFACE == 7)
	vUsbCdc_DeInit();
#else
	rcu_periph_clock_enable(printUSART_GPIO_TX_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(printUSART_GPIO_TX_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, printUSART_GPIO_TX_PIN);
	#else
	gpio_init(printUSART_GPIO_TX_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, printUSART_GPIO_TX_PIN);
	#endif  /* boardIC_TYPE */

	rcu_periph_clock_enable(printUSART_GPIO_RX_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(printUSART_GPIO_RX_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, printUSART_GPIO_RX_PIN);
	#else
	gpio_init(printUSART_GPIO_RX_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, printUSART_GPIO_RX_PIN);
	#endif  /* boardIC_TYPE */
	
	rcu_periph_clock_disable(printUSART_GPIO_RX_RCU);
	rcu_periph_clock_disable(printUSART_GPIO_TX_RCU);
	rcu_periph_clock_disable(printUSART_RCU);
	usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_RBNE);
	usart_interrupt_disable(printUSART, USART_INT_RBNE); 
	usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TBE);
	usart_interrupt_disable(printUSART, USART_INT_TBE); 
	usart_disable(printUSART);
#endif  /* boardPRINT_IFACE == 7 */
}
#endif  /* boardLOW_POWER */

#if (boardPRINT_IFACE != 7)

#if (boardPRINT_IFACE_DMA_EN)
/***********************************************************************************************************************
 * 函数功能    : DMA 发送完成中断服务函数
 * 说明(备注)  : 发送完成后关闭通道并切换接收模式
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void printUSART_DMA_TX_IRQ_HANDLER(void)
{
	if (dma_interrupt_flag_get(printUSART_DMA, printUSART_DMA_TX_CH, DMA_INT_FLAG_FTF)) 
	{
		#if (boardIC_TYPE == boardIC_GD32F30X)
		dma_interrupt_flag_clear(printUSART_DMA, printUSART_DMA_TX_CH, DMA_INT_FLAG_G);
		#elif (boardIC_TYPE == boardIC_GD32F50X)
		dma_interrupt_flag_clear(printUSART_DMA, printUSART_DMA_TX_CH, DMA_INT_FLAG_GIF);
		#endif  /* boardIC_TYPE */

		/* 关闭DMA发送 */
		dma_channel_disable(printUSART_DMA, printUSART_DMA_TX_CH);
		/* 发送完成 */
		s_us_data_send_size = 0;
		
		/* 使用TC中断来关闭RS485收发 */
		#if (boardRS485_HARDWARE_TC_PATCH_EN && boardPRINT_485_IFACE_EN)
		usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TC);
		usart_interrupt_enable(printUSART, USART_INT_TC);
		#else
		/* 使用延时来关闭RS485收发 */
		#if (boardPRINT_485_IFACE_EN)
		#if (boardUSE_OS)
		xTimerResetFromISR(tPrintRxEnTimer, 0);
		#else
		bSysTick_PrintSendFinish = true;
		#endif  /* boardUSE_OS */
		#endif  /* boardPRINT_485_IFACE_EN */
		#endif  /* boardRS485_HARDWARE_TC_PATCH_EN */
	}
}

static u16 s_us_read_buff_len = 0;
/***********************************************************************************************************************
 * 函数功能    : 串口空闲/接收中断服务函数
 * 说明(备注)  : 处理空闲帧接收并转存数据至环形缓冲区，通知处理任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void printUSART_IRQ_HANDLER(void)
{
	#if (boardUSE_OS)
	BaseType_t x_woken = pdFALSE;	/* 高优先级任务唤醒标记 */
	#endif  /* boardUSE_OS */

	/* 使用TC中断来关闭RS485收发 */
	#if (boardRS485_HARDWARE_TC_PATCH_EN && boardPRINT_485_IFACE_EN)
	if (RESET != usart_interrupt_flag_get(printUSART, USART_INT_FLAG_TC))
	{
		usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TC);
		usart_interrupt_disable(printUSART, USART_INT_TC);
		printGPIO_485_TX_EN_OFF();  /* 最后一个停止位完全发出，纳秒级瞬间切回接收模式 */
	}
	#endif  /* boardRS485_HARDWARE_TC_PATCH_EN */

	if (RESET != usart_interrupt_flag_get(printUSART, USART_INT_FLAG_IDLE)) 
	{
		/* 清除中断 */
		usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_IDLE);
		/* 清除空闲标志位 */
		usart_data_receive(printUSART);
		/* 关闭DMA传输 */
		dma_channel_disable(printUSART_DMA, printUSART_DMA_RX_CH); 
		
		/* 获取接收到的数据长度，单位：字节 */
		s_us_read_buff_len = printRX_DMA_BUFF_SIZE - dma_transfer_number_get(printUSART_DMA, printUSART_DMA_RX_CH);
		
		/* 转存数据到待处理数据缓冲区 */
		if (s_us_read_buff_len && tpPrintProtoRx != NULL)
			lwrb_write(&tpPrintProtoRx->tRxBuff, s_uca_print_rx_dma_buff, s_us_read_buff_len);

		/* 通知接收任务 */
		#if (boardUSE_OS)
		if (tPrintTaskHandler != NULL)
			vTaskNotifyGiveFromISR(tPrintTaskHandler, &x_woken);
		#endif  /* boardUSE_OS */

		/* 重新设置DMA传输 */
		dma_transfer_number_config(printUSART_DMA, printUSART_DMA_RX_CH, printRX_DMA_BUFF_SIZE);
		dma_channel_enable(printUSART_DMA, printUSART_DMA_RX_CH); 
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
void printUSART_IRQ_HANDLER(void)
{
	#if (boardUSE_OS)
	BaseType_t x_woken = pdFALSE;	/* 高优先级任务唤醒标记 */
	#endif  /* boardUSE_OS */

	/* 串口空闲中断：一帧数据接收完成 */
	if (RESET != usart_interrupt_flag_get(printUSART, USART_INT_FLAG_IDLE))
	{
		usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_IDLE);
		usart_data_receive(printUSART);
		#if (boardUSE_OS)
		if (tPrintTaskHandler != NULL)
			vTaskNotifyGiveFromISR(tPrintTaskHandler, &x_woken);
		#endif  /* boardUSE_OS */
	}

	if (RESET != usart_interrupt_flag_get(printUSART, USART_INT_FLAG_RBNE))
	{
		if (tpPrintProtoRx != NULL)
		{
			uint8_t uc_data = (uint8_t)USART_DATA(printUSART);
			lwrb_write(&tpPrintProtoRx->tRxBuff, &uc_data, 1); 
		}

		usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_RBNE); /* 清除串口接收中断 */

		#if (boardUSE_OS)
		if (tPrintTaskHandler != NULL)
			vTaskNotifyGiveFromISR(tPrintTaskHandler, &x_woken);
		#endif  /* boardUSE_OS */
	} 

	if (RESET != usart_interrupt_flag_get(printUSART, USART_INT_FLAG_TBE))
	{
		usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TBE);
		
		if (s_us_data_send_cnt < s_us_data_send_size)
		{    
			USART_DATA(printUSART) = s_uca_print_tx_dma_buff[s_us_data_send_cnt];
			s_us_data_send_cnt++;
		}
		else
		{
			usart_interrupt_disable(printUSART, USART_INT_TBE);
			s_us_data_send_cnt  = 0;
			s_us_data_send_size = 0;
			
			#if (boardPRINT_485_IFACE_EN)
			#if (boardUSE_OS)
			xTimerResetFromISR(tPrintRxEnTimer, 0);
			#else
			bSysTick_PrintSendFinish = true;
			#endif  /* boardUSE_OS */
			#endif  /* boardPRINT_485_IFACE_EN */
		}               
	}    

	#if (boardUSE_OS)
	portYIELD_FROM_ISR(x_woken);	/* 立即切换到被唤醒的接收任务，不再等待下一个 tick */
	#endif  /* boardUSE_OS */
}
#endif  /* boardPRINT_IFACE_DMA_EN */
#endif  /* boardPRINT_IFACE != 7 */

#endif  /* boardPRINT_IFACE */
