/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示工程模式 - TFT+LVGL版本                                           *
 *                                                                                                                *
 *  段码屏版本的显示函数 vDisp_EnginModeDis 已废弃, TFT+LVGL 工程模式下由 eng_mode_ui.c 接管。                    *
 *  本文件仅保留 vDisp_MemParamSet (LCD 记忆参数调整) 和 vDisp_TypeSelect (亮度类型选择) 供其他模块调用。            *
 *                                                                                                                *
******************************************************************************************************************/
#include "MD_Display/md_display_eng_mode.h"

#if(boardENG_MODE_EN && boardDISPLAY_EN)
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_eng.h"
#include "MD_Display/md_display_task.h"

#include "app_info.h"


/***********************************************************************************************************************
-----函数功能    工程模式显示函数 (段码屏版已废弃)
-----说明(备注)  TFT+LVGL 模式下, 工程模式 UI 由 eng_mode_ui.c 中的 vEngMode_UiCreate/UitTick/UiDelete 接管,
                本函数保留为空实现以兼容头文件声明。
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_EnginModeDis(void)
{
    /* TFT+LVGL 模式: 工程模式 UI 由 eng_mode_ui.c 接管, 本函数不再使用 */
}


/*****************************************************************************************************************
-----函数功能    选择设置的类型
-----说明(备注)  切换亮度设置类型: 高亮 / 低亮 / 息屏时间
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vDisp_TypeSelect(void)
{
	tDisp.eLightSetType++;
	if(tDisp.eLightSetType == LTS_NULL)
		tDisp.eLightSetType = LTS_HIGH;
}

/*****************************************************************************************************************
-----函数功能    设置记忆参数 (LCD)
-----说明(备注)  根据 tEngMode.ucEngModeItem 调整 LCD 显示参数
-----传入参数    add:true 增加   false:减少
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vDisp_MemParamSet(bool add)
{
	if(add == true)
	{
		if(tEngMode.ucEngModeItem == 0)
		{
			if(tAppMemParam.tDISP.ucHighLightValue < 0x8F)
				tAppMemParam.tDISP.ucHighLightValue++;
		}
		else if(tEngMode.ucEngModeItem == 1)
		{
			if(tAppMemParam.tDISP.ucLowLightValue < 0x8F)
				tAppMemParam.tDISP.ucLowLightValue++;
		}
		else if(tEngMode.ucEngModeItem == 2)
		{
			tAppMemParam.tDISP.usAutoOffTime++;
		}
		
	}
	else 
	{
		if(tEngMode.ucEngModeItem == 0)
		{
			if(tAppMemParam.tDISP.ucHighLightValue > 0x88)
				tAppMemParam.tDISP.ucHighLightValue--;
		}
		else if(tEngMode.ucEngModeItem == 1)
		{
			if(tAppMemParam.tDISP.ucLowLightValue > 0x88)
				tAppMemParam.tDISP.ucLowLightValue--;
		}
		else if(tEngMode.ucEngModeItem == 2)
		{
			tAppMemParam.tDISP.usAutoOffTime--;
		}
	}
}
#endif  //boardDISPLAY_EN  && boardENG_MODE_EN
