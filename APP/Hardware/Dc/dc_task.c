/*****************************************************************************************************************
*                                                                                                                *
 *                                         PD100W任务                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "Dc/dc_task.h"

#if(boardDC_EN)
#include "Dc/dc_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "i2c.h"
#include "app_info.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if(boardADC_EN)
#include "Adc/adc_task.h"
#endif  //boardADC_EN


#define     	dcTASK_CYCLE_TIME         			200


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define     	DC_TASK_PRIO                 			1   	//任务优先级 
#define     	DC_TASK_SIZE                 			256   	//任务堆栈  实际字节数 *4
TaskHandle_t    tDcTaskHandler = NULL; 
void          	vDc_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	Dc_T tDc;
////PD100W温度滤波器
//#define usbPD_TEMP_FILTER_BUFF_SIZE     10 
//static vu16 usa_pd_temp_buff[usbPD_TEMP_FILTER_BUFF_SIZE];
//FilterHandler_T    tAdc_PDTempFilterMadAvg = {usa_pd_temp_buff, usbPD_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//****************************************************函数声明****************************************************//
static void v_dc_param_init(void );
static void v_dc_set_work_state(DevState_E stat);
static void v_dc_set_error_code(DcErrCode_E code ,bool set);
static void v_dc_protect_process(void);
static s8 c_dc_info_init(void);
static s8 c_dc_check_out_volt(void);
static s8 c_dc_check_in_volt(void);

