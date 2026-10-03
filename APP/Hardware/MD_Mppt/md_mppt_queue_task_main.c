/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_queue_task_main.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 主轮询队列任务 (参数查询、故障判定与功率下发)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Mppt/md_mppt_queue_task.h"

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

//****************************************************Macros********************************************************************//
#define			mpptTASK_GET_PARAM_CYCLE_TIME			1000

//****************************************************Function Declaration******************************************************//
static void v_check_chg_perm(void);
static void v_proc_rec_param(void);
static void v_set_total_chg_pwr(void);

/***********************************************************************************************************************
 * 函数功能    : MPPT 主轮询队列任务函数
 * 说明(备注)  : 周期查询 MPPT 参数并下发功率
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_mppt_queue_task_main(Task_T *p_task)
{
    s8 result = 0;

    /* 非允许充电状态下，若有输入功率则强制关断充电 */
    if ((tSysInfo.uPerm.tPerm.bChgPerm == false || tMppt.bChgPerm == false) && tMpptRx.usInPwr != 0)
        cMppt_SetChgPwr(0);

    /* 队列中有高优先级任务则退出主轮询 */
    if (lwrb_get_full(&p_task->tQueueBuff))
    {
        cQueue_GotoStep(p_task, STEP_END);
        return;
    }

    switch (p_task->ucStep)
    {
        case 0:
        {
            result = c_mppt_cs_get_param();
            if (result > 0)
                cQueue_GotoStep(p_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            v_proc_rec_param();
            v_check_chg_perm();
            v_set_total_chg_pwr();
            cQueue_GotoStep(p_task, 0);
        }
        break;

        default:
        {
            cQueue_GotoStep(p_task, STEP_END);
        }
        break;
    }

    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, mpptTASK_GET_PARAM_CYCLE_TIME);
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 解析物理量与错误检测
 * 说明(备注)  : 更新输入功率，执行过压与过流检测
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_proc_rec_param(void)
{
    tMppt.usInPwr = tMpptRx.usInPwr / 10;

    /* 输入过压检查 */
    if (tMpptRx.uErrCode.tCode.bInOV)
    {
        if (tMppt.uErrCode.tCode.bMpptInOV == false)
            bMppt_SetErrCode(MEC_MPPT_IN_OV, true);
    }
    else if (tMppt.uErrCode.tCode.bMpptInOV == true)
        bMppt_SetErrCode(MEC_MPPT_IN_OV, false);

	// //输入欠压
	// if(tMpptRx.uErrCode.tCode.bInUV)
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptInUV == false)
	// 		bMppt_SetErrCode(MEC_MPPT_IN_UV, true);
	// }
	// else 
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptInUV == true)
	// 		bMppt_SetErrCode(MEC_MPPT_IN_UV, false);
	// }

    /* 输入过流检查 */
    if (tMpptRx.uErrCode.tCode.bInOC)
    {
        if (tMppt.uErrCode.tCode.bMpptInOC == false)
            bMppt_SetErrCode(MEC_MPPT_IN_OC, true);
    }
    else
    {
        if (tMppt.uErrCode.tCode.bMpptInOC == true)
            bMppt_SetErrCode(MEC_MPPT_IN_OC, false);
    }

	//输入短路
	if(tMpptRx.uErrCode.tCode.bInSC)
	{
		if(tMppt.uErrCode.tCode.bMpptInSC == false)
			bMppt_SetErrCode(MEC_MPPT_IN_SC, true);
	}
	else 
	{
		if(tMppt.uErrCode.tCode.bMpptInSC == true)
			bMppt_SetErrCode(MEC_MPPT_IN_SC, false);
	}

	//输出过压
	if(tMpptRx.uErrCode.tCode.bOutOV)
	{
		if(tMppt.uErrCode.tCode.bMpptOutOV == false)
			bMppt_SetErrCode(MEC_MPPT_OUT_OV, true);
	}
	else 
	{
		if(tMppt.uErrCode.tCode.bMpptOutOV == true)
			bMppt_SetErrCode(MEC_MPPT_OUT_OV, false);
	}

	//输出欠压
	if(tMpptRx.uErrCode.tCode.bOutUV)
	{
		if(tMppt.uErrCode.tCode.bMpptOutUV == false)
			bMppt_SetErrCode(MEC_MPPT_OUT_UV, true);
	}
	else 
	{
		if(tMppt.uErrCode.tCode.bMpptOutUV == true)
			bMppt_SetErrCode(MEC_MPPT_OUT_UV, false);
	}

	//输出过流
	// if(tMpptRx.uErrCode.tCode.bOutOC)
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptOutOC == false)
	// 		bMppt_SetErrCode(MEC_MPPT_OUT_OC, true);
	// }
	// else 
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptOutOC == true)
	// 		bMppt_SetErrCode(MEC_MPPT_OUT_OC, false);
	// }

	// //输出短路
	// if(tMpptRx.uErrCode.tCode.bOutSC)
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptOutSC == false)
	// 		bMppt_SetErrCode(MEC_MPPT_OUT_SC, true);
	// }
	// else 
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptInSC == true)
	// 		bMppt_SetErrCode(MEC_MPPT_OUT_SC, false);
	// }

	//过温
	// if(tMpptRx.uErrCode.tCode.bOT)
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptOT == false)
	// 		bMppt_SetErrCode(MEC_MPPT_OT, true);
	// }
	// else 
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptOT == true)
	// 		bMppt_SetErrCode(MEC_MPPT_OT, false);
	// }

	//过载
	// if(tMpptRx.uErrCode.tCode.bOL)
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptOL == false)
	// 		bMppt_SetErrCode(MEC_MPPT_OL, true);
	// }
	// else 
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptOL == true)
	// 		bMppt_SetErrCode(MEC_MPPT_OL, false);
	// }

	//输入欠功率
	if(tMpptRx.uErrCode.tCode.bInUP)
	{
		if(tMppt.uErrCode.tCode.bMpptInUP == false)
			bMppt_SetErrCode(MEC_MPPT_IN_UP, true);
	}
	else 
	{
		if(tMppt.uErrCode.tCode.bMpptInUP == true)
			bMppt_SetErrCode(MEC_MPPT_IN_UP, false);
	}
}

