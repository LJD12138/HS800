/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_err.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : LCD错误代码轮播显示(X-macro单一数据源表驱动)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"

#if(boardDISPLAY_EN)
#include "Sys/sys_task.h"

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //boardUSB_EN

#if(boardDC_EN)
#include "Dc/dc_task.h"
#endif  //boardDC_EN

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#endif  //boardMPPT_EN

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  //boardDCAC_EN

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
typedef bool (*fnErrActive_T)(void);

typedef struct
{
	uint16_t			usCode;				/* 故障显示码(1~10, 100~136, 200~216, 300~326, 400~406, 500~508) */
	fnErrActive_T		pfActive;			/* 故障激活检测函数指针 */
}ErrItem_T;

//****************************************************Macros********************************************************************//
/*--------------------------------------------------
 * 故障清单(单一数据源): 每个故障仅此一行, 下方自动展开生成
 * "检测函数" 与 "轮播表", 新增故障只需在此追加一行即可
 * 行格式: X(显示码, 检测函数名, 位域表达式)
 *-------------------------------------------------*/
#define			ERR_LIST_SYS(X)							\
	X(1,   sys_ot,                tSysInfo.uErrCode.tCode.bOT) \
	X(2,   sys_ut,                tSysInfo.uErrCode.tCode.bUT) \
	X(3,   sys_ov,                tSysInfo.uErrCode.tCode.bOV) \
	X(4,   sys_uv,                tSysInfo.uErrCode.tCode.bUV) \
	X(5,   sys_ol,                tSysInfo.uErrCode.tCode.bOL) \
	X(7,   sys_0_soc,             tSysInfo.uErrCode.tCode.b0SOC) \
	X(8,   sys_close_fault,       tSysInfo.uErrCode.tCode.bCloseFault) \
	X(9,   sys_boot_fault,        tSysInfo.uErrCode.tCode.bBootFault) \
	X(10,  sys_task_fault,        tSysInfo.uErrCode.tCode.bTaskFault)

