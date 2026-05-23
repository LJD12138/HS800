#include "MD_Bms/md_bms_iface.h"

#if(boardBMS_IFACE && boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "lwrb.h"

// #if(boardMPPT_EN)
// #include "MD_Mppt/md_mppt_rec_task.h"
// #include "MD_Mppt/md_mppt_prot_frame.h"
// #endif  //boardMPPT_EN

#if(boardBMS_485_IFACE_EN)
#if(boardUSE_OS)
#include "timer_task.h"
#else
#include "systick.h"
#endif
#endif  //boardCONSOLE_485_IFACE_EN


//****************************************************参数初始化**************************************************//
#define    		bmsRX_DMA_BUFF_SIZE   					256
#define    		bmsTX_DMA_BUFF_SIZE        				256      //DMA数组大小

//0:MPPT 1:BMS  
__IO bool bBmsUseFlag = true;

static vu16  	S_DataSendSize = 0;
static vu16  	S_DataSendCnt = 0;

#if(boardBMS_IFACE_DMA_EN)
static __ALIGNED(4) u8   ucaBmsRxDmaBuffData[bmsRX_DMA_BUFF_SIZE];   //用于把数据装载到DMA发送 
#endif
static __ALIGNED(4) u8   ucaBmsTxDmaBuffData[bmsTX_DMA_BUFF_SIZE];   //取出接收缓存器中的数据,用于DMA发送


/*****************************************************************************************************************
-----函数功能    相关IO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_bms_io_init(void)//IO设置
{
	/*使能复用时钟*/
	rcu_periph_clock_enable(RCU_AF); 
	
    /* enable COM GPIO clock */
    rcu_periph_clock_enable(bmsUSART_GPIO_TX_RCU);
    /* connect port to USARTx_Tx */
    gpio_init(bmsUSART_GPIO_TX_GPIO, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, bmsUSART_GPIO_TX_PIN);	
	
	/* enable COM GPIO clock */
    rcu_periph_clock_enable(bmsUSART_GPIO_RX_RCU);
    /* connect port to USARTx_Rx */
    gpio_init(bmsUSART_GPIO_RX_GPIO, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ,bmsUSART_GPIO_RX_PIN);	
	
	#if(boardBMS_485_IFACE_EN)
    //-------BMS 458 发射使能 --------------------------------------------------------
	rcu_periph_clock_enable(bmsGPIO_485_TX_EN_RCU);
	gpio_init(bmsGPIO_485_TX_EN_GPIO, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, bmsGPIO_485_TX_EN_PIN);
	bmsGPIO_485_TX_EN_OFF();  //默认接收
	#endif
}


/*****************************************************************************************************************
-----函数功能    串口配置
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_bms_usart_init(void)
{
    /* enable USART clock */
    rcu_periph_clock_enable(bmsUSART_RCU);

    /* USART configure */
    usart_deinit(bmsUSART);
    usart_baudrate_set(bmsUSART, bmsUSART_BAUD);
    usart_word_length_set(bmsUSART, USART_WL_8BIT);
    usart_stop_bit_set(bmsUSART, USART_STB_1BIT);
    usart_parity_config(bmsUSART, USART_PM_NONE);
    usart_hardware_flow_rts_config(bmsUSART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(bmsUSART, USART_CTS_DISABLE);
    
    usart_receive_config(bmsUSART, USART_RECEIVE_ENABLE);
    usart_transmit_config(bmsUSART, USART_TRANSMIT_ENABLE);
	
	#if(!boardBMS_IFACE_DMA_EN)
	/* USART interrupt configuration */
    nvic_irq_enable(bmsUSART_IRQ, 2, 0);
	
    usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_RBNE);
    usart_interrupt_enable(bmsUSART, USART_INT_RBNE);   /* 接收中断 */
    
    usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_TBE);
    usart_interrupt_disable(bmsUSART, USART_INT_TBE); 
	#endif

    usart_enable(bmsUSART);
	
	
}



