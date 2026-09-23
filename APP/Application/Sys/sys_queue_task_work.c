/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_work.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统正常工作状态队列任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  //boardDCAC_EN

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#endif  //boardMPPT_EN

#if (boardADC_EN)
#include "Adc/adc_task.h"
#endif  //boardADC_EN

#include "app_info.h"
#include "gpio_init.h"

//****************************************************Macros********************************************************************//
#define			sysTASK_WORK_CYCLE_TIME					10		//任务时间

//****************************************************Function Declaration******************************************************//
static void v_chg_pwr_manage(void);

/***********************************************************************************************************************
 * 函数功能    : 系统工作状态队列任务执行函数
 * 说明(备注)  : 定周期管理充放电许可与各通道充电功率配比
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 更新任务状态
 * 返回值      : void
 ************************************************************************************************************************/
void v_sys_queue_task_work(Task_T *p_task)
{
	if (tSysInfo.eDevState != DS_WORK)
		bSys_SetDevState(DS_WORK, false);

	if (tSysInfo.uErrCode.tCode.bBootFault)
		bSys_SetErrCode(SEC_BOOT_FAULT, false);

	//检查系统活跃状态
	if (bSys_CheckActState() == true)
		bSys_SetAutoOffTime(tAppMemParam.tSYS.usAutoOffTime);

	//队列里面有任务
	if (lwrb_get_full(&p_task->tQueueBuff))
	{
		cQueue_GotoStep(p_task, STEP_END);  //结束
		return;
	}

	switch (p_task->ucStep)
	{
		case 0:
		{
			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
		}
		break;

		case 1:
		{
			v_chg_pwr_manage();
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  //结束
		}
		break;
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, sysTASK_WORK_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
	#endif  //boardUSE_OS
}

/***********************************************************************************************************************
 * 函数功能    : 系统充电功率管理函数
 * 说明(备注)  : 根据 BMS 允许充电功率及温度动态调节 MPPT 与 DCAC 的充电配比
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
__STATIC_INLINE void v_chg_pwr_manage(void)
{
	#if (boardBMS_EN)

	//1:充满  0:刚插上电可以充电  -1:一直插着电可以充电
	static s8 s_c_chg_full_flag = 0;

	memset(&tSysInfo.tSetChgPwr, 0, sizeof(tSysInfo.tSetChgPwr));

	//充满后,需要降低到90才再次开始充电
	if (ucBms_GetSoc() == 100)
		s_c_chg_full_flag = 1;
	else if (ucBms_GetSoc() <= 90)
		s_c_chg_full_flag = -1;

	//移除充电,可以再次充电
	if (
		#if (boardMPPT_EN)
		tMppt.eDevState == DS_SHUT_DOWN
		#else
		true
		#endif  //boardMPPT_EN
		&&
		#if (boardDCAC_EN)
		tDcac.eChgState == IOS_SHUT_DOWN
		#else
		true
		#endif  //boardDCAC_EN
	)
		s_c_chg_full_flag = 0;

	//不许可充电
	if ((tSysInfo.uPerm.tPerm.bChgPerm == false) || (tSysInfo.uPerm.tPerm.bForceClose == true))
		return;

	//充满未释放
	if (s_c_chg_full_flag > 0)
		return;

	//===== 核心改造: 直接使用BMS许可功率作为总充电功率限制 =====
	//BMS上报的usPermMaxChgPwr已包含SOC和温度限制
	u16 us_bms_perm = tBmsRx.usPermMaxChgPwr;

	//BMS不允许充电(通信异常或BMS主动禁止)
	if (us_bms_perm == 0)
		return;

	//设置MPPT充电功率 (MPPT优先: 给MPPT最大可用额度, MPPT尽力输出)
	#if (boardMPPT_EN)
	if ((tMppt.bChgPerm == true) && (tMppt.eDevState >= DS_BOOTING))
	{
		tSysInfo.tSetChgPwr.usMPPT = tAppMemParam.tMPPT.usInPwrRating / 10;

		//设置MPPT充电功率,根据温度降功率
		//0:全功率  1:0.75功率 2:0.5功率
		static u8 s_uc_temp_gear = 0;
		if (s_uc_temp_gear == 1)
		{
			if (tDcac.sMaxTemp <= 60)
				s_uc_temp_gear = 0;
			else if (tDcac.sMaxTemp > 70)
				s_uc_temp_gear = 2;

			tSysInfo.tSetChgPwr.usMPPT = (tSysInfo.tSetChgPwr.usMPPT * 3) / 4;
		}
		else if (s_uc_temp_gear == 2)
		{
			if (tDcac.sMaxTemp <= 65)
				s_uc_temp_gear = 1;

			tSysInfo.tSetChgPwr.usMPPT = tSysInfo.tSetChgPwr.usMPPT / 2;
		}
		else
		{
			if (tDcac.sMaxTemp > 70)
				s_uc_temp_gear = 2;
			else if (tDcac.sMaxTemp > 65)
				s_uc_temp_gear = 1;

			tSysInfo.tSetChgPwr.usMPPT = tSysInfo.tSetChgPwr.usMPPT;
		}

		//根据接口,限制功率
		//DC输入模式:严格限制输入电流不超过7A,通过输入电压计算最大功率上限
		//PV输入模式:不进行电流限制,保持当前功率设置
		//原理:功率(W)=电压(V)×电流(A),tMpptRx.usInVolt单位为0.1V
		//      7A对应最大功率=usInVolt*0.1V*7A=usInVolt*0.7W
		if(tMppt.eWorkMode == MWM_DC)
		{
			u16 us_dc_curr_limit_pwr = (u16)(tMpptRx.usInVolt * 0.7f);
			if(tSysInfo.tSetChgPwr.usMPPT > us_dc_curr_limit_pwr)
				tSysInfo.tSetChgPwr.usMPPT = us_dc_curr_limit_pwr;
		}
		//MWM_PV模式不限制电流,无需处理

		tSysInfo.tSetChgPwr.usMPPT = MIN3(us_bms_perm,
		                                  tAppMemParam.tMPPT.usInPwrRating / 10,
		                                  tSysInfo.tSetChgPwr.usMPPT);
	}
	#endif  //boardMPPT_EN

	//设置DCAC充电功率 (DCAC管理PV+AC总功率, 固件自动补偿MPPT实际输出)
	#if (boardDCAC_EN)
	if ((tDcac.uPerm.tPerm.bChgPerm == true) && (tDcac.eChgState >= IOS_STARTING))
		tSysInfo.tSetChgPwr.usDCAC = MIN2(us_bms_perm,
		                                  tAppMemParam.tDCAC.usInPwrRating);
	#endif  //boardDCAC_EN

	#endif  //boardBMS_EN
}

