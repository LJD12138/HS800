/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Usb/usb_queue_task.h"

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#include "Usb/usb_prot_frame.h"
#include "Usb/usb_iface.h"
#include "Sys/sys_task.h"

#include "app_info.h"
#include "filtration.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  //boardPRINT_IFACE

#define       	usbTASK_WORK_CYCLE_TIME               		100

s32 us_usb_total_out_pwr = 0;

//PD100W温度滤波器
// #define 		usbPD_TEMP_FILTER_BUFF_SIZE     		10 
// static s32 usa_pd_temp_buff[usbPD_TEMP_FILTER_BUFF_SIZE];
// FilterHandler_T    tAdc_PDTempFilterMadAvg = {usa_pd_temp_buff, usbPD_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

// //无线充温度滤波器
// #define 		usbCHG_TEMP_FILTER_BUFF_SIZE     		10 
// static s32 usa_chg_temp_buff[usbCHG_TEMP_FILTER_BUFF_SIZE];
// FilterHandler_T    tAdc_ChgTempFilterMadAvg = {usa_chg_temp_buff, usbCHG_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//USB电压滤波器
#define 		adcUSB_PWR_FILTER_BUFF_SIZE     		4 
static s32 	usa_adc_usb_pwr_buff[adcUSB_PWR_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_UsbPwrFilterMadAvg = {usa_adc_usb_pwr_buff, adcUSB_PWR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_usb_queue_task_work(Task_T *tp_task)
{
	//有任务,退出
	if(lwrb_get_full(&tp_task->tQueueBuff))
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
		return;
	}
	
	switch (tp_task->ucStep)
    {
		case 0:
        {
			us_usb_total_out_pwr = 0;
			c_usb_cs_get_ic_param(&tUSB_IC1_I2C);
			cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
        }

		case 1:
        {
			c_usb_cs_get_ic_param(&tUSB_IC2_I2C);
			cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
        }

		case 2:
		{
			if(tUsb.usPdPwr < 2)
				tUsb.usPdPwr = 0;
			us_usb_total_out_pwr += tUsb.usPdPwr;

			if(tUsb.usWcPwr < 2)
				tUsb.usWcPwr = 0;
			us_usb_total_out_pwr += tUsb.usWcPwr;

			tUsb.usOutPwr = lFilter_MadianAverage(&tAdc_UsbPwrFilterMadAvg, &us_usb_total_out_pwr);

			vTaskDelay(400);
			cQueue_GotoStep(tp_task, 0);
		}
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }

	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_WORK_CYCLE_TIME);
	#endif  //boardUSE_OS
}
#endif  //boardUSB_EN
