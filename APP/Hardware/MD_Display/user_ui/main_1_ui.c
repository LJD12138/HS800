/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display\user_ui
 * File    : main_1_ui.c
 * Date    : 2026-05-28
 * Author  : LJD(291483914@qq.com)
 * Desc    : 主界面 UI 刷新与设备状态显示
 * -------------------------------------------------------
 * todo    :
 * 1.APP\Hardware\MD_Display\eez_ui该文件夹里面的文件只可以读取,不可以修改,
 *  因为里面的文件是由UI设计工具eez studio生成的,修改后会被覆盖掉.如果需要修改UI界面,
 *  请修改APP\Hardware\MD_Display\user_ui文件夹里面的文件,
 *  这些文件是由开发人员编写的,不会被覆盖掉.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "MD_Display/user_ui/main_1_ui.h"
#include "main.h"
#include <stdbool.h>

#if(boardDISPLAY_EN)
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/eez_ui/images.h"
#include "MD_Display/eez_ui/vars.h"
#include "MD_Display/user_ui/energy_ring.h"

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"			/* MpptWorkMode_E/MWM_DC (PV 图标选源比较, 数据经快照传入) */
#endif  /* boardMPPT_EN */

#include <stdio.h>
#include "lvgl.h"

//****************************************************Macros********************************************************************//



//****************************************************Parameter Initialization**************************************************//
// 设备状态显隐标志 (仅本文件使用, 由 v_disp_apply_port_flags 依据快照位图刷新)
static bool S_bDevAcOutShow;  // AC输出设备显示(true=开, false=关)
static bool S_bDevAcInShow;   // AC输入设备显示(true=开, false=关)
static bool S_bDevPvShow;     // PV设备显示(true=开, false=关)
static bool S_bDevLightShow;  // Light设备显示(true=开, false=关)
static bool S_bDevUsbShow;    // USB设备显示(true=开, false=关)
// bool S_bDevUsbAState;   // USB A设备显示(true=开, false=关)
// bool S_bDevUsbC1State;  // USB C1设备显示(true=开, false=关)
// bool S_bDevUsbC2State;  // USB C2设备显示(true=开, false=关)
static bool S_bDevDcShow;     // DC设备显示(true=开, false=关)

static EnergyRing_T s_tEnergyRing;
static bool S_bMain1InitialFinish = false;
static bool S_bTestAllParam = false;

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 设置图标可见性
 * 传入参数    : obj: 图标对象
 * 传入参数    : img_src: 图标图片资源
 * 传入参数    : visible: 是否可见
 * 说明(备注)  : indent:LJD
 * 输出参数    : 无
 * 返回值      : 无
 * 日期        : 2026-05-28
 ************************************************************************************************************************/
static void v_disp_set_icon_visible(lv_obj_t *obj, bool visible)
{
	if(obj == NULL)
		return;

	if(visible)
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
	else
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}


/***********************************************************************************************************************
 * 函数功能    : 由快照端口位图刷新设备图标显隐标志
 * 说明(备注)  : 闪烁位按 500ms 相位统一翻转(替代旧 vDisp_SetDevStateIcon 分散计时, 各端口同步闪烁);
 *               激活位稳态点亮, 两者皆无则隐藏
 * 传入参数    : p_data: 数据快照
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_disp_apply_port_flags(const DispDataSnapshot_T *p_data)
{
	static uint32_t	s_ul_blink_tick = 0;		/* 上次闪烁相位翻转时刻 */
	static bool		s_b_blink_phase = true;		/* 闪烁相位(true=亮) */
	uint32_t ul_now = xTaskGetTickCount();
	bool b_blink_show;

	if (ul_now - s_ul_blink_tick >= 500U)
	{
		s_ul_blink_tick = ul_now;
		s_b_blink_phase = !s_b_blink_phase;
	}
	b_blink_show = s_b_blink_phase;

	S_bDevAcOutShow = (p_data->ulPortBlinkFlags & DISP_PORT_BIT_AC_OUT) ? b_blink_show : ((p_data->ulPortActiveFlags & DISP_PORT_BIT_AC_OUT) != 0);
	S_bDevAcInShow  = (p_data->ulPortBlinkFlags & DISP_PORT_BIT_AC_IN)  ? b_blink_show : ((p_data->ulPortActiveFlags & DISP_PORT_BIT_AC_IN)  != 0);
	S_bDevPvShow    = (p_data->ulPortBlinkFlags & DISP_PORT_BIT_DC_IN)  ? b_blink_show : ((p_data->ulPortActiveFlags & DISP_PORT_BIT_DC_IN)  != 0);
	S_bDevUsbShow   = (p_data->ulPortBlinkFlags & DISP_PORT_BIT_USB_OUT)? b_blink_show : ((p_data->ulPortActiveFlags & DISP_PORT_BIT_USB_OUT)!= 0);
	S_bDevDcShow    = (p_data->ulPortBlinkFlags & DISP_PORT_BIT_DC_OUT) ? b_blink_show : ((p_data->ulPortActiveFlags & DISP_PORT_BIT_DC_OUT) != 0);
	S_bDevLightShow = (p_data->ulPortActiveFlags & DISP_PORT_BIT_LIGHT) != 0;	/* 照明无错误闪烁态 */
}


