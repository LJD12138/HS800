/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_queue_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "app_info.h"

#define       	bmsTASK_PARAM_CYCLE_TIME               		500


//****************************************************函数声明****************************************************//
static s8 c_bms_proc_rec_param(void);

/*****************************************************************************************************************
-----函数功能    任务函数:主任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_main(Task_T *tp_task)
{
	s8 result = 0;

	//非工作状态下,检查电池包是否开启中
	if(tSysInfo.eDevState == DS_SHUT_DOWN)
	{
		//处于非关闭状态,发送指令关闭
		if(tBms.eDevState >= DS_ERR)
			cBms_Switch(SO_KEY, ST_OFF, false);
	}
	
	//队列里面有任务
	if(lwrb_get_full(&tp_task->tQueueBuff))  
	{
		cQueue_GotoStep( tp_task, STEP_END );  //结束
		return;
	}
	
	switch (tp_task->ucStep)
    {
        case 0:
        {
			//主机
			result = c_bms_cs_get_param(1);
			//发送成功
			if(result > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);
			else
				break;
        }
		
		case 1:
        {
			//处理接受数据
			c_bms_proc_rec_param();
			cBms_CheckPerm();

			if(bSys_LowVoltReqChg() == true)
				c_bms_cs_req_chg();
			
			cQueue_GotoStep(tp_task, 0);  //结束
        }break;
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	//队列里面有任务
	if(lwrb_get_full(&tp_task->tQueueBuff))  
	{
		cQueue_GotoStep( tp_task, STEP_END );  //结束
		return;
	}
	
	#if(boardUSE_OS)
	vTaskDelay(bmsTASK_PARAM_CYCLE_TIME);
	#endif  //boardUSE_OS
}

/*****************************************************************************************************************
-----函数功能    获取参数处理
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE s8 c_bms_proc_rec_param(void)
{
	if(tSysInfo.eDevState < DS_BOOTING)
		return 0;

	//BMS状态不同步
	if(bSys_IsWorkState() == true && tBmsRx.tState.ucSysState < DS_BOOTING)
	{
		cBms_Switch(SO_KEY, ST_ON, false);

		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_w("bBmsTask:系统和BMS状态不一致 系统状态%d BMS状态%d \r\n",tSysInfo.eDevState,tBmsRx.tState.ucSysState);
	}
	else if(bSys_IsShutDownState() == true && tBmsRx.tState.ucSysState > DS_BOOTING)
	{
		cBms_Switch(SO_KEY, ST_ON, false);

		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_w("bBmsTask:系统和BMS状态不一致 系统状态%d BMS状态%d \r\n",tSysInfo.eDevState,tBmsRx.tState.ucSysState);
	}
		
	
	//----------------------------------故障处理--------------------------------------
	//充电过温检测
	static u8 uc_chg_temp_err_cnt = 0;
	static u8 uc_clear_err_cnt = 0;
	if(tBms.sMaxTemp >= tAppMemParam.tBMS.cChgMaxTemp)
    {
		if(tBms.uErrCode.tCode.bSysChgOT == 0 && bBms_GetBmsChgState() == true)
		{
			uc_chg_temp_err_cnt++;
			if(uc_chg_temp_err_cnt >= 2)
			{
				uc_chg_temp_err_cnt = 0;
				uc_clear_err_cnt = 0;
				bBms_SetErrCode(BEC_SYS_CHG_OT,true);
			}
		}
    }
	else 
	{
		 uc_chg_temp_err_cnt = 0;
		if(tBms.uErrCode.tCode.bSysChgOT == 1)
		{
			if(tBms.sMaxTemp <= (tAppMemParam.tBMS.cChgMaxTemp - 5))
			{
				uc_clear_err_cnt++;
				if(uc_clear_err_cnt >= 6)
				{
					uc_clear_err_cnt = 0;
					bBms_SetErrCode(BEC_SYS_CHG_OT,false);
				}
			}
		}
	}
	
	//放电过温检测
	static u8 uc_dischg_temp_err_cnt = 0;
	if(tBms.sMaxTemp >= tAppMemParam.tBMS.cDisChgMaxTemp)  
    {
		if(tBms.uErrCode.tCode.bSysDisChgOT == 0 && bBms_GetBmsChgState() == false)
		{
			uc_dischg_temp_err_cnt++;
			if(uc_dischg_temp_err_cnt)
			{
				uc_dischg_temp_err_cnt = 0;
				uc_clear_err_cnt = 0;
				bBms_SetErrCode(BEC_SYS_DISCHG_OT,true);
			}
		}
    }
	else 
	{
		uc_dischg_temp_err_cnt = 0;
		if(tBms.uErrCode.tCode.bSysDisChgOT == 1)
		{
			if(tBms.sMaxTemp <= (tAppMemParam.tBMS.cDisChgMaxTemp - 5))
			{
				uc_clear_err_cnt++;
				if(uc_clear_err_cnt >= 6)
				{
					uc_clear_err_cnt = 0;
					bBms_SetErrCode(BEC_SYS_DISCHG_OT,false);
				}
			}	
		}
	}
	
	//充电低温报警
	static u8 uc_low_temp_err_cnt = 0;
    if(tBms.sMinTemp <= tAppMemParam.tBMS.cChgMinTemp)
	{
		if(tBms.uErrCode.tCode.bSysChgUT == 0 && bBms_GetBmsChgState() == true)
		{
			uc_low_temp_err_cnt++;
			if(uc_low_temp_err_cnt >= 2)
			{
				uc_low_temp_err_cnt = 0;
				uc_clear_err_cnt = 0;
				bBms_SetErrCode(BEC_SYS_CHG_UT,true);
			}
		}
	}
	else if(tBms.sMinTemp > (tAppMemParam.tBMS.cChgMinTemp + 5))
	{
        uc_low_temp_err_cnt = 0;
		if(tBms.uErrCode.tCode.bSysChgUT == 1)
		{
			uc_clear_err_cnt++;
			if(uc_clear_err_cnt >= 6)
			{
				uc_clear_err_cnt = 0;
				bBms_SetErrCode(BEC_SYS_CHG_UT,false);
			}
			
		}
	}
	
    //放电低温报警
	static u8 uc_dischg_low_temp_err_cnt = 0;
    if(tBms.sMinTemp <= tAppMemParam.tBMS.cDisChgMinTemp)
	{
		if(tBms.uErrCode.tCode.bSysDisChgUT == 0 && bBms_GetBmsChgState() == false)
		{
			uc_dischg_low_temp_err_cnt++;
			if(uc_dischg_low_temp_err_cnt >= 2)
			{
				uc_dischg_low_temp_err_cnt = 0;
				uc_clear_err_cnt = 0;
				bBms_SetErrCode(BEC_SYS_DISCHG_UT,true);
			}
		}
	}
	else if(tBms.sMinTemp > (tAppMemParam.tBMS.cDisChgMinTemp + 5))
	{
        uc_dischg_low_temp_err_cnt = 0;
		if(tBms.uErrCode.tCode.bSysDisChgUT == 1)
		{
			uc_clear_err_cnt++;
			if(uc_clear_err_cnt >= 6)
			{
				uc_clear_err_cnt = 0;
				bBms_SetErrCode(BEC_SYS_DISCHG_UT,false);
			}
		}
	}
	
	return 1;
}

#endif  //boardBMS_EN
