#include "board_config.h"

#if(boardPRINT_IFACE)
#include "Print/print_iface.h"
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"

#include "lwrb.h"

#if(!boardUSE_OS)
#include "systick.h"
#else
#include "timer_task.h"
#endif  //boardUSE_OS

#define    		printRX_DMA_BUFF_SIZE   				256
#define    		printTX_DMA_BUFF_SIZE   				256

static vu16 	S_DataSendSize = 0;
static vu16 	S_DataSendCnt = 0;

#if(boardPRINT_IFACE_DMA_EN)
static __ALIGNED(4) u8 ucaPrintRxDmaBuffData[printRX_DMA_BUFF_SIZE];   //用于把数据装载到DMA发送 
#endif
static __ALIGNED(4) u8 ucaPrintTxDmaBuffData[printTX_DMA_BUFF_SIZE];   	//用于把数据装载到DMA发送


/***********************************************************************************************************************
-----函数功能    printf接口映射
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if boardCM_BACKTRACE
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 
 
FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{
	#if(boardPRINT_485_IFACE_EN)
	vPrint_485TransEnable(true);
	#endif

	#if(boardIC_TYPE == boardIC_GD32F30X)
	usart_data_transmit(printUSART, (uint8_t) ch);
    while(RESET == usart_flag_get(printUSART, USART_FLAG_TBE));
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, 0xFFFF);
	#endif

	#if(boardPRINT_485_IFACE_EN)
	vPrint_485TransEnable(false);
	#endif
    return ch;
}
#endif 

/***********************************************************************************************************************
-----函数功能    IO口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_print_gpio_init(void)
{
	/*使能复用时钟*/
	rcu_periph_clock_enable(RCU_AF); //开启复用外设时钟使能
	#if(gpioUSART0_REMAP_EN)
	//重映射串口0
	gpio_pin_remap_config(GPIO_USART0_REMAP,ENABLE);
	#endif
	
	//TX
    rcu_periph_clock_enable(printUSART_GPIO_TX_RCU);
    gpio_init(printUSART_GPIO_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, printUSART_GPIO_TX_PIN);
    
	//RX
    rcu_periph_clock_enable(printUSART_GPIO_RX_RCU);
    gpio_init(printUSART_GPIO_RX_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, printUSART_GPIO_RX_PIN);
	
	#if(boardPRINT_485_IFACE_EN)
    //458 发射使能 
	rcu_periph_clock_enable(printGPIO_485_TX_EN_RCU);
	gpio_init(printGPIO_485_TX_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, printGPIO_485_TX_EN_PIN);
	printGPIO_485_TX_EN_OFF();  //默认接收
	#endif
	
	//接口使能
	rcu_periph_clock_enable(printIFACE_EN_RCU);
	gpio_init(printIFACE_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, printIFACE_EN_PIN);
	printIFACE_EN_ON();  //默认接收
}

/***********************************************************************************************************************
-----函数功能    串口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_print_usart_init( void )
{
    /* enable USART clock */
    rcu_periph_clock_enable(printUSART_RCU);

    /* USART configure */
    usart_deinit(printUSART);
    usart_baudrate_set(printUSART, printUSART_BAUD);
    usart_word_length_set(printUSART, USART_WL_8BIT);
    usart_stop_bit_set(printUSART, USART_STB_1BIT);
    usart_parity_config(printUSART, USART_PM_NONE);
    usart_hardware_flow_rts_config(printUSART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(printUSART, USART_CTS_DISABLE);
    
    usart_receive_config(printUSART, USART_RECEIVE_ENABLE);
    usart_transmit_config(printUSART, USART_TRANSMIT_ENABLE);
	
    #if(!boardPRINT_IFACE_DMA_EN)
    /* USART interrupt configuration */
    nvic_irq_enable(printUSART_IRQ, 2, 0);
    
    usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_RBNE);
    usart_interrupt_enable(printUSART, USART_INT_RBNE); 
    
    usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TBE);
    usart_interrupt_disable(printUSART, USART_INT_TBE); 	
    #endif	

    usart_enable(printUSART);
}