/*****************************************************************************************************************
-----函数功能    参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_dc_param_init(void )
{
	memset((u8*)&tDc, 0, sizeof(tDc));
	tDc.usAutoOffTime = tAppMemParam.tDC.usAutoOffTime;
	v_dc_set_work_state(DS_INIT);
}


/*****************************************************************************************************************
-----函数功能    系统任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vDc_TaskInit(void)
{
	vDc_IfaceInit();
	
	v_dc_param_init();
	
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vDc_Task,              //任务函数
                (const char* )"bDcTask",               //任务名称
                (uint16_t ) DC_TASK_SIZE,               //任务堆栈大小
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) DC_TASK_PRIO,            //任务优先级
                (TaskHandle_t*)&tDcTaskHandler);        //任务句柄  
	#endif  //boardUSE_OS
}

/*****************************************************************************************************************
-----函数功能    DC任务
-----说明(备注)  none
-----传入参数    value单位是W
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vDc_Task(void *pvParameters)
{
	s8 c_ret = 0;
	static vu16 us_pwr_exist_cnt = 0;
	
	#if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
		//关机状态下
		if(tSysInfo.uPerm.tPerm.bDisChgPerm == false)
		{
			//USB处于开机
			if(tDc.eDevState >= DS_BOOTING)  
			{
				//关闭USB设备
				v_dc_set_work_state(DS_CLOSING); 
			}
		}
		
		//***********************************DC工作状态的任务*********************************************
        switch (tDc.eDevState)
        {
			case DS_INIT:
            {
				v_dc_param_init();
				dcPOWER_EN_OFF();
				
				//等待获取APP信息
				if(tSysInfo.uInit.tFinish.bIF_AppInfo == false)
					break;
				
				static bool b_ret = true;
				c_ret = c_dc_info_init();
				if(c_ret > 0)
				{
					if((uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant) && b_ret == false)
						log_w("bDcTask:tDC获取错误清除");
					
					b_ret = true;
				}
				else
				{
					if((uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant) && b_ret == true)
					{
						log_w("bDcTask:tDC初始化失败 代码%d",c_ret);
						b_ret = false;
					}
					break;
				}
				
				tSysInfo.uInit.tFinish.bIF_DcTask = true;
				
				v_dc_set_work_state(DS_SHUT_DOWN);
            }
            break;
			
			default:
            case DS_CLOSING:
            {
				dcPOWER_EN_OFF();
				
				if(c_dc_check_out_volt() < 0)  //已经关闭
				{
					v_dc_param_init();
					v_dc_set_work_state(DS_SHUT_DOWN);
				}
				
				v_dc_protect_process();
            }
            break;
			
            case DS_SHUT_DOWN:
            {
				dcPOWER_EN_OFF();
				
				#if(boardUSE_OS)
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //等待任务通知  一直等待,直到释放通知
				#endif  //boardUSE_OS
            }
            break;
            
            case DS_ERR:
            {
				dcPOWER_EN_OFF();
				
				v_dc_protect_process();
            }
            break;
            
            case DS_BOOTING:
            {
				dcPOWER_EN_ON();
				
				static vu16 us_boot_fali_cnt = 0;
				if(c_dc_check_out_volt() == 0)
				{
					us_boot_fali_cnt = 0;
					v_dc_set_work_state(DS_WORK);
				}
				else
				{
					us_boot_fali_cnt++;
					if(us_boot_fali_cnt >= (3000 / dcTASK_CYCLE_TIME))
					{
						us_boot_fali_cnt = 0;
						v_dc_set_error_code(DC_EC_OUT_LOW,true);  
					}
				}
				
            }
            break;
            
            case DS_WORK:
            {
				dcPOWER_EN_ON();
				v_dc_protect_process();
            }
            break;
            
        }
		
		tDc.usInVolt = tAdcSamp.usSysInVolt;
		tDc.sMaxTemp = tAdcSamp.sDcTemp;
		//*********************************功率**********************************
		if( tDc.eDevState >= DS_BOOTING)
		{
			tDc.usOutVolt = tAdcSamp.usDcOutVolt;
			tDc.usOutCurr = tAdcSamp.fDcOutCurr * 10;//0.1A

			//计算DC 总功率
            tDc.usOutPwr = (tDc.usOutCurr * tDc.usOutVolt) / 100;
			
			if(tDc.usOutPwr > 1)
				us_pwr_exist_cnt ++;
			else 
				us_pwr_exist_cnt = 0;
			
			if(us_pwr_exist_cnt < 3)
				tDc.usOutPwr = 0;
			
			//有功率刷新关闭时间
			if(tDc.usOutPwr > 1)
				vDc_RefreshOffTime();
		}
		else
		{
			tDc.usOutVolt = 0;
			tDc.usOutCurr = 0;
			tDc.usOutPwr = 0;
		}
			
		
		#if(boardUSE_OS)
		vTaskDelay(dcTASK_CYCLE_TIME);
		#endif  //boardUSE_OS
    }
}



/*****************************************************************************************************************
-----函数功能    工作状态设置
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_dc_set_work_state(DevState_E stat)
{
    tDc.eDevState = stat;
	
	//启动和关闭都清除一次错误
	if(stat == DS_CLOSING || stat ==DS_BOOTING)
	{
		if(tDc.uErrCode.ucErrCode)
			v_dc_set_error_code(DC_EC_CLEAR_ALL,false);
	}
		
}


/*****************************************************************************************************************
-----函数功能    设置错误代码
-----说明(备注)  none
-----传入参数    
				 DC_EC_NULL = 0,
				 DC_EC_LOW_VBMS,
				 DC_EC_OUT_ERR,
				 DC_EC_OT,
				 DC_EC_OL,
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_dc_set_error_code(DcErrCode_E code ,bool set)
{
	static DcErrCode_E e_next_code;
	static bool b_next_set;
	
	if(uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
	{
		if(e_next_code != code || b_next_set != set)
		{
			log_e("bDcTask:任务错误 代码%d 类型%d",code,set);
			e_next_code = code;
			b_next_set = set;
		}
	}
	
	//清除所有错误
	if(code == DC_EC_CLEAR_ALL)
	{
		tDc.uErrCode.ucErrCode = 0;
		return;
	}
	
	//有错误
	if(set)
	{
		ERR_SET(tDc.uErrCode.ucErrCode, (code - 1));

		#if(boardBUZ_EN)
		bBuz_Tweet(LONG_3);
		#endif  //boardBUZ_EN
	}
	else
		ERR_CLR(tDc.uErrCode.ucErrCode, (code - 1));
	
	
	
	if(tDc.uErrCode.ucErrCode)
	{
		if(tDc.eDevState != DS_ERR)
			v_dc_set_work_state(DS_ERR);
	}
	else 
	{
		if(code == DC_EC_OUT_LOW)
		{
			cDc_Switch(ST_ON, false);
			
			if(uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
				log_i("bDcTask:错误清除,重新开启");
		}
	}	
}


/*****************************************************************************************************************
-----函数功能    保护处理
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_dc_protect_process(void)
{
	static u8 uc_pwr_err_cnt = 0;
	static u8 uc_clear_pwr_err_cnt = 0;
	static u8 uc_over_temp_cnt = 0;
	static u8 uc_clear_over_temp_cnt = 0;
	static u8 uc_overload_cnt = 0;
	static u8 uc_over_curr_cnt = 0;
	static u8 uc_output_low_cnt = 0;
	static u8 uc_output_high_cnt = 0;
	static u8 uc_clear_output_err_cnt = 0;
	static u8 uc_close_fail_cnt = 0;
	static u8 uc_clear_close_fail_cnt = 0;
	static u8 uc_ntc_lost_cnt = 0;
	static u8 uc_temp = 0;
	
	//NTC检测
	if(tDc.eDevState == DS_WORK)
	{
		if(tDc.sMaxTemp == 0 && tDc.uErrCode.tCode.bNtcLost == 0)
		{
			uc_ntc_lost_cnt++;
			if(uc_ntc_lost_cnt >= (3000 / dcTASK_CYCLE_TIME))
			{
				uc_ntc_lost_cnt = 0;
				v_dc_set_error_code(DC_EC_NTC_LOST,true);
			}
		}
		else if(tDc.sMaxTemp > 5)
		{
			uc_ntc_lost_cnt = 0;
			if(tDc.uErrCode.tCode.bNtcLost == 1)
				v_dc_set_error_code(DC_EC_NTC_LOST,false);
		}
	}
	else
	{
		uc_ntc_lost_cnt = 0;
	}
	
	//--------------------------------------电源错误检查-------------------------------------
	//工作状态才检查
	if(tDc.eDevState == DS_BOOTING ||
	   tDc.eDevState == DS_WORK)
	{
		if(c_dc_check_in_volt() != 0)
		{
			uc_clear_pwr_err_cnt = 0;
			if(tDc.uErrCode.tCode.bPowerErr == 0)
			{
				uc_pwr_err_cnt++;
				if(uc_pwr_err_cnt >= 10)
				{
					uc_pwr_err_cnt = 0;
					v_dc_set_error_code(DC_EC_PWR_ERR,true); //设置错误
				}
			}
		}
		else
		{
			uc_pwr_err_cnt = 0;
			if(tDc.uErrCode.tCode.bPowerErr == 1)
			{
				uc_clear_pwr_err_cnt++;
				if(uc_clear_pwr_err_cnt >= 10)
				{
					uc_clear_pwr_err_cnt = 0;
					v_dc_set_error_code(DC_EC_PWR_ERR,false); //清除错误
				}
			}
		}
    }
	else
	{
		uc_pwr_err_cnt = 0;
		uc_clear_pwr_err_cnt = 0;
	}
	
	
	//-------------------------------过温检查------------------------------------------------
	if(tDc.sMaxTemp > tAppMemParam.tDC.sMaxTemp)
	{
		uc_clear_over_temp_cnt = 0;
		if(tDc.uErrCode.tCode.bOT == 0)
		{
			uc_over_temp_cnt++;
			if(uc_over_temp_cnt >= 20)
			{
				v_dc_set_error_code(DC_EC_OT,true);  //设置错误
				uc_over_temp_cnt = 0;
			}
		}
	}
	//相差10摄氏度则开始退出高温报警
	else  if(tDc.sMaxTemp < (tAppMemParam.tDC.sMaxTemp - 10))
	{
		uc_over_temp_cnt = 0;
		if(tDc.uErrCode.tCode.bOT == 1)
		{
			uc_clear_over_temp_cnt++;
			if(uc_clear_over_temp_cnt >= 20)
			{
				v_dc_set_error_code(DC_EC_OT,false);   //清除错误
				uc_clear_over_temp_cnt = 0;
			}
		}
	}
	
	
	//------------------------------------------过流检查-------------------------------------
	if(tAdcSamp.fDcOutCurr > 13.0f)
	{
		if(tDc.uErrCode.tCode.bOL == 0)
		{
			uc_over_curr_cnt++;
			if(uc_over_curr_cnt >= 5)  //1S
			{
				v_dc_set_error_code(DC_EC_OL,true);  //设置错误
				uc_over_curr_cnt = 0;
			}
		}
		else
			uc_over_curr_cnt = 0;
	}
	else
		uc_over_curr_cnt = 0;
	
	//需要关闭清除
	if(tAdcSamp.fDcOutCurr > 11.6f)
	{
		if(tDc.uErrCode.tCode.bOL == 0)
		{
			uc_overload_cnt++;
			if(uc_overload_cnt >= 25)  //5S
			{
				v_dc_set_error_code(DC_EC_OL,true);  //设置错误
				uc_overload_cnt = 0;
			}
		}
		else 
			uc_overload_cnt = 0;
	}
	else
		uc_overload_cnt = 0;
	
	
	
	//----------------------------------------------输出丢失----------------------------------
	//工作状态才检查
	if(tDc.eDevState == DS_WORK)
	{
		if(c_dc_check_out_volt() < 0)
		{
			if(tDc.eDevState == DS_WORK)
				uc_temp = 4;
			else 
				uc_temp = 15;
			
			uc_clear_output_err_cnt = 0;
			uc_output_high_cnt = 0;
			if(tDc.uErrCode.tCode.bOutLow == 0)
			{
				uc_output_low_cnt++;
				if(uc_output_low_cnt >= uc_temp)
				{
					uc_output_low_cnt = 0;
					v_dc_set_error_code(DC_EC_OUT_LOW,true);  
				}
			}
		}
		else if(c_dc_check_out_volt() > 0)
		{
			uc_clear_output_err_cnt = 0;
			uc_output_low_cnt = 0;
			if(tDc.uErrCode.tCode.bOutHigh == 0)
			{
				uc_output_high_cnt++;
				if(uc_output_high_cnt >= 3)
				{
					uc_output_high_cnt = 0;
					v_dc_set_error_code(DC_EC_OUT_HIGH,true); 
				}
			}
		}
		else
		{
			uc_output_low_cnt = 0;
			uc_output_high_cnt = 0;
			if(tDc.uErrCode.tCode.bOutHigh == 1 || 
			   tDc.uErrCode.tCode.bOutLow == 1)
			{
				uc_clear_output_err_cnt++;
				if(uc_clear_output_err_cnt >= 4)
				{
					uc_clear_output_err_cnt = 0;
					
					if(tDc.uErrCode.tCode.bOutHigh == 1)
						v_dc_set_error_code(DC_EC_OUT_HIGH,false); 
					
					if(tDc.uErrCode.tCode.bOutLow == 1)
						v_dc_set_error_code(DC_EC_OUT_LOW,false); 
				}
			}
		}
	}
	else
	{
		uc_output_low_cnt = 0;
		uc_output_high_cnt = 0;
		uc_clear_output_err_cnt = 0;
	}
	
	
	//----------------------------------------------关闭输出失败----------------------------------
	//关闭中或错误才检查
	if(tDc.eDevState == DS_CLOSING ||
	   tDc.eDevState == DS_ERR)
	{
		if(c_dc_check_out_volt() >= 0)
		{
			uc_clear_close_fail_cnt = 0;
			if(tDc.uErrCode.tCode.bCloseFail == 0)
			{
				uc_close_fail_cnt++;
				if(uc_close_fail_cnt >= 50)
				{
					uc_close_fail_cnt = 0;
					v_dc_set_error_code(DC_EC_CLOSE_FAIL,true); 
				}
			}
		}
		else
		{
			uc_close_fail_cnt = 0;
			if(tDc.uErrCode.tCode.bCloseFail == 1)
			{
				uc_clear_close_fail_cnt++;
				if(uc_clear_close_fail_cnt >=10)
				{
					uc_clear_close_fail_cnt = 0;
					v_dc_set_error_code(DC_EC_CLOSE_FAIL,false); 
				}
			}
		}
	}
	else
	{
		uc_close_fail_cnt = 0;
	}		
}

/*****************************************************************************************************************
-----函数功能	获取信息
-----说明(备注)	none
-----传入参数	none
-----输出参数	none
-----返回值		小于0:失败	
				0:未完成
				大于0:完成
******************************************************************************************************************/
static s8 c_dc_info_init(void)
{
	s8 ret = 0;
	const char* p_obj_str = tDcMemParamStr;
	static bool b_ret = true;
	
	//已经初始化
	if(tSysInfo.uInit.tFinish.bIF_SysInit == true)
	{
		ret = cApp_GetMemParam(p_obj_str);
		if(ret > 0)//成功
			return 1;

		if((uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant) && b_ret == true)
		{
			log_e("bDcTask:当前系统已经初始化完成,但是tDC读取依旧为空,准备重置");
			b_ret = false;
		}	
	}
	
	//重新初始化
	ret = cApp_MemParamInit(p_obj_str);
	if(ret <= 0)//失败
		return -1;
	
	ret = cApp_UpdateMemParam(p_obj_str);
	if(ret <= 0)//失败
		return -2;
	
	b_ret = true;
	return 2;
}

