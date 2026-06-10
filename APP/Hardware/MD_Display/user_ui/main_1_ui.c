/*******************************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS803\APP\Hardware\MD_Display\user_ui
 * File    : main_1_ui.c
 * Date    : 2026-05-28 10:23:07
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
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
#include "MD_Bms/md_bms_task.h"
#include <stdbool.h>

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/user_ui/energy_ring.h"
#include "Sys/sys_task.h"

#include "MD_Dcac/md_dcac_task.h"

#include "lvgl.h"

//****************************************************Macros*******************************************************************//



//****************************************************Parameter Initialization************************************************//
// 设备状态
bool S_bDevAcOutState;  // AC输出设备状态(true=开, false=关)
bool S_bDevAcInState;   // AC输入设备状态(true=开, false=关)
bool S_bDevPvState;     // PV设备状态
// bool S_bDevLightState;  // Light设备状态
bool S_bDevUsbState;    // USB设备状态
// bool S_bDevUsbAState;   // USB A设备状态
// bool S_bDevUsbC1State;  // USB C1设备状态
// bool S_bDevUsbC2State;  // USB C2设备状态
bool S_bDevDcState;     // DC设备状态

static EnergyRing_T s_tEnergyRing;
static bool S_bMain1InitialFinish = false;
static bool S_bTestAllParam = false;

//****************************************************Function Declaration****************************************************//

/***********************************************************************************************************************
-----函数功能    设置图标可见性
-----传入参数    obj: 图标对象
-----传入参数    img_src: 图标图片资源
-----传入参数    visible: 是否可见
-----备注        indent:LJD
-----日期        2026-05-28
************************************************************************************************************************/
static void v_disp_set_icon_visible(lv_obj_t *obj, bool visible)
{
	if(obj == NULL)
		return;

	if(visible)
	{
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}
	else
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}


/*****************************************************************************************************************
-----函数功能    更新所有设备状态图标UI
-----说明(备注)  在display_task中调用，根据状态标志更新所有设备UI显示
-----传入参数    none
-----输出参数    none
-----返回值      true:更新 false:无更新
*****************************************************************************************************************/
bool b_disp_update_all_dev_states(bool b_force)
{
    bool b_ret = false;

	extern objects_t objects;
	
	static int8_t s_last_dev_ac_out_state = -1;
	static int8_t s_last_dev_ac_in_state = -1;
	static int8_t s_last_dev_pv_state = -1;
	static int8_t s_last_dev_usb_state = -1;
	static int8_t s_last_dev_dc_state = -1;

	bool b_ac_out_state = S_bDevAcOutState;
	bool b_ac_in_state = S_bDevAcInState;
	bool b_pv_state = S_bDevPvState;
	bool b_usb_state = S_bDevUsbState;
	bool b_dc_state = S_bDevDcState;

	if (S_bTestAllParam)
	{
		b_ac_out_state = true;
		b_ac_in_state = true;
		b_pv_state = true;
		b_usb_state = true;
		b_dc_state = true;
	}
	
	if(s_last_dev_ac_out_state != b_ac_out_state || b_force)
	{
		s_last_dev_ac_out_state = b_ac_out_state;
		v_disp_set_icon_visible(objects.b_dev_ac_out_state, b_ac_out_state);
        b_ret = true;
	}

	if(s_last_dev_ac_in_state != b_ac_in_state || b_force)
	{
		s_last_dev_ac_in_state = b_ac_in_state;
		v_disp_set_icon_visible(objects.b_dev_ac_in_state, b_ac_in_state);
        b_ret = true;
	}
	
	if(s_last_dev_pv_state != b_pv_state || b_force)
	{
		s_last_dev_pv_state = b_pv_state;
		v_disp_set_icon_visible(objects.b_dev_pv_state, b_pv_state);
        b_ret = true;
	}

	if(s_last_dev_usb_state != b_usb_state || b_force)
	{
		s_last_dev_usb_state = b_usb_state;
		v_disp_set_icon_visible(objects.b_dev_usb_state, b_usb_state);
        b_ret = true;
	}
	
	if(s_last_dev_dc_state != b_dc_state || b_force)
	{
		s_last_dev_dc_state = b_dc_state;
		v_disp_set_icon_visible(objects.b_dev_dc_state, b_dc_state);
        b_ret = true;
	}

    return b_ret;
}