/***********************************************************************************************************************
-----函数功能    DMA初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if(boardPRINT_IFACE_DMA_EN)
static void v_print_dma_init(void)
{
	/* USART interrupt configuration */
	nvic_irq_enable(printUSART_DMA_TX_IRQ, 2, 0);
	nvic_irq_enable(printUSART_IRQ, 2, 0);
	
	dma_parameter_struct dma_init_struct;
	
	/* enable DMA0 clock */
	rcu_periph_clock_enable(printUSART_DMA_RCU);
	
    /* initialize DMA channel(USART TX) */
    dma_deinit(printUSART_DMA, printUSART_DMA_TX_CH);
	/* initialize DMA parameters */
    dma_struct_para_init(&dma_init_struct);
	
    dma_init_struct.direction    = DMA_MEMORY_TO_PERIPHERAL;            /* 外设到内存 */              
    dma_init_struct.memory_addr  = (uint32_t)ucaPrintTxDmaBuffData;    	/* 设置内存接收基地址 */
    dma_init_struct.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;          /* 内存地址递增 */
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;               /* 8位内存数据 */
    dma_init_struct.number       = 0;  									/* Buff数组的大小 */
    dma_init_struct.periph_addr  = (uint32_t)(&USART_DATA(printUSART));	/* 外设基地址,USART数据寄存器地址 */
    dma_init_struct.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;         /* 外设地址不递增 */
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;           /* 8位外设数据 */
    dma_init_struct.priority     = DMA_PRIORITY_ULTRA_HIGH;             /* 最高DMA通道优先级 */
    dma_init(printUSART_DMA, printUSART_DMA_TX_CH, &dma_init_struct);
    
    
	/* initialize DMA channel(USART RX) */
    dma_deinit(printUSART_DMA, printUSART_DMA_RX_CH);

    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.number = printRX_DMA_BUFF_SIZE;
    dma_init_struct.memory_addr = (uint32_t)ucaPrintRxDmaBuffData;
    dma_init(printUSART_DMA, printUSART_DMA_RX_CH, &dma_init_struct);
	
	
	dma_circulation_disable(printUSART_DMA, printUSART_DMA_TX_CH);                    /* 关闭DMA_TX循环模式 */
	dma_memory_to_memory_disable(printUSART_DMA, printUSART_DMA_TX_CH);               /* DMA内存到内存模式不开启 */
	dma_circulation_disable(printUSART_DMA, printUSART_DMA_RX_CH);                    /* 关闭DMA_RX循环模式 */
    dma_memory_to_memory_disable(printUSART_DMA, printUSART_DMA_RX_CH);               /* DMA内存到内存模式不开启 */
	
	/* enable USART DMA for reception */
    usart_dma_receive_config(printUSART, USART_RECEIVE_DMA_ENABLE);
    /* enable DMA0 channel4 transfer complete interrupt */
//    dma_interrupt_enable(printUSART_DMA, printUSART_DMA_RX_CH, DMA_INT_FTF);
    /* enable DMA0 channel4 */
    dma_channel_enable(printUSART_DMA, printUSART_DMA_RX_CH);
	
    /* enable USART DMA for transmission */
    usart_dma_transmit_config(printUSART,USART_TRANSMIT_DMA_ENABLE);;
    /* enable DMA0 channel3 transfer complete interrupt */
    dma_interrupt_enable(printUSART_DMA, printUSART_DMA_TX_CH, DMA_INT_FTF);
    /* enable DMA0 channel3 */
    dma_channel_disable(printUSART_DMA, printUSART_DMA_TX_CH);
    
	//串口空闲中断
    usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_IDLE);
    usart_interrupt_enable(printUSART, USART_INT_IDLE); 
}
#endif

