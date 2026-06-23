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
#include "main.h"
#include <stdbool.h>

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/eez_ui/images.h"
#include "MD_Display/eez_ui/vars.h"
#include "MD_Display/user_ui/energy_ring.h"
#include "Sys/sys_task.h"

#include "MD_Dcac/md_dcac_task.h"
#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#endif

#include "Usb/usb_task.h"
#include "Dc/dc_task.h"
#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif
#include <stdio.h>

#include "lvgl.h"

//****************************************************Macros*******************************************************************//



//****************************************************Parameter Initialization************************************************//
// 设备状态
bool S_bDevAcOutShow;  // AC输出设备显示(true=开, false=关)
bool S_bDevAcInShow;   // AC输入设备显示(true=开, false=关)
bool S_bDevPvShow;     // PV设备显示(true=开, false=关)
bool S_bDevLightShow;   // Light设备显示(true=开, false=关)
bool S_bDevUsbShow;    // USB设备显示(true=开, false=关)
// bool S_bDevUsbAState;   // USB A设备显示(true=开, false=关)
// bool S_bDevUsbC1State;  // USB C1设备显示(true=开, false=关)
// bool S_bDevUsbC2State;  // USB C2设备显示(true=开, false=关)
bool S_bDevDcShow;     // DC设备显示(true=开, false=关)

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
	#if(boardMPPT_EN)
	static int8_t s_last_pv_work_mode = -1;
	#endif
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
	MpptWorkMode_E e_work_mode = tMppt.eWorkMode;
	if(s_last_dev_pv_state != b_pv_show || s_last_pv_work_mode != (int8_t)e_work_mode || b_force)
	{
		s_last_dev_pv_state = b_pv_show;
		s_last_pv_work_mode = (int8_t)e_work_mode;
		if(b_pv_show)
		{
			if(e_work_mode == MWM_DC)
			{
				lv_image_set_src(objects.b_dev_pv_state, &img_icon_dc);
			}
			else
			{
				lv_image_set_src(objects.b_dev_pv_state, &img_icon_tx60);
			}
			v_disp_set_icon_visible(objects.b_dev_pv_state, true);
		}
		else
		{
			v_disp_set_icon_visible(objects.b_dev_pv_state, false);
		}
        b_ret = true;
	}
	#else
	if(s_last_dev_pv_state != b_pv_show || b_force)
	{
		s_last_dev_pv_state = b_pv_show;
		v_disp_set_icon_visible(objects.b_dev_pv_state, b_pv_show);
        b_ret = true;
	}
	#endif

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

