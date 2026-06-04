#ifndef ADC_IFACE_H
#define ADC_IFACE_H


#include "board_config.h"
#if(boardADC_EN)

#define     	ADC_DMAX              					2
#define     	ADC_CHANNEL_NUM       					7   //DMA缓存大小

//电源输入电压   BAT_ADC
#define     	adcSYS_IN_VOLT_RCU     					RCU_GPIOA
#define     	adcSYS_IN_VOLT_PORT    					GPIOA
#define     	adcSYS_IN_VOLT_PIN     					GPIO_PIN_7
#define     	adcSYS_IN_VOLT_CH      					ADC_CHANNEL_7

//DC_360W   	温度 DC-NTC1
#define     	adcDC_TEMP_RCU           				RCU_GPIOA
#define     	adcDC_TEMP_PORT           				GPIOA
#define     	adcDC_TEMP_PIN            				GPIO_PIN_6
#define     	adcDC_TEMP_CH             				ADC_CHANNEL_6

//DC_360W    	电流 DC-I
#define     	adcDC_CURR_RCU            				RCU_GPIOA
#define     	adcDC_CURR_PORT           				GPIOA
#define     	adcDC_CURR_PIN            				GPIO_PIN_1
#define     	adcDC_CURR_CH             				ADC_CHANNEL_1

// DC_360W    	电压 DC-V
#define     	adcDC_VOLT_RCU            				RCU_GPIOC
#define     	adcDC_VOLT_PORT           				GPIOC 
#define     	adcDC_VOLT_PIN            				GPIO_PIN_1
#define     	adcDC_VOLT_CH             				ADC_CHANNEL_11

// //USB     	    温度 USB-NTC2
// #define     	adcUSB_TEMP_RCU            				RCU_GPIOA
// #define     	adcUSB_TEMP_PORT           				GPIOA
// #define     	adcUSB_TEMP_PIN            				GPIO_PIN_4
// #define     	adcUSB_TEMP_CH             				ADC_CHANNEL_4

// //USB-PD     	电流 USB-I
// #define     	adcUSB_PD_CURR_RCU            			RCU_GPIOC
// #define     	adcUSB_PD_CURR_PORT           			GPIOC
// #define     	adcUSB_PD_CURR_PIN            			GPIO_PIN_2
// #define     	adcUSB_PD_CURR_CH             			ADC_CHANNEL_12

// //USB-PD    	电压 USB-V
// #define     	adcUSB_PD_VOLT_RCU            			RCU_GPIOC
// #define     	adcUSB_PD_VOLT_PORT           			GPIOC
// #define     	adcUSB_PD_VOLT_PIN            			GPIO_PIN_3
// #define     	adcUSB_PD_VOLT_CH             			ADC_CHANNEL_13

// //USB-WC     	电流 WX-ADI
// #define     	adcUSB_WC_CURR_RCU            			RCU_GPIOA
// #define     	adcUSB_WC_CURR_PORT           			GPIOA
// #define     	adcUSB_WC_CURR_PIN            			GPIO_PIN_7
// #define     	adcUSB_WC_CURR_CH             			ADC_CHANNEL_7

// //USB-WC    	电压 WX-ADV
// #define     	adcUSB_WC_VOLT_RCU            			RCU_GPIOA
// #define     	adcUSB_WC_VOLT_PORT           			GPIOA
// #define     	adcUSB_WC_VOLT_PIN            			GPIO_PIN_6
// #define     	adcUSB_WC_VOLT_CH             			ADC_CHANNEL_6

//Key	     	Power
#define     	adcKEY_POWER_RCU            			RCU_GPIOA
#define     	adcKEY_POWER_PORT           			GPIOA
#define     	adcKEY_POWER_PIN            			GPIO_PIN_0
#define     	adcKEY_POWER_CH             			ADC_CHANNEL_0

//DC_DC_IN    	电压 DC-INAD
#define     	adcDC_IN_1_RCU            				RCU_GPIOC
#define     	adcDC_IN_1_PORT           				GPIOC 
#define     	adcDC_IN_1_PIN            				GPIO_PIN_5
#define     	adcDC_IN_1_CH             				ADC_CHANNEL_15

//DC_DC_IN    	电压 XT-INAD
#define     	adcDC_IN_2_RCU            				RCU_GPIOB
#define     	adcDC_IN_2_PORT           				GPIOB
#define     	adcDC_IN_2_PIN            				GPIO_PIN_0
#define     	adcDC_IN_2_CH             				ADC_CHANNEL_8



#if (ADC_DMAX == 1)
#define     	ADCX_RCU                    			RCU_ADC0
#define     	ADCX                        			ADC0
#define     	adcDMA_RCU                    			RCU_DMA1
#define     	adcDMA                        			DMA1
#define     	adcDMA_CH                     			DMA_CH4
// #define     	DMA_SUBPERIX                			DMA_SUBPERI0
#elif (ADC_DMAX == 2)
#define     	ADCX_RCU                    			RCU_ADC0
#define     	ADCX                        			ADC0
#define     	adcDMA_RCU                    			RCU_DMA0
#define     	adcDMA                        			DMA0
#define     	adcDMA_CH                     			DMA_CH0
#if (boardIC_TYPE == boardIC_GD32F50X)
#define     	adcDMA_REQUEST                     		DMA_REQUEST_ADC0_ROUTINE
#endif  /* boardIC_TYPE */
#endif

extern u16 adc_value[];

void vAdc_Init(void);
void vAdc_DeInit(void);

#if(boardLOW_POWER)
void vAdc_IoEnterLowPower(void);
#endif

#endif  //boardADC_EN

#endif  //ADC_IFACE_H

