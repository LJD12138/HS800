/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_queue_task_main.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器主队列任务实现(定时轮询参数、故障检测与功率调节)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Dcac/md_dcac_queue_task.h"
#include "main.h"

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "Adc/adc_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  /* boardBMS_EN */

#include "app_info.h"

//****************************************************Macros********************************************************************//
#define			dcacTASK_GET_PARAM_CYCLE_TIME			100

//****************************************************Function Declaration******************************************************//
static void v_proc_rec_param(void);
static void v_check_dischg_or_chg_perm(void);
static void v_check_close_dischg(void);
static void v_set_total_chg_pwr(void);
static void v_set_ac_chg_pwr(void);
static void v_check_freq_auto_mem(void);
static inline void v_sync_err_state(bool b_cond, DCAC_ErrCode_E e_code, uint32_t ul_cur_flag);



/***********************************************************************************************************************
 * 函数功能    : 逆变器主队列任务
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dcac_queue_task_main(Task_T *p_task)
{
    /* 非工作模式，检查逆变是否仍在开启中 */
    if (tDcac.eDisChgState >= IOS_STARTING &&
        (tSysInfo.uPerm.tPerm.bDisChgPerm == false || tDcac.uPerm.tPerm.bDisChgPerm == false))
    {
        cQueue_AddQueueTask(p_task, DTI_CTRL_DCAC_OUT, ST_OFF, false);
        if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
            log_w("bDcacTask:当前不许可放电,添加关闭逆变输出任务");
    }
    else if (tDcac.eParanInState >= IOS_STARTING && tDcac.uPerm.tPerm.bParaInPerm == false)
    {
        cQueue_AddQueueTask(p_task, DTI_CTRL_PARA_IN, ST_OFF, false);
        if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
            log_w("bDcacTask:当前不许可并网,添加关闭并网任务");
    }

    /* 队列中有外部优先任务，立即跳出 */
    if (lwrb_get_full(&p_task->tQueueBuff))
    {
        cQueue_GotoStep(p_task, STEP_END);
        return;
    }

    switch (p_task->ucStep)
    {
        case 0:
        {
            if (b_dcac_cs_get_param1() == true)
                cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            if (b_dcac_cs_get_param2() == true)
                cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        case 2:
        {
            if (b_dcac_cs_get_param3() == true)
                cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        case 3:
            v_proc_rec_param();
            v_check_dischg_or_chg_perm();
            v_check_close_dischg();
            v_set_total_chg_pwr();
            v_set_ac_chg_pwr();
            v_check_freq_auto_mem();

            cQueue_GotoStep(p_task, 0);
            #if (boardUSE_OS)
            ulTaskNotifyTake(pdTRUE, 1000 - (dcacTASK_GET_PARAM_CYCLE_TIME * 4));   /* 补足1s周期；新任务投递立即唤醒抢占 */
            #endif  /* boardUSE_OS */
            break;

        default:
        {
            cQueue_GotoStep(p_task, STEP_END);
        }
        break;
    }

    if (lwrb_get_full(&p_task->tQueueBuff))
    {
        cQueue_GotoStep(p_task, STEP_END);
        return;
    }

    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, dcacTASK_GET_PARAM_CYCLE_TIME);    /* 周期节拍；新任务投递立即唤醒抢占 */
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 获取参数后数据解析与故障检测
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static inline void v_proc_rec_param(void)
{
    tDcac.sMaxTemp = tDcacRx.sMaxTemp;

    if (tDcac.eChgState != IOS_WORK)
        tDcacRx.uErrCode.usCode[2] &= ~0x68;

    /* 统一同步硬件上报状态标志 */
    v_sync_err_state(tDcacRx.uErrCode.tCode.tIn.bOV1 || tDcacRx.uErrCode.tCode.tIn.bOV2,
                     DEC_DCAC_IN_VOLT, tDcac.uErrCode.tCode.bDcacInVolt);

    v_sync_err_state(tDcacRx.uErrCode.tCode.tAc.bOV || tDcacRx.uErrCode.tCode.tAc.bUV,
                     DEC_DCAC_OUT_VOLT, tDcac.uErrCode.tCode.bDcacOutVolt);

    v_sync_err_state(tDcacRx.uErrCode.tCode.tAc.bBootErr || tDcacRx.uErrCode.tCode.tAc.bSysErr || tDcacRx.uErrCode.tCode.tAc.bMsgErr,
                     DEC_DCAC_OUT_OTHER, tDcac.uErrCode.tCode.bDcacOutOther);

    v_sync_err_state(tDcacRx.uErrCode.usCode[0] != 0,
                     DEC_DCAC_HIGH_VOLT, tDcac.uErrCode.tCode.bDcacHighVolt);

    v_sync_err_state(tDcacRx.uErrCode.tCode.tAc.bBusOV != 0,
                     DEC_DCAC_BAT_OV, tDcac.uErrCode.tCode.bDcacBatOV);

    v_sync_err_state(tDcacRx.uErrCode.tCode.tDc.bOT || tDcacRx.uErrCode.tCode.tAc.bOT,
                     DEC_DCAC_OT, tDcac.uErrCode.tCode.bDcacOT);

    v_sync_err_state(tDcacRx.uErrCode.tCode.tDc.bOC || tDcacRx.uErrCode.tCode.tAc.bOC,
                     DEC_DCAC_OC, tDcac.uErrCode.tCode.bDcacOC);

    v_sync_err_state(tDcacRx.uErrCode.tCode.tAc.bOL != 0,
                     DEC_DCAC_OL, tDcac.uErrCode.tCode.bDcacOL);

    v_sync_err_state(tDcacRx.uErrCode.tCode.tAc.bSC != 0,
                     DEC_DCAC_SC, tDcac.uErrCode.tCode.bDcacSC);

    v_sync_err_state(tDcacRx.uErrCode.tCode.tDc.bNtcErr != 0,
                     DEC_DCAC_NTC, tDcac.uErrCode.tCode.bDcacNtc);

    /* ---------------------------------- 故障处理 ---------------------------------- */
    /* 1. 过温保护 */
    static uint16_t s_us_over_temp_cnt = 0;
    if (tDcac.sMaxTemp >= tAppMemParam.tDCAC.sMaxTemp)
    {
        if (tDcac.uErrCode.tCode.bSysOT == 0)
        {
            s_us_over_temp_cnt++;
            if (s_us_over_temp_cnt >= 2)
            {
                s_us_over_temp_cnt = 0;
                bDcac_SetErrCode(DEC_SYS_OT, true);
            }
        }
        else
            s_us_over_temp_cnt = 0;
    }
    else if (tDcac.sMaxTemp < (((int32_t)tAppMemParam.tDCAC.sMaxTemp * 93) / 100))
    {
        if (tDcac.uErrCode.tCode.bSysOT == 1)
        {
            s_us_over_temp_cnt++;
            if (s_us_over_temp_cnt >= 2)
            {
                s_us_over_temp_cnt = 0;
                bDcac_SetErrCode(DEC_SYS_OT, false);
            }
        }
        else
            s_us_over_temp_cnt = 0;
    }

    /* 2. 供电低压保护 */
    static uint16_t s_us_pwr_volt_low_cnt = 0;
    if (tAdcSamp.usSysInVolt < tAppMemParam.tDCAC.usMinOpenVolt)
    {
        if (tDcac.uErrCode.tCode.bSysLV == 0 && tDcac.eDisChgState >= IOS_STARTING)
        {
            s_us_pwr_volt_low_cnt++;
            if (s_us_pwr_volt_low_cnt >= 2)
            {
                s_us_pwr_volt_low_cnt = 0;
                bDcac_SetErrCode(DEC_SYS_UV, true);
            }
        }
        else
            s_us_pwr_volt_low_cnt = 0;
    }
    else if (tAdcSamp.usSysInVolt > (tAppMemParam.tDCAC.usMinOpenVolt + 20))
    {
        if (tDcac.uErrCode.tCode.bSysLV == 1)
        {
            s_us_pwr_volt_low_cnt++;
            if (s_us_pwr_volt_low_cnt >= 5)
            {
                s_us_pwr_volt_low_cnt = 0;
                bDcac_SetErrCode(DEC_SYS_UV, false);
            }
        }
        else
            s_us_pwr_volt_low_cnt = 0;
    }

    /* 3. 过流保护 */
    static uint16_t s_us_dyn_delay     = 0;
    static uint16_t s_us_over_curr_cnt = 0;
    uint16_t us_total_curr = tDcacRx.usInCurr;

    if (us_total_curr >= (((uint32_t)tAppMemParam.tDCAC.usMaxInCurr * 133) / 100))
    {
        if (s_us_dyn_delay != 4)
            s_us_over_curr_cnt = 0;
        s_us_dyn_delay = 4;
    }
    else
    {
        if (s_us_dyn_delay != 10)
            s_us_over_curr_cnt = 0;
        s_us_dyn_delay = 10;
    }

    if (tDcac.uErrCode.tCode.bSysInOC == 0)
    {
        if (us_total_curr >= (((uint32_t)tAppMemParam.tDCAC.usMaxInCurr * 105) / 100))
        {
            s_us_over_curr_cnt++;
            if (s_us_over_curr_cnt >= s_us_dyn_delay)
            {
                s_us_over_curr_cnt = 0;
                bDcac_SetErrCode(DEC_SYS_IN_OC, true);
            }
        }
    }
    else
        s_us_over_curr_cnt = 0;

    /* 4. 过载保护 */
    uint16_t us_overload_pwr  = ((uint32_t)tAppMemParam.tDCAC.usOutPwrRating * 6) / 5;
    uint16_t us_overload_pwr1 = ((uint32_t)tAppMemParam.tDCAC.usOutPwrRating * 7) / 5;
    static uint16_t s_us_overload_delay = 0;
    static uint16_t s_us_overload_cnt   = 0;

    if (tDcacRx.usOutPwr > tAppMemParam.tDCAC.usOverLoadPwr)
    {
        if (tDcacRx.usOutPwr >= us_overload_pwr)
        {
            if (s_us_overload_delay != 3)
                s_us_overload_cnt = 0;
            s_us_overload_delay = 3;
        }
        else
        {
            if (s_us_overload_delay != 50)
                s_us_overload_cnt = 0;
            s_us_overload_delay = 50;
        }

        s_us_overload_cnt++;
        if (s_us_overload_cnt >= s_us_overload_delay || tDcacRx.usOutPwr >= us_overload_pwr1)
        {
            s_us_overload_cnt = 0;
            bDcac_SetErrCode(DEC_SYS_OUT_OL, true);
        }
    }
    else
        s_us_overload_cnt = 0;

    /* 5. 输出状态检测 */
    static uint8_t s_uc_lost_err_cnt = 0;
    if ((tDcac.eDisChgState == IOS_WORK && (tDcacRx.usOutVolt < tAppMemParam.tDCAC.usMinInVolt)) ||
        (tDcac.eDisChgState == IOS_SHUT_DOWN && (tDcacRx.usOutVolt > tAppMemParam.tDCAC.usMinInVolt)))
    {
        if (tDcac.uErrCode.tCode.bSysOutErr == 0)
        {
            s_uc_lost_err_cnt++;
            if (s_uc_lost_err_cnt >= 10)
            {
                s_uc_lost_err_cnt = 0;
                bDcac_SetErrCode(DEC_SYS_OUT_ERR, true);
                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_e("bDcacTask:输出状态错误 输出电压%dV", tDcacRx.usOutVolt / 10);
            }
        }
        else
            s_uc_lost_err_cnt = 0;
    }
    else
    {
        if (tDcac.uErrCode.tCode.bSysOutErr == 1)
        {
            s_uc_lost_err_cnt++;
            if (s_uc_lost_err_cnt > 3)
            {
                s_uc_lost_err_cnt = 0;
                bDcac_SetErrCode(DEC_SYS_OUT_ERR, false);
                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_i("bDcacTask:清除输出状态错误 输出电压=%dV", tDcacRx.usOutVolt / 10);
            }
        }
        else
            s_uc_lost_err_cnt = 0;
    }

    /* 6. 输入状态监测 */
    static uint8_t s_uc_in_volt_state = 0;
    static uint8_t s_uc_volt_state_cnt = 0;
    if (tDcacRx.usInVolt < (tAppMemParam.tDCAC.usMinInVolt - 60))
    {
        if (tDcacRx.usInVolt < 100)
        {
            if (tDcac.uErrCode.tCode.bSysOV == 1)
                bDcac_SetErrCode(DEC_SYS_OV, false);
            if (tDcac.uErrCode.tCode.bSysSetInProte == 1)
                bDcac_InProteFuncSwitch(false);
            if (tDcac.uErrCode.tCode.bSysInOC == 1)
                bDcac_SetErrCode(DEC_SYS_IN_OC, false);
        }

        if (s_uc_in_volt_state != 0)
        {
            s_uc_volt_state_cnt++;
            if (s_uc_volt_state_cnt >= 2)
            {
                s_uc_volt_state_cnt = 0;
                s_uc_in_volt_state  = 0;
            }
        }
        else
            s_uc_volt_state_cnt = 0;
    }

    static uint16_t s_us_close_cnt = 0;
    if (tDcac.uErrCode.tCode.bSysOutErr == 1)
    {
        s_us_close_cnt++;
        if (s_us_close_cnt >= 5)
        {
            cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_OUT, ST_OFF, false);
            s_us_close_cnt = 0;
        }
    }
    else
        s_us_close_cnt = 0;
}

