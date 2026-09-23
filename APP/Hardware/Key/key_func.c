/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Key
 * File    : key_func.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : 按键功能业务动作分发处理实现文件(表驱动优化方案)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Key/key_func.h"

#if (boardKEY_EN)
#include "Key/key_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "function.h"

#if (boardUSB_EN)
#include "Usb/usb_task.h"
#endif  /* boardUSB_EN */

#if (boardDC_EN)
#include "Dc/dc_task.h"
#endif  /* boardDC_EN */

#if (boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif  /* boardLIGHT_EN */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  /* boardDCAC_EN */

#if (boardENG_MODE_EN)
#include "key_func_eng.h"
#endif  /* boardENG_MODE_EN */

#if (1)
//****************************************************Macros********************************************************************//
typedef void (*fnKeyAction_T)(void);

/* 适用系统状态掩码 */
#define			KEY_MASK_ANY							0xFFFFFFFF
#define			KEY_MASK_WORK							(1U << DS_WORK)
#define			KEY_MASK_ERR							(1U << DS_ERR)
#define			KEY_MASK_NORMAL							(KEY_MASK_WORK | KEY_MASK_ERR)

//****************************************************Parameter Initialization**************************************************//
/* 动作映射表项 */
typedef struct
{
	const uint8_t		*pSeq;				/* 触发序列特征数组指针 */
	uint8_t				ucSeqLen;			/* 序列长度 (字节数) */
	uint32_t			ulStateMask;		/* 适用的工作状态掩码 */
	fnKeyAction_T		pfAction;			/* 业务处理函数指针 */
	const char			*pLog;				/* 调试打印日志 */
}KeyActionItem_T;

/* 编译期保证 (1U << eDevState) 移位安全 */
typedef char __ds_shift_safe[(DS_WORK < 32 && DS_ERR < 32) ? 1 : -1];

/* 长按 开关机 */
static const u8 s_uc_key_tri_sys_on_off_buff[2] = { KTE_POWER_LONG, KTE_FUN_NULL };
/* 连击 开关机/系统充放保护 */
static const u8 s_uc_key_tri_sys_prote_on_off_buff[4] = { KTE_POWER_LONG, KTE_POWER_LONG, KTE_POWER_LONG, KTE_FUN_NULL };

#if (boardDCAC_EN)
/* 单击 开关AC */
static const u8 s_uc_key_tri_ac_on_off_buff[2] = { KTE_AC_SHORT, KTE_FUN_NULL };
/* 长按 开关AC(常开) */
static const u8 s_uc_key_tri_ac_on_off_buff1[2] = { KTE_AC_LONG, KTE_FUN_NULL };
/* 连击 开启AC保护(10连击占满缓冲) */
static const u8 s_uc_key_tri_ac_prote_on_off_buff[10] = { KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT,
                                                          KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT };
#endif  /* boardDCAC_EN */

#if (boardLIGHT_EN)
/* 单击 切换照明模式 */
static const u8 s_uc_key_tri_light_charge_buff[2] = { KTE_LIGHT_SHORT, KTE_FUN_NULL };
/* 长按 开关照明 */
static const u8 s_uc_key_tri_light_on_off_buff[2] = { KTE_LIGHT_LONG, KTE_FUN_NULL };
#endif  /* boardLIGHT_EN */

#if (boardUSB_EN)
/* 单击 开关USB */
static const u8 s_uc_key_tri_usb_on_off_buff[2] = { KTE_USB_SHORT, KTE_FUN_NULL };
/* 长按 开关USB */
static const u8 s_uc_key_tri_usb_on_off_buff1[2] = { KTE_USB_LONG, KTE_FUN_NULL };
#endif  /* boardUSB_EN */

#if (boardDC_EN)
/* 单击 开关DC */
static const u8 s_uc_key_tri_dc_on_off_buff[2] = { KTE_DC_SHORT, KTE_FUN_NULL };
/* 长按 开关DC */
static const u8 s_uc_key_tri_dc_on_off_buff1[2] = { KTE_DC_LONG, KTE_FUN_NULL };
#endif  /* boardDC_EN */