/***********************************************************************************************************************
-----函数功能    DMA初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if(boardBMS_IFACE_DMA_EN)
static void v_bms_dma_init(void)
{
	/* USART interrupt configuration */
	nvic_irq_enable(bmsUSART_DMA_TX_IRQ, 2, 0);
	nvic_irq_enable(bmsUSART_IRQ, 2, 0);
	
	dma_parameter_struct dma_init_struct;
	
	/* enable DMA0 clock */
	rcu_periph_clock_enable(bmsUSART_DMA_RCU);
	
    /* initialize DMA channel(USART TX) */
    dma_deinit(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
	/* initialize DMA parameters */
    dma_struct_para_init(&dma_init_struct);
	
    dma_init_struct.direction    = DMA_MEMORY_TO_PERIPHERAL;            /* 外设到内存 */              
    dma_init_struct.memory_addr  = (uint32_t)ucaBmsTxDmaBuffData;       /* 设置内存接收基地址 */
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

    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.number = bmsRX_DMA_BUFF_SIZE;
    dma_init_struct.memory_addr = (uint32_t)ucaBmsRxDmaBuffData;
    dma_init(bmsUSART_DMA, bmsUSART_DMA_RX_CH, &dma_init_struct);
	
	
	dma_circulation_disable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);			/* 关闭DMA_TX循环模式 */
	dma_memory_to_memory_disable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);		/* DMA内存到内存模式不开启 */
	dma_circulation_disable(bmsUSART_DMA, bmsUSART_DMA_RX_CH);			/* 关闭DMA_RX循环模式 */
    dma_memory_to_memory_disable(bmsUSART_DMA, bmsUSART_DMA_RX_CH);		/* DMA内存到内存模式不开启 */
	
	/* enable USART DMA for reception */
    usart_dma_receive_config(bmsUSART, USART_RECEIVE_DMA_ENABLE);
    /* enable DMA0 channel4 transfer complete interrupt */
//    dma_interrupt_enable(bmsUSART_DMA, bmsUSART_DMA_RX_CH, DMA_INT_FTF);
    /* enable DMA0 channel4 */
    dma_channel_enable(bmsUSART_DMA, bmsUSART_DMA_RX_CH);
	
    /* enable USART DMA for transmission */
    usart_dma_transmit_config(bmsUSART,USART_TRANSMIT_DMA_ENABLE);;
    /* enable DMA0 channel3 transfer complete interrupt */
    dma_interrupt_enable(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_INT_FTF);
    /* enable DMA0 channel3 */
    dma_channel_disable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
    
	//串口空闲中断
    usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_IDLE);
    usart_interrupt_enable(bmsUSART, USART_INT_IDLE); 
}
#endif

/*****************************************************************************************************************
-----函数功能    串口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vBms_IfaceInit(void)
{
    v_bms_io_init();
	
    v_bms_usart_init();
	
	#if(boardBMS_IFACE_DMA_EN)
	v_bms_dma_init();
	#endif
}

/*****************************************************************************************************************
-----函数功能    串口重置
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vBms_IfaceDeInit(void)
{
     usart_deinit(bmsUSART);
	
	#if(boardBMS_IFACE_DMA_EN)
	/* initialize DMA channel(USART TX) */
    dma_deinit(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
	/* initialize DMA channel(USART RX) */
    dma_deinit(bmsUSART_DMA, bmsUSART_DMA_RX_CH);
	#endif  //boardBMS_IFACE_DMA_EN
}


/*****************************************************************************************************************
-----函数功能    串口发射函数
-----说明(备注)  none
-----传入参数    addr:要发送数据的地址
-----输出参数    none
-----返回值      true:成功    false:失败
******************************************************************************************************************/
bool bBms_DataSendStart(u8* data,u16 len)
{
	#if(boardBMS_485_IFACE_EN)
	vBms_485TransEnable(true);
	#endif
	
	if(data == NULL || len == 0)
		return false;
	
	if(len > bmsTX_DMA_BUFF_SIZE)
		len = bmsTX_DMA_BUFF_SIZE;
	
	memcpy(ucaBmsTxDmaBuffData, data, len);
	
	#if(boardBMS_IFACE_DMA_EN)
	//清除全部发送完成标志位
	dma_flag_clear(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_FLAG_FTF); 
	//装载数据
	dma_memory_address_config(bmsUSART_DMA, bmsUSART_DMA_TX_CH,(uint32_t)ucaBmsTxDmaBuffData);
	//装载长度
	dma_transfer_number_config(bmsUSART_DMA,bmsUSART_DMA_TX_CH,len);
	//开始DMA发送
	dma_channel_enable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
	return true;
	
	#else
	
    if(S_DataSendCnt) //发送中
        return false; 
    
    S_DataSendSize = length;
    S_DataSendCnt = 0; 
	usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_TBE);
	//空闲就产生中断
	usart_interrupt_enable(bmsUSART, USART_INT_TBE);           
    return true;
	
	#endif
}