/* 6 虽属SYS(bDisChgOL), 仅随BMS使能参与轮播 */
#if(boardBMS_EN)
#define			ERR_LIST_BMS(X)							\
	X(6,   sys_dischg_ol,         tSysInfo.uErrCode.tCode.bDisChgOL) \
	X(100, bms_cell_ov,           tBms.uErrCode.tCode.uBmsCode.tCode.bCellOV) \
	X(101, bms_cell_uv,           tBms.uErrCode.tCode.uBmsCode.tCode.bCellUV) \
	X(102, bms_env_ot,            tBms.uErrCode.tCode.uBmsCode.tCode.bEnvOT) \
	X(103, bms_env_ut,            tBms.uErrCode.tCode.uBmsCode.tCode.bEnvUT) \
	X(104, bms_cot,               tBms.uErrCode.tCode.uBmsCode.tCode.bCOT) \
	X(105, bms_cut,               tBms.uErrCode.tCode.uBmsCode.tCode.bCUT) \
	X(106, bms_dcot,              tBms.uErrCode.tCode.uBmsCode.tCode.bDCOT) \
	X(107, bms_dcut,              tBms.uErrCode.tCode.uBmsCode.tCode.bDCUT) \
	X(108, bms_coc,               tBms.uErrCode.tCode.uBmsCode.tCode.bCOC) \
	X(109, bms_dcoc,              tBms.uErrCode.tCode.uBmsCode.tCode.bDCOC) \
	X(110, bms_sc,                tBms.uErrCode.tCode.uBmsCode.tCode.bSC) \
	X(111, bms_bat_full,          tBms.uErrCode.tCode.uBmsCode.tCode.bBatFull) \
	X(112, bms_afe_lost,          tBms.uErrCode.tCode.uBmsCode.tCode.bAfeLost) \
	X(113, bms_curr_err,          tBms.uErrCode.tCode.uBmsCode.tCode.bCurrErr) \
	X(114, bms_prechg_fault,      tBms.uErrCode.tCode.uBmsCode.tCode.bPrechgFault) \
	X(115, bms_low_volt_ol,       tBms.uErrCode.tCode.uBmsCode.tCode.bLowVoltOL) \
	X(116, bms_para_lost,         tBms.uErrCode.tCode.uBmsCode.tCode.bParaLost) \
	X(117, bms_console_lost,      tBms.uErrCode.tCode.uBmsCode.tCode.bConsoleLost) \
	X(118, bms_dischg_mos_err,    tBms.uErrCode.tCode.uBmsCode.tCode.bDisChgMosErr) \
	X(119, bms_chg_mos_err,       tBms.uErrCode.tCode.uBmsCode.tCode.bChgMosErr) \
	X(120, bms_mos_err1,          tBms.uErrCode.tCode.uBmsCode.tCode.bMosErr1) \
	X(121, bms_mos_err2,          tBms.uErrCode.tCode.uBmsCode.tCode.bMosErr2) \
	X(122, bms_mos_err3,          tBms.uErrCode.tCode.uBmsCode.tCode.bMosErr3) \
	X(123, bms_afe_err,           tBms.uErrCode.tCode.uBmsCode.tCode.bAfeErr) \
	X(124, bms_volt_low,          tBms.uErrCode.tCode.uBmsCode.tCode.bVoltLow) \
	X(125, bms_ntc_lost,          tBms.uErrCode.tCode.uBmsCode.tCode.bNtcLost) \
	X(126, bms_close_fault,       tBms.uErrCode.tCode.uBmsCode.tCode.bCloseFault) \
	X(127, bms_boot_fault,        tBms.uErrCode.tCode.uBmsCode.tCode.bBootFault) \
	X(128, bms_err,               tBms.uErrCode.tCode.uBmsCode.tCode.bBmsErr) \
	X(129, bms_unbalanced,        tBms.uErrCode.tCode.uBmsCode.tCode.bUnbalanced) \
	X(130, bms_balance_wire_lost, tBms.uErrCode.tCode.uBmsCode.tCode.bBalanceWireLost) \
	X(131, bms_sys_dev_lost,      tBms.uErrCode.tCode.bSysDevLost) \
	X(132, bms_sys_chg_ot,        tBms.uErrCode.tCode.bSysChgOT) \
	X(133, bms_sys_dischg_ot,     tBms.uErrCode.tCode.bSysDisChgOT) \
	X(134, bms_sys_chg_ut,        tBms.uErrCode.tCode.bSysChgUT) \
	X(135, bms_sys_dischg_ut,     tBms.uErrCode.tCode.bSysDisChgUT) \
	X(136, bms_sys_lv,            tBms.uErrCode.tCode.bSysLV)
#endif  //boardBMS_EN

#if(boardMPPT_EN)
#define			ERR_LIST_MPPT(X)						\
	X(200, mppt_in_ov,    tMppt.uErrCode.tCode.bMpptInOV) \
	X(201, mppt_in_uv,    tMppt.uErrCode.tCode.bMpptInUV) \
	X(202, mppt_in_oc,    tMppt.uErrCode.tCode.bMpptInOC) \
	X(203, mppt_in_sc,    tMppt.uErrCode.tCode.bMpptInSC) \
	X(204, mppt_out_ov,   tMppt.uErrCode.tCode.bMpptOutOV) \
	X(205, mppt_out_uv,   tMppt.uErrCode.tCode.bMpptOutUV) \
	X(206, mppt_out_oc,   tMppt.uErrCode.tCode.bMpptOutOC) \
	X(207, mppt_out_sc,   tMppt.uErrCode.tCode.bMpptOutSC) \
	X(208, mppt_ol,       tMppt.uErrCode.tCode.bMpptOL) \
	X(209, mppt_ot,       tMppt.uErrCode.tCode.bMpptOT) \
	X(210, mppt_en_fault, tMppt.uErrCode.tCode.bMpptEnFault) \
	X(211, mppt_in_up,    tMppt.uErrCode.tCode.bMpptInUP) \
	X(212, mppt_dev_lost, tMppt.uErrCode.tCode.bDevLost) \
	X(213, mppt_sys_ot,   tMppt.uErrCode.tCode.bSysOT) \
	X(214, mppt_sys_ut,   tMppt.uErrCode.tCode.bSysUT) \
	X(215, mppt_sys_ov,   tMppt.uErrCode.tCode.bSysOV) \
	X(216, mppt_sys_ol,   tMppt.uErrCode.tCode.bSysOL)
