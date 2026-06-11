#ifndef ADC_TASK_H
#define ADC_TASK_H

#include "board_config.h"

#if(boardADC_EN)
#include "Adc/adc_iface.h"


#define     	adcVBMS_R1                      		1800.0f //(Kohm)  分压的上电阻
#define     	adcVBMS_R2                      		100.0f  //(Kohm)  分压的对地电阻
#define     	adcVBMS_RES_RATIO               		((((3.3f / 4095.0f) * (adcVBMS_R1 + adcVBMS_R2)) / adcVBMS_R2) * 10.0f) //*10 电压单位为 0.1V 

#define     	adcDC_VOLT_R1                      		100.0f //(Kohm)   分压的上电阻
#define     	adcDC_VOLT_R2                      		22.0f  //(Kohm)  分压的对地电阻
#define     	adcDC_VOLT_RES_RATIO               		((((3.3f / 4095.0f) * (adcDC_VOLT_R1 + adcDC_VOLT_R2)) / adcDC_VOLT_R2)* 10.0f)          //电压单位为1V

//#define     	adcUSB_PD_VOLT_R1                       47.0f //(Kohm)  分压的上电阻
//#define     	adcUSB_PD_VOLT_R2                       5.1f  //(Kohm)  分压的对地电阻
//#define     	adcUSB_PD_VOLT_RES_RATIO                ((((3.3f / 4095.0f) * (adcUSB_PD_VOLT_R1 + adcUSB_PD_VOLT_R2)) / adcUSB_PD_VOLT_R2) * 10.0f) //*10 电压单位为 0.1V

//#define     	adcUSB_WC_VOLT_R1                       130.0f //(Kohm)  分压的上电阻
//#define     	adcUSB_WC_VOLT_R2                       20.0f  //(Kohm)  分压的对地电阻
//#define     	adcUSB_WC_VOLT_RES_RATIO                ((((3.3f / 4095.0f) * (adcUSB_WC_VOLT_R1 + adcUSB_WC_VOLT_R2)) / adcUSB_WC_VOLT_R2) * 10.0f) //*10 电压单位为 0.1V

#define     	adcDC_IN_1_R1                      		300.0f //(Kohm)   分压的上电阻
#define     	adcDC_IN_1_R2                      		10.0f  //(Kohm)  分压的对地电阻
#define     	adcDC_IN_1_RES_RATIO               		((((3.3f / 4095.0f) * (adcDC_IN_1_R1 + adcDC_IN_1_R2)) / adcDC_IN_1_R2)* 10.0f)          //电压单位为1V

#define     	adcDC_IN_2_R1                      		300.0f //(Kohm)   分压的上电阻
#define     	adcDC_IN_2_R2                      		10.0f  //(Kohm)  分压的对地电阻
#define     	adcDC_IN_2_RES_RATIO               		((((3.3f / 4095.0f) * (adcDC_IN_2_R1 + adcDC_IN_2_R2)) / adcDC_IN_2_R2)* 10.0f)          //电压单位为1V

#define     	adcSYS_IN_VOLT    						0   // 电池电压
#define     	adcDC_OUT_TEMP           				1   // DC温度
#define     	adcDC_OUT_CURR           				2   // DC电流
#define     	adcDC_OUT_VOLT          				3   // DC电压
#define     	adcKEY_POWER          					4   // 按键电源
#define     	adcDC_IN_1            					5   // DC输入电压1
#define     	adcDC_IN_2            					6   // DC输入电压2

//电压状态
typedef enum
{
	VS_NORMAL = 0,
	VS_LOW,
	VS_HIGH,
}VoltSate_E;

typedef struct
{
    vu16           		usSysInVolt;    	// 电池电压 0.1V
	s16            		sDcOutTemp;         // DC输出温度 摄氏度
	float				fDcOutCurr;     	// DC输出电流 A
    vu16           		usDcOutVolt;       	// DC输出电压 0.1V
    vu16           		usKeyPower;     	// 按键电源 AD值
    vu16           		usDcIn1Volt;    	// DC输入电压1 0.1V
    vu16           		usDcIn2Volt;    	// DC输入电压2 0.1V
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