/*****************************************************************************************************************
-----函数功能    更新所有错误状态图标UI
-----说明(备注)  返回 0~99 错误码时 -> 标签可见; 返回 100(切换间隙)时 -> 标签隐藏
-----传入参数    b_force: 是否强制更新
-----输出参数    none
-----返回值      true:更新 false:无更新
*****************************************************************************************************************/
static bool b_update_error_states(bool b_force)
{
	bool b_ret = false;
	
	// Determine if OT and OL are active
	bool b_ot_active = false;
	bool b_ol_active = false;
	bool b_has_error = (usDisp_ErrCodeDisplay() != 100);
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
		if (tSysInfo.uErrCode.tCode.bOT)
			b_ot_active = true;

		// OL check
		if (tSysInfo.uErrCode.tCode.bOL)
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

/*****************************************************************************************************************
-----函数功能    更新AC工作模式图标UI
-----说明(备注)  在display_task中调用，根据状态标志更新AC工作模式图标显示
-----传入参数    b_force: 是否强制更新
-----输出参数    none
-----返回值      true:更新 false:无更新
*****************************************************************************************************************/
static bool b_update_ac_work_mode(bool b_force)
{
	bool b_ret = false;
	
	//更新AC工作模式
	static ImgAnimMode_E s_last_ac_mode = (ImgAnimMode_E)-1;
	//充放电
	if((cSys_IsChgState()> 0 && tDcac.eDisChgState == IOS_WORK) || S_bTestAllParam)
	{
		if(s_last_ac_mode != IMG_ANIM_MODE_CHG_DISCHG || b_force)
		{
			s_last_ac_mode = IMG_ANIM_MODE_CHG_DISCHG;
			vDisp_SetAcWorkMode(IMG_ANIM_MODE_CHG_DISCHG);
			b_ret |= true;
		}
	}
	//充电
	else if(cSys_IsChgState() == 2 && tDcac.eDisChgState != IOS_WORK)
	{
		//快充
		if(ucBms_GetSoc() > 2 && ucBms_GetSoc() < 90)
		{
			if(s_last_ac_mode != IMG_ANIM_MODE_CHARGE_FAST || b_force)
			{
				s_last_ac_mode = IMG_ANIM_MODE_CHARGE_FAST;
				vDisp_SetAcWorkMode(IMG_ANIM_MODE_CHARGE_FAST);
				b_ret |= true;
			}
		}
		//慢充
		else
		{
			if(s_last_ac_mode != IMG_ANIM_MODE_CHARGE_SLOW || b_force)
			{
				s_last_ac_mode = IMG_ANIM_MODE_CHARGE_SLOW;
				vDisp_SetAcWorkMode(IMG_ANIM_MODE_CHARGE_SLOW);
				b_ret |= true;
			}
		}
	}
	//放电
	else if(tDcac.eChgState != IOS_WORK && tDcac.eDisChgState == IOS_WORK)
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


/*****************************************************************************************************************
-----函数功能    启动主界面
-----说明(备注)  将工作态页面的动画初始化和启动统一封装到主界面模块
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
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

/*****************************************************************************************************************
-----函数功能    主界面退出
-----说明(备注)  退出工作态时统一释放动画资源并隐藏呼吸图标
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vDisp_Main1Exit(void)
{
	EnergyRing_Stop(&s_tEnergyRing);
	vImgAnim_Stop(); /* 全部关闭并隐藏 */
	S_bMain1InitialFinish = false;
}

/***********************************************************************************************************************
-----函数功能    更新主界面数据
-----说明(备注)  刷新主界面显示的数据
-----传入参数    none
-----输出参数    none
-----返回值      true:更新 false:无更新
************************************************************************************************************************/
bool bDisp_Main1DataUpdate(void)
{
    bool b_ret = false;
    
	//更新设备状态
    b_ret |= b_disp_update_all_dev_states(false);
	//更新错误状态
    b_ret |= b_update_error_states(false);
	//更新AC工作模式
	b_ret |= b_update_ac_work_mode(false);
	//更新电池图标
	static int16_t s_last_soc = -1;
	static int8_t b_last_chg_flag = -1;
	bool b_chg_flag = (cSys_IsChgState() >= 2 || S_bTestAllParam) ? true : false;
	if(s_last_soc != ucBms_GetSoc() || b_chg_flag != b_last_chg_flag)
	{
		s_last_soc = ucBms_GetSoc();
		b_last_chg_flag = b_chg_flag;
		EnergyRing_UpdateSoc(&s_tEnergyRing, s_last_soc, b_chg_flag);
		b_ret = true;
	}
	

    return b_ret;
}

/*****************************************************************************************************************
-----函数功能    统一设置设备状态
-----说明(备注)  设置设备状态标志，实际的UI更新在displaytask中处理
-----传入参数    devType: 设备类型, bState: true=开启状态, false=关闭状态
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vDisp_SetDevStateIcon(DevType_E devType, bool bState)
{
	if (S_bTestAllParam)
		bState = true;

	switch(devType)
	{
		case DEV_TYPE_AC_OUT:
			S_bDevAcOutState = bState;
			break;
		
		case DEV_TYPE_AC_IN:
			S_bDevAcInState = bState;
			break;
		
		case DEV_TYPE_PV:
			S_bDevPvState = bState;
			break;
		
		case DEV_TYPE_USB:
			S_bDevUsbState = bState;
			break;
		
		case DEV_TYPE_DC:
			S_bDevDcState = bState;
			break;
		
		default:
			break;
	}
}

/*****************************************************************************************************************
-----函数功能    设置AC工作模式
-----说明(备注)  设置AC工作模式标志，实际的UI更新在displaytask中处理
-----传入参数    eMode: AC工作模式
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vDisp_SetAcWorkMode(ImgAnimMode_E eMode)
{
	switch(eMode)
	{
		case IMG_ANIM_MODE_NONE:
			vImgAnim_SetMode(IMG_ANIM_MODE_NONE, 800);
			break;
		
		case IMG_ANIM_MODE_CHARGE_SLOW:
			vImgAnim_SetMode(IMG_ANIM_MODE_CHARGE_SLOW, 800);
			break;
		
		case IMG_ANIM_MODE_CHARGE_FAST:
			vImgAnim_SetMode(IMG_ANIM_MODE_CHARGE_FAST, 800);
			break;
		
		case IMG_ANIM_MODE_DISCHARGE:
			vImgAnim_SetMode(IMG_ANIM_MODE_DISCHARGE, 800);
			break;
		
		case IMG_ANIM_MODE_CHG_DISCHG:
			vImgAnim_SetMode(IMG_ANIM_MODE_CHG_DISCHG, 800);
			break;
		
		default:
			break;
	}
}

#endif  //boardDISPLAY_EN