#endif  //boardMPPT_EN

#if(boardDCAC_EN)
#define			ERR_LIST_DCAC(X)						\
	X(300, dcac_in_volt,         tDcac.uErrCode.tCode.bDcacInVolt) \
	X(301, dcac_in_freq,         tDcac.uErrCode.tCode.bDcacInFreq) \
	X(302, dcac_in_other,        tDcac.uErrCode.tCode.bDcacInOther) \
	X(303, dcac_out_volt,        tDcac.uErrCode.tCode.bDcacOutVolt) \
	X(304, dcac_out_other,       tDcac.uErrCode.tCode.bDcacOutOther) \
	X(305, dcac_high_volt,       tDcac.uErrCode.tCode.bDcacHighVolt) \
	X(306, dcac_bat_ov,          tDcac.uErrCode.tCode.bDcacBatOV) \
	X(307, dcac_bat_uv,          tDcac.uErrCode.tCode.bDcacBatUV) \
	X(308, dcac_ot,              tDcac.uErrCode.tCode.bDcacOT) \
	X(309, dcac_ol,              tDcac.uErrCode.tCode.bDcacOL) \
	X(310, dcac_oc,              tDcac.uErrCode.tCode.bDcacOC) \
	X(311, dcac_sc,              tDcac.uErrCode.tCode.bDcacSC) \
	X(312, dcac_fuse,            tDcac.uErrCode.tCode.bDcacFuse) \
	X(313, dcac_relay,           tDcac.uErrCode.tCode.bDcacRelay) \
	X(314, dcac_para,            tDcac.uErrCode.tCode.bDcacPara) \
	X(315, dcac_ntc,             tDcac.uErrCode.tCode.bDcacNtc) \
	X(316, dcac_other,           tDcac.uErrCode.tCode.bDcacOther) \
	X(317, dcac_eeprom,          tDcac.uErrCode.tCode.bDcacEeprom) \
	X(318, dcac_sys_dev_lost,    tDcac.uErrCode.tCode.bSysDevLost) \
	X(319, dcac_sys_ot,          tDcac.uErrCode.tCode.bSysOT) \
	X(320, dcac_sys_ut,          tDcac.uErrCode.tCode.bSysUT) \
	X(321, dcac_sys_ov,          tDcac.uErrCode.tCode.bSysOV) \
	X(322, dcac_sys_lv,          tDcac.uErrCode.tCode.bSysLV) \
	X(323, dcac_sys_set_in_prote, tDcac.uErrCode.tCode.bSysSetInProte) \
	X(324, dcac_sys_out_ol,      tDcac.uErrCode.tCode.bSysOutOL) \
	X(325, dcac_sys_out_err,     tDcac.uErrCode.tCode.bSysOutErr) \
	X(326, dcac_sys_in_oc,       tDcac.uErrCode.tCode.bSysInOC)
#endif  //boardDCAC_EN

#if(boardDC_EN)
#define			ERR_LIST_DC(X)							\
	X(400, dc_power_err, tDc.uErrCode.tCode.bPowerErr) \
	X(401, dc_ot,        tDc.uErrCode.tCode.bOT) \
	X(402, dc_ol,        tDc.uErrCode.tCode.bOL) \
	X(403, dc_close_fail, tDc.uErrCode.tCode.bCloseFail) \
	X(404, dc_out_low,   tDc.uErrCode.tCode.bOutLow) \
	X(405, dc_out_high,  tDc.uErrCode.tCode.bOutHigh) \
	X(406, dc_ntc_lost,  tDc.uErrCode.tCode.bNtcLost)
#endif  //boardDC_EN

