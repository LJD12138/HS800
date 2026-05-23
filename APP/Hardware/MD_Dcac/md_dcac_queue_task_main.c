/*****************************************************************************************************************
*                                                                                                                *
 *                                         队列函数                                                  			*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "Adc/adc_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "app_info.h"


#define       	dcacTASK_GET_PARAM_CYCLE_TIME			1000


//****************************************************参数初始化**************************************************//


//****************************************************函数声明****************************************************//
static void v_proc_rec_param(void);
static void v_check_dischg_or_chg_perm(void);
static void v_check_close_dischg(void);
static void v_set_total_chg_pwr(void);
static void v_set_ac_chg_pwr(void);
static u16 us_get_total_chg_pwr_by_in_curr(vu16 us_total_chg_pwr);

/*****************************************************************************************************************
-----函数功能    任务函数:主任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_main(Task_T *tp_task)
{
	//*****************************************非工作模式,检查逆变是否开启中**************************************
	if(tDcac.eDisChgState >= IOS_STARTING && 
		(tSysInfo.uPerm.tPerm.bDisChgPerm ==false || tDcac.uPerm.tPerm.bDisChgPerm ==false))  //AC输出还在打开
	{
		cQueue_AddQueueTask(tp_task, DTI_CTRL_DCAC_OUT, ST_OFF,false);//关闭逆变
		
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:当前不许可放电,添加关闭逆变输出任务");
	}
	else if(tDcac.eParanInState >= IOS_STARTING && 
		tDcac.uPerm.tPerm.bParaInPerm == false)
	{
		cQueue_AddQueueTask(tp_task, DTI_CTRL_PARA_IN, ST_OFF,false);//关闭逆变
		
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:当前不许可并网,添加关闭并网任务");
	}
//	else if(tDcac.eChgState >= IOS_STARTING && 
//		(tSysInfo.uPerm.tPerm.bChgPerm ==false || tDcac.uPerm.tPerm.bChgPerm == false))
//	{
//		cQueue_AddQueueTask(tp_task, DTI_CTRL_DCAC_IN, ST_OFF,false);//关闭逆变
	
//		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
//			log_w("bDcacTask:当前不许可充电,添加关闭逆变输入任务");
//	}
	
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
			if(b_dcac_cs_get_param1() == true)  //获取参数
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			else
				break;
        }
		
		case 1:
        {
			if(b_dcac_cs_get_param2() == true)  //获取参数
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			else
				break;
        }
		
		case 2:
        {
			if(b_dcac_cs_get_param3() == true)  //获取参数
				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			else
				break;
        }
		
		case 3:
        {
			//处理接受数据
			v_proc_rec_param();
			//检查充放电许可
			v_check_dischg_or_chg_perm();
			//放电控制
			v_check_close_dischg();
			//设置总的充电功率
			v_set_total_chg_pwr();
			//设置AC的充电功率
			v_set_ac_chg_pwr();
			
			cQueue_GotoStep(tp_task, 0);
        }
		break;
			
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
	
	vTaskDelay(dcacTASK_GET_PARAM_CYCLE_TIME);
}

/*****************************************************************************************************************
-----函数功能    获取参数处理
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_proc_rec_param(void)
{
	//获取温度
	tDcac.sMaxTemp = tDcacRx.sMaxTemp;
	
	//输入
	if(tDcac.eChgState != IOS_WORK)
		tDcacRx.uErrCode.usCode[2] &= ~0x68;
	
	if(tDcacRx.uErrCode.tCode.tIn.bOV1 ||
		tDcacRx.uErrCode.tCode.tIn.bOV2)
	{
		if(tDcac.uErrCode.tCode.bDcacInVolt == 0)
			bDcac_SetErrCode(DEC_DCAC_IN_VOLT, true);
	}
	else 
	{
		if(tDcac.uErrCode.tCode.bDcacInVolt == 1)
			bDcac_SetErrCode(DEC_DCAC_IN_VOLT, false);
	}
	
	// if((tDcacRx.uErrCode.tCode.tIn.bOF1 ||
	// 	tDcacRx.uErrCode.tCode.tIn.bUF1) &&
	// 	tDcacRx.usInVolt > tAppMemParam.tDCAC.usMinInVolt)
	// {
	// 	if(tDcac.uErrCode.tCode.bDcacInFreq == 0)
	// 		bDcac_SetErrCode(DEC_DCAC_IN_FREQ, true);
	// }
	// else 
	// {
	// 	if(tDcac.uErrCode.tCode.bDcacInFreq == 1)
	// 		bDcac_SetErrCode(DEC_DCAC_IN_FREQ, false);
	// }
	
	//输出
	if(tDcacRx.uErrCode.tCode.tAc.bOV ||
		tDcacRx.uErrCode.tCode.tAc.bUV)
	{
		if(tDcac.uErrCode.tCode.bDcacOutVolt == 0)
			bDcac_SetErrCode(DEC_DCAC_OUT_VOLT, true);
	}
	else 
	{
		if(tDcac.uErrCode.tCode.bDcacOutVolt == 1)
			bDcac_SetErrCode(DEC_DCAC_OUT_VOLT, false);
	}
	
	if(tDcacRx.uErrCode.tCode.tAc.bBootErr ||
		tDcacRx.uErrCode.tCode.tAc.bSysErr ||
		tDcacRx.uErrCode.tCode.tAc.bMsgErr)
	{
		if(tDcac.uErrCode.tCode.bDcacOutOther == 0)
			bDcac_SetErrCode(DEC_DCAC_OUT_OTHER, true);
	}
	else 
	{
		if(tDcac.uErrCode.tCode.bDcacOutOther == 1)
			bDcac_SetErrCode(DEC_DCAC_OUT_OTHER, false);
	}
	
	//高压母线
	if(tDcacRx.uErrCode.usCode[0])
	{
		if(tDcac.uErrCode.tCode.bDcacHighVolt == 0)
			bDcac_SetErrCode(DEC_DCAC_HIGH_VOLT, true);
	}
	else
	{
		if(tDcac.uErrCode.tCode.bDcacHighVolt == 1)
			bDcac_SetErrCode(DEC_DCAC_HIGH_VOLT, false);
	}
	
	//电池
	if(tDcacRx.uErrCode.tCode.tAc.bBusOV)
	{
		if(tDcac.uErrCode.tCode.bDcacBatOV == 0)
			bDcac_SetErrCode(DEC_DCAC_BAT_OV, true);
	}
	else 
	{
		if(tDcac.uErrCode.tCode.bDcacBatOV == 1)
			bDcac_SetErrCode(DEC_DCAC_BAT_OV, false);
	}
	
	//过温
	if(tDcacRx.uErrCode.tCode.tDc.bOT ||
		tDcacRx.uErrCode.tCode.tAc.bOT)
	{
		if(tDcac.uErrCode.tCode.bDcacOT == 0)
			bDcac_SetErrCode(DEC_DCAC_OT, true);
	}
	else 
	{
		if(tDcac.uErrCode.tCode.bDcacOT == 1)
			bDcac_SetErrCode(DEC_DCAC_OT, false);
	}
	
	//过流
	if(tDcacRx.uErrCode.tCode.tDc.bOC ||
		tDcacRx.uErrCode.tCode.tAc.bOC)
	{
		if(tDcac.uErrCode.tCode.bDcacOC == 0)
			bDcac_SetErrCode(DEC_DCAC_OC, true);
	}
	else 
	{
		if(tDcac.uErrCode.tCode.bDcacOC == 1)
			bDcac_SetErrCode(DEC_DCAC_OC, false);
	}
	
	//过载
	if(tDcacRx.uErrCode.tCode.tAc.bOL)
	{
		if(tDcac.uErrCode.tCode.bDcacOL == 0)
			bDcac_SetErrCode(DEC_DCAC_OL, true);
	}
	else 
	{
		if(tDcac.uErrCode.tCode.bDcacOL == 1)
			bDcac_SetErrCode(DEC_DCAC_OL, false);
	}
	
	//短路
	if(tDcacRx.uErrCode.tCode.tAc.bSC)
	{
		if(tDcac.uErrCode.tCode.bDcacSC == 0)
			bDcac_SetErrCode(DEC_DCAC_SC, true);
	}
	else 
	{
		if(tDcac.uErrCode.tCode.bDcacSC == 1)
			bDcac_SetErrCode(DEC_DCAC_SC, false);
	}
	
	//NTC
	if(tDcacRx.uErrCode.tCode.tDc.bNtcErr)
	{
		if(tDcac.uErrCode.tCode.bDcacNtc == 0)
			bDcac_SetErrCode(DEC_DCAC_NTC, true);
	}
	else 
	{
		if(tDcac.uErrCode.tCode.bDcacNtc == 1)
			bDcac_SetErrCode(DEC_DCAC_NTC, false);
	}
	
	
	//----------------------------------故障处理--------------------------------------
	//过温
	static vu16  us_over_temp_cnt = 0;
	if(tDcac.sMaxTemp >= tAppMemParam.tDCAC.sMaxTemp)  
	{
		if(tDcac.uErrCode.tCode.bSysOT == 0)
		{
			us_over_temp_cnt++;
			if(us_over_temp_cnt >= 2)
			{
				us_over_temp_cnt = 0;
				bDcac_SetErrCode(DEC_SYS_OT,true);//过温
			}
		}
		else 
			us_over_temp_cnt = 0;
	}
	else if(tDcac.sMaxTemp < (tAppMemParam.tDCAC.sMaxTemp * 0.93f))  //0.1V
	{
		if(tDcac.uErrCode.tCode.bSysOT == 1)
		{
			us_over_temp_cnt++;
			if(us_over_temp_cnt >= 2)
			{
				us_over_temp_cnt = 0;
				bDcac_SetErrCode(DEC_SYS_OT,false);
			}
		}
		else 
			us_over_temp_cnt = 0;
	}
	
	//供电低压
	static vu16  us_pwr_volt_low_cnt = 0;
	if(tAdcSamp.usSysInVolt < tAppMemParam.tDCAC.usMinOpenVolt ||
		(ucBms_GetSoc() <= 5 && tDcacRx.usOutPwr > 1500))  
	{
		if(tDcac.uErrCode.tCode.bSysLV == 0 && tDcac.eDisChgState >= IOS_STARTING)
		{
			us_pwr_volt_low_cnt++;
			if(us_pwr_volt_low_cnt >= 2)
			{
				us_pwr_volt_low_cnt = 0;
				bDcac_SetErrCode(DEC_SYS_UV,true);//欠压故障
			}
		}
		else 
			us_pwr_volt_low_cnt = 0;
	}
	else if(tAdcSamp.usSysInVolt > (tAppMemParam.tDCAC.usMinOpenVolt + 20))  //0.1V
	{
		if(tDcac.uErrCode.tCode.bSysLV == 1)
		{
			us_pwr_volt_low_cnt++;
			if(us_pwr_volt_low_cnt >= 5)
			{
				us_pwr_volt_low_cnt = 0;
				bDcac_SetErrCode(DEC_SYS_UV,false);
			}
		}
		else 
			us_pwr_volt_low_cnt = 0;
	}
	
	//过流保护
	static vu16  us_total_curr = 0;
    static vu16  us_dyn_delay = 0;
	static vu16  us_over_curr_cnt=0; 
	us_total_curr = tDcacRx.usInCurr;  //单位0.1A
	if(us_total_curr >= (tAppMemParam.tDCAC.usMaxInCurr * 1.33))
	{
		if(us_dyn_delay != 4)
			us_over_curr_cnt = 0;

		us_dyn_delay = 4;
	}
	else
	{
		if(us_dyn_delay != 10)
			us_over_curr_cnt = 0;

		us_dyn_delay = 10;
	}
	
	if(tDcac.uErrCode.tCode.bSysInOC == 0)
	{
		if(us_total_curr >= (tAppMemParam.tDCAC.usMaxInCurr * 1.05))
		{
			us_over_curr_cnt++;
			if(us_over_curr_cnt >= us_dyn_delay)
			{
				us_over_curr_cnt = 0;
				bDcac_SetErrCode(DEC_SYS_IN_OC,true);
			}
		}
	}
	else 
		us_over_curr_cnt = 0;
	
	//过载保护
	//如果是边冲边放,就不检测过载,只检测过压
	
	vu16  us_overload_pwr = 0;
	vu16  us_overload_pwr1 = 0;
	static vu16  us_temp = 0;
	static vu16  us_overload_cnt=0;

	// if(tDcac.eChgState > IOS_STARTING)
	// {
	// 	us_overload_pwr = tAppMemParam.tDCAC.usOutPwrRating * 1.2f;
	// 	us_overload_pwr1 = tAppMemParam.tDCAC.usOutPwrRating * 1.4f;
	// }
	// else
	{
		us_overload_pwr = tAppMemParam.tDCAC.usOutPwrRating * 1.2f;
		us_overload_pwr1 = tAppMemParam.tDCAC.usOutPwrRating * 1.3f;
	}		
	
	if(tDcacRx.usOutPwr > tAppMemParam.tDCAC.usOverLoadPwr)
	{
		if(tDcacRx.usOutPwr >= us_overload_pwr1)
		{
			if(us_temp != 4)
				us_overload_cnt = 0;
			us_temp = 4;
		}
		else if(tDcacRx.usOutPwr >= us_overload_pwr)
		{
			if(us_temp != 28)
				us_overload_cnt = 0;
			us_temp = 28;
		}
		else
		{
			if(us_temp != 85)
				us_overload_cnt = 0;
			us_temp = 85;
		}
				
		us_overload_cnt++;
		if(us_overload_cnt >= us_temp || tDcacRx.usOutPwr >= us_overload_pwr1)
		{
			us_overload_cnt = 0;
			bDcac_SetErrCode(DEC_SYS_OUT_OL,true);
		}
	}
	else 
	{
		us_overload_cnt = 0;
	}
	
	//输出状态检测
	static vu8 lost_err_cnt=0;
	if((tDcac.eDisChgState == IOS_WORK && //系统判断为开启,但是检测到为关闭
		(tDcacRx.usOutVolt < tAppMemParam.tDCAC.usMinInVolt)) ||
	   (tDcac.eDisChgState == IOS_SHUT_DOWN && 
		(tDcacRx.usOutVolt > tAppMemParam.tDCAC.usMinInVolt))
	  )
	{
		if(tDcac.uErrCode.tCode.bSysOutErr == 0)
		{
			lost_err_cnt++;
			if(lost_err_cnt >= 10)
			{
				lost_err_cnt = 0;
				bDcac_SetErrCode(DEC_SYS_OUT_ERR,true);
				if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
					log_e("bDcacTask:错误输出状态错误 输出电压%dV",
								tDcacRx.usOutVolt / 10);
			}
		}
		else
			lost_err_cnt = 0;
	}
	else//系统判断为关闭,但是检测到为开启
	{
		if(tDcac.uErrCode.tCode.bSysOutErr == 1)
		{
			lost_err_cnt++;
			if(lost_err_cnt > 3)
			{
				lost_err_cnt = 0;
				bDcac_SetErrCode(DEC_SYS_OUT_ERR,false);
				if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
					log_i("bDcacTask:清除输出状态错误 输出电压=%dV",
								tDcacRx.usOutVolt / 10);
			}
			
		}
		else
			lost_err_cnt = 0;
	}
	
	//输入状态监测
	static vu8 uc_in_volt_state = 0;
	static u8 uc_volt_state_cnt = 0;
	if(tDcacRx.usInVolt < tAppMemParam.tDCAC.usMinInVolt - 60)
	{
		//完全掉电,清空错误
		if(tDcacRx.usInVolt < 100)
		{
			//过压
			if(tDcac.uErrCode.tCode.bSysOV == 1)
				bDcac_SetErrCode(DEC_SYS_OV,false); 
			//清除输入保护错误
			if(tDcac.uErrCode.tCode.bSysSetInProte == 1)
				bDcac_InProteFuncSwitch(false);  
			//清除输入过流错误
			if(tDcac.uErrCode.tCode.bSysInOC == 1)
				bDcac_SetErrCode(DEC_SYS_IN_OC,false); 
		}

		if(0 != uc_in_volt_state)
		{
			uc_volt_state_cnt++;
			if(uc_volt_state_cnt >= 2)
			{
				uc_volt_state_cnt = 0;
				//输入欠压
				uc_in_volt_state = 0;
			}
		}
		else 
			uc_volt_state_cnt = 0;
	}

	
	static vu16 us_colse_cnt = 0;
	if(tDcac.uErrCode.tCode.bSysOutErr == 1)
	{
		us_colse_cnt++;
		if(us_colse_cnt >= 5)
		{
			cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_OUT, ST_OFF,false);//关闭逆变
			us_colse_cnt = 0;
		}
	}
	else
		us_colse_cnt = 0;
}



/*****************************************************************************************************************
-----函数功能    检查充放电许可
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_check_dischg_or_chg_perm(void)
{
	//充电许可
	if(tDcacRx.uErrCode.usCode[0] != 0				||	//DC侧故障
		tDcacRx.uErrCode.usCode[1] != 0				||	//逆变侧故障
		tDcacRx.uErrCode.usCode[2] != 0				||	//电网侧故障
		tDcacRx.uErrCode.usCode[3] != 0				||	//系统故障
//		tDcacRx.uState.tState.bInit == 1			||	//
		tDcac.uErrCode.tCode.bSysDevLost == 1		||
		tDcac.uErrCode.tCode.bSysOT == 1			||
//		tDcac.uErrCode.tCode.bSysUT == 1			||
//		tDcac.uErrCode.tCode.bSysOV == 1			||
//		tDcac.uErrCode.tCode.bSysSetInProte == 1	||
		tDcac.uErrCode.tCode.bSysInOC == 1			||
		tSysInfo.uPerm.tPerm.bChgPerm == false)
	{
		if(tDcac.uPerm.tPerm.bChgPerm == true)
			bDcac_SetPerm(DPO_CHG, false);
	}
	else 
	{
		if(tDcac.uPerm.tPerm.bChgPerm == false)
			bDcac_SetPerm(DPO_CHG, true);
	}
	
	if(tDcacRx.uErrCode.usCode[0] != 0				||	//DC侧故障
		tDcacRx.uErrCode.usCode[1] != 0				||	//逆变侧故障
		tDcacRx.uErrCode.usCode[3] != 0				||	//系统故障
//		tDcacRx.uState.tState.bInit == 1			||	//
		tDcac.uErrCode.tCode.bSysDevLost == 1		||
		tDcac.uErrCode.tCode.bSysOT == 1			||
		tDcac.uErrCode.tCode.bSysUT == 1			||
		tDcac.uErrCode.tCode.bSysLV == 1			||
		tDcac.uErrCode.tCode.bSysOutOL == 1			||
		tDcac.uErrCode.tCode.bSysOutErr == 1		||
		tDcac.uErrCode.tCode.bSysInOC == 1			||
		tSysInfo.uPerm.tPerm.bDisChgPerm == false)
	{
		if(tDcac.uPerm.tPerm.bDisChgPerm == true)
			bDcac_SetPerm(DPO_DISCHG, false);
	}
	else 
	{
		if(tDcac.uPerm.tPerm.bDisChgPerm == false)
			bDcac_SetPerm(DPO_DISCHG, true);
	}
	
	if(tDcac.uPerm.tPerm.bDisChgPerm == false 	||
		ucBms_GetSoc() <= 10)
	{
		if(tDcac.uPerm.tPerm.bParaInPerm == true)
			bDcac_SetPerm(DPO_PARA_IN, false);
	}
	else 
	{
		if(tDcac.uPerm.tPerm.bParaInPerm == false)
			bDcac_SetPerm(DPO_PARA_IN, true);
	}
}


/*****************************************************************************************************************
-----函数功能    检查关闭放电
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_check_close_dischg(void)
{
	if(tSysInfo.eDevState != DS_WORK)
		return;
	
	//输出电压打开
	static vu16 us_out_volt_wait_cnt = 0;
	if(tDcac.uPerm.tPerm.bDisChgPerm == false)
	{
		if(tDcacRx.usOutVolt > tAppMemParam.tDCAC.usMaxInVolt || 
		tDcac.eDisChgState >= DS_BOOTING)
		{
			us_out_volt_wait_cnt++;
			if(us_out_volt_wait_cnt >= 5)
			{
				us_out_volt_wait_cnt = 0;
				
				if(uPrint.tFlag.bDcacTask)
					log_w("bDcacTask:当前设备状态0x%x,不许可放电,强制关闭",tDcac.eDevState);
				
				//关闭输出
				cDCAC_Switch(DSO_AC_OUT, ST_OFF, true);
			}
		}
		else 
			us_out_volt_wait_cnt = 0;
	}
	else 
		us_out_volt_wait_cnt = 0;
}


/*****************************************************************************************************************
-----函数功能    根据输入电流限制总充电功率
-----说明(备注)  none
-----传入参数    us_total_chg_pwr:当前总充电功率
-----输出参数    none
-----返回值      调整后的总充电功率
******************************************************************************************************************/
__STATIC_INLINE u16 us_get_total_chg_pwr_by_in_curr(vu16 us_total_chg_pwr)
{
	static s8 c_in_curr_limit = 0; //1:限制  0:不限制
	static vu16 us_in_curr_limit_reduce_pwr = 0;
	vu16 us_base_total_chg_pwr = us_total_chg_pwr;

	if(us_base_total_chg_pwr == 0)
	{
		us_in_curr_limit_reduce_pwr = 0;
		c_in_curr_limit = 0;
	}
	else
	{
		if(tDcacRx.usInCurr > 105)
		{
			vu16 us_reduce_step = 50 + ((tDcacRx.usInCurr - 105) * 20);
			c_in_curr_limit = 1;

			if((us_in_curr_limit_reduce_pwr + us_reduce_step) >= us_base_total_chg_pwr)
				us_in_curr_limit_reduce_pwr = us_base_total_chg_pwr;
			else
				us_in_curr_limit_reduce_pwr += us_reduce_step;
		}
		else if(c_in_curr_limit == 1)
		{
			if(tDcacRx.usInCurr <= 100)
			{
				if(us_in_curr_limit_reduce_pwr > 50)
					us_in_curr_limit_reduce_pwr -= 50;
				else
					us_in_curr_limit_reduce_pwr = 0;
			}

			if(us_in_curr_limit_reduce_pwr == 0)
				c_in_curr_limit = 0;
		}

		if(us_in_curr_limit_reduce_pwr >= us_base_total_chg_pwr)
			us_total_chg_pwr = 0;
		else
			us_total_chg_pwr = us_base_total_chg_pwr - us_in_curr_limit_reduce_pwr;
	}

	return us_total_chg_pwr;
}