/***********************************************************************************************************************
-----函数功能    格式化剩余使用时间
-----说明(备注)  将分钟数转换为固定宽度的小时/分钟显示文本
-----传入参数    pc_str:输出缓存  str_size:缓存大小  us_total_minutes:总分钟数
-----输出参数    none
-----返回值      none
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
-----传入参数    devType: 设备类型, ucState: 设备状态
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vDisp_SetDevStateIcon(DevType_E devType, u8 ucState)
{
	DevState_E eState;
	InOutState_E eInOutState;

	switch(devType)
	{
		case DEV_TYPE_AC_OUT:
		{
			eInOutState = (InOutState_E)ucState;

			//0.5S频率闪烁
			if(eInOutState == IOS_ERR || eInOutState == IOS_PROTE)
			{
				static uint32_t s_ul_last_tick = 0;
				uint32_t ul_now_tick = xTaskGetTickCount();
				
				if(ul_now_tick - s_ul_last_tick >= 500)
				{
					s_ul_last_tick = ul_now_tick;
					S_bDevAcOutShow = !S_bDevAcOutShow;
				}
			}
			else if(eInOutState >= IOS_STARTING)
				S_bDevAcOutShow = true;
			else
			   	S_bDevAcOutShow = false;
		}
		break;
		
		case DEV_TYPE_AC_IN:
		{
			eInOutState = (InOutState_E)ucState;

			//0.5S频率闪烁
			if(eInOutState == IOS_ERR || eInOutState == IOS_PROTE)
			{
				static uint32_t s_ul_last_tick = 0;
				uint32_t ul_now_tick = xTaskGetTickCount();
				
				if(ul_now_tick - s_ul_last_tick >= 500)
				{
					s_ul_last_tick = ul_now_tick;
					S_bDevAcInShow = !S_bDevAcInShow;
				}
			}
			else if(eInOutState >= IOS_STARTING)
				S_bDevAcInShow = true;
			else
			   	S_bDevAcInShow = false;
		}
		break;
		
		case DEV_TYPE_PV:
		{
			eState = (DevState_E)ucState;

			//0.5S频率闪烁
			if(eState == DS_ERR)
			{
				static uint32_t s_ul_last_tick = 0;
				uint32_t ul_now_tick = xTaskGetTickCount();
				
				if(ul_now_tick - s_ul_last_tick >= 500)
				{
					s_ul_last_tick = ul_now_tick;
					S_bDevPvShow = !S_bDevPvShow;
				}
			}
			else if(eState >= DS_BOOTING)
				S_bDevPvShow = true;
			else
			   	S_bDevPvShow = false;
		}
		break;
		
		case DEV_TYPE_USB:
		{
			eState = (DevState_E)ucState;

			//0.5S频率闪烁
			if(eState == DS_ERR)
			{
				static uint32_t s_ul_last_tick = 0;
				uint32_t ul_now_tick = xTaskGetTickCount();
				
				if(ul_now_tick - s_ul_last_tick >= 500)
				{
					s_ul_last_tick = ul_now_tick;
					S_bDevUsbShow = !S_bDevUsbShow;
				}
			}
			else if(eState >= DS_BOOTING)
				S_bDevUsbShow = true;
			else
			   	S_bDevUsbShow = false;
		}
		break;
		
		case DEV_TYPE_DC:
		{
			eState = (DevState_E)ucState;

			//0.5S频率闪烁
			if(eState == DS_ERR)
			{
				static uint32_t s_ul_last_tick = 0;
				uint32_t ul_now_tick = xTaskGetTickCount();
				
				if(ul_now_tick - s_ul_last_tick >= 500)
				{
					s_ul_last_tick = ul_now_tick;
					S_bDevDcShow = !S_bDevDcShow;
				}
			}
			else if(eState >= DS_BOOTING)
				S_bDevDcShow = true;
			else
			   	S_bDevDcShow = false;
		}
		break;
		
		case DEV_TYPE_LIGHT:
		{
			eState = (DevState_E)ucState;

			if(eState >= DS_WORK)
				S_bDevLightShow = true;
			else
			   	S_bDevLightShow = false;
		}
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

/***********************************************************************************************************************
-----函数功能    更新设备参数
-----说明(备注)  这里只需要更新数据,eez_ui里面会根据数据变化自动更新显示;但是DevStateIcon需要单独更新,在user_ui中实现
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_UpdateDevParam(void)
{
    char cStr[10];

    #if (boardBMS_EN)
    snprintf(cStr, sizeof(cStr), "%d", tBmsRx.usSOC);
    set_var_uca_bat_soc_value(cStr);

    u16 usTotalMinutes;
    if (tBms.eWorkState == BWS_CHG)
        usTotalMinutes = tBmsRx.usChgFullTime;
    else
        usTotalMinutes = tBmsRx.usDisChgEmptyTime;
    v_disp_work_format_remaining_time(cStr, sizeof(cStr), usTotalMinutes);
    set_var_uca_remaining_usage_time(cStr);
    #endif // boardBMS_EN

    #if (boardUSB_EN)
    vDisp_SetDevStateIcon(DEV_TYPE_USB, tUsb.eDevState);
    #endif // boardUSB_EN

    #if (boardDC_EN)
    vDisp_SetDevStateIcon(DEV_TYPE_DC, tDc.eDevState);
    #endif // boardDC_EN

    #if (boardLIGHT_EN)
    vDisp_SetDevStateIcon(DEV_TYPE_LIGHT, tLight.eDevState);
    #endif // boardLIGHT_EN

    #if (boardMPPT_EN)
    vDisp_SetDevStateIcon(DEV_TYPE_PV, tMppt.eDevState);
    #endif // boardMPPT_EN

    #if (boardDCAC_EN)
    vDisp_SetDevStateIcon(DEV_TYPE_AC_OUT, tDcac.eDisChgState);
    vDisp_SetDevStateIcon(DEV_TYPE_AC_IN, tDcac.eChgState);
    #endif // boardDCAC_EN
    
    snprintf(cStr, sizeof(cStr), "%d", tSysInfo.usOutPwr);
    set_var_uca_out_pwr_value(cStr);

    snprintf(cStr, sizeof(cStr), "%d", tSysInfo.usInPwr);
    set_var_uca_in_pwr_value(cStr);

    // 故障码轮询显示: 切换间隙(返回100)时不更新错误码文本,
    // 避免将"100"字符串显示到标签上, 同时配合 user_ui 的可见性控制实现闪烁效果
    u16 us_err_code = usDisp_ErrCodeDisplay();
    if (us_err_code != 100)
    {
        snprintf(cStr, sizeof(cStr), "E%d", us_err_code);
        set_var_uca_err_code_value(cStr);
    }
}

#endif  //boardDISPLAY_EN