/***********************************************************************************************************************
 * 函数功能    : 检查充电许可
 * 说明(备注)  : 综合判断各故障标志位与系统充电许可位
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_check_chg_perm(void)
{
    if (tMppt.uErrCode.tCode.bMpptInOV  == 1 ||
        tMppt.uErrCode.tCode.bMpptInSC  == 1 ||
        tMppt.uErrCode.tCode.bMpptInOC  == 1 ||
        tMppt.uErrCode.tCode.bMpptOutOV == 1 ||
        tMppt.uErrCode.tCode.bMpptOutOC == 1 ||
        tMppt.uErrCode.tCode.bMpptOutSC == 1 ||
        tMppt.uErrCode.tCode.bMpptOT    == 1 ||
        tMppt.uErrCode.tCode.bMpptOL    == 1 ||
        tMppt.uErrCode.tCode.bSysOL     == 1 ||
        tMppt.uErrCode.tCode.bMpptInUP  == 1 ||
        tSysInfo.uPerm.tPerm.bChgPerm == false)
    {
        if (tMppt.bChgPerm == true)
            bMppt_SetChgPerm(false);
    }
    else if (tMppt.bChgPerm == false)
        bMppt_SetChgPerm(true);
}

/***********************************************************************************************************************
 * 函数功能    : 充电功率控制
 * 说明(备注)  : 当目标充电功率发生变更或出现异常偏差时，向 MPPT 下发功率设置指令
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void v_set_total_chg_pwr(void)
{
    vu16 us_chg_pwr = 0;
    static vu16 us_last_chg_pwr = 100;
    static vu16 us_chg_pwr_err  = 0;

    if (tSysInfo.eDevState != DS_WORK)
        return;

    if (tMppt.bChgPerm == false)
        tSysInfo.tSetChgPwr.usMPPT = 0;

    us_chg_pwr = tSysInfo.tSetChgPwr.usMPPT;

    if (abs((int)((tMpptRx.usMaxInPwr / 10) - us_chg_pwr)) > 100 ||
        (abs((int)((tMpptRx.usInPwr / 10) - us_chg_pwr)) > 100 && tMpptRx.usInPwr < 100))
        us_chg_pwr_err++;
    else
        us_chg_pwr_err = 0;

    if (us_last_chg_pwr == us_chg_pwr && us_chg_pwr_err < (5000 / mpptTASK_GET_PARAM_CYCLE_TIME))
        return;

    if (c_mppt_cs_set_pwr(us_chg_pwr) > 0)
    {
        us_last_chg_pwr = us_chg_pwr;
        us_chg_pwr_err  = 0;

        if (uPrint.tFlag.bMpptTask)
            sMyPrint("设置MPPT充电功率 %d \r\n", us_chg_pwr);
    }
}

#endif  /* boardMPPT_EN */

