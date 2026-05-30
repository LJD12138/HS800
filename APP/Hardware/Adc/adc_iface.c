#include "Adc/adc_iface.h"

#if(boardADC_EN)

u16 adc_value[ADC_CHANNEL_NUM];

/*****************************************************************************************************************
-----函数功能    IO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void gpio_config(void)
{
    // 电池电压 PA7
    rcu_periph_clock_enable(adcSYS_IN_VOLT_RCU);
	gpio_init(adcSYS_IN_VOLT_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcSYS_IN_VOLT_PIN);
	
    // DC温度 PA6
    rcu_periph_clock_enable(adcDC_TEMP_RCU);
	gpio_init(adcDC_TEMP_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcDC_TEMP_PIN);

    // DC电流 PA1
    rcu_periph_clock_enable(adcDC_CURR_RCU);
	gpio_init(adcDC_CURR_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcDC_CURR_PIN);

    // DC电压 PC1
    rcu_periph_clock_enable(adcDC_VOLT_RCU);
	gpio_init(adcDC_VOLT_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcDC_VOLT_PIN);

    // 按键电源 PA0
    rcu_periph_clock_enable(adcKEY_POWER_RCU);
	gpio_init(adcKEY_POWER_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcKEY_POWER_PIN);
	
    // DC输入电压1 PC5
    rcu_periph_clock_enable(adcDC_IN_1_RCU);
	gpio_init(adcDC_IN_1_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcDC_IN_1_PIN);
    
    // DC输入电压2 PB0
    rcu_periph_clock_enable(adcDC_IN_2_RCU);
	gpio_init(adcDC_IN_2_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcDC_IN_2_PIN);
}



static void delay_1ms(u16 time)
{    
   vu32 i = 0;
   while(time--)
   {
      i = 24000;  
      while(i--);    
   }
}

/*****************************************************************************************************************
-----函数功能    DMA初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
#if(ADC_DMAX)
static void dma_config(void)
{
	/* ADC_DMA_channel configuration */
	dma_parameter_struct dma_data_parameter;

    /* ADC DMA_channel configuration */
    dma_deinit(adcDMA, adcDMA_CH);

	 /* initialize DMA single data mode */
    dma_data_parameter.periph_addr  = (uint32_t)(&ADC_RDATA(ADCX));
    dma_data_parameter.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
    dma_data_parameter.memory_addr  = (uint32_t)(&adc_value);
    dma_data_parameter.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
    dma_data_parameter.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_data_parameter.memory_width = DMA_MEMORY_WIDTH_16BIT;  
    dma_data_parameter.direction    = DMA_PERIPHERAL_TO_MEMORY;
    dma_data_parameter.number       = ADC_CHANNEL_NUM;
    dma_data_parameter.priority     = DMA_PRIORITY_HIGH;
    dma_init(adcDMA, adcDMA_CH, &dma_data_parameter);

    /* enable DMA circulation mode */
    dma_circulation_enable(adcDMA, adcDMA_CH);

    /* enable DMA channel */
    dma_channel_enable(adcDMA, adcDMA_CH);
}
#endif

/*****************************************************************************************************************
-----函数功能    ADC初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void adc_config(void)
{
    /* ADC mode config */
	adc_mode_config(ADC_MODE_FREE); 
    /* ADC contineous function disable */
    adc_special_function_config(ADCX, ADC_CONTINUOUS_MODE, ENABLE);
    /* ADC scan mode disable */
    adc_special_function_config(ADCX, ADC_SCAN_MODE, ENABLE);
    /* ADC data alignment config */
    adc_data_alignment_config(ADCX, ADC_DATAALIGN_RIGHT);

    /* ADC channel length config */
    adc_channel_length_config(ADCX, ADC_REGULAR_CHANNEL, ADC_CHANNEL_NUM);

    /* ADC regular channel config */ 
    adc_regular_channel_config(ADCX, 0, adcSYS_IN_VOLT_CH , ADC_SAMPLETIME_239POINT5);  // 电池电压
    adc_regular_channel_config(ADCX, 1, adcDC_TEMP_CH , ADC_SAMPLETIME_239POINT5);      // DC温度
    adc_regular_channel_config(ADCX, 2, adcDC_CURR_CH, ADC_SAMPLETIME_239POINT5);       // DC电流
    adc_regular_channel_config(ADCX, 3, adcDC_VOLT_CH, ADC_SAMPLETIME_239POINT5);       // DC电压
    adc_regular_channel_config(ADCX, 4, adcKEY_POWER_CH, ADC_SAMPLETIME_239POINT5);     // 按键电源
    adc_regular_channel_config(ADCX, 5, adcDC_IN_1_CH, ADC_SAMPLETIME_239POINT5);       // DC输入电压1
    adc_regular_channel_config(ADCX, 6, adcDC_IN_2_CH, ADC_SAMPLETIME_239POINT5);       // DC输入电压2
	
    /* ADC trigger config */
	adc_external_trigger_source_config(ADCX, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADCX, ADC_REGULAR_CHANNEL, ENABLE);

    /* ADC DMA function enable */
    adc_dma_mode_enable(ADCX);

    /* enable ADC interface */
    adc_enable(ADCX);
    /* wait for ADC stability */
    delay_1ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADCX);

    /* enable ADC software trigger */
    adc_software_trigger_enable(ADCX, ADC_REGULAR_CHANNEL);
}


void vAdc_Init(void)
{
	/* enable ADCX clock */
	rcu_periph_clock_enable(ADCX_RCU);
	/* enable adcDMA clock */
	rcu_periph_clock_enable(adcDMA_RCU);  
	 /* config ADC clock */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV16);
   /*=============================配置ADC=============================*/    
	
    gpio_config();
	/* DMA configuration */
	#if (ADC_DMAX)
    dma_config();
	#endif
    /* ADC configuration */
    adc_config();
}

void vAdc_DeInit(void)
{
	adc_deinit(ADCX);
}

#if(boardLOW_POWER)
void vAdc_IoEnterLowPower(void)
{
	gpio_init(adcMppt_TEMP_EN_GPIO, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, adcMppt_TEMP_EN_PIN);
	
	adc_disable(ADCX);
	dma_channel_disable(ADCX, adcDMA_CH);
	rcu_periph_clock_disable(ADCX_RCU);
	rcu_periph_clock_disable(adcDMA_RCU); 
}
#endif

#endif  //boardADC_EN