#if(boardUSB_EN)
#define			ERR_LIST_USB(X)							\
	X(500, usb_power_err,   tUsb.uErrCode.tCode.bPowerErr) \
	X(501, usb_ot,          tUsb.uErrCode.tCode.bOT) \
	X(502, usb_ol,          tUsb.uErrCode.tCode.bOL) \
	X(503, usb_bat_uv,      tUsb.uErrCode.tCode.bBatUV) \
	X(504, usb_ic1_lost,    tUsb.uErrCode.tCode.bIc1Lost) \
	X(505, usb_ic2_lost,    tUsb.uErrCode.tCode.bIc2Lost) \
	X(506, usb_boot_fault,  tUsb.uErrCode.tCode.bBootFault) \
	X(507, usb_close_fault, tUsb.uErrCode.tCode.bCloseFault) \
	X(508, usb_qc_power_err, tUsb.uErrCode.tCode.bQcPowerErr)
#endif  //boardUSB_EN

/*--------------------------------------------------
 * 展开一: 由故障清单生成全部检测函数
 *-------------------------------------------------*/
#define			ERR_DEF_FN(code, name, expr)			\
	static bool b_##name(void) { return (expr); }

ERR_LIST_SYS(ERR_DEF_FN)
#if(boardBMS_EN)
ERR_LIST_BMS(ERR_DEF_FN)
#endif  //boardBMS_EN
#if(boardMPPT_EN)
ERR_LIST_MPPT(ERR_DEF_FN)
#endif  //boardMPPT_EN
#if(boardDCAC_EN)
ERR_LIST_DCAC(ERR_DEF_FN)
#endif  //boardDCAC_EN
#if(boardDC_EN)
ERR_LIST_DC(ERR_DEF_FN)
#endif  //boardDC_EN
#if(boardUSB_EN)
ERR_LIST_USB(ERR_DEF_FN)
#endif  //boardUSB_EN
#undef ERR_DEF_FN

//****************************************************Parameter Initialization**************************************************//
/* 故障轮播映射表: 顺序即轮播优先级(SYS -> BMS -> MPPT -> DCAC -> DC -> USB) */
static const ErrItem_T S_tErrTable[] =
{
	/*--------------------------------------------------
	 * 展开二: 由故障清单生成全部表项
	 *-------------------------------------------------*/
#define			ERR_ADD_ITEM(code, name, expr)			{code, b_##name},

	ERR_LIST_SYS(ERR_ADD_ITEM)
	#if (boardBMS_EN)
	ERR_LIST_BMS(ERR_ADD_ITEM)
	#endif  /* boardBMS_EN */
	#if (boardMPPT_EN)
	ERR_LIST_MPPT(ERR_ADD_ITEM)
	#endif  /* boardMPPT_EN */
	#if (boardDCAC_EN)
	ERR_LIST_DCAC(ERR_ADD_ITEM)
	#endif  /* boardDCAC_EN */
	#if (boardDC_EN)
	ERR_LIST_DC(ERR_ADD_ITEM)
	#endif  /* boardDC_EN */
	#if (boardUSB_EN)
	ERR_LIST_USB(ERR_ADD_ITEM)
	#endif  /* boardUSB_EN */
#undef ERR_ADD_ITEM
};

#define			ERR_TABLE_NUM							(sizeof(S_tErrTable) / sizeof(S_tErrTable[0]))


#if (boardBMS_EN)
/***********************************************************************************************************************
 * 函数功能    : 安全读取 64 位 BMS 故障码 (防 32 位核双指令撕裂)
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : uint64_t: 当前 64 位 BMS 故障码（临界区内读取，防 32 位核双指令撕裂）
 ************************************************************************************************************************/
static uint64_t ull_bms_get_err_code(void)
{
	uint64_t ull_code;
	mainENTER_CRITICAL();
	ull_code = tBms.uErrCode.ullCode;
	mainEXIT_CRITICAL();
	return ull_code;
}
#endif  /* boardBMS_EN */

/***********************************************************************************************************************
 * 函数功能    : LCD错误代码显示
 * 说明(备注)  : 表驱动轮播架构，扫描并轮显当前所有激活的故障代码
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 错误代码, 0表示无错误或处于切换空白间隔期
 *               错误码分类:
 *               Sys:1-99      Bms:100-199   Mppt:200-299
 *               Dcac:300-399  Dc:400-499    Usb:500-599
 ************************************************************************************************************************/
