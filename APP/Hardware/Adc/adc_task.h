#ifndef ADC_TASK_H
#define ADC_TASK_H

#include "board_config.h"

#if(boardADC_EN)
#include "Adc/adc_iface.h"


#define     	adcVBMS_R1                      		300.0f //(Kohm)  分压的上电阻
#define     	adcVBMS_R2                      		10.0f  //(Kohm)  分压的对地电阻
#define     	adcVBMS_RES_RATIO               		((((3.3f / 4095.0f) * (adcVBMS_R1 + adcVBMS_R2)) / adcVBMS_R2) * 10.0f) //*10 电压单位为 0.1V 

#define     	adcDC_VOLT_R1                      		47.0f //(Kohm)   分压的上电阻
#define     	adcDC_VOLT_R2                      		10.0f  //(Kohm)  分压的对地电阻
#define     	adcDC_VOLT_RES_RATIO               		((((3.3f / 4095.0f) * (adcDC_VOLT_R1 + adcDC_VOLT_R2)) / adcDC_VOLT_R2)* 10.0f)          //电压单位为1V

#define     	adcUSB_PD_VOLT_R1                       47.0f //(Kohm)  分压的上电阻
#define     	adcUSB_PD_VOLT_R2                       5.1f  //(Kohm)  分压的对地电阻
#define     	adcUSB_PD_VOLT_RES_RATIO                ((((3.3f / 4095.0f) * (adcUSB_PD_VOLT_R1 + adcUSB_PD_VOLT_R2)) / adcUSB_PD_VOLT_R2) * 10.0f) //*10 电压单位为 0.1V

#define     	adcUSB_WC_VOLT_R1                       130.0f //(Kohm)  分压的上电阻
#define     	adcUSB_WC_VOLT_R2                       20.0f  //(Kohm)  分压的对地电阻
#define     	adcUSB_WC_VOLT_RES_RATIO                ((((3.3f / 4095.0f) * (adcUSB_WC_VOLT_R1 + adcUSB_WC_VOLT_R2)) / adcUSB_WC_VOLT_R2) * 10.0f) //*10 电压单位为 0.1V

#define     	adcSYS_IN_VOLT    						0
#define     	adcDC_TEMP           					1
#define     	adcDC_CURR           					2
#define     	adcDC_VOLT           					3
#define     	adcUSB_TEMP          					4
#define     	adcUSB_PD_CURR          				5
#define     	adcUSB_PD_VOLT          				6
#define     	adcUSB_WC_CURR          				7
#define     	adcUSB_WC_VOLT          				8
// #define     	adcKEY_POWER          					7

//电压状态
typedef enum
{
	VS_NORMAL = 0,
	VS_LOW,
	VS_HIGH,
}VoltSate_E;

typedef struct
{
	s16            		sDcTemp;          	//摄氏度
	vu16           		usDcOutVolt;    	//0.1V
	float				fDcOutCurr;     	//A
	
	vu16           		usSysInVolt;    	//0.1V
	
	s16            		sUsbTemp;
	vu16           		usUsbPdVolt;    	//0.1V
	float				fUsbPdCurr;     	//A
	vu16           		usUsbWcVolt;    	//0.1V
	float				fUsbWcCurr;     	//A
}AdcSamp_T;
extern AdcSamp_T 	tAdcSamp;

void vAdc_TaskInit(void);
u16 usAdc_GetChannelValue(u8 channel);

#if(!boardUSE_OS)
void vAdc_Task(void *pvParameters);
#endif  //boardUSE_OS

#if(boardLOW_POWER)
bool bAdc_EnterLowPower(void);
bool bAdc_ExitLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardADC_EN

#endif  //ADC_TASK_H
