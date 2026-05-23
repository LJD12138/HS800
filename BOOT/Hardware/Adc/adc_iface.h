#ifndef ADC_IFACE_H
#define ADC_IFACE_H


#include "board_config.h"
#if(boardADC_EN)

#define     	ADC_DMAX              					2
#define     	ADC_CHANNEL_NUM       					8   //DMA缓存大小

//电源输入电压   BAT_ADC
#define     	adcSYS_IN_VOLT_RCU     					RCU_GPIOC
#define     	adcSYS_IN_VOLT_PORT    					GPIOC
#define     	adcSYS_IN_VOLT_PIN     					GPIO_PIN_5
#define     	adcSYS_IN_VOLT_CH      					ADC_CHANNEL_15

//DC_360W   	温度 DC-NTC1
#define     	adcDC_TEMP_RCU           				RCU_GPIOC
#define     	adcDC_TEMP_PORT           				GPIOC
#define     	adcDC_TEMP_PIN            				GPIO_PIN_3
#define     	adcDC_TEMP_CH             				ADC_CHANNEL_13

//DC_360W    	电流 DC-I
#define     	adcDC_CURR_RCU            				RCU_GPIOC
#define     	adcDC_CURR_PORT           				GPIOC 
#define     	adcDC_CURR_PIN            				GPIO_PIN_1
#define     	adcDC_CURR_CH             				ADC_CHANNEL_11

//DC_360W    	电压 DC-V
#define     	adcDC_VOLT_RCU            				RCU_GPIOC
#define     	adcDC_VOLT_PORT           				GPIOC 
#define     	adcDC_VOLT_PIN            				GPIO_PIN_0
#define     	adcDC_VOLT_CH             				ADC_CHANNEL_10

//USB_PD     	温度 DC-NTC2
#define     	adcUSB_TEMP_RCU            				RCU_GPIOC
#define     	adcUSB_TEMP_PORT           				GPIOC
#define     	adcUSB_TEMP_PIN            				GPIO_PIN_2
#define     	adcUSB_TEMP_CH             				ADC_CHANNEL_12

//USB     		电流 USB-I
#define     	adcUSB_PD_CURR_RCU            			RCU_GPIOA
#define     	adcUSB_PD_CURR_PORT           			GPIOA
#define     	adcUSB_PD_CURR_PIN            			GPIO_PIN_0
#define     	adcUSB_PD_CURR_CH             			ADC_CHANNEL_0

//USB     		电流 USB-V
#define     	adcUSB_PD_VOLT_RCU            			RCU_GPIOB
#define     	adcUSB_PD_VOLT_PORT           			GPIOB
#define     	adcUSB_PD_VOLT_PIN            			GPIO_PIN_0
#define     	adcUSB_PD_VOLT_CH             			ADC_CHANNEL_8

//Key	     	Power
#define     	adcKEY_POWER_RCU            			RCU_GPIOA
#define     	adcKEY_POWER_PORT           			GPIOA
#define     	adcKEY_POWER_PIN            			GPIO_PIN_5
#define     	adcKEY_POWER_CH             			ADC_CHANNEL_5



#if (ADC_DMAX == 1)
#define     	ADCX_RCU                    			RCU_ADC0
#define     	ADCX                        			ADC0
#define     	adcDMA_RCU                    			RCU_DMA1
#define     	adcDMA                        			DMA1
#define     	adcDMA_CH                     			DMA_CH4

#define     	DMA_SUBPERIX                			DMA_SUBPERI0
#elif (ADC_DMAX == 2)
#define     	ADCX_RCU                    			RCU_ADC0
#define     	ADCX                        			ADC0
#define     	adcDMA_RCU                    			RCU_DMA0
#define     	adcDMA                        			DMA0
#define     	adcDMA_CH                     			DMA_CH0
//#define     	DMA_SUBPERIX                			DMA_SUBPERI0
#endif

extern u16 adc_value[];

void vAdc_Init(void);
void vAdc_DeInit(void);

#if(boardLOW_POWER)
void vAdc_IoEnterLowPower(void);
#endif

#endif  //boardADC_EN

#endif  //ADC_IFACE_H

