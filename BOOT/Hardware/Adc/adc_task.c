/*****************************************************************************************************************
*                                                                                                                *
 *                                         ADC任务****                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "board_config.h"

#if(boardADC_EN)
#include "Adc/adc_task.h"
#include "Adc/adc_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "filtration.h"
#include "gpio_init.h"
#include "math.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define       	ADC_TASK_PRIO                  			2         // 任务优先级 
#define       	ADC_TASK_STK_SIZE              			256       // 任务堆栈  实际字节数 *4
TaskHandle_t    tAdcTaskHandler = NULL; 
void           	vAdc_Task(void *pvParameters);
#endif  //boardUSE_OS


//****************************************************参数初始化**************************************************//

////系统输入电压滤波器
//#define 		adcSYS_IN_VOLT_FILTER_BUFF_SIZE     	6 
//static s32 	usa_acd_sys_input_volt_buff[adcSYS_IN_VOLT_FILTER_BUFF_SIZE];
//FilterHandler_T	tAdc_SysInVoltFilterMadAvg = {usa_acd_sys_input_volt_buff, adcSYS_IN_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

////DC温度滤波器
//#define 		adcDC_TEMP_FILTER_BUFF_SIZE     		6 
//static s32 	usa_acd_dc_temp_buff[adcDC_TEMP_FILTER_BUFF_SIZE];
//FilterHandler_T tAdc_DcTempFilterMadAvg = {usa_acd_dc_temp_buff, adcDC_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

////DC电流滤波器
//#define 		adcDC_CURR_FILTER_BUFF_SIZE     		12 
//static s32 	usa_adc_dc_curr_buff[adcDC_CURR_FILTER_BUFF_SIZE];
//FilterHandler_T tAdc_DcCurrFilterMadAvg = {usa_adc_dc_curr_buff, adcDC_CURR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

////DC电压滤波器
//#define 		adcDC_VOLT_FILTER_BUFF_SIZE     		6 
//static s32 	usa_adc_dc_volt_buff[adcDC_VOLT_FILTER_BUFF_SIZE];
//FilterHandler_T tAdc_DcVoltFilterMadAvg = {usa_adc_dc_volt_buff, adcDC_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

////USB温度滤波器
//#define 		adcUSB_TEMP_FILTER_BUFF_SIZE     		6 
//static s32 	usa_acd_usb_temp_buff[adcUSB_TEMP_FILTER_BUFF_SIZE];
//FilterHandler_T tAdc_UsbTempFilterMadAvg = {usa_acd_usb_temp_buff, adcUSB_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

////USB电流滤波器
//#define 		adcUSB_PD_CURR_FILTER_BUFF_SIZE     		12 
//static s32 	usa_adc_usb_curr_buff[adcUSB_PD_CURR_FILTER_BUFF_SIZE];
//FilterHandler_T tAdc_UsbCurrFilterMadAvg = {usa_adc_usb_curr_buff, adcUSB_PD_CURR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

////USB电压滤波器
//#define 		adcUSB_PD_VOLT_FILTER_BUFF_SIZE     		6 
//static s32 	usa_adc_usb_volt_buff[adcUSB_PD_VOLT_FILTER_BUFF_SIZE];
//FilterHandler_T tAdc_UsbVoltFilterMadAvg = {usa_adc_usb_volt_buff, adcUSB_PD_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};


//AdcSamp_T 	tAdcSamp;


//****************************************************函数定义*****************************************************//
//static void v_power_select(bool en);
static void v_adc_param_init(void);