uint16_t usDisp_ErrCodeDisplay(void)
{
	static __IO uint16_t s_us_idx       = 0;      /* 当前表内下标 */
	static __IO uint16_t s_us_disp_ms   = 0;      /* 当前故障已显示时长累计 ms */
	static __IO uint16_t s_us_blank     = 0;      /* 1: 当前帧为空白间隔 */
	static __IO uint16_t s_us_curr_code = 0;      /* 当前展示中的故障码 */

	uint16_t us_scan;
	uint8_t uc_active_cnt = 0;

	/* 无任何故障: 快速退出并重置内部状态 */
	if (tSysInfo.uErrCode.usCode == 0
		#if (boardUSB_EN)
		&& tUsb.uErrCode.ucErrCode == 0
		#endif  /* boardUSB_EN */

		#if (boardDC_EN)
		&& tDc.uErrCode.ucErrCode == 0
		#endif  /* boardDC_EN */

		#if (boardBMS_EN)
		&& ull_bms_get_err_code() == 0
		#endif  /* boardBMS_EN */

		#if (boardMPPT_EN)
		&& tMppt.uErrCode.ulCode == 0
		#endif  /* boardMPPT_EN */

		#if (boardDCAC_EN)
		&& tDcac.uErrCode.ulCode == 0
		#endif  /* boardDCAC_EN */
		)
	{
		s_us_idx       = 0;
		s_us_disp_ms   = 0;
		s_us_blank     = 0;
		s_us_curr_code = 0;
		return 0;
	}

	/* 统计当前激活故障总数 */
	for (us_scan = 0; us_scan < ERR_TABLE_NUM; us_scan++)
	{
		if (S_tErrTable[us_scan].pfActive != NULL && S_tErrTable[us_scan].pfActive())
		{
			uc_active_cnt++;
			if (uc_active_cnt > 1)
				break;
		}
	}

	/* 没有任何激活故障 */
	if (uc_active_cnt == 0)
	{
		s_us_idx       = 0;
		s_us_disp_ms   = 0;
		s_us_blank     = 0;
		s_us_curr_code = 0;
		return 0;
	}

	/* 单故障场景: 持续常显, 绝不插入灭屏空白帧, 避免单故障呼吸闪烁 */
	if (uc_active_cnt <= 1)
		s_us_blank = 0;
	else if (s_us_blank)
	{
		/* 多故障轮播切换时的单帧空白间隔 */
		s_us_blank = 0;
		return 0;
	}

	/* 自 s_us_idx 起环形扫描下一个激活的故障 */
	for (us_scan = 0; us_scan < ERR_TABLE_NUM; us_scan++)
	{
		if (S_tErrTable[s_us_idx].pfActive != NULL && S_tErrTable[s_us_idx].pfActive())
			break;
		s_us_idx = (s_us_idx + 1) % ERR_TABLE_NUM;
	}

	if (us_scan >= ERR_TABLE_NUM)
		return 0;

	uint16_t us_active_code = S_tErrTable[s_us_idx].usCode;

	/* 故障码发生变更(如某故障中途解除或轮播至新故障), 重置计时 */
	if (s_us_curr_code != us_active_code)
	{
		s_us_curr_code = us_active_code;
		s_us_disp_ms   = 0;
	}

	/* 显示当前激活故障: 持续 2000ms, 按当前页面帧周期累计 (帧周期随页面可变) */
	s_us_disp_ms += usDisp_GetFramePeriod();
	if (s_us_disp_ms >= 2000)
	{
		s_us_disp_ms = 0;
		if (uc_active_cnt > 1)
		{
			s_us_idx   = (s_us_idx + 1) % ERR_TABLE_NUM;   /* 指向下一个, 准备后续扫描 */
			s_us_blank = 1;                                /* 多故障时插入一帧空白间隔 */
			return 0;
		}
	}

	return us_active_code;
}

#endif  /* boardDISPLAY_EN */
