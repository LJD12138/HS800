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

//系统输入电压滤波器
#define 		adcSYS_IN_VOLT_FILTER_BUFF_SIZE     	6 
static s32 	usa_acd_sys_input_volt_buff[adcSYS_IN_VOLT_FILTER_BUFF_SIZE];
FilterHandler_T	tAdc_SysInVoltFilterMadAvg = {usa_acd_sys_input_volt_buff, adcSYS_IN_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

#if(boardDC_EN)
//DC温度滤波器
#define 		adcDC_TEMP_FILTER_BUFF_SIZE     		6 
static s32 	usa_acd_dc_temp_buff[adcDC_TEMP_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_DcTempFilterMadAvg = {usa_acd_dc_temp_buff, adcDC_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//DC电流滤波器
#define 		adcDC_CURR_FILTER_BUFF_SIZE     		12 
static s32 	usa_adc_dc_curr_buff[adcDC_CURR_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_DcCurrFilterMadAvg = {usa_adc_dc_curr_buff, adcDC_CURR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//DC电压滤波器
#define 		adcDC_VOLT_FILTER_BUFF_SIZE     		6 
static s32 	usa_adc_dc_volt_buff[adcDC_VOLT_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_DcVoltFilterMadAvg = {usa_adc_dc_volt_buff, adcDC_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};
#endif  //boardDC_EN

#if(boardUSB_EN)
//USB温度滤波器
#define 		adcUSB_TEMP_FILTER_BUFF_SIZE     		6 
static s32 	usa_acd_usb_temp_buff[adcUSB_TEMP_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_UsbTempFilterMadAvg = {usa_acd_usb_temp_buff, adcUSB_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//USB-PD电流滤波器
#define 		adcUSB_PD_CURR_FILTER_BUFF_SIZE     		12 
static s32 	usa_adc_usb_pd_curr_buff[adcUSB_PD_CURR_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_UsbPdCurrFilterMadAvg = {usa_adc_usb_pd_curr_buff, adcUSB_PD_CURR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//USB-PD电压滤波器
#define 		adcUSB_PD_VOLT_FILTER_BUFF_SIZE     		6 
static s32 	usa_adc_usb_pd_volt_buff[adcUSB_PD_VOLT_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_UsbPdVoltFilterMadAvg = {usa_adc_usb_pd_volt_buff, adcUSB_PD_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//USB-WC电流滤波器
#define 		adcUSB_WC_CURR_FILTER_BUFF_SIZE     		12 
static s32 	usa_adc_usb_wc_curr_buff[adcUSB_WC_CURR_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_UsbWcCurrFilterMadAvg = {usa_adc_usb_wc_curr_buff, adcUSB_WC_CURR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//USB-WC电压滤波器
#define 		adcUSB_WC_VOLT_FILTER_BUFF_SIZE     		6 
static s32 	usa_adc_usb_wc_volt_buff[adcUSB_WC_VOLT_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_UsbWcVoltFilterMadAvg = {usa_adc_usb_wc_volt_buff, adcUSB_WC_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};
#endif  //boardUSB_EN

AdcSamp_T 	tAdcSamp;


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
	memset((u8*)&usa_acd_sys_input_volt_buff, 0, sizeof(usa_acd_sys_input_volt_buff));
	
	#if(boardDC_EN)
	memset((u8*)&usa_acd_dc_temp_buff, 0, sizeof(usa_acd_dc_temp_buff));
	memset((u8*)&usa_adc_dc_curr_buff, 0, sizeof(usa_adc_dc_curr_buff));
	memset((u8*)&usa_adc_dc_volt_buff, 0, sizeof(usa_adc_dc_volt_buff));
	#endif  //boardDC_EN

	#if(boardUSB_EN)
	memset((u8*)&usa_acd_usb_temp_buff, 0, sizeof(usa_acd_usb_temp_buff));
	memset((u8*)&usa_adc_usb_pd_curr_buff, 0, sizeof(usa_adc_usb_pd_curr_buff));
	memset((u8*)&usa_adc_usb_pd_volt_buff, 0, sizeof(usa_adc_usb_pd_volt_buff));
	memset((u8*)&usa_adc_usb_wc_curr_buff, 0, sizeof(usa_adc_usb_wc_curr_buff));
	memset((u8*)&usa_adc_usb_wc_volt_buff, 0, sizeof(usa_adc_usb_wc_volt_buff));
	#endif  //boardUSB_EN
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
	s32 temp = 0;
	vu16 us_filter_sys_input_volt_ad = 0;

	#if(boardDC_EN)
	vu16 us_filter_dc_temp_ad = 0;
	vu16 us_filter_dc_curr_ad = 0;
	vu16 us_filter_dc_volt_ad = 0;
	#endif  //boardDC_EN

	#if(boardUSB_EN)
	vu16 us_filter_usb_temp_ad = 0;
	vu16 us_filter_usb_pd_curr_ad = 0;
	vu16 us_filter_usb_pd_volt_ad = 0;
	vu16 us_filter_usb_wc_curr_ad = 0;
	vu16 us_filter_usb_wc_volt_ad = 0;
	#endif  //boardUSB_EN
	
	static vu8  uc_init_adc_cnt = 0;
	static vu8  uc_delay_cnt = 0;
	
    #if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
//		v_power_select(true);
		
        //***********************************滤波************************************************************************
		//没有满足一次滤波数据,输出等于输入
		
		//系统输入电压
		temp = usAdc_GetChannelValue(adcSYS_IN_VOLT);
		us_filter_sys_input_volt_ad = lFilter_MadianAverage(&tAdc_SysInVoltFilterMadAvg, &temp);
		
		#if(boardDC_EN)
		//DC 温度
		temp = usAdc_GetChannelValue(adcDC_TEMP);
		us_filter_dc_temp_ad = lFilter_MadianAverage(&tAdc_DcTempFilterMadAvg, &temp);   
		
		//DC 电流
		temp = usAdc_GetChannelValue(adcDC_CURR);
		us_filter_dc_curr_ad = lFilter_MadianAverage(&tAdc_DcCurrFilterMadAvg, &temp);
		
		//DC 电压
		temp = usAdc_GetChannelValue(adcDC_VOLT);
		us_filter_dc_volt_ad = lFilter_MadianAverage(&tAdc_DcVoltFilterMadAvg, &temp);
		#endif  //boardDC_EN

		#if(boardUSB_EN)
		//USB 温度
		temp = usAdc_GetChannelValue(adcUSB_TEMP);
		us_filter_usb_temp_ad = lFilter_MadianAverage(&tAdc_UsbTempFilterMadAvg, &temp);   
		
		//USB 电流
		temp = usAdc_GetChannelValue(adcUSB_PD_CURR);
		us_filter_usb_pd_curr_ad = lFilter_MadianAverage(&tAdc_UsbPdCurrFilterMadAvg, &temp);
		
		//USB 电压
		temp = usAdc_GetChannelValue(adcUSB_PD_VOLT);
		us_filter_usb_pd_volt_ad = lFilter_MadianAverage(&tAdc_UsbPdVoltFilterMadAvg, &temp);

		//USB-WC 电流
		temp = usAdc_GetChannelValue(adcUSB_WC_CURR);
		us_filter_usb_wc_curr_ad = lFilter_MadianAverage(&tAdc_UsbWcCurrFilterMadAvg, &temp);
		
		//USB-WC 电压
		temp = usAdc_GetChannelValue(adcUSB_WC_VOLT);
		us_filter_usb_wc_volt_ad = lFilter_MadianAverage(&tAdc_UsbWcVoltFilterMadAvg, &temp);
		#endif  //boardUSB_EN
		
		
        //*************************************计算******************************************************************        
		
		//系统输入电压
		tAdcSamp.usSysInVolt = us_filter_sys_input_volt_ad * adcVBMS_RES_RATIO;

		#if(boardDC_EN)
		//DC温度
		tAdcSamp.sDcTemp = LIMIT((307 - (37 * log((float)us_filter_dc_temp_ad))), -128, 127);
		
		//DC电压
		tAdcSamp.usDcOutVolt = us_filter_dc_volt_ad * adcDC_VOLT_RES_RATIO;
		
		//DC电流
		tAdcSamp.fDcOutCurr = us_filter_dc_curr_ad * 0.0034f;
		#endif  //boardDC_EN

		#if(boardUSB_EN)
		//USB温度
		tAdcSamp.sUsbTemp = LIMIT((307 - (37 * log((float)us_filter_usb_temp_ad))), -128, 127); 
		
		//USB-PD电流
		tAdcSamp.fUsbPdCurr = us_filter_usb_pd_curr_ad * 0.0017f;
		
		//USB-PD电压
		tAdcSamp.usUsbPdVolt = us_filter_usb_pd_volt_ad * adcUSB_PD_VOLT_RES_RATIO;

		//USB-WC电流
		tAdcSamp.fUsbWcCurr = us_filter_usb_wc_curr_ad * 0.0017f;

		//USB-WC电压
		tAdcSamp.usUsbWcVolt = us_filter_usb_wc_volt_ad * adcUSB_WC_VOLT_RES_RATIO;
		#endif  //boardUSB_EN
		
		if(uPrint.tFlag.bAdcTask)
		{
			uc_delay_cnt++;
			if(uc_delay_cnt >= 100)
			{
				uc_delay_cnt = 0;
				sMyPrint("电池电压 = %.2fV\r\n",tAdcSamp.usSysInVolt / 10.0f);
				sMyPrint("DC温度 = %d 摄氏度\r\n",tAdcSamp.sDcTemp);
				sMyPrint("DC输出电流 = %0.2fA\r\n",tAdcSamp.fDcOutCurr);
				sMyPrint("DC输出电压 = %.2fV\r\n",tAdcSamp.usDcOutVolt / 10.0f);
				sMyPrint("USB温度 = %d 摄氏度\r\n",tAdcSamp.sUsbTemp);
                sMyPrint("USB-PD输出电流 = %0.2fA\r\n",tAdcSamp.fUsbPdCurr);
				sMyPrint("USB-PD输出电压 = %.2fV\r\n",tAdcSamp.usUsbPdVolt / 10.0f);
			}
		}	
		
		//***********************************等待ADC采集稳定*************************************************************
		if(uc_init_adc_cnt < 0xff)
			uc_init_adc_cnt++;
		
		if(uc_init_adc_cnt == 10)
			tSysInfo.uInit.tFinish.bIF_AdcTask = true;	
		
		#if(boardUSE_OS)
		if(tSysInfo.eDevState == DS_INIT)
			vTaskDelay(30);
		else 
			vTaskDelay(100);
		#endif  //boardUSE_OS
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