/***********************************************************************************************************************
 * 函数功能    : 检查充放电及并网许可
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static inline void v_check_dischg_or_chg_perm(void)
{
    /* 充电许可 */
    if (tDcacRx.uErrCode.usCode[0] != 0 ||
        tDcacRx.uErrCode.usCode[1] != 0 ||
        tDcacRx.uErrCode.usCode[2] != 0 ||
        tDcacRx.uErrCode.usCode[3] != 0 ||
        tDcac.uErrCode.tCode.bSysDevLost == 1 ||
        tDcac.uErrCode.tCode.bSysOT == 1 ||
        tDcac.uErrCode.tCode.bSysInOC == 1 ||
        tSysInfo.uPerm.tPerm.bChgPerm == false)
    {
        if (tDcac.uPerm.tPerm.bChgPerm == true)
            bDcac_SetPerm(DPO_CHG, false);
    }
    else
    {
        if (tDcac.uPerm.tPerm.bChgPerm == false)
            bDcac_SetPerm(DPO_CHG, true);
    }

    /* 放电许可 */
    if (tDcacRx.uErrCode.usCode[0] != 0 ||
        tDcacRx.uErrCode.usCode[1] != 0 ||
        tDcacRx.uErrCode.usCode[3] != 0 ||
        tDcac.uErrCode.tCode.bSysDevLost == 1 ||
        tDcac.uErrCode.tCode.bSysOT == 1 ||
        tDcac.uErrCode.tCode.bSysUT == 1 ||
        tDcac.uErrCode.tCode.bSysLV == 1 ||
        tDcac.uErrCode.tCode.bSysOutOL == 1 ||
        tDcac.uErrCode.tCode.bSysOutErr == 1 ||
        tDcac.uErrCode.tCode.bSysInOC == 1 ||
        tSysInfo.uPerm.tPerm.bDisChgPerm == false)
    {
        if (tDcac.uPerm.tPerm.bDisChgPerm == true)
            bDcac_SetPerm(DPO_DISCHG, false);
    }
    else
    {
        if (tDcac.uPerm.tPerm.bDisChgPerm == false)
            bDcac_SetPerm(DPO_DISCHG, true);
    }

    /* 并网许可 */
    if (tDcac.uPerm.tPerm.bDisChgPerm == false || ucBms_GetSoc() <= 10)
    {
        if (tDcac.uPerm.tPerm.bParaInPerm == true)
            bDcac_SetPerm(DPO_PARA_IN, false);
    }
    else
    {
        if (tDcac.uPerm.tPerm.bParaInPerm == false)
            bDcac_SetPerm(DPO_PARA_IN, true);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 检查关闭放电
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_check_close_dischg(void)
{
    if (tSysInfo.eDevState != DS_WORK)
        return;

    static uint16_t s_us_out_volt_wait_cnt = 0;
    if (tDcac.uPerm.tPerm.bDisChgPerm == false)
    {
        if (tDcacRx.usOutVolt > tAppMemParam.tDCAC.usMaxInVolt || tDcac.eDisChgState >= IOS_STARTING)
        {
            s_us_out_volt_wait_cnt++;
            if (s_us_out_volt_wait_cnt >= 5)
            {
                s_us_out_volt_wait_cnt = 0;
                if (uPrint.tFlag.bDcacTask)
                    log_w("bDcacTask:当前设备状态0x%x,不许可放电,强制关闭", tDcac.eDevState);
                cDCAC_Switch(DSO_AC_OUT, ST_OFF, true);
            }
        }
        else
            s_us_out_volt_wait_cnt = 0;
    }
    else
        s_us_out_volt_wait_cnt = 0;
}

/***********************************************************************************************************************
 * 函数功能    : 设置总充电功率
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_set_total_chg_pwr(void)
{
    uint16_t        us_total_chg_pwr = 0;
    static uint16_t s_us_last_total_chg_pwr = 0;
    static uint16_t s_us_total_chg_pwr_err_cnt = 0;

    if (tSysInfo.eDevState != DS_WORK)
        return;

    if (tDcac.uPerm.tPerm.bChgPerm == false)
        us_total_chg_pwr = 0;
    else if (bSys_LowVoltReqChg() == true)
        us_total_chg_pwr = 100;
    else
        us_total_chg_pwr = MIN2(tBmsRx.usPermMaxChgPwr, sysCHG_PWR_LEVEL3);

    if ((tSysInfo.tSetChgPwr.usDCAC + tSysInfo.tSetChgPwr.usMPPT) == 0)
        us_total_chg_pwr = 0;

    if (abs((int16_t)tDcacRx.usChgPwr - (int16_t)us_total_chg_pwr) > 100)
        s_us_total_chg_pwr_err_cnt++;
    else
        s_us_total_chg_pwr_err_cnt = 0;

    if (us_total_chg_pwr == s_us_last_total_chg_pwr &&
        (s_us_total_chg_pwr_err_cnt < (5000 / dcacTASK_GET_PARAM_CYCLE_TIME)))
    {
        return;
    }

    if (b_dcac_cs_set_total_chg_pwr(us_total_chg_pwr) == true)
    {
        s_us_last_total_chg_pwr     = us_total_chg_pwr;
        s_us_total_chg_pwr_err_cnt = 0;

        if (uPrint.tFlag.bDcacTask)
            sMyPrint("设置总的充电功率 %d \r\n", us_total_chg_pwr);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 设置AC充电功率
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
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
	
	if(tDcac.uPerm.tPerm.bChgPerm == false)
		tSysInfo.tSetChgPwr.usDCAC = 0;
	
	us_chg_pwr = tSysInfo.tSetChgPwr.usDCAC;

	//AC设置的功率和采样到的不一致
	if(abs(tDcacRx.usInPwr - us_chg_pwr) > 100)
		us_chg_pwr_err++;
	else 
		us_chg_pwr_err = 0;

	//功率没变化,退出
	if(us_last_chg_pwr == us_chg_pwr && 
		(us_chg_pwr_err < (5000 / dcacTASK_GET_PARAM_CYCLE_TIME)))
		return;
	
	//设置AC充电功率
	if(b_dcac_cs_set_chg_pwr(us_chg_pwr) == true)
	{
		us_last_chg_pwr = us_chg_pwr;
		us_chg_pwr_err = 0;
		sMyPrint("设置AC充电功率 %d\r\n",us_chg_pwr);
	}	
}

/***********************************************************************************************************************
 * 函数功能    : 频率自动记忆
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_check_freq_auto_mem(void)
{
    static uint16_t s_us_freq_diff_cnt = 0;
    uint16_t        us_expected_freq;
    uint16_t        us_new_freq;

    if (tDcac.eDisChgState != IOS_WORK)
    {
        s_us_freq_diff_cnt = 0;
        return;
    }

    us_expected_freq = (tAppMemParam.tDCAC.usAcOutFreq == 1) ? 600 : 500;

    if (abs((int16_t)tDcacRx.usOutFreq - (int16_t)us_expected_freq) > 50)
    {
        us_new_freq = (tDcacRx.usOutFreq > 550) ? 1 : 0;
        if (us_new_freq != tAppMemParam.tDCAC.usAcOutFreq)
        {
            s_us_freq_diff_cnt++;
            if (s_us_freq_diff_cnt >= 5)
            {
                s_us_freq_diff_cnt = 0;
                tAppMemParam.tDCAC.usAcOutFreq = us_new_freq;
                cApp_UpdateMemParam(tDcacMemParamStr);

                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_i("bDcacTask:频率自动记忆,更新为%sHZ", tAppMemParam.tDCAC.usAcOutFreq ? "60" : "50");
            }
        }
        else
            s_us_freq_diff_cnt = 0;
    }
    else
        s_us_freq_diff_cnt = 0;
}

/***********************************************************************************************************************
 * 函数功能    : 辅助同步错误状态标志
 * 说明(备注)  : 无
 * 传入参数    : b_cond: 触发条件, e_code: 错误码, ul_cur_flag: 当前错误标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static inline void v_sync_err_state(bool b_cond, DCAC_ErrCode_E e_code, uint32_t ul_cur_flag)
{
    if (b_cond)
    {
        if (!ul_cur_flag)
            bDcac_SetErrCode(e_code, true);
    }
    else
    {
        if (ul_cur_flag)
            bDcac_SetErrCode(e_code, false);
    }
}

#endif  /* boardDCAC_EN */