/***********************************************************************************************************************
-----函数功能    Print初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vPrint_Init( void )//串口初始化
{
	v_print_gpio_init();
	v_print_usart_init();
	#if(boardPRINT_IFACE_DMA_EN)
	v_print_dma_init();
	#endif
}

/***********************************************************************************************************************
-----函数功能    端口重置
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送完成    false:发送中  
************************************************************************************************************************/
void vPrint_DeInit( void )
{
	usart_deinit(printUSART);
	
	#if(boardPRINT_IFACE_DMA_EN)
    dma_deinit(printUSART_DMA, printUSART_DMA_TX_CH);
	dma_deinit(printUSART_DMA, printUSART_DMA_RX_CH);
	#endif
}

/*****************************************************************************************************************
-----函数功能    串口发射函数
-----说明(备注)  none
-----传入参数    SendSize:要发送数据的大小
-----输出参数    none
-----返回值      true:成功    false:失败
******************************************************************************************************************/
bool bPrint_DataSendStart(u16 len)
{
	#if(boardPRINT_485_IFACE_EN)
	vPrint_485TransEnable(true);
	#endif
	
	if(len == 0)
		return false;
	
	if(len > printTX_DMA_BUFF_SIZE)
		len = printTX_DMA_BUFF_SIZE;
	
	//取出数据
	S_DataSendSize = lwrb_read(&tPrintTxBuff, ucaPrintTxDmaBuffData, len);
	
	#if(boardPRINT_IFACE_DMA_EN)
	//清除全部发送完成标志位
	dma_flag_clear(printUSART_DMA, printUSART_DMA_TX_CH, DMA_FLAG_FTF); 
	//装载数据
	dma_memory_address_config(printUSART_DMA, printUSART_DMA_TX_CH,(uint32_t)ucaPrintTxDmaBuffData);
	//装载长度
	dma_transfer_number_config(printUSART_DMA,printUSART_DMA_TX_CH,S_DataSendSize);
	//开始DMA发送
	dma_channel_enable(printUSART_DMA, printUSART_DMA_TX_CH);
	return true;
	
	#else
	
    S_DataSendCnt = 0; 
	usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TBE);
	//空闲就产生中断
	usart_interrupt_enable(printUSART, USART_INT_TBE);           
    return true;
	#endif  //boardPRINT_IFACE_DMA_EN
}

/*****************************************************************************************************************
-----函数功能    485发送使能
-----说明(备注)  none
-----传入参数    en true:开启    false:关闭
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
#if(boardPRINT_485_IFACE_EN)
void vPrint_485TransEnable(bool en)
{
	if(en == true)
		printGPIO_485_TX_EN_ON();  //切换为发送模式
	else
	{
		printGPIO_485_TX_EN_OFF();  //切换为接收模式
		
		#if(!boardUSE_OS)
		bSysTick_PrintSendFinish = false;
		#endif
	}
}
#endif

/***********************************************************************************************************************
-----函数功能    检查发射情况
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送完成    false:发送中  
************************************************************************************************************************/
bool bPrint_CheckSendFinish(void)
{
	if(S_DataSendSize)
		return false;
	else 
		return true;
}