/***********************************************************************************************************************
-----函数功能    检查DC输出状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      电压状态 小于0;欠压  0:电压正常  1:过压
************************************************************************************************************************/
static s8 c_dc_check_out_volt(void)
{
	if(RANGE(tDc.usOutVolt, 
	   tAppMemParam.tDC.usMinOutVolt, 
	   tAppMemParam.tDC.usMaxOutVolt))
	{
		return 0;
	}
	else if(tDc.usOutVolt > tAppMemParam.tDC.usMaxOutVolt)
	{
		return 1;
	}
	else
		return -1;
}

/***********************************************************************************************************************
-----函数功能    检查DC供电状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      电压状态 小于0;欠压  0:电压正常  1:过压
************************************************************************************************************************/
static s8 c_dc_check_in_volt(void)
{
	if(tDc.usInVolt >= tAppMemParam.tDC.usMinOpenVolt)
	{
		return 0;
	}
	else
		return -1;
}


























/*****************************************************************************************************************
-----函数功能    快充开关
-----说明(备注)  none
-----传入参数    ST_NULL=0,//进行取反
				 ST_ON,
				 ST_OFF,
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 cDc_Switch(SwitchType_E Tri_Type, bool fore_en)
{
	switch(Tri_Type)
	{
		case ST_ON:
		{
			if((tDc.eDevState == DS_WORK || 
				tDc.eDevState == DS_BOOTING) && 
				fore_en == false)  //已经开启,则退出
			{
				if(uPrint.tFlag.bDcTask)
					sMyPrint("bDcTask:当前状态为工作,不允许开机\r\n");
				 
				return 0;
			}
			
			goto LoopOn;
		}
		
		case ST_OFF:
		{
			if((tDc.eDevState == DS_SHUT_DOWN ||
				tDc.eDevState == DS_CLOSING) && 
				fore_en == false)  //已经关闭,则退出
			{
				if(uPrint.tFlag.bDcTask)
					sMyPrint("bDcTask:当前状态为关闭,不允许关机\r\n");
				 
				return 0;
			}
			
			goto LoopOff;
		}
		
		default:
		    if(tDc.eDevState <= DS_SHUT_DOWN)  //开启
			{
				LoopOn:
				if(tSysInfo.uPerm.tPerm.bDisChgPerm == false)
				{
					#if(boardBUZ_EN)
					bBuz_Tweet(SHORT_2);
					#endif  //boardBUZ_EN

					if(uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
						log_w("bDcTask:系统不允许开启放电");
					
					return -1;
				}

				// if(c_dc_check_in_volt() != 0)  //电池电压低于保护值
				// {
				// 	v_dc_set_error_code(DC_EC_PWR_ERR,true);
					
				// 	if(uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
				// 		log_w("bDcTask:开启欠压 电压=%dV",tDc.usInVolt/10);
					
				// 	return -2;
				// } 

				v_dc_set_work_state(DS_BOOTING); 

				#if(boardBUZ_EN)
				bBuz_Tweet(LONG_1);
				#endif  //boardBUZ_EN

				if(uPrint.tFlag.bDcTask)
					sMyPrint("bDcTask:----DC 开启----\r\n");
				
				xTaskNotifyGive(tDcTaskHandler); //发通知

			}
			else                       //其他情况都是关闭
			{
				LoopOff:
				if(tDc.eDevState != DS_SHUT_DOWN)       //故障中、开启中 或 已开启  ： 进行关闭
					v_dc_set_work_state(DS_CLOSING);    

				#if(boardBUZ_EN)
				bBuz_Tweet(LONG_1);
				#endif  //boardBUZ_EN
				
				if(uPrint.tFlag.bDcTask)
					sMyPrint("bDcTask:----DC 关闭----\r\n");
				
			}
			break;
	}
	
	#if(boardSYS_DATA_UPADATA)
	Sys_Update_Mod(DC_Mod,true);
	#endif  //boardLOW_POWER

	return 1 ;
}


/***********************************************************************************************************************
-----函数功能    自动关闭计时
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDc_TickTimer(void) 
{
	//非工作状态下退出
	if(bSys_IsWorkState() == false) 
		return;
	
	//非工作模式不计时
	if(tDc.eDevState != DS_WORK)
		return;
	
	//-----自动关闭--------------------------------------   
	if(tDc.usAutoOffTime)
	{
		if(tDc.usAutoOffCnt)
		{
			tDc.usAutoOffCnt--;
			if(tDc.usAutoOffCnt == 0)
			{
				cDc_Switch(ST_OFF, false);
				if(uPrint.tFlag.bDcTask || uPrint.tFlag.bImportant)
					sMyPrint("Dc_Task:====倒计时结束,关闭DC  时间=%dS====\r\n", tDc.usAutoOffTime);
			}
		}
	}
}


/***********************************************************************************************************************
-----函数功能    刷新关闭时间
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDc_RefreshOffTime(void) 
{  
	if(tDc.usAutoOffTime)
	{
		tDc.usAutoOffCnt = tDc.usAutoOffTime;
	}
}

/*****************************************************************************************************************
-----函数功能    初始化参数
-----说明(备注)  none
-----传入参数    p_dc_mem : DC记忆参数结构体
-----输出参数    none
-----返回值      true:设置成功  反之失败
*****************************************************************************************************************/
bool bDc_MemParamInit(DcMemParam_T* p_dc_mem)
{
	p_dc_mem->usAutoOffTime = boardDC_OFF_TIME;
	p_dc_mem->usMaxOutVolt = boardDC_MAX_OUT_VOLT;
	p_dc_mem->usMinOutVolt = boardDC_MIN_OUT_VOLT;
	p_dc_mem->usOverLoadPwr = boardDC_OVERLOAD_PWR;
	p_dc_mem->usMinOpenVolt = boardDC_OPEN_MIN_VOLT;
	p_dc_mem->sMaxTemp = boardDC_MAX_TEMP;
	return true;
}