#if (boardDISPLAY_EN)
/* 单击 开关背光 */
static const u8 s_uc_key_tri_bl_on_off_buff[2] = { KTE_POWER_SHORT, KTE_FUN_NULL };
/* 组合 强制开关背光 */
static const u8 s_uc_key_tri_force_open_bl_buff1[3] = { KTE_DC_LONG, KTE_USB_LONG, KTE_FUN_NULL };
static const u8 s_uc_key_tri_force_open_bl_buff2[3] = { KTE_USB_LONG, KTE_DC_LONG, KTE_FUN_NULL };
#endif  /* boardDISPLAY_EN */

//****************************************************Function Declaration******************************************************//
static void v_act_sys_on_off(void);
static void v_act_sys_prote(void);

#if (boardDCAC_EN)
static void v_act_dcac_short(void);
static void v_act_dcac_long(void);
static void v_act_dcac_prote(void);
#endif  /* boardDCAC_EN */

#if (boardLIGHT_EN)
static void v_act_light_mode(void);
static void v_act_light_switch(void);
#endif  /* boardLIGHT_EN */

#if (boardUSB_EN)
static void v_act_usb_switch(void);
#endif  /* boardUSB_EN */

#if (boardDC_EN)
static void v_act_dc_switch(void);
#endif  /* boardDC_EN */

#if (boardDISPLAY_EN)
static void v_act_backlight_switch(void);
static void v_act_backlight_force_on(void);
#endif  /* boardDISPLAY_EN */

static bool b_key_dispatch(const KeyActionItem_T *p_table, uint8_t uc_num, u8 *p_buff);


/* 跨状态动作表: 判序最高, 先于工程模式判断 */
static const KeyActionItem_T s_t_key_action_global[] =
{
	/* 序列特征                                   长度                                       有效系统状态       回调处理函数               日志 */
	{s_uc_key_tri_sys_on_off_buff,              sizeof(s_uc_key_tri_sys_on_off_buff),      KEY_MASK_ANY,    v_act_sys_on_off,         "开关机"},
};
#define			KEY_ACTION_GLOBAL_NUM					(sizeof(s_t_key_action_global) / sizeof(s_t_key_action_global[0]))

/* 工作态动作表: 仅在 DS_WORK / DS_ERR 生效 */
static const KeyActionItem_T s_t_key_action_work[] =
{
	{s_uc_key_tri_sys_prote_on_off_buff,         sizeof(s_uc_key_tri_sys_prote_on_off_buff),  KEY_MASK_NORMAL, v_act_sys_prote,          "系统充放保护"},

	#if (boardDCAC_EN)
	{s_uc_key_tri_ac_on_off_buff,               sizeof(s_uc_key_tri_ac_on_off_buff),        KEY_MASK_NORMAL, v_act_dcac_short,         "开关逆变"},
	{s_uc_key_tri_ac_on_off_buff1,              sizeof(s_uc_key_tri_ac_on_off_buff1),       KEY_MASK_NORMAL, v_act_dcac_long,          "开关逆变1"},
	{s_uc_key_tri_ac_prote_on_off_buff,          sizeof(s_uc_key_tri_ac_prote_on_off_buff),   KEY_MASK_NORMAL, v_act_dcac_prote,         "开启逆变输入保护"},
	#endif  /* boardDCAC_EN */

	#if (boardLIGHT_EN)
	{s_uc_key_tri_light_charge_buff,            sizeof(s_uc_key_tri_light_charge_buff),      KEY_MASK_NORMAL, v_act_light_mode,         "切换照明模式"},
	{s_uc_key_tri_light_on_off_buff,            sizeof(s_uc_key_tri_light_on_off_buff),      KEY_MASK_NORMAL, v_act_light_switch,       "开关照明"},
	#endif  /* boardLIGHT_EN */

	#if (boardUSB_EN)
	{s_uc_key_tri_usb_on_off_buff,              sizeof(s_uc_key_tri_usb_on_off_buff),        KEY_MASK_NORMAL, v_act_usb_switch,         "开关USB"},
	{s_uc_key_tri_usb_on_off_buff1,             sizeof(s_uc_key_tri_usb_on_off_buff1),       KEY_MASK_NORMAL, v_act_usb_switch,         "开关USB"},
	#endif  /* boardUSB_EN */

	#if (boardDC_EN)
	{s_uc_key_tri_dc_on_off_buff,               sizeof(s_uc_key_tri_dc_on_off_buff),         KEY_MASK_NORMAL, v_act_dc_switch,          "开关DC"},
	{s_uc_key_tri_dc_on_off_buff1,              sizeof(s_uc_key_tri_dc_on_off_buff1),        KEY_MASK_NORMAL, v_act_dc_switch,          "开关DC"},
	#endif  /* boardDC_EN */

	#if (boardDISPLAY_EN)
	{s_uc_key_tri_bl_on_off_buff,               sizeof(s_uc_key_tri_bl_on_off_buff),         KEY_MASK_NORMAL, v_act_backlight_switch,   "开关背光"},
	{s_uc_key_tri_force_open_bl_buff1,          sizeof(s_uc_key_tri_force_open_bl_buff1),    KEY_MASK_NORMAL, v_act_backlight_force_on, "强制开启背光"},
	{s_uc_key_tri_force_open_bl_buff2,          sizeof(s_uc_key_tri_force_open_bl_buff2),    KEY_MASK_NORMAL, v_act_backlight_force_on, "强制开启背光"},
	#endif  /* boardDISPLAY_EN */
};
#define			KEY_ACTION_WORK_NUM						(sizeof(s_t_key_action_work) / sizeof(s_t_key_action_work[0]))




