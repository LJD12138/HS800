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
    rcu_periph_clock_enable(adcSYS_IN_VOLT_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(adcSYS_IN_VOLT_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, adcSYS_IN_VOLT_PIN);
	#else
	gpio_init(adcSYS_IN_VOLT_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcSYS_IN_VOLT_PIN);
	#endif
	
    rcu_periph_clock_enable(adcDC_TEMP_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(adcDC_TEMP_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, adcDC_TEMP_PIN);
	#else
	gpio_init(adcDC_TEMP_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcDC_TEMP_PIN);
	#endif

    rcu_periph_clock_enable(adcDC_CURR_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(adcDC_CURR_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, adcDC_CURR_PIN);
	#else
	gpio_init(adcDC_CURR_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcDC_CURR_PIN);
	#endif

    rcu_periph_clock_enable(adcDC_VOLT_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(adcDC_VOLT_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, adcDC_VOLT_PIN);
	#else
	gpio_init(adcDC_VOLT_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcDC_VOLT_PIN);
	#endif
	
	rcu_periph_clock_enable(adcUSB_TEMP_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(adcUSB_TEMP_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, adcUSB_TEMP_PIN);
	#else
	gpio_init(adcUSB_TEMP_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcUSB_TEMP_PIN);
	#endif
	
	rcu_periph_clock_enable(adcUSB_PD_CURR_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(adcUSB_PD_CURR_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, adcUSB_PD_CURR_PIN);
	#else
	gpio_init(adcUSB_PD_CURR_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcUSB_PD_CURR_PIN);
	#endif
	
	rcu_periph_clock_enable(adcUSB_PD_VOLT_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(adcUSB_PD_VOLT_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, adcUSB_PD_VOLT_PIN);
	#else
	gpio_init(adcUSB_PD_VOLT_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcUSB_PD_VOLT_PIN);
	#endif
	
	rcu_periph_clock_enable(adcKEY_POWER_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(adcKEY_POWER_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, adcKEY_POWER_PIN);
	#else
	gpio_init(adcKEY_POWER_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, adcKEY_POWER_PIN);
	#endif
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
    adc_regular_channel_config(ADCX, 0, adcSYS_IN_VOLT_CH , ADC_SAMPLETIME_239POINT5);
    adc_regular_channel_config(ADCX, 1, adcDC_TEMP_CH , ADC_SAMPLETIME_239POINT5);
    adc_regular_channel_config(ADCX, 2, adcDC_CURR_CH, ADC_SAMPLETIME_239POINT5);
    adc_regular_channel_config(ADCX, 3, adcDC_VOLT_CH, ADC_SAMPLETIME_239POINT5);
    adc_regular_channel_config(ADCX, 4, adcUSB_TEMP_CH, ADC_SAMPLETIME_239POINT5);
    adc_regular_channel_config(ADCX, 5, adcUSB_PD_CURR_CH, ADC_SAMPLETIME_239POINT5);
	adc_regular_channel_config(ADCX, 6, adcUSB_PD_VOLT_CH, ADC_SAMPLETIME_239POINT5);
	adc_regular_channel_config(ADCX, 7, adcKEY_POWER_CH, ADC_SAMPLETIME_239POINT5);
	
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
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(adcMppt_TEMP_EN_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, adcMppt_TEMP_EN_PIN);
	#else
	gpio_init(adcMppt_TEMP_EN_GPIO, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, adcMppt_TEMP_EN_PIN);
	#endif
	
	adc_disable(ADCX);
	dma_channel_disable(ADCX, adcDMA_CH);
	rcu_periph_clock_disable(ADCX_RCU);
	rcu_periph_clock_disable(adcDMA_RCU); 
}
#endif

#endif  //boardADC_EN