/*****************************************************************************************************************
-----函数功能    设置总的充电功率
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_set_total_chg_pwr(void)
{
	//设置最大充电功率
	vu16 us_total_chg_pwr = 0;
	static vu16 us_last_total_chg_pwr = 0;
	static vu16 us_total_chg_pwr_err_cnt = 0;
	
	if(tSysInfo.eDevState != DS_WORK)
		return;

	if(tDcac.uPerm.tPerm.bChgPerm == false)
		us_total_chg_pwr = 0;
	else if(bSys_LowVoltReqChg() == true)
		us_total_chg_pwr = 100;
	else if(ucBms_GetSoc() >= 98)
		us_total_chg_pwr = sysCHG_PWR_LEVEL1;
	else if(ucBms_GetSoc() <= 2 || ucBms_GetSoc() >= 90)
		us_total_chg_pwr = sysCHG_PWR_LEVEL2;
	else
		us_total_chg_pwr = sysCHG_PWR_LEVEL3;

	if((tSysInfo.tSetChgPwr.usDCAC + tSysInfo.tSetChgPwr.usMPPT) == 0)
		us_total_chg_pwr = 0;

	//电芯温度超过45°,降低功率
	if(tBms.sMaxTemp >= 45)
	{
		if(us_total_chg_pwr > (sysCHG_PWR_LEVEL3 / 2))
			us_total_chg_pwr = sysCHG_PWR_LEVEL3 / 2;
	}

	us_total_chg_pwr = us_get_total_chg_pwr_by_in_curr(us_total_chg_pwr);

	if(abs(tDcacRx.usChgPwr - us_total_chg_pwr) > 100)
		us_total_chg_pwr_err_cnt++;
	else 
		us_total_chg_pwr_err_cnt = 0;

	if(us_total_chg_pwr == us_last_total_chg_pwr && 
		(us_total_chg_pwr_err_cnt < (5000 / dcacTASK_GET_PARAM_CYCLE_TIME)))
		return;

	if(b_dcac_cs_set_total_chg_pwr(us_total_chg_pwr) == true)
	{
		us_last_total_chg_pwr = us_total_chg_pwr;
		us_total_chg_pwr_err_cnt = 0;
		// sMyPrint("设置总的充电功率 %d",us_total_chg_pwr);
	}
}

/*****************************************************************************************************************
-----函数功能    设置AC充电功率
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_set_ac_chg_pwr(void)
{
	vu16 us_chg_pwr = 0;
	static vu16 us_last_chg_pwr = 100;
	static vu16 us_chg_pwr_err = 0;
	
	if(tSysInfo.eDevState != DS_WORK)
		return;
	
	//设置AC充电状态
	if(tDcacRx.usInVolt > tAppMemParam.tDCAC.usMinInVolt)
	{
		if(tDcac.uErrCode.ulCode != 0)
			bDcac_SetAcState(OO_CHG, IOS_ERR);
		else if(tDcac.eChgState != IOS_WORK && tDcac.eChgState != IOS_STARTING)
			bDcac_SetAcState(OO_CHG, IOS_STARTING);
		else if(tDcac.eChgState == IOS_STARTING)
			bDcac_SetAcState(OO_CHG, IOS_WORK);
	}
	else
		bDcac_SetAcState(OO_CHG, IOS_SHUT_DOWN);
	
	// if(tDcac.uPerm.tPerm.bChgPerm == false)
	// 	tSysInfo.tSetChgPwr.usDCAC = 0;
	
	// us_chg_pwr = tSysInfo.tSetChgPwr.usDCAC;

	// //AC设置的功率和采样到的不一致
	// if(abs(tDcacRx.usInPwr - us_chg_pwr) > 200)
	// 	us_chg_pwr_err++;
	// else 
	// 	us_chg_pwr_err = 0;

	// //功率没变化,退出
	// if(us_last_chg_pwr == us_chg_pwr && 
	// 	(us_chg_pwr_err < (5000 / dcacTASK_GET_PARAM_CYCLE_TIME)))
	// 	return;
	
	// //设置AC充电功率
	// if(b_dcac_cs_set_chg_pwr(us_chg_pwr) == true)
	// {
	// 	us_last_chg_pwr = us_chg_pwr;
	// 	us_chg_pwr_err = 0;
	// 	// sMyPrint("设置AC充电功率 %d",us_chg_pwr);
	// }	
}
#endif  //boardDCAC_EN
