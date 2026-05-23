#include "lv_port_tft_gpio.h"

#if(boardDISPLAY)

#include "lv_port_disp.h"
//#include "display_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  //boardUSE_OS

SPI_HandleTypeDef  hspi4;
SPI_HandleTypeDef  hspi2;
DMA_HandleTypeDef hdma_spi4_tx;

/* SPI2 init function */
void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;   //太快反而获取速度低,暂时不知道是什么原因,应该是IC的速率低
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x0;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}


/* SPI4 init function */
void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */
	
  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 0x0;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */
}


/* SPI4 init function */
void MX_SPI4_DMA_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi4.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi4.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 0x0;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}


void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

  static GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(spiHandle->Instance==SPI2)
  {
  /* USER CODE BEGIN SPI2_MspInit 0 */

  /* USER CODE END SPI2_MspInit 0 */
  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
    PeriphClkInitStruct.PLL2.PLL2M = 10;
    PeriphClkInitStruct.PLL2.PLL2N = 129;
    PeriphClkInitStruct.PLL2.PLL2P = 2;
    PeriphClkInitStruct.PLL2.PLL2Q = 2;
    PeriphClkInitStruct.PLL2.PLL2R = 2;
    PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
    PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
    PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
    PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI2 clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**SPI2 GPIO Configuration
    PB13     ------> SPI2_SCK
    PB14     ------> SPI2_MISO
    PB15     ------> SPI2_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI2_MspInit 1 */

  /* USER CODE END SPI2_MspInit 1 */
  }
  else if(spiHandle->Instance==SPI4)
  {
  /* USER CODE BEGIN SPI4_MspInit 0 */

  /* USER CODE END SPI4_MspInit 0 */
  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI4;
    PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI4 clock enable */
    __HAL_RCC_SPI4_CLK_ENABLE();
	
	__HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**SPI4 GPIO Configuration
    PE11     ------> SPI4_NSS
    PE12     ------> SPI4_SCK
    PE14     ------> SPI4_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	/*Configure GPIO pin : PE15 */
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	/*Configure GPIO pins : PB10 PB11 */
	GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* USER CODE BEGIN SPI4_MspInit 1 */
	/* DMA1_Stream1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
	/* SPI4 DMA Init */
    /* SPI4_TX Init */
	__HAL_RCC_DMA1_CLK_ENABLE();
    hdma_spi4_tx.Instance = DMA1_Stream1;
    hdma_spi4_tx.Init.Request = DMA_REQUEST_SPI4_TX;
    hdma_spi4_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi4_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi4_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi4_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi4_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_spi4_tx.Init.Mode = DMA_NORMAL;
    hdma_spi4_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi4_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi4_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle,hdmatx,hdma_spi4_tx);

    /* SPI4 interrupt Init */
    HAL_NVIC_SetPriority(SPI4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(SPI4_IRQn);
  /* USER CODE BEGIN SPI4_MspInit 1 */
  /* USER CODE END SPI4_MspInit 1 */
  }
}



void vTFT_TouchSpInit(void)
{
	static GPIO_InitTypeDef gpio_init_struct;
	
	//INT
	tftTOUCH_INT_GPIO_RCU();
	gpio_init_struct.Pin = tftTOUCH_INT_PIN;     
	gpio_init_struct.Mode = GPIO_MODE_INPUT;             /* 推完输出 */
	gpio_init_struct.Pull = GPIO_PULLUP;                     /* 不上下拉 */
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;            /* 高速 */
	HAL_GPIO_Init(tftTOUCH_INT_PORT, &gpio_init_struct); 
	
	//CS
	tftTOUCH_CS_GPIO_RCU();
	gpio_init_struct.Pin = tftTOUCH_CS_PIN;     
	gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;             /* 推完输出 */
	gpio_init_struct.Pull = GPIO_NOPULL;                     /* 不上下拉 */
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;            /* 高速 */
	HAL_GPIO_Init(tftTOUCH_CS_PORT, &gpio_init_struct);
	
	#if(tftTOUCH_SPI_SELECT == 2)
	MX_SPI2_Init();
	#elif(tftTOUCH_SPI_SELECT == 1)
	//MISO
	tftTOUCH_MISO_GPIO_RCU();
	gpio_init_struct.Pin = tftTOUCH_MISO_PIN;     
	gpio_init_struct.Mode = GPIO_MODE_INPUT;             /* 推完输出 */
	gpio_init_struct.Pull = GPIO_PULLUP;                     /* 不上下拉 */
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;            /* 高速 */
	HAL_GPIO_Init(tftTOUCH_MISO_PORT, &gpio_init_struct);
	
	//MOSI
	tftTOUCH_MOSI_GPIO_RCU();
	gpio_init_struct.Pin = tftTOUCH_MOSI_PIN;     
	gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;             /* 推完输出 */
	gpio_init_struct.Pull = GPIO_NOPULL;                     /* 不上下拉 */
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;            /* 高速 */
	HAL_GPIO_Init(tftTOUCH_MOSI_PORT, &gpio_init_struct);
	
	//SCLK
	tftTOUCH_SCLK_GPIO_RCU();
	gpio_init_struct.Pin = tftTOUCH_SCLK_PIN;     
	gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;             /* 推完输出 */
	gpio_init_struct.Pull = GPIO_NOPULL;                     /* 不上下拉 */
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;            /* 高速 */
	HAL_GPIO_Init(tftTOUCH_SCLK_PORT, &gpio_init_struct);
	
	
	#endif
}


void DMA1_Stream1_IRQHandler(void)
{
	/* USER CODE BEGIN DMA1_Stream1_IRQn 0 */
	if(__HAL_DMA_GET_FLAG(&hdma_spi4_tx, DMA_FLAG_TCIF0_4) != RESET)
	{
		__HAL_DMA_CLEAR_FLAG(&hdma_spi4_tx,DMA_FLAG_TCIF0_4);
	}
	/* USER CODE END DMA1_Stream1_IRQn 0 */
	HAL_DMA_IRQHandler(&hdma_spi4_tx);
	/* USER CODE BEGIN DMA1_Stream1_IRQn 1 */

	/* USER CODE END DMA1_Stream1_IRQn 1 */
}

void SPI4_IRQHandler(void)
{
	/* USER CODE BEGIN SPI4_IRQn 0 */

	/* USER CODE END SPI4_IRQn 0 */
	HAL_SPI_IRQHandler(&hspi4);
	/* USER CODE BEGIN SPI4_IRQn 1 */

	/* USER CODE END SPI4_IRQn 1 */
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef * hspi)
{
	#if(boardUSE_OS)
	//释放信号量
	disp_enable_update(1);
	#else
	bDispDataSending = 0;
	#endif
}

#endif  //#if(boardDISPLAY)
