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
#include "main_1_ui.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/user_ui/energy_ring.h"
#include "MD_Display/user_ui/img_breath.h"
#include "Sys/sys_task.h"

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
static ImgBreath_T s_tImgOutBreath;
static bool S_bMain1InitialFinish = false;

//****************************************************Function Declaration****************************************************//

/***********************************************************************************************************************
-----函数功能    设置图标可见性
-----传入参数    obj: 图标对象
-----传入参数    img_src: 图标图片资源
-----传入参数    visible: 是否可见
-----备注        indent:LJD
-----日期        2026-05-28
************************************************************************************************************************/
static void v_disp_set_icon_visible(lv_obj_t *obj, const lv_img_dsc_t *img_src, bool visible)
{
	if(obj == NULL)
		return;

	if(visible)
	{
		lv_image_set_src(obj, img_src);
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
bool b_disp_update_all_dev_states(void)
{
    bool b_ret = false;

	extern objects_t objects;
	extern const lv_img_dsc_t img_icon_out;
	extern const lv_img_dsc_t img_icon_in;
	extern const lv_img_dsc_t img_icon_w;
	extern const lv_img_dsc_t img_icon_usb;
	extern const lv_img_dsc_t img_icon_dc;
	
	// 静态变量记录上一次的状态
	static bool s_last_dev_ac_out_state = true;
	static bool s_last_dev_ac_in_state = true;
	static bool s_last_dev_pv_state = true;
	static bool s_last_dev_usb_state = true;
	static bool s_last_dev_dc_state = true;
	
	// 更新AC输出设备状态 (main屏幕)
	if(s_last_dev_ac_out_state != S_bDevAcOutState)
	{
		s_last_dev_ac_out_state = S_bDevAcOutState;
		v_disp_set_icon_visible(objects.b_dev_ac_out_state, &img_icon_out, S_bDevAcOutState);
        b_ret = true;
	}

	// 更新AC输入设备状态 (main屏幕)
	if(s_last_dev_ac_in_state != S_bDevAcInState)
	{
		s_last_dev_ac_in_state = S_bDevAcInState;
		v_disp_set_icon_visible(objects.b_dev_ac_in_state, &img_icon_in, S_bDevAcInState);
        b_ret = true;
	}
	
	// 更新PV设备状态 (main_2, main_3屏幕)
	if(s_last_dev_pv_state != S_bDevPvState)
	{
		s_last_dev_pv_state = S_bDevPvState;
		v_disp_set_icon_visible(objects.b_dev_pv_state, &img_icon_w, S_bDevPvState);
        b_ret = true;
	}

	// 更新USB设备状态 (main屏幕)
	if(s_last_dev_usb_state != S_bDevUsbState)
	{
		s_last_dev_usb_state = S_bDevUsbState;
		v_disp_set_icon_visible(objects.b_dev_usb_state, &img_icon_usb, S_bDevUsbState);
        b_ret = true;
	}
	
	// 更新DC设备状态 (main_3屏幕)
	if(s_last_dev_dc_state != S_bDevDcState)
	{
		s_last_dev_dc_state = S_bDevDcState;
		v_disp_set_icon_visible(objects.b_dev_dc_state, &img_icon_dc, S_bDevDcState);
        b_ret = true;
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
		ImgBreath_StartImgOut(&s_tImgOutBreath, 500, 30, 255);
		S_bMain1InitialFinish = true;
	}

	EnergyRing_UpdateSoc(&s_tEnergyRing, 50, true);
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
	ImgBreath_StopImgOut(&s_tImgOutBreath);
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
    
    b_ret = b_disp_update_all_dev_states();

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

#endif  //boardDISPLAY_EN