/***********************************************************************************************************************
 * 函数功能    : 更新所有设备状态图标UI
 * 说明(备注)  : 在display_task中调用，根据显隐标志更新所有设备UI显示 (标志由 v_disp_apply_port_flags 自快照位图刷新)
 * 传入参数    : p_data: 数据快照, b_force: 是否强制更新
 * 输出参数    : none
 * 返回值      : true:更新 false:无更新
 ************************************************************************************************************************/
static bool b_disp_update_all_dev_states(const DispDataSnapshot_T *p_data, bool b_force)
{
    bool b_ret = false;

	extern objects_t objects;

	static int8_t s_last_dev_ac_out_state = -1;
	static int8_t s_last_dev_ac_in_state = -1;
	static int8_t s_last_dev_pv_state = -1;
	#if(boardMPPT_EN)
	static int8_t s_last_pv_work_mode = -1;
	#endif  /* boardMPPT_EN */
	static int8_t s_last_dev_usb_state = -1;
	static int8_t s_last_dev_dc_state = -1;
	static int8_t s_last_dev_light_state = -1;

	bool b_ac_out_show = S_bDevAcOutShow;
	bool b_ac_in_show = S_bDevAcInShow;
	bool b_pv_show = S_bDevPvShow;
	bool b_usb_show = S_bDevUsbShow;
	bool b_dc_show = S_bDevDcShow;
	bool b_light_show = S_bDevLightShow;

	if (S_bTestAllParam)
	{
		b_ac_out_show = true;
		b_ac_in_show = true;
		b_pv_show = true;
		b_usb_show = true;
		b_dc_show = true;
		b_light_show = true;
	}

	if(s_last_dev_ac_out_state != b_ac_out_show || b_force)
	{
		s_last_dev_ac_out_state = b_ac_out_show;
		v_disp_set_icon_visible(objects.b_dev_ac_out_state, b_ac_out_show);
        b_ret = true;
	}

	if(s_last_dev_ac_in_state != b_ac_in_show || b_force)
	{
		s_last_dev_ac_in_state = b_ac_in_show;
		v_disp_set_icon_visible(objects.b_dev_ac_in_state, b_ac_in_show);
        b_ret = true;
	}

	#if(boardMPPT_EN)
	MpptWorkMode_E e_work_mode = (MpptWorkMode_E)p_data->ucPvWorkMode;
	if(s_last_dev_pv_state != b_pv_show || s_last_pv_work_mode != (int8_t)e_work_mode || b_force)
	{
		s_last_dev_pv_state = b_pv_show;
		s_last_pv_work_mode = (int8_t)e_work_mode;
		if(b_pv_show)
		{
			if(e_work_mode == MWM_DC)
				lv_image_set_src(objects.b_dev_pv_state, &img_icon_dc);
			else
				lv_image_set_src(objects.b_dev_pv_state, &img_icon_tx60);
			v_disp_set_icon_visible(objects.b_dev_pv_state, true);
		}
		else
			v_disp_set_icon_visible(objects.b_dev_pv_state, false);
        b_ret = true;
	}
	#else
	if(s_last_dev_pv_state != b_pv_show || b_force)
	{
		s_last_dev_pv_state = b_pv_show;
		v_disp_set_icon_visible(objects.b_dev_pv_state, b_pv_show);
        b_ret = true;
	}
	#endif  /* boardMPPT_EN */

	if(s_last_dev_usb_state != b_usb_show || b_force)
	{
		s_last_dev_usb_state = b_usb_show;
		v_disp_set_icon_visible(objects.b_dev_usb_state, b_usb_show);
        b_ret = true;
	}
	
	if(s_last_dev_dc_state != b_dc_show || b_force)
	{
		s_last_dev_dc_state = b_dc_show;
		v_disp_set_icon_visible(objects.b_dev_dc_state, b_dc_show);
        b_ret = true;
	}

	if(s_last_dev_light_state != b_light_show || b_force)
	{
		s_last_dev_light_state = b_light_show;
		v_disp_set_icon_visible(objects.b_dev_light_state, b_light_show);
        b_ret = true;
	}

    return b_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 更新所有错误状态图标UI
 * 说明(备注)  : 返回 0~99 错误码时 -> 标签可见; 返回 100(切换间隙)时 -> 标签隐藏
 * 传入参数    : p_data: 数据快照, b_force: 是否强制更新
 * 输出参数    : none
 * 返回值      : true:更新 false:无更新
 ************************************************************************************************************************/
static bool b_update_error_states(const DispDataSnapshot_T *p_data, bool b_force)
{
	bool b_ret = false;

	// Determine if OT and OL are active
	bool b_ot_active = false;
	bool b_ol_active = false;
	bool b_has_error = (p_data->usPrimaryErrCode != 0);
	// bool b_has_error = true;//测试
	if (S_bTestAllParam)
	{
		b_ot_active = true;
		b_ol_active = true;
		b_has_error = true;
	}
	else
	{
		// OT check
		if (p_data->usSysErrCode & DISP_SYSERR_BIT_OT)
			b_ot_active = true;

		// OL check
		if (p_data->usSysErrCode & DISP_SYSERR_BIT_OL)
			b_ol_active = true;
	}
	
	// Static caches
	static int8_t s_last_ot_active = -1;
	static int8_t s_last_ol_active = -1;
	static int8_t s_last_has_error = -1;
	
	extern objects_t objects;

	// OT Icon
	if (s_last_ot_active != b_ot_active || b_force)
	{
		s_last_ot_active = b_ot_active;
		v_disp_set_icon_visible(objects.b_err_icon_ot, b_ot_active);
		b_ret = true;
	}
	
	// OL Icon
	if (s_last_ol_active != b_ol_active || b_force)
	{
		s_last_ol_active = b_ol_active;
		v_disp_set_icon_visible(objects.b_err_icon_ol, b_ol_active);
		b_ret = true;
	}
	
	// Error code
	if (s_last_has_error != b_has_error || b_force)
	{
		s_last_has_error = b_has_error;
		v_disp_set_icon_visible(objects.uca_err_code, b_has_error);
		b_ret = true;
	}
	
	return b_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 更新AC工作模式图标UI
 * 说明(备注)  : 在display_task中调用，根据快照充放电状态更新AC工作模式图标显示
 * 传入参数    : p_data: 数据快照, b_force: 是否强制更新
 * 输出参数    : none
 * 返回值      : true:更新 false:无更新
 ************************************************************************************************************************/
static bool b_update_ac_work_mode(const DispDataSnapshot_T *p_data, bool b_force)
{
	bool b_ret = false;
	bool b_chg_flag = false;
	bool b_dis_chg_flag = false;

	if(p_data->ucChgState > 0)
		b_chg_flag = true;

	if(p_data->bDisChgActive)
		b_dis_chg_flag = true;
	
	//更新AC工作模式
	static ImgAnimMode_E s_last_ac_mode = (ImgAnimMode_E)-1;
	//充放电
	if((b_chg_flag && b_dis_chg_flag) || S_bTestAllParam)
	{
		if(s_last_ac_mode != IMG_ANIM_MODE_CHG_DISCHG || b_force)
		{
			s_last_ac_mode = IMG_ANIM_MODE_CHG_DISCHG;
			vDisp_SetAcWorkMode(IMG_ANIM_MODE_CHG_DISCHG);
			b_ret |= true;
		}
	}
	//充电
	else if(b_chg_flag)
	{
		//快充
		// if(ucBms_GetSoc() > 2 && ucBms_GetSoc() < 90)
		{
			if(s_last_ac_mode != IMG_ANIM_MODE_CHARGE_FAST || b_force)
			{
				s_last_ac_mode = IMG_ANIM_MODE_CHARGE_FAST;
				vDisp_SetAcWorkMode(IMG_ANIM_MODE_CHARGE_FAST);
				b_ret |= true;
			}
		}
		//慢充
		// else
		// {
		// 	if(s_last_ac_mode != IMG_ANIM_MODE_CHARGE_SLOW || b_force)
		// 	{
		// 		s_last_ac_mode = IMG_ANIM_MODE_CHARGE_SLOW;
		// 		vDisp_SetAcWorkMode(IMG_ANIM_MODE_CHARGE_SLOW);
		// 		b_ret |= true;
		// 	}
		// }
	}
	//放电
	else if(b_dis_chg_flag)
	{
		if(s_last_ac_mode != IMG_ANIM_MODE_DISCHARGE || b_force)
		{
			s_last_ac_mode = IMG_ANIM_MODE_DISCHARGE;
			vDisp_SetAcWorkMode(IMG_ANIM_MODE_DISCHARGE);
			b_ret |= true;
		}
	}
	//未工作
	else
	{
		if(s_last_ac_mode != IMG_ANIM_MODE_NONE || b_force)
		{
			s_last_ac_mode = IMG_ANIM_MODE_NONE;
			vDisp_SetAcWorkMode(IMG_ANIM_MODE_NONE);
			b_ret |= true;
		}
	}
	
	return b_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 格式化剩余使用时间
 * 说明(备注)  : 将分钟数转换为固定宽度的小时/分钟显示文本
 * 传入参数    : pc_str:输出缓存  str_size:缓存大小  us_total_minutes:总分钟数
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
#if (boardBMS_EN)
static void v_disp_work_format_remaining_time(char *pc_str, size_t str_size, u16 us_total_minutes)
{
    u16 us_hours = us_total_minutes / 60U;
    u16 us_minutes = us_total_minutes % 60U;

    if (us_hours > 99U)
    {
        us_hours = 99U;
        us_minutes = 99U;
    }

    if (us_hours >= 10U && us_minutes >= 10U)
        snprintf(pc_str, str_size, "%2uh %2um", (unsigned int)us_hours, (unsigned int)us_minutes);
    else if (us_hours >= 10U)
        snprintf(pc_str, str_size, "%2uh  %1um", (unsigned int)us_hours, (unsigned int)us_minutes);
    else if (us_minutes >= 10U)
        snprintf(pc_str, str_size, " %1uh %2um", (unsigned int)us_hours, (unsigned int)us_minutes);
    else
        snprintf(pc_str, str_size, " %1uh  %1um", (unsigned int)us_hours, (unsigned int)us_minutes);
}
#endif // boardBMS_EN


















/***********************************************************************************************************************
 * 函数功能    : 启动主界面
 * 说明(备注)  : 将工作态页面的动画初始化和启动统一封装到主界面模块
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_Main1UiStart(void)
{
	if(S_bMain1InitialFinish == false)
	{
		EnergyRing_Start(&s_tEnergyRing);

		// 1. 初始化（全局执行一次即可）
		vImgAnim_Init(objects.main_work);

		// 2. 为各模式分别注入专属的 X/Y 拼接对齐坐标点
		ImgAnimPosConfig_T slow_pos = { .lLeftX = 30, .lLeftY = 30 }; // 慢充只需对齐2.png的左图位置
		vImgAnim_SetPosConfig(IMG_ANIM_MODE_CHARGE_SLOW, &slow_pos);

		ImgAnimPosConfig_T fast_pos = { .lLeftX = 30, .lLeftY = 30, .lRightX = 50, .lRightY = 30 }; // 快充对齐2和3
		vImgAnim_SetPosConfig(IMG_ANIM_MODE_CHARGE_FAST, &fast_pos);

		ImgAnimPosConfig_T discharge_pos = { .lLeftX = 30, .lLeftY = 30, .lRightX = 45, .lRightY = 30 };
		vImgAnim_SetPosConfig(IMG_ANIM_MODE_DISCHARGE, &discharge_pos);

		ImgAnimPosConfig_T chg_dischg_pos = { .lLeftX = 30, .lLeftY = 30, .lRightX = 50, .lRightY = 30 };
		vImgAnim_SetPosConfig(IMG_ANIM_MODE_CHG_DISCHG, &chg_dischg_pos);

		S_bMain1InitialFinish = true;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 主界面退出
 * 说明(备注)  : 退出工作态时统一释放动画资源并隐藏呼吸图标
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_Main1Exit(void)
{
	EnergyRing_Stop(&s_tEnergyRing);
	vImgAnim_Stop(); /* 全部关闭并隐藏 */
	S_bMain1InitialFinish = false;
}

/***********************************************************************************************************************
 * 函数功能    : 更新主界面数据
 * 说明(备注)  : 依据数据快照刷新主界面显示的数据
 * 传入参数    : p_data: 数据快照
 * 输出参数    : none
 * 返回值      : true:更新 false:无更新
 ************************************************************************************************************************/
bool bDisp_Main1DataUpdate(const DispDataSnapshot_T *p_data)
{
    bool b_ret = false;

    if (p_data == NULL)
        return false;

	//更新设备状态
    b_ret |= b_disp_update_all_dev_states(p_data, false);
	//更新错误状态
    b_ret |= b_update_error_states(p_data, false);
	//更新AC工作模式
	b_ret |= b_update_ac_work_mode(p_data, false);
	//更新电池图标
	static int16_t s_last_soc = -1;
	static int8_t b_last_chg_flag = -1;
	bool b_chg_flag = (p_data->ucChgState >= 2 || S_bTestAllParam) ? true : false;
	int16_t s_soc = p_data->ucSoc;
	if(s_last_soc != s_soc || b_chg_flag != b_last_chg_flag)
	{
		s_last_soc = s_soc;
		b_last_chg_flag = b_chg_flag;
		EnergyRing_UpdateSoc(&s_tEnergyRing, s_last_soc, b_chg_flag);
		b_ret = true;
	}


    return b_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 设置AC工作模式
 * 说明(备注)  : 设置AC工作模式标志，实际的UI更新在displaytask中处理
 * 传入参数    : eMode: AC工作模式
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_SetAcWorkMode(ImgAnimMode_E eMode)
{
	switch(eMode)
	{
		case IMG_ANIM_MODE_NONE:
		{
			vImgAnim_SetMode(IMG_ANIM_MODE_NONE, 1500);
		}
		break;
		
		case IMG_ANIM_MODE_CHARGE_SLOW:
		{
			vImgAnim_SetMode(IMG_ANIM_MODE_CHARGE_SLOW, 1500);
		}
		break;
		
		case IMG_ANIM_MODE_CHARGE_FAST:
		{
			vImgAnim_SetMode(IMG_ANIM_MODE_CHARGE_FAST, 1500);
		}
		break;
		
		case IMG_ANIM_MODE_DISCHARGE:
		{
			vImgAnim_SetMode(IMG_ANIM_MODE_DISCHARGE, 1500);
		}
		break;
		
		case IMG_ANIM_MODE_CHG_DISCHG:
		{
			vImgAnim_SetMode(IMG_ANIM_MODE_CHG_DISCHG, 1500);
		}
		break;
		
		default:
		{
		}
		break;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 更新设备参数
 * 说明(备注)  : 这里只需要依据快照更新数据,eez_ui里面会根据数据变化自动更新显示;
 *               端口图标显隐标志由 v_disp_apply_port_flags 依据快照位图统一刷新
 * 传入参数    : p_data: 数据快照
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vDisp_UpdateDevParam(const DispDataSnapshot_T *p_data)
{
    char cStr[10];

    if (p_data == NULL)
        return;

    /* 端口图标显隐标志刷新 (闪烁位 0.5s 相位翻转, 激活位稳态点亮) */
    v_disp_apply_port_flags(p_data);

    #if (boardBMS_EN)
    snprintf(cStr, sizeof(cStr), "%d", p_data->ucSoc);
    set_var_uca_bat_soc_value(cStr);

    u16 usTotalMinutes;
    if (p_data->bBmsChg)
        usTotalMinutes = p_data->usChgFullTimeMin;
    else
        usTotalMinutes = p_data->usDisChgEmptyTimeMin;
    v_disp_work_format_remaining_time(cStr, sizeof(cStr), usTotalMinutes);
    set_var_uca_remaining_usage_time(cStr);
    #endif // boardBMS_EN

    snprintf(cStr, sizeof(cStr), "%d", p_data->usOutPwr);
    set_var_uca_out_pwr_value(cStr);

    snprintf(cStr, sizeof(cStr), "%d", p_data->usInPwr);
    set_var_uca_in_pwr_value(cStr);

    // 故障码轮询显示: 切换间隙(返回0)时不更新错误码文本,
    // 避免将"0"字符串显示到标签上, 同时配合 user_ui 的可见性控制实现闪烁效果
    u16 us_err_code = p_data->usPrimaryErrCode;
    if (us_err_code != 0)
    {
        snprintf(cStr, sizeof(cStr), "E%d", us_err_code);
        set_var_uca_err_code_value(cStr);
    }
}

#endif  //boardDISPLAY_EN