/***********************************************************************************************************************
-----函数功能    ADC任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vAdc_TaskInit(void)
{
	#if(boardLOW_POWER)
	vAdc_IoEnterLowPower();
	#endif
	
	vAdc_Init(); //AD初始化
	
	v_adc_param_init();
	
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vAdc_Task,				// 任务函数 (1)
                (const char* )"AdcTask",				// 任务名称
                (uint16_t ) ADC_TASK_STK_SIZE,			// 任务堆栈大小
                (void* )NULL,							// 传递给任务函数的参数
                (UBaseType_t ) ADC_TASK_PRIO,			// 任务优先级
                (TaskHandle_t*)&tAdcTaskHandler);		// 任务句柄
	#endif  //boardUSE_OS
}

/***********************************************************************************************************************
-----函数功能    ADC参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_adc_param_init(void)
{
//	v_power_select(true);	//开启温度采样电源
//	memset((u8*)&usa_acd_sys_input_volt_buff, 0, sizeof(usa_acd_sys_input_volt_buff));
//	
//	memset((u8*)&usa_acd_dc_temp_buff, 0, sizeof(usa_acd_dc_temp_buff));
//	
//	memset((u8*)&usa_adc_dc_curr_buff, 0, sizeof(usa_adc_dc_curr_buff));
//	
//	memset((u8*)&usa_adc_dc_volt_buff, 0, sizeof(usa_adc_dc_volt_buff));
//	
//	memset((u8*)&usa_acd_usb_temp_buff, 0, sizeof(usa_acd_usb_temp_buff));
//	
//	memset((u8*)&usa_adc_usb_curr_buff, 0, sizeof(usa_adc_usb_curr_buff));
//	
//	memset((u8*)&usa_adc_usb_volt_buff, 0, sizeof(usa_adc_usb_volt_buff));
}

/***********************************************************************************************************************
-----函数功能    ADC循环任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vAdc_Task(void *pvParameters)
{
//	s32 temp = 0;
//	vu16 us_filter_sys_input_volt_ad = 0;
//	vu16 us_filter_dc_temp_ad = 0;
//	vu16 us_filter_dc_curr_ad = 0;
//	vu16 us_filter_dc_volt_ad = 0;
//	vu16 us_filter_usb_temp_ad = 0;
//	vu16 us_filter_usb_curr_ad = 0;
//	vu16 us_filter_usb_volt_ad = 0;
//	
//	static vu8  uc_init_adc_cnt = 0;
//	static vu8  uc_delay_cnt = 0;
	
    #if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
//		v_power_select(true);
		
        //***********************************滤波************************************************************************
		//没有满足一次滤波数据,输出等于输入
		
		//系统输入电压
//		temp = usAdc_GetChannelValue(adcSYS_IN_VOLT);
//		us_filter_sys_input_volt_ad = lFilter_MadianAverage(&tAdc_SysInVoltFilterMadAvg, &temp);
//		
//		//DC 温度
//		temp = usAdc_GetChannelValue(adcDC_TEMP);
//		us_filter_dc_temp_ad = lFilter_MadianAverage(&tAdc_DcTempFilterMadAvg, &temp);   
//		
//		//DC 电流
//		temp = usAdc_GetChannelValue(adcDC_CURR);
//		if(tDc.eDevState != DS_WORK)
//			temp = 0;
//		us_filter_dc_curr_ad = lFilter_MadianAverage(&tAdc_DcCurrFilterMadAvg, &temp);
//		
//		//DC 电压
//		temp = usAdc_GetChannelValue(adcDC_VOLT);
//		us_filter_dc_volt_ad = lFilter_MadianAverage(&tAdc_DcVoltFilterMadAvg, &temp);
//		
//		//USB 温度
//		temp = usAdc_GetChannelValue(adcUSB_TEMP);
//		us_filter_usb_temp_ad = lFilter_MadianAverage(&tAdc_UsbTempFilterMadAvg, &temp);   
//		
//		//USB 电流
//		temp = usAdc_GetChannelValue(adcUSB_CURR);
//		if(tUsb.eDevState != DS_WORK)
//			temp = 0;
//		us_filter_usb_curr_ad = lFilter_MadianAverage(&tAdc_UsbCurrFilterMadAvg, &temp);
//		
//		//USB 电压
//		temp = usAdc_GetChannelValue(adcUSB_VOLT);
//		us_filter_usb_volt_ad = lFilter_MadianAverage(&tAdc_UsbVoltFilterMadAvg, &temp);
//		
//		
//        //*************************************计算******************************************************************        
//		
//		//系统输入电压
//		tAdcSamp.usSysInVolt = us_filter_sys_input_volt_ad * adcVBMS_RES_RATIO;
//		tDc.usInVolt = tAdcSamp.usSysInVolt;

//		//DC温度
//		tAdcSamp.sDcTemp = LIMIT((307 - (37 * log((float)us_filter_dc_temp_ad))), -128, 127);
//		tDc.sMaxTemp = tAdcSamp.sDcTemp;
//		
//		//DC电压
//		tAdcSamp.usDcOutVolt = us_filter_dc_volt_ad * adcDC_VOLT_RES_RATIO;
//		tDc.usOutVolt = tAdcSamp.usDcOutVolt;
//		
//		//DC电流
//		tAdcSamp.fDcOutCurr = us_filter_dc_curr_ad * 0.0034f;
//		tDc.usOutCurr = tAdcSamp.fDcOutCurr * 10;//0.1A
//		
//		//USB温度
//		tAdcSamp.sUsbTemp = LIMIT((307 - (37 * log((float)us_filter_usb_temp_ad))), -128, 127); 
//		tUsb.sMaxTemp = tAdcSamp.sUsbTemp;
//		
//		//USB电流
//		tAdcSamp.fUsbInCurr = us_filter_usb_curr_ad * 0.0034f;
//		tUsb.usInCurr = tAdcSamp.fUsbInCurr * 10;//0.1A
//		
//		//USB电压
//		tAdcSamp.usUsbInVolt = us_filter_usb_volt_ad * adcUSB_PD_VOLT_RES_RATIO;
//		tUsb.usInVolt = tAdcSamp.usUsbInVolt;//0.1V
//		
//		if(uPrint.tFlag.bAdcTask)
//		{
//			uc_delay_cnt++;
//			if(uc_delay_cnt >= 100)
//			{
//				uc_delay_cnt = 0;
//				sMyPrint("电池电压 = %.2fV\r\n",tAdcSamp.usSysInVolt / 10.0f);
//				sMyPrint("DC温度 = %d 摄氏度\r\n",tAdcSamp.sDcTemp);
//				sMyPrint("DC输出电流 = %0.2fA\r\n",tAdcSamp.fDcOutCurr);
//				sMyPrint("DC输出电压 = %.2fV\r\n",tAdcSamp.usDcOutVolt / 10.0f);
//				sMyPrint("USB温度 = %d 摄氏度\r\n",tAdcSamp.sUsbTemp);
//                sMyPrint("USB输出电流 = %0.2fA\r\n",tAdcSamp.fUsbInCurr);
//				sMyPrint("USB输出电压 = %.2fV\r\n",tAdcSamp.usUsbInVolt / 10.0f);
//			}
//		}	
//		
//		//***********************************等待ADC采集稳定*************************************************************
//		if(uc_init_adc_cnt < 0xff)
//			uc_init_adc_cnt++;
//		
//		if(uc_init_adc_cnt == 10)
//			tSysInfo.uInit.tFinish.bIF_AdcTask = true;	
//		
//		#if(boardUSE_OS)
//		if(tSysInfo.eDevState == DS_INIT)
//			vTaskDelay(30);
//		else 
//			vTaskDelay(100);
//		#endif  //boardUSE_OS
    }
}

/***********************************************************************************************************************
-----函数功能    ADC循环任务
-----说明(备注)  none
-----传入参数    channel             通道数
		#define     adcSYS_IN_VOLT    	 0
		#define     adcDC_TEMP           1
		#define     adcDC_CURR           2
		#define     adcDC_VOLT           3
		#define     adcUSB_TEMP          4
		#define     adcUSB_CURR          5
-----输出参数    none
-----返回值      选择通道的16位AD数据
************************************************************************************************************************/
u16 usAdc_GetChannelValue(u8 channel)	
{
	if(channel >= ADC_CHANNEL_NUM) return 0;
	
	return adc_value[channel];
}

#if(boardLOW_POWER)
/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:执行成功   false:执行失败
*****************************************************************************************************************/
bool bAdc_EnterLowPower(void)
{
	vTaskSuspend(ADC_Task_Handler);  //先挂起任务
	vAdc_IoEnterLowPower();
	v_power_select(false); 
	return true;
}


/*****************************************************************************************************************
-----函数功能    退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:执行成功   false:执行失败
*****************************************************************************************************************/
bool bAdc_ExitLowPower(void)
{
	vAdc_Init();
	vTaskResume(ADC_Task_Handler);  //初始化外设后再恢复任务
	return true;
}
#endif  //boardLOW_POWER

#endif  //boardADC_EN

