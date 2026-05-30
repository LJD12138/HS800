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
#define       	ADC_TASK_STK_SIZE              			192       // 任务堆栈  实际字节数 *4
TaskHandle_t    tAdcTaskHandler = NULL; 
void           	vAdc_Task(void *pvParameters);
#endif  //boardUSE_OS


//****************************************************参数初始化**************************************************//

//系统输入电压滤波器 (adcSYS_IN_VOLT = 0)
#define 		adcSYS_IN_VOLT_FILTER_BUFF_SIZE     	6 
static s32 	usa_acd_sys_input_volt_buff[adcSYS_IN_VOLT_FILTER_BUFF_SIZE];
FilterHandler_T	tAdc_SysInVoltFilterMadAvg = {usa_acd_sys_input_volt_buff, adcSYS_IN_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//DC温度滤波器 (adcDC_TEMP = 1)
#define 		adcDC_TEMP_FILTER_BUFF_SIZE     		6 
static s32 	usa_acd_dc_temp_buff[adcDC_TEMP_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_DcTempFilterMadAvg = {usa_acd_dc_temp_buff, adcDC_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//DC电流滤波器 (adcDC_CURR = 2)
#define 		adcDC_CURR_FILTER_BUFF_SIZE     		12 
static s32 	usa_adc_dc_curr_buff[adcDC_CURR_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_DcCurrFilterMadAvg = {usa_adc_dc_curr_buff, adcDC_CURR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//DC电压滤波器 (adcDC_VOLT = 3)
#define 		adcDC_VOLT_FILTER_BUFF_SIZE     		6 
static s32 	usa_adc_dc_volt_buff[adcDC_VOLT_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_DcVoltFilterMadAvg = {usa_adc_dc_volt_buff, adcDC_VOLT_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//DC输入电压1滤波器 (adcDC_IN_1 = 4)
#define 		adcDC_IN_1_FILTER_BUFF_SIZE     		6 
static s32 	usa_dc_in_1_buff[adcDC_IN_1_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_DcIn1FilterMadAvg = {usa_dc_in_1_buff, adcDC_IN_1_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//DC输入电压2滤波器 (adcDC_IN_2 = 5)
#define 		adcDC_IN_2_FILTER_BUFF_SIZE     		6 
static s32 	usa_dc_in_2_buff[adcDC_IN_2_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_DcIn2FilterMadAvg = {usa_dc_in_2_buff, adcDC_IN_2_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//注意: adcKEY_POWER = 3 不需要滤波，在Key任务中直接调用

AdcSamp_T 	tAdcSamp;


//****************************************************函数定义*****************************************************//
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
	memset((u8*)&usa_acd_sys_input_volt_buff, 0, sizeof(usa_acd_sys_input_volt_buff));
	memset((u8*)&usa_acd_dc_temp_buff, 0, sizeof(usa_acd_dc_temp_buff));
	memset((u8*)&usa_adc_dc_curr_buff, 0, sizeof(usa_adc_dc_curr_buff));
	memset((u8*)&usa_adc_dc_volt_buff, 0, sizeof(usa_adc_dc_volt_buff));
	memset((u8*)&usa_dc_in_1_buff, 0, sizeof(usa_dc_in_1_buff));
	memset((u8*)&usa_dc_in_2_buff, 0, sizeof(usa_dc_in_2_buff));
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
	vu16 us_filter_dc_temp_ad = 0;
	vu16 us_filter_dc_curr_ad = 0;
	vu16 us_filter_dc_volt_ad = 0;
	vu16 us_filter_dc_in_1_ad = 0;
	vu16 us_filter_dc_in_2_ad = 0;
	
	static vu8  uc_init_adc_cnt = 0;
	static vu8  uc_delay_cnt = 0;
	
    #if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
        //***********************************滤波************************************************************************
		//没有满足一次滤波数据,输出等于输入
		
		//系统输入电压 (adcSYS_IN_VOLT = 0)
		temp = usAdc_GetChannelValue(adcSYS_IN_VOLT);
		us_filter_sys_input_volt_ad = lFilter_MadianAverage(&tAdc_SysInVoltFilterMadAvg, &temp);
		
		//DC温度 (adcDC_OUT_TEMP = 1)
		temp = usAdc_GetChannelValue(adcDC_OUT_TEMP);
		us_filter_dc_temp_ad = lFilter_MadianAverage(&tAdc_DcTempFilterMadAvg, &temp);   
		
		//DC电流 (adcDC_OUT_CURR = 2)
		temp = usAdc_GetChannelValue(adcDC_OUT_CURR);
		us_filter_dc_curr_ad = lFilter_MadianAverage(&tAdc_DcCurrFilterMadAvg, &temp);
		
		//DC电压 (adcDC_OUT_VOLT = 3)
		temp = usAdc_GetChannelValue(adcDC_OUT_VOLT);
		us_filter_dc_volt_ad = lFilter_MadianAverage(&tAdc_DcVoltFilterMadAvg, &temp);
		
		//DC输入电压1 (adcDC_IN_1 = 5)
		temp = usAdc_GetChannelValue(adcDC_IN_1);
		us_filter_dc_in_1_ad = lFilter_MadianAverage(&tAdc_DcIn1FilterMadAvg, &temp);
		
		//DC输入电压2 (adcDC_IN_2 = 6)
		temp = usAdc_GetChannelValue(adcDC_IN_2);
		us_filter_dc_in_2_ad = lFilter_MadianAverage(&tAdc_DcIn2FilterMadAvg, &temp);
		
		//注意: adcKEY_POWER = 4 不需要滤波，在Key任务中直接调用usAdc_GetChannelValue(adcKEY_POWER)
		
        //*************************************计算******************************************************************        
		
		//系统输入电压 (单位0.1V)
		tAdcSamp.usSysInVolt = us_filter_sys_input_volt_ad * adcVBMS_RES_RATIO;

		//DC温度 (摄氏度)
		tAdcSamp.sDcOutTemp = LIMIT((307 - (37 * log((float)us_filter_dc_temp_ad))), -128, 127);
		
		//DC电流 (A)
		tAdcSamp.fDcOutCurr = us_filter_dc_curr_ad * 0.0034f;
		
		//DC电压 (单位0.1V)
		tAdcSamp.usDcOutVolt = us_filter_dc_volt_ad * adcDC_VOLT_RES_RATIO;
		
		//按键电源 (AD值，直接存储，Key任务可直接读取)
		tAdcSamp.usKeyPower = usAdc_GetChannelValue(adcKEY_POWER);
		
		//DC输入电压1 (单位0.1V)
		tAdcSamp.usDcIn1Volt = us_filter_dc_in_1_ad * adcDC_IN_1_RES_RATIO;
		
		//DC输入电压2 (单位0.1V)
		tAdcSamp.usDcIn2Volt = us_filter_dc_in_2_ad * adcDC_IN_2_RES_RATIO;
		
		if(uPrint.tFlag.bAdcTask)
		{
			uc_delay_cnt++;
			if(uc_delay_cnt >= 100)
			{
				uc_delay_cnt = 0;
				sMyPrint("电池电压 = %.2fV\r\n", tAdcSamp.usSysInVolt / 10.0f);
				sMyPrint("DC温度 = %d 摄氏度\r\n", tAdcSamp.sDcOutTemp);
				sMyPrint("DC电流 = %.2fA\r\n", tAdcSamp.fDcOutCurr);
				sMyPrint("DC电压 = %.2fV\r\n", tAdcSamp.usDcOutVolt / 10.0f);
				sMyPrint("按键电源AD = %d\r\n", tAdcSamp.usKeyPower);
				sMyPrint("DC输入电压1 = %.2fV\r\n", tAdcSamp.usDcIn1Volt / 10.0f);
				sMyPrint("DC输入电压2 = %.2fV\r\n", tAdcSamp.usDcIn2Volt / 10.0f);
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
-----函数功能    获取ADC通道值
-----说明(备注)  通道定义:
		#define     adcSYS_IN_VOLT    	 0   // 电池电压
		#define     adcDC_TEMP           1   // DC温度
		#define     adcDC_CURR           2   // DC电流
		#define     adcDC_VOLT           3   // DC电压
		#define     adcKEY_POWER         4   // 按键电源
		#define     adcDC_IN_1           5   // DC输入电压1
		#define     adcDC_IN_2           6   // DC输入电压2
-----传入参数    channel             通道数
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
	vTaskSuspend(tAdcTaskHandler);  //先挂起任务
	vAdc_IoEnterLowPower();
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
	vTaskResume(tAdcTaskHandler);  //初始化外设后再恢复任务
	return true;
}
#endif  //boardLOW_POWER

#endif  //boardADC_EN