/***********************************************************************************************************************
 * 函数功能    : 按键功能处理函数
 * 说明(备注)  : 查表分发按键事件序列
 * 传入参数    : p_uc_key_tri_type_buff: 事件序列缓冲区指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_ProcKeyFunc(u8 *p_uc_key_tri_type_buff)
{
	/* 第一优先级: 跨状态动作 (原逻辑中"开关机"优先于工程模式判断, 保持一致) */
	if (b_key_dispatch(s_t_key_action_global, KEY_ACTION_GLOBAL_NUM, p_uc_key_tri_type_buff) == true)
	{
		vKey_ParamInit();
		return;
	}

	#if (boardENG_MODE_EN)
	/* 第二优先级: 工程模式处理 */
	if (tSysInfo.eDevState == DS_ENG_MODE)
	{
		v_key_func_eng(p_uc_key_tri_type_buff);
		vKey_ParamInit();
		return;
	}
	#endif  /* boardENG_MODE_EN */

	/* 第三优先级: 工作态/错误态动作 */
	b_key_dispatch(s_t_key_action_work, KEY_ACTION_WORK_NUM, p_uc_key_tri_type_buff);

	vKey_ParamInit();
}

/***********************************************************************************************************************
 * 函数功能    : 系统开关机动作
 * 说明(备注)  : 触发系统开关机状态切换
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_sys_on_off(void)
{
	cSys_Switch(SO_KEY, ST_NULL, false);
}

/***********************************************************************************************************************
 * 函数功能    : 系统充放保护动作
 * 说明(备注)  : 触发强制关闭保护标志，长鸣并关机
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_sys_prote(void)
{
	bSys_SetPerm(SPO_FORCE_CLOSE, true);

	#if (boardBUZ_EN)
	bBuz_Tweet(LONG_1);
	#endif  /* boardBUZ_EN */

	cSys_Switch(SO_KEY, ST_OFF, false);
}

#if (boardDCAC_EN)
/***********************************************************************************************************************
 * 函数功能    : 单击开关逆变动作
 * 说明(备注)  : 开启时设置自动关机延时
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_dcac_short(void)
{
	if (cDCAC_Switch(DSO_AC_OUT, ST_NULL, true) == true)
		bDcac_SetAutoOffTime(boardDCAC_OFF_TIME);
}

/***********************************************************************************************************************
 * 函数功能    : 长按开关逆变动作
 * 说明(备注)  : 常开模式，不设置自动关机延时
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_dcac_long(void)
{
	if (cDCAC_Switch(DSO_AC_OUT, ST_NULL, true) == true)
		bDcac_SetAutoOffTime(0);
}

/***********************************************************************************************************************
 * 函数功能    : 开启逆变输入保护动作
 * 说明(备注)  : 切换逆变输入保护功能
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_dcac_prote(void)
{
	bDcac_InProteFuncSwitch(true);
}
#endif  /* boardDCAC_EN */