/***********************************************************************************************************************
-----函数功能    进入低功耗/清除中断
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if(boardLOW_POWER)
void vPrint_EnterLowPower( void )
{
	rcu_periph_clock_enable(printUSART_GPIO_TX_RCU);
    gpio_init(printUSART_GPIO_TX_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, printUSART_GPIO_TX_PIN);

    rcu_periph_clock_enable(printUSART_GPIO_RX_RCU);
    gpio_init(printUSART_GPIO_RX_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, printUSART_GPIO_RX_PIN);
	
	rcu_periph_clock_disable(printUSART_GPIO_RX_RCU);
	rcu_periph_clock_disable(printUSART_GPIO_TX_RCU);
	rcu_periph_clock_disable(printUSART_RCU);
	usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_RBNE);
    usart_interrupt_disable(printUSART, USART_INT_RBNE); 
    usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TBE);
    usart_interrupt_disable(printUSART, USART_INT_TBE); 
	usart_disable(printUSART);
}
#endif //boardLOW_POWER


#if(boardPRINT_IFACE_DMA_EN)
/***********************************************************************************************************************
-----函数功能    DMA发送完成中断
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void printUSART_DMA_TX_IRQ_HANDLER(void)
{
    if(dma_interrupt_flag_get(printUSART_DMA, printUSART_DMA_TX_CH, DMA_INT_FLAG_FTF)) 
	{
        dma_interrupt_flag_clear(printUSART_DMA, printUSART_DMA_TX_CH, DMA_INT_FLAG_G);
		//关闭DMA发送
	    dma_channel_disable(printUSART_DMA, printUSART_DMA_TX_CH);
		//发送完成
		S_DataSendSize = 0;
		
		//延时关闭
		#if(boardPRINT_485_IFACE_EN)
		#if(!boardUSE_OS)
		bSysTick_PrintSendFinish = true;
		#else
		xTimerResetFromISR(tPrintRxEnTimer,0);
		#endif  //boardUSE_OS
		#endif  //boardPRINT_485_IFACE_EN
    }
}


/***********************************************************************************************************************
-----函数功能    串口接收完成中断
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static u8 uc_read_buff_len=0;
void printUSART_IRQ_HANDLER(void)
{
    if(RESET != usart_interrupt_flag_get(printUSART, USART_INT_FLAG_IDLE)) 
	{
		//清除中断
        usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_IDLE);
		//清除空闲标志位
		usart_data_receive(printUSART);
		//关闭DMA传输
		dma_channel_disable(printUSART_DMA, printUSART_DMA_RX_CH); 
		
		//获取接收到的数据长度，单位：字节
		uc_read_buff_len = printRX_DMA_BUFF_SIZE - dma_transfer_number_get(printUSART_DMA,printUSART_DMA_RX_CH);
		//转存数据到待处理数据缓冲区
		if(tpPrintProtoRx != NULL)
			lwrb_write(&tpPrintProtoRx->tRxBuff, ucaPrintRxDmaBuffData, uc_read_buff_len);
		//通知接收任务
		#if(boardUSE_OS)
		vTaskNotifyGiveFromISR(tPrintTaskHandler,NULL);
		#endif  //boardUSE_OS

		//重新设置DMA传输
		//装载长度
		dma_transfer_number_config(printUSART_DMA,printUSART_DMA_RX_CH,printRX_DMA_BUFF_SIZE);
		//开启DMA传输
		dma_channel_enable(printUSART_DMA, printUSART_DMA_RX_CH); 
    }
}

#else
/***********************************************************************************************************************
-----函数功能    串口中断
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void printUSART_IRQ_HANDLER(void)
{
    if(RESET != usart_interrupt_flag_get(printUSART, USART_INT_FLAG_RBNE))
	{
		usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_RBNE);

		if(tpPrintProtoRx != NULL)
			lwrb_write(&tpPrintProtoRx->tRxBuff, (u8*)&USART_DATA(printUSART), 1); 
    } 

	if(RESET != usart_interrupt_flag_get(printUSART, USART_INT_FLAG_TBE))
    {
        usart_interrupt_flag_clear(printUSART, USART_INT_FLAG_TBE);
        
        if(S_DataSendCnt < S_DataSendSize)
        {    
            USART_DATA(printUSART) = ucaPrintTxDmaBuffData[S_DataSendCnt];
			S_DataSendCnt++;
        }
        else
        {
            usart_interrupt_disable(printUSART, USART_INT_TBE);  //清除空闲中断
            S_DataSendCnt = 0;
            S_DataSendSize = 0;
			
			#if(boardPRINT_485_IFACE_EN)
			#if(!boardUSE_OS)
			bSysTick_PrintSendFinish = true;
			#else
			xTimerResetFromISR(tPrintRxEnTimer,0);
			#endif  //boardUSE_OS
			#endif  //boardPRINT_485_IFACE_EN
        }               
    }    
}
#endif  //boardPRINT_IFACE_DMA_EN

#endif  //boardPRINT_IFACE