/*****************************************************************************************************************
-----函数功能    设置记忆参数
-----说明(备注)  none
-----传入参数    add:true 增加   false:减少
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vDc_MemParamSet(u8 item, bool add)
{
	switch(item)
	{
		case 0:
		{
			if(add == true)
			{
				if(tAppMemParam.tDC.usAutoOffTime < 3600)
					tAppMemParam.tDC.usAutoOffTime++;
			}
			else
			{
				if(tAppMemParam.tDC.usAutoOffTime > 0)
					tAppMemParam.tDC.usAutoOffTime--;
			}
		}
		break;
		
		case 1:
		{
			if(add == true)
				tAppMemParam.tDC.usMaxOutVolt++;
			else
				tAppMemParam.tDC.usMaxOutVolt--;
		}
		break;
		
		case 2:
		{
			if(add == true)
				tAppMemParam.tDC.usMinOutVolt++;
			else
				tAppMemParam.tDC.usMinOutVolt--;
		}
		break;
		
		case 3:
		{
			if(add == true)
				tAppMemParam.tDC.usOverLoadPwr++;
			else
				tAppMemParam.tDC.usOverLoadPwr--;
		}
		break;
		
		case 4:
		{
			if(add == true)
				tAppMemParam.tDC.usMinOpenVolt++;
			else
				tAppMemParam.tDC.usMinOpenVolt--;
		}
		break;
		
		case 5:
		{
			if(add == true)
			{
				if(tAppMemParam.tDC.sMaxTemp < 127)
					tAppMemParam.tDC.sMaxTemp++;
			}
			else
			{
				if(tAppMemParam.tDC.sMaxTemp > -127)
					tAppMemParam.tDC.sMaxTemp--;
			}
		}
		break;
	}
}

#if(boardLOW_POWER)
/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vDc_EnterLowPower(void)
{
	rcu_periph_clock_enable(DC2_SDA_RCU);
	gpio_init(DC2_SDA_GPIO, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ,DC2_SDA_PIN);
	
	vTaskSuspend(tDcTaskHandler); //暂停任务
}



/*****************************************************************************************************************
-----函数功能    退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vDc_ExitLowPower(void)
{
	vDc_Init();
	vTaskResume(tDcTaskHandler); //恢复任务
}
#endif  //boardLOW_POWER

#endif  //boardDC_EN