#if (boardLIGHT_EN)
/***********************************************************************************************************************
 * 函数功能    : 切换照明模式动作
 * 说明(备注)  : 循环切换照明模式
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_light_mode(void)
{
	vLight_CircSelectMode();
}

/***********************************************************************************************************************
 * 函数功能    : 开关照明动作
 * 说明(备注)  : 翻转照明开启/关闭状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_light_switch(void)
{
	bLight_Switch(ST_NULL);
}
#endif  /* boardLIGHT_EN */

#if (boardUSB_EN)
/***********************************************************************************************************************
 * 函数功能    : 开关 USB 动作
 * 说明(备注)  : 联动或独立开关控制
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_usb_switch(void)
{
	#if (boardDCAC_PARA_IN)
	if (tDc.eDevState >= DS_BOOTING || tUsb.eDevState >= DS_BOOTING)
	{
		cUsb_Switch(ST_OFF, false);
		cDc_Switch(ST_OFF, false);
	}
	else
	{
		cUsb_Switch(ST_ON, false);
		cDc_Switch(ST_ON, false);
	}
	#else
	cUsb_Switch(ST_NULL, false);
	#endif  /* boardDCAC_PARA_IN */
}
#endif  /* boardUSB_EN */

#if (boardDC_EN)
/***********************************************************************************************************************
 * 函数功能    : 开关 DC 动作
 * 说明(备注)  : 联动或独立开关控制
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_dc_switch(void)
{
	#if (boardDCAC_PARA_IN)
	cDCAC_Switch(DSO_PARA_IN, ST_NULL, true);
	#else
	cDc_Switch(ST_NULL, false);
	#endif  /* boardDCAC_PARA_IN */
}
#endif  /* boardDC_EN */

#if (boardDISPLAY_EN)
/***********************************************************************************************************************
 * 函数功能    : 开关背光动作
 * 说明(备注)  : 翻转显示屏背光状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_backlight_switch(void)
{
	bDisp_Switch(ST_NULL, false);
}

/***********************************************************************************************************************
 * 函数功能    : 强制开启背光动作
 * 说明(备注)  : 组合键强制拉高背光
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_backlight_force_on(void)
{
	bDisp_Switch(ST_ON, true);
}
#endif  /* boardDISPLAY_EN */


/***********************************************************************************************************************
 * 函数功能    : 按键动作查表分发内核
 * 说明(备注)  : 遍历动作表比对当前系统状态掩码与事件序列特征
 * 传入参数    : p_table: 动作表指针, uc_num: 表项数量, p_buff: 事件序列缓冲区指针
 * 输出参数    : 无
 * 返回值      : bool: true 命中并执行动作, false 未命中
 ************************************************************************************************************************/
static bool b_key_dispatch(const KeyActionItem_T *p_table, uint8_t uc_num, u8 *p_buff)
{
	uint8_t i;
	uint32_t ul_curr_state_mask = (1U << tSysInfo.eDevState);

	for (i = 0; i < uc_num; i++)
	{
		/* 1. 校验当前系统工作状态是否匹配该按键动作 */
		if ((p_table[i].ulStateMask & ul_curr_state_mask) != 0)
		{
			/* 2. 比对按键序列特征 */
			if (bFun_DataCompare(p_buff, (u8 *)p_table[i].pSeq, p_table[i].ucSeqLen))
			{
				if (p_table[i].pfAction != NULL)
					p_table[i].pfAction();

				if (uPrint.tFlag.bKeyTask && p_table[i].pLog != NULL)
					sMyPrint("Key_Task:%s\r\n", p_table[i].pLog);

				return true;
			}
		}
	}
	return false;
}
#endif  /* 1 */

#endif  /* boardKEY_EN */