/*****************************************************************************************************************
-----函数功能    485发送使能
-----说明(备注)  none
-----传入参数    en true:开启    false:关闭
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
#if(boardBMS_485_IFACE_EN)
void vBms_485TransEnable(bool en)
{
	if(en == true)
		bmsGPIO_485_TX_EN_ON();  //切换为发送模式
	else
	{
		bmsGPIO_485_TX_EN_OFF();  //切换为接收模式
		#if(!boardUSE_OS)
		bSysTick_BmsSendFinish = false;
		#endif  //boardUSE_OS
	}
}
#endif



#if(boardBMS_IFACE_DMA_EN)
/***********************************************************************************************************************
-----函数功能    DMA发送完成中断
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void bmsUSART_DMA_TX_IRQ_HANDLER(void)
{
    if(dma_interrupt_flag_get(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_INT_FLAG_FTF)) 
	{
        dma_interrupt_flag_clear(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_INT_FLAG_G);
		//关闭DMA发送
	    dma_channel_disable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
		//发送完成
		S_DataSendSize = 0;
		
		//延时关闭
		#if(boardBMS_485_IFACE_EN)
		#if(boardUSE_OS)
		xTimerResetFromISR(tBmsRxEnTimer,0);
		#else
		bSysTick_BmsSendFinish = true;
		#endif  //boardUSE_OS
		#endif  //boardBMS_485_IFACE_EN
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
void bmsUSART_IRQ_HANDLER(void)
{
    if(RESET != usart_interrupt_flag_get(bmsUSART, USART_INT_FLAG_IDLE)) 
	{
		//清除中断
        usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_IDLE);
		//清除空闲标志位
		usart_data_receive(bmsUSART);
		//关闭DMA传输
		dma_channel_disable(bmsUSART_DMA, bmsUSART_DMA_RX_CH); 
		
		//获取接收到的数据长度，单位：字节
		uc_read_buff_len = bmsRX_DMA_BUFF_SIZE - dma_transfer_number_get(bmsUSART_DMA,bmsUSART_DMA_RX_CH);

		//转存数据到待处理数据缓冲区
		//BMS
		if(bBmsUseFlag == true)
		{
			if(uc_read_buff_len && tpBmsProtoRx != NULL)
				lwrb_write(&tpBmsProtoRx->tRxBuff, ucaBmsRxDmaBuffData, uc_read_buff_len);

			//通知接收任务
			#if(boardUSE_OS)
			vTaskNotifyGiveFromISR(tBmsRecTaskHandle,NULL);
			#endif  //boardUSE_OS
		}
		// MPPT
		// else
		// {
		// 	if(uc_read_buff_len && tpMpptProtoRx != NULL)
		// 		lwrb_write(&tpMpptProtoRx->tRxBuff, ucaBmsRxDmaBuffData, uc_read_buff_len);

		// 	//通知接收任务
		// 	#if(boardUSE_OS)
		// 	vTaskNotifyGiveFromISR(tMpptRecTaskHandle,NULL);
		// 	#endif  //boardUSE_OS
		// }

		//重新设置DMA传输
		//装载长度
		dma_transfer_number_config(bmsUSART_DMA,bmsUSART_DMA_RX_CH,bmsRX_DMA_BUFF_SIZE);
		//开启DMA传输
		dma_channel_enable(bmsUSART_DMA, bmsUSART_DMA_RX_CH);    
    }
}
#else
/*****************************************************************************************************************
-----函数功能    串口中断函数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void bmsUSART_IRQ_HANDLER(void)
{

    if(RESET != usart_interrupt_flag_get(bmsUSART, USART_INT_FLAG_RBNE))
    {
		if(tpBmsProtoRx != NULL)
			lwrb_write(&tpBmsProtoRx->tRxBuff, USART_DATA(bmsUSART), 1);  

        usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_RBNE);//清除串口接收中断 

		//通知接收任务
		#if(boardUSE_OS)
        vTaskNotifyGiveFromISR(tBmsRecTaskHandle,NULL);
		#endif  //boardUSE_OS
    }

    if(RESET != usart_interrupt_flag_get(bmsUSART, USART_INT_FLAG_TBE))
    {
        usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_TBE);
        
        if(S_DataSendCnt < S_DataSendSize)
        {    
            USART_DATA(bmsUSART) = ucaBmsTxDmaBuffData[S_DataSendCnt];
			S_DataSendCnt++;
        }
        else
        {
            usart_interrupt_disable(bmsUSART, USART_INT_TBE);
            S_DataSendCnt = 0;
            S_DataSendSize = 0;
			
			//延时关闭
			#if(boardBMS_485_IFACE_EN)
			#if(boardUSE_OS)
			xTimerResetFromISR(tBmsRxEnTimer,0);
			#else
			bSysTick_BmsSendFinish = true;
			#endif  //boardUSE_OS
			#endif  //boardBMS_485_IFACE_EN
        }               
    }    
}
#endif  //boardBMS_IFACE_DMA_EN

#endif  //boardBMS_IFACE && boardBMS_EN














