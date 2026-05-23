/*****************************************************************************************************************
*                                                                                                                *
 *                                         Disp显示任务                                                          *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_task.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_iface.h"
#include "MD_Display/md_display_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "app_info.h"


#if(boardKEY_EN)
#include "Key/key_task.h"
#endif  //boardKEY_EN

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //boardUSB_EN

#if(boardDC_EN)
#include "Dc/dc_task.h"
#endif  //boardDC_EN

#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif  //boardLIGHT_EN

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN


#if(boardUPDATA)
#include "Sys/sys_queue_task_updata.h"
#endif  //boardUPDATA

#if(boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_hm_task.h"
#endif  //boardHEAT_MANAGE_EN

#if(boardWATER_PUMP_EN)
#include "Pump/pump_task.h"
#endif

#if(boardO2PUMP_EN)
#include "O2Pump/o2pump_task.h"
#endif


//****************************************************任务参数初始化**********************************************//
#if(boardUSE_OS)
#define			dispTASK_PRIO                   2       //任务优先级 
#define			dispTASK_STK_SIZE               256     //任务堆栈  实际字节数 *4
TaskHandle_t tDispTaskHandler = NULL; 
void vDisp_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
Disp_T tDisp; 
u8g2_t u8g2;  // 显示器初始化结构体
bool g_bDispPageDirty = true;   //页面脏标志
DispPageCtx_T tDispPageCtx;

//****************************************************局部函数定义************************************************//
static void v_disp_param_init(void);
static DispPageId_E e_disp_state_to_page(DevState_E state);
static void v_disp_setting_cache_load(void);
static void v_disp_setting_cache_load_default(void);
static void v_disp_page_set_hint(const char *msg);
static void v_disp_apply_setting_runtime(void);
static bool b_disp_save_setting_cache(void);
static bool b_disp_toggle_alarm_buzzer(void);
static bool b_disp_adjust_setting_item(DispSettingItem_E item, bool add);
static bool b_disp_is_work_page(DispPageId_E page_id);
static void v_disp_home_update_visible_window(void);
static void v_disp_text_scroll_reset(void);
static void v_disp_home_save_restore_point(void);
static void v_disp_home_restore_from_point(void);

/***********************************************************************************************************************
-----函数功能    复位文本滚动状态
-----说明(备注)  页面、焦点或文本内容变化时清空滚动参数
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_text_scroll_reset(void)
{
	memset(&tDispPageCtx.tTextScroll, 0, sizeof(tDispPageCtx.tTextScroll));
}

/***********************************************************************************************************************
-----函数功能    保存主页恢复点
-----说明(备注)  进入错误页前保存当前工作页位置和焦点信息
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_home_save_restore_point(void)
{
	tDispPageCtx.tHomeRestore.ePageId = tDispPageCtx.ePageId;
	tDispPageCtx.tHomeRestore.eMainPage = tDispPageCtx.eMainPage;
	tDispPageCtx.tHomeRestore.eHomeMode = tDispPageCtx.eHomeMode;
	tDispPageCtx.tHomeRestore.eHomeTab = tDispPageCtx.eHomeTab;
	tDispPageCtx.tHomeRestore.eHomeModule = tDispPageCtx.eHomeModule;
	tDispPageCtx.tHomeRestore.ucTabVisibleStart = tDispPageCtx.ucTabVisibleStart;
	tDispPageCtx.tHomeRestore.ucTabItemIndex = tDispPageCtx.ucTabItemIndex;
	tDispPageCtx.tHomeRestore.ucFieldIndex = tDispPageCtx.ucFieldIndex;
	tDispPageCtx.tHomeRestore.ucAdcGroupIndex = tDispPageCtx.ucAdcGroupIndex;
	tDispPageCtx.tHomeRestore.bEditing = tDispPageCtx.bEditing;
}

/***********************************************************************************************************************
-----函数功能    恢复主页位置
-----说明(备注)  错误解除后恢复进入错误页前的页面位置和焦点
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_home_restore_from_point(void)
{
	u8 focus_index;

	if(tDispPageCtx.tHomeRestore.ePageId == DPI_NONE)
		return;

	tDispPageCtx.eMainPage = tDispPageCtx.tHomeRestore.eMainPage;
	tDispPageCtx.eHomeMode = tDispPageCtx.tHomeRestore.eHomeMode;
	tDispPageCtx.eHomeTab = tDispPageCtx.tHomeRestore.eHomeTab;
	tDispPageCtx.eHomeModule = tDispPageCtx.tHomeRestore.eHomeModule;
	tDispPageCtx.ucTabVisibleStart = tDispPageCtx.tHomeRestore.ucTabVisibleStart;
	tDispPageCtx.ucTabItemIndex = tDispPageCtx.tHomeRestore.ucTabItemIndex;
	tDispPageCtx.ucFieldIndex = tDispPageCtx.tHomeRestore.ucFieldIndex;
	tDispPageCtx.ucAdcGroupIndex = tDispPageCtx.tHomeRestore.ucAdcGroupIndex;
	tDispPageCtx.bEditing = tDispPageCtx.tHomeRestore.bEditing;
	focus_index = (tDispPageCtx.ePageId == DPI_HOME) ? (u8)tDispPageCtx.eHomeModule : tDispPageCtx.ucFieldIndex;
	if(focus_index > 3U)
		focus_index = 3U;
	tDispPageCtx.eFocusId = (DispFocusId_E)(DFI_CARD_1 + focus_index);
	v_disp_home_update_visible_window();
	v_disp_text_scroll_reset();
}

/***********************************************************************************************************************
 -----函数功能    参数初始化
 -----说明(备注)  none
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_disp_param_init(void)
{
	memset(&tDisp, 0, sizeof(tDisp));
	memset(&tDispPageCtx, 0, sizeof(tDispPageCtx));
	
	tDisp.eDevState = DS_INIT;
	tDisp.usAutoOffTime = boardDISP_OFF_TIME;
	tDisp.bSleepShow =true;//待机强制打开亮屏
	vDisp_PageInitContext();
}

/***********************************************************************************************************************
-----函数功能    设备状态转换显示页面
-----说明(备注)  根据系统设备状态选择对应显示页面
-----传入参数    state:设备状态
-----输出参数    none
-----返回值      显示页面ID
************************************************************************************************************************/
static DispPageId_E e_disp_state_to_page(DevState_E state)
{
	switch(state)
	{
		case DS_INIT:
			return DPI_INIT;

		case DS_BOOTING:
			return DPI_BOOTING;

		case DS_WORK:
			return DPI_HOME;

		case DS_ERR:
			return DPI_ERROR;

		case DS_UPDATA_MODE:
			return DPI_UPGRADE;

		case DS_CLOSING:
			return DPI_CLOSING;

		case DS_SHUT_DOWN:
			return DPI_SLEEP;

		default:
			return DPI_HOME;
	}
}

/***********************************************************************************************************************
-----函数功能    加载设置缓存
-----说明(备注)  从APP记忆参数读取显示设置到临时编辑缓存
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_setting_cache_load(void)
{
	tDispPageCtx.tSettingCache.usSleepTime = tAppMemParam.tDISP.usAutoOffTime;
	tDispPageCtx.tSettingCache.bBuzOff = tAppMemParam.tSYS.bBuzSwitchOff;
	tDispPageCtx.tSettingCache.ucHighLightValue = tAppMemParam.tDISP.ucHighLightValue;
	tDispPageCtx.tSettingCache.ucLowLightValue = tAppMemParam.tDISP.ucLowLightValue;
	tDispPageCtx.tSettingCache.bRestoreDefault = false;
	tDispPageCtx.tSettingCache.bCacheValid = true;
	tDispPageCtx.tSettingCache.bDirty = false;
}

/***********************************************************************************************************************
-----函数功能    加载默认设置缓存
-----说明(备注)  将显示和系统默认参数写入临时编辑缓存
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_setting_cache_load_default(void)
{
	DispMemParam_T t_disp_def = {0};
	SysMemParam_T t_sys_def = {0};

	bDisp_MemParamInit(&t_disp_def);
	bSys_MemParamInit(&t_sys_def);

	tDispPageCtx.tSettingCache.usSleepTime = t_disp_def.usAutoOffTime;
	tDispPageCtx.tSettingCache.bBuzOff = t_sys_def.bBuzSwitchOff;
	tDispPageCtx.tSettingCache.ucHighLightValue = t_disp_def.ucHighLightValue;
	tDispPageCtx.tSettingCache.ucLowLightValue = t_disp_def.ucLowLightValue;
	tDispPageCtx.tSettingCache.bRestoreDefault = true;
	tDispPageCtx.tSettingCache.bCacheValid = true;
	tDispPageCtx.tSettingCache.bDirty = true;
}

/***********************************************************************************************************************
-----函数功能    设置页面提示
-----说明(备注)  设置底部短提示并标记内容和底部提示区域刷新
-----传入参数    msg:提示字符串
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_page_set_hint(const char *msg)
{
	memset(tDispPageCtx.acHint, 0, sizeof(tDispPageCtx.acHint));
	if(msg != NULL)
		strncpy(tDispPageCtx.acHint, msg, sizeof(tDispPageCtx.acHint) - 1U);
	tDispPageCtx.usHintCnt = 3U;
	tDispPageCtx.usDirtyMask |= DDM_CONTENT | DDM_BOTTOM;
	g_bDispPageDirty = true;
}

/***********************************************************************************************************************
-----函数功能    应用运行设置
-----说明(备注)  把编辑缓存应用到运行参数和APP记忆参数结构体
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_apply_setting_runtime(void)
{
	tAppMemParam.tDISP.usAutoOffTime = tDispPageCtx.tSettingCache.usSleepTime;
	tAppMemParam.tDISP.ucHighLightValue = tDispPageCtx.tSettingCache.ucHighLightValue;
	tAppMemParam.tDISP.ucLowLightValue = tDispPageCtx.tSettingCache.ucLowLightValue;
	tAppMemParam.tSYS.bBuzSwitchOff = tDispPageCtx.tSettingCache.bBuzOff;

	tDisp.usAutoOffTime = tAppMemParam.tDISP.usAutoOffTime;
	if(tDisp.usAutoOffTime > 0U)
		tDisp.usAutoOffCnt = tDisp.usAutoOffTime;
	vDisp_SetContrast(tAppMemParam.tDISP.ucHighLightValue);
}

/***********************************************************************************************************************
-----函数功能    保存设置缓存
-----说明(备注)  保存显示和系统记忆参数, 两个参数块都成功才认为保存成功
-----传入参数    none
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_save_setting_cache(void)
{
	s16 disp_ret;
	s16 sys_ret;

	v_disp_apply_setting_runtime();
	disp_ret = cApp_UpdataMemParam(tDispMemParamStr);
	sys_ret = cApp_UpdataMemParam(tSysMemParamStr);

	if(disp_ret > 0 && sys_ret > 0)
	{
		tDispPageCtx.tSettingCache.bDirty = false;
		tDispPageCtx.tSettingCache.bRestoreDefault = false;
		v_disp_page_set_hint("SAVE OK");
		return true;
	}

	v_disp_page_set_hint("SAVE ERR");
	return false;
}

/***********************************************************************************************************************
-----函数功能    切换报警蜂鸣器
-----说明(备注)  错误页切换报警静音状态, 保存失败时回滚原状态
-----传入参数    none
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_toggle_alarm_buzzer(void)
{
	bool buz_off_next = !tAppMemParam.tSYS.bBuzSwitchOff;
	s16 sys_ret;

	tAppMemParam.tSYS.bBuzSwitchOff = buz_off_next;
	if(tDispPageCtx.tSettingCache.bCacheValid == true)
		tDispPageCtx.tSettingCache.bBuzOff = buz_off_next;

	sys_ret = cApp_UpdataMemParam(tSysMemParamStr);
	if(sys_ret > 0)
	{
		v_disp_page_set_hint(buz_off_next ? "ALARM MUTE" : "BUZZER ON");
		return true;
	}

	tAppMemParam.tSYS.bBuzSwitchOff = !buz_off_next;
	if(tDispPageCtx.tSettingCache.bCacheValid == true)
		tDispPageCtx.tSettingCache.bBuzOff = tAppMemParam.tSYS.bBuzSwitchOff;
	v_disp_page_set_hint("MUTE ERR");
	return false;
}

/***********************************************************************************************************************
-----函数功能    调整设置项目
-----说明(备注)  只修改设置缓存中的字段, 保存前不写入实际记忆参数
-----传入参数    item:设置项  add:true增加 false减少
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_adjust_setting_item(DispSettingItem_E item, bool add)
{
	if(tDispPageCtx.tSettingCache.bCacheValid == false)
		v_disp_setting_cache_load();

	switch(item)
	{
		case DSI_SLEEP:
			if(add == true)
			{
				if(tDispPageCtx.tSettingCache.usSleepTime < 3600U)
					tDispPageCtx.tSettingCache.usSleepTime += 30U;
			}
			else if(tDispPageCtx.tSettingCache.usSleepTime >= 30U)
				tDispPageCtx.tSettingCache.usSleepTime -= 30U;
			break;

		case DSI_BUZZER:
			tDispPageCtx.tSettingCache.bBuzOff = !tDispPageCtx.tSettingCache.bBuzOff;
			break;

		case DSI_HIGH_LIGHT:
			if(add == true)
			{
				if(tDispPageCtx.tSettingCache.ucHighLightValue < 250U)
					tDispPageCtx.tSettingCache.ucHighLightValue += 5U;
			}
			else if(tDispPageCtx.tSettingCache.ucHighLightValue > 5U)
				tDispPageCtx.tSettingCache.ucHighLightValue -= 5U;
			break;

		case DSI_LOW_LIGHT:
			if(add == true)
			{
				if(tDispPageCtx.tSettingCache.ucLowLightValue < 250U)
					tDispPageCtx.tSettingCache.ucLowLightValue += 5U;
			}
			else if(tDispPageCtx.tSettingCache.ucLowLightValue > 5U)
				tDispPageCtx.tSettingCache.ucLowLightValue -= 5U;
			break;

		case DSI_RESET:
			tDispPageCtx.tSettingCache.bRestoreDefault = !tDispPageCtx.tSettingCache.bRestoreDefault;
			break;

		default:
			return false;
	}

	tDispPageCtx.tSettingCache.bDirty = true;
	tDispPageCtx.usDirtyMask |= DDM_CONTENT | DDM_BOTTOM;
	g_bDispPageDirty = true;
	return true;
}

/***********************************************************************************************************************
-----函数功能    判断工作页面
-----说明(备注)  判断页面是否属于可恢复和可周期刷新实时数据的工作页面
-----传入参数    page_id:页面ID
-----输出参数    none
-----返回值      true:是 false:否
************************************************************************************************************************/
static bool b_disp_is_work_page(DispPageId_E page_id)
{
	switch(page_id)
	{
		case DPI_HOME:
		case DPI_LIGHT:
		case DPI_HEAT:
		case DPI_WPUMP:
		case DPI_O2PUMP:
		case DPI_SETTING:
		case DPI_ADC:
			return true;

		default:
			break;
	}

	return false;
}

/***********************************************************************************************************************
-----函数功能    判断详情页面
-----说明(备注)  判断页面是否为照明、温控、水泵或氧气泵详情页
-----传入参数    page_id:页面ID
-----输出参数    none
-----返回值      true:是 false:否
************************************************************************************************************************/
static bool b_disp_is_detail_page(DispPageId_E page_id)
{
	return (page_id == DPI_LIGHT || page_id == DPI_HEAT || page_id == DPI_WPUMP || page_id == DPI_O2PUMP);
}

/***********************************************************************************************************************
-----函数功能    主页面转页面ID
-----说明(备注)  将主页面枚举映射为实际显示页面ID
-----传入参数    main_page:主页面枚举
-----输出参数    none
-----返回值      显示页面ID
************************************************************************************************************************/
static DispPageId_E e_disp_main_page_id(DispMainPage_E main_page)
{
	switch(main_page)
	{
		case DMP_SETTING: return DPI_SETTING;
		case DMP_ADC: return DPI_ADC;
		case DMP_HOME:
		default: break;
	}

	return DPI_HOME;
}

/***********************************************************************************************************************
-----函数功能    页面ID转主页面
-----说明(备注)  根据页面ID反推所属主页面分组
-----传入参数    page_id:页面ID
-----输出参数    none
-----返回值      主页面枚举
************************************************************************************************************************/
static DispMainPage_E e_disp_page_to_main(DispPageId_E page_id)
{
	switch(page_id)
	{
		case DPI_SETTING: return DMP_SETTING;
		case DPI_ADC: return DMP_ADC;
		default: break;
	}

	return DMP_HOME;
}

/***********************************************************************************************************************
-----函数功能    页面ID转主页模块
-----说明(备注)  根据详情页ID反推对应主页模块
-----传入参数    page_id:页面ID
-----输出参数    none
-----返回值      主页模块枚举
************************************************************************************************************************/
static DispHomeModule_E e_disp_page_to_home_module(DispPageId_E page_id)
{
	switch(page_id)
	{
		case DPI_HEAT: return DHM_HEAT;
		case DPI_WPUMP: return DHM_WPUMP;
		case DPI_O2PUMP: return DHM_O2PUMP;
		case DPI_LIGHT:
		default: break;
	}

	return DHM_LIGHT;
}

/***********************************************************************************************************************
-----函数功能    主页模块转页面ID
-----说明(备注)  根据主页模块选择对应详情页ID
-----传入参数    module:主页模块
-----输出参数    none
-----返回值      显示页面ID
************************************************************************************************************************/
static DispPageId_E e_disp_home_module_page_id(DispHomeModule_E module)
{
	switch(module)
	{
		case DHM_HEAT: return DPI_HEAT;
		case DHM_WPUMP: return DPI_WPUMP;
		case DHM_O2PUMP: return DPI_O2PUMP;
		case DHM_LIGHT:
		default: break;
	}

	return DPI_LIGHT;
}

/***********************************************************************************************************************
-----函数功能    标记内容刷新
-----说明(备注)  焦点或数值变化时标记内容和底部提示区域重绘
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_mark_content_dirty(void)
{
	tDispPageCtx.usDirtyMask |= DDM_CONTENT | DDM_BOTTOM;
	g_bDispPageDirty = true;
	v_disp_text_scroll_reset();
}

/***********************************************************************************************************************
-----函数功能    设置字段索引
-----说明(备注)  同步字段索引、标签项目索引和卡片焦点
-----传入参数    field_index:字段索引
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_set_field_index(u8 field_index)
{
	u8 focus_index = field_index;

	tDispPageCtx.ucFieldIndex = field_index;
	tDispPageCtx.ucTabItemIndex = field_index;
	if(focus_index > 3U)
		focus_index = 3U;
	tDispPageCtx.eFocusId = (DispFocusId_E)(DFI_CARD_1 + focus_index);
	v_disp_mark_content_dirty();
}

/***********************************************************************************************************************
-----函数功能    获取页面字段数量
-----说明(备注)  返回当前页面可循环切换的字段数量
-----传入参数    page_id:页面ID
-----输出参数    none
-----返回值      字段数量
************************************************************************************************************************/
static u8 uc_disp_page_field_count(DispPageId_E page_id)
{
	switch(page_id)
	{
		case DPI_HOME: return (u8)DHM_COUNT;
		case DPI_LIGHT:
			return 8U;
		case DPI_WPUMP:
		case DPI_O2PUMP:
			return 4U;
		case DPI_HEAT:
			return 6U;
		case DPI_SETTING:
		case DPI_ADC:
			return 5U;
		default:
			break;
	}

	return 1U;
}

/***********************************************************************************************************************
-----函数功能    丢弃未保存设置
-----说明(备注)  离开设置页且缓存已修改时重新加载记忆参数
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_discard_setting_if_dirty(void)
{
	if(tDispPageCtx.ePageId == DPI_SETTING && tDispPageCtx.tSettingCache.bDirty == true)
	{
		v_disp_setting_cache_load();
		v_disp_page_set_hint("DISCARD");
	}
}

/***********************************************************************************************************************
-----函数功能    切换主页面
-----说明(备注)  在主页、设置页、ADC页之间切换, 必要时丢弃未保存设置缓存
-----传入参数    main_page:主页面ID
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_switch_main_page(DispMainPage_E main_page)
{
	if(main_page >= DMP_COUNT)
		main_page = DMP_HOME;

	v_disp_discard_setting_if_dirty();
	tDispPageCtx.eMainPage = main_page;
	return bDisp_PageSwitch(e_disp_main_page_id(main_page), false);
}

/***********************************************************************************************************************
-----函数功能    循环切换主页面
-----说明(备注)  左右键在主页、设置页和ADC页之间循环切换
-----传入参数    add:true下一个 false上一个
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_cycle_main_page(bool add)
{
	DispMainPage_E main_page = tDispPageCtx.eMainPage;

	if(add == true)
		main_page = (main_page < (DispMainPage_E)(DMP_COUNT - 1U)) ? (DispMainPage_E)(main_page + 1U) : DMP_HOME;
	else
		main_page = (main_page > DMP_HOME) ? (DispMainPage_E)(main_page - 1U) : (DispMainPage_E)(DMP_COUNT - 1U);

	return b_disp_switch_main_page(main_page);
}

/***********************************************************************************************************************
-----函数功能    选择主页模块
-----说明(备注)  移动主页四宫格选择, 并同步标签和模块索引
-----传入参数    add:true下一个 false上一个
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_select_home_module(bool add)
{
	if(add == true)
		tDispPageCtx.eHomeModule = (tDispPageCtx.eHomeModule < (DispHomeModule_E)(DHM_COUNT - 1U)) ?
			(DispHomeModule_E)(tDispPageCtx.eHomeModule + 1U) : DHM_LIGHT;
	else
		tDispPageCtx.eHomeModule = (tDispPageCtx.eHomeModule > DHM_LIGHT) ?
			(DispHomeModule_E)(tDispPageCtx.eHomeModule - 1U) : (DispHomeModule_E)(DHM_COUNT - 1U);

	tDispPageCtx.eHomeTab = (DispHomeTabId_E)tDispPageCtx.eHomeModule;
	tDispPageCtx.eFocusId = (DispFocusId_E)(DFI_CARD_1 + (u8)tDispPageCtx.eHomeModule);
	tDispPageCtx.ucFieldIndex = 0U;
	tDispPageCtx.ucTabItemIndex = 0U;
	v_disp_mark_content_dirty();
	return true;
}

/***********************************************************************************************************************
-----函数功能    切换字段焦点
-----说明(备注)  在当前详情页、设置页或ADC页内循环切换字段焦点
-----传入参数    add:true下一个 false上一个
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_focus_field(bool add)
{
	u8 count = uc_disp_page_field_count(tDispPageCtx.ePageId);
	u8 index = tDispPageCtx.ucFieldIndex;

	if(count <= 1U)
		return false;

	if(add == true)
		index = (index < (u8)(count - 1U)) ? (u8)(index + 1U) : 0U;
	else
		index = (index > 0U) ? (u8)(index - 1U) : (u8)(count - 1U);

	v_disp_set_field_index(index);
	return true;
}

/***********************************************************************************************************************
-----函数功能    切换ADC分组
-----说明(备注)  在ADC温度、电源、光照分组间切换, 并将行焦点复位到第一项
-----传入参数    add:true下一个 false上一个
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_adjust_adc_group(bool add)
{
	if(add == true)
		tDispPageCtx.ucAdcGroupIndex = (tDispPageCtx.ucAdcGroupIndex < 2U) ? (u8)(tDispPageCtx.ucAdcGroupIndex + 1U) : 0U;
	else
		tDispPageCtx.ucAdcGroupIndex = (tDispPageCtx.ucAdcGroupIndex > 0U) ? (u8)(tDispPageCtx.ucAdcGroupIndex - 1U) : 2U;

	v_disp_set_field_index(0U);
	switch(tDispPageCtx.ucAdcGroupIndex)
	{
		case 0U: v_disp_page_set_hint("ADC TEMP"); break;
		case 1U: v_disp_page_set_hint("ADC PWR"); break;
		default: v_disp_page_set_hint("ADC LIGHT"); break;
	}
	return true;
}

/***********************************************************************************************************************
-----函数功能    调整水泵模式
-----说明(备注)  根据按键方向循环切换水泵档位
-----传入参数    add:true下一个 false上一个
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_adjust_pump_mode(bool add)
{
	#if(boardWATER_PUMP_EN)
	switch(tPump.eMode)
	{
		case PUMP_OFF:  bPump_SetMode(add ? PUMP_LOW : PUMP_MAX); break;
		case PUMP_LOW:  bPump_SetMode(add ? PUMP_MID : PUMP_OFF); break;
		case PUMP_MID:  bPump_SetMode(add ? PUMP_HIGH : PUMP_LOW); break;
		case PUMP_HIGH: bPump_SetMode(add ? PUMP_MAX : PUMP_MID); break;
		default:        bPump_SetMode(add ? PUMP_OFF : PUMP_HIGH); break;
	}
	v_disp_page_set_hint("WP MODE");
	return true;
	#else
	return false;
	#endif
}

/***********************************************************************************************************************
-----函数功能    调整氧气泵模式
-----说明(备注)  根据按键方向循环切换氧气泵档位
-----传入参数    add:true下一个 false上一个
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_adjust_o2pump_mode(bool add)
{
	#if(boardO2PUMP_EN)
	switch(tO2Pump.eMode)
	{
		case O2PUMP_OFF:  bO2Pump_SetMode(add ? O2PUMP_LOW : O2PUMP_MAX); break;
		case O2PUMP_LOW:  bO2Pump_SetMode(add ? O2PUMP_MID : O2PUMP_OFF); break;
		case O2PUMP_MID:  bO2Pump_SetMode(add ? O2PUMP_HIGH : O2PUMP_LOW); break;
		case O2PUMP_HIGH: bO2Pump_SetMode(add ? O2PUMP_MAX : O2PUMP_MID); break;
		default:          bO2Pump_SetMode(add ? O2PUMP_OFF : O2PUMP_HIGH); break;
	}
	v_disp_page_set_hint("O2 MODE");
	return true;
	#else
	return false;
	#endif
}

/***********************************************************************************************************************
-----函数功能    调整详情页当前项目
-----说明(备注)  处理照明、温控、水泵、氧气泵详情页的左右键和确认键动作
-----传入参数    add:true增加/下一个 false减少/上一个
-----输出参数    none
-----返回值      true:成功 false:失败
************************************************************************************************************************/
static bool b_disp_adjust_detail_current(bool add)
{
	switch(tDispPageCtx.ePageId)
	{
		case DPI_LIGHT:
			#if(boardLIGHT_EN)
			if(tDispPageCtx.ucFieldIndex == 0U)
			{
				vLight_CircSelectMode();
				v_disp_page_set_hint("WHITE MODE");
				return true;
			}
			else if(tDispPageCtx.ucFieldIndex == 1U)
			{
				vLight_CircSelectRGBMode();
				v_disp_page_set_hint("RGB MODE");
				return true;
			}
			#endif
			v_disp_page_set_hint("LIGHT VIEW");
			return true;

		case DPI_HEAT:
			#if(boardHEAT_MANAGE_EN)
			{
				s16 temp;
				switch(tDispPageCtx.ucFieldIndex)
				{
					case 1U: /* HEAT开关 */
						if(bHM_IsForceOn() == true)
							cHm_Switch(HM_OBJ_HEAT, ST_OFF, true);
						else
							cHm_Switch(HM_OBJ_HEAT, ST_ON, true);
						v_disp_page_set_hint("HEAT MODE");
						return true;
					case 2U: /* HeatTargetTemp */
						temp = tHM.sHeatTargetTemp;
						if(add == true) { if(temp < 50) temp++; }
						else { if(temp > 10) temp--; }
						vHM_HeatSetTargetTemp(temp);
						v_disp_page_set_hint("HT TARGET");
						return true;
					case 3U: /* FAN开关 */
						if(tHM.bFanEnable == true)
						{
							tHM.bFanEnable = false;
							cHm_Switch(HM_OBJ_FAN, ST_OFF, true);
						}
						else
						{
							tHM.bFanEnable = true;
							cHm_Switch(HM_OBJ_FAN, ST_ON, true);
						}
						v_disp_page_set_hint("FAN MODE");
						return true;
					case 4U: /* FanTempStart */
						temp = tHM.sFanTempStart;
						if(add == true) { if(temp < tHM.sFanTempFull - 1) temp++; }
						else { if(temp > 10) temp--; }
						vHM_FanSetTargetTemp(temp, tHM.sFanTempFull);
						v_disp_page_set_hint("FAN START");
						return true;
					case 5U: /* FanTempFull */
						temp = tHM.sFanTempFull;
						if(add == true) { if(temp < 60) temp++; }
						else { if(temp > tHM.sFanTempStart + 1) temp--; }
						vHM_FanSetTargetTemp(tHM.sFanTempStart, temp);
						v_disp_page_set_hint("FAN FULL");
						return true;
					default:
						break;
				}
			}
			#endif
			v_disp_page_set_hint("HEAT VIEW");
			return true;

		case DPI_WPUMP:
			if(tDispPageCtx.ucFieldIndex == 0U)
				return b_disp_adjust_pump_mode(add);
			v_disp_page_set_hint("WP VIEW");
			return true;

		case DPI_O2PUMP:
			if(tDispPageCtx.ucFieldIndex == 0U)
				return b_disp_adjust_o2pump_mode(add);
			v_disp_page_set_hint("O2 VIEW");
			return true;

		default:
			break;
	}

	return false;
}

/***********************************************************************************************************************
-----函数功能    更新主页标签窗口
-----说明(备注)  根据当前标签位置调整标签窗口起始索引
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_home_update_visible_window(void)
{
	if(tDispPageCtx.eHomeTab < tDispPageCtx.ucTabVisibleStart)
		tDispPageCtx.ucTabVisibleStart = (u8)tDispPageCtx.eHomeTab;
	else if(tDispPageCtx.eHomeTab >= (DispHomeTabId_E)(tDispPageCtx.ucTabVisibleStart + 4U))
		tDispPageCtx.ucTabVisibleStart = (u8)(tDispPageCtx.eHomeTab - 3U);

	if(tDispPageCtx.ucTabVisibleStart > (u8)(DHT_COUNT - 4U))
		tDispPageCtx.ucTabVisibleStart = (u8)(DHT_COUNT - 4U);
}

/***********************************************************************************************************************
-----函数功能    初始化页面上下文
-----说明(备注)  复位页面、焦点、脏标志、设置缓存和提示状态
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_PageInitContext(void)
{
	memset(&tDispPageCtx, 0, sizeof(tDispPageCtx));
	tDispPageCtx.ePageId = DPI_HOME;
	tDispPageCtx.ePrevPageId = DPI_NONE;
	tDispPageCtx.eOverlayId = DOI_NONE;
	tDispPageCtx.eFocusId = DFI_CARD_1;
	tDispPageCtx.ePrevFocusId = DFI_NONE;
	tDispPageCtx.eMainPage = DMP_HOME;
	tDispPageCtx.eHomeMode = DHM_OVERVIEW;
	tDispPageCtx.eHomeTab = DHT_LIGHT;
	tDispPageCtx.eHomeModule = DHM_LIGHT;
	tDispPageCtx.usDirtyMask = DDM_FULL;
	tDispPageCtx.ucTabVisibleStart = 0U;
	tDispPageCtx.ucTabItemIndex = 0U;
	tDispPageCtx.ucFieldIndex = 0U;
	tDispPageCtx.ucAdcGroupIndex = 0U;
	tDispPageCtx.bEditing = false;
	tDispPageCtx.bOverlayLock = false;
	v_disp_text_scroll_reset();
	v_disp_setting_cache_load();
	memset(tDispPageCtx.acHint, 0, sizeof(tDispPageCtx.acHint));
	tDispPageCtx.usHintCnt = 0U;
	tDispPageCtx.tHomeRestore.ePageId = DPI_HOME;
	g_bDispPageDirty = true;
}

/***********************************************************************************************************************
-----函数功能    切换显示页面
-----说明(备注)  统一处理页面切换时的前页、焦点、编辑状态和刷新标志
-----传入参数    page_id:目标页面  force_refresh:true强制刷新
-----输出参数    none
-----返回值      true:切换成功 false:未切换
************************************************************************************************************************/
bool bDisp_PageSwitch(DispPageId_E page_id, bool force_refresh)
{
	u8 focus_index = 0U;
	bool restore_setting_cache = false;

	if((page_id == DPI_NONE) || (page_id == tDispPageCtx.ePageId && force_refresh == false))
		return false;

	if(page_id == DPI_SETTING && tDispPageCtx.ePageId == DPI_ERROR && tDispPageCtx.tHomeRestore.ePageId == DPI_SETTING)
		restore_setting_cache = true;

	tDispPageCtx.ePrevPageId = tDispPageCtx.ePageId;
	tDispPageCtx.ePrevFocusId = tDispPageCtx.eFocusId;
	tDispPageCtx.ePageId = page_id;
	tDispPageCtx.eOverlayId = DOI_NONE;
	tDispPageCtx.bEditing = false;
	v_disp_text_scroll_reset();
	tDispPageCtx.usDirtyMask = DDM_FULL;
	tDispPageCtx.eMainPage = e_disp_page_to_main(page_id);

	if(page_id == DPI_HOME)
	{
		tDispPageCtx.eHomeMode = DHM_OVERVIEW;
		tDispPageCtx.ucFieldIndex = 0U;
		tDispPageCtx.ucTabItemIndex = 0U;
		focus_index = (u8)tDispPageCtx.eHomeModule;
		tDispPageCtx.eHomeTab = (DispHomeTabId_E)tDispPageCtx.eHomeModule;
		v_disp_home_update_visible_window();
	}
	else if(b_disp_is_detail_page(page_id))
	{
		tDispPageCtx.eHomeModule = e_disp_page_to_home_module(page_id);
		tDispPageCtx.eHomeTab = (DispHomeTabId_E)tDispPageCtx.eHomeModule;
		tDispPageCtx.ucFieldIndex = 0U;
		tDispPageCtx.ucTabItemIndex = 0U;
		focus_index = 0U;
	}
	else if(page_id == DPI_SETTING)
	{
		tDispPageCtx.ucFieldIndex = 0U;
		tDispPageCtx.ucTabItemIndex = 0U;
		focus_index = 0U;
		if(restore_setting_cache == false)
			v_disp_setting_cache_load();
	}
	else if(page_id == DPI_ADC)
	{
		tDispPageCtx.ucFieldIndex = 0U;
		tDispPageCtx.ucTabItemIndex = 0U;
		focus_index = 0U;
	}

	if(focus_index > 3U)
		focus_index = 3U;
	tDispPageCtx.eFocusId = (DispFocusId_E)(DFI_CARD_1 + focus_index);
	g_bDispPageDirty = true;
	return true;
}

/***********************************************************************************************************************
-----函数功能    请求页面刷新
-----说明(备注)  设置页面脏标志, OS模式下唤醒显示任务
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_RequestRefresh(void)
{
	g_bDispPageDirty = true;

	#if(boardUSE_OS)
	if(tDispTaskHandler != NULL)
		xTaskNotifyGive(tDispTaskHandler);
	#endif
}

/***********************************************************************************************************************
-----函数功能    按设备状态同步页面
-----说明(备注)  根据系统状态切换页面, 处理错误页进入和恢复逻辑
-----传入参数    state:设备状态
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_PageSyncByState(DevState_E state)
{
	DispPageId_E page_id = e_disp_state_to_page(state);

	if(state == DS_ERR)
	{
		if(b_disp_is_work_page(tDispPageCtx.ePageId))
			v_disp_home_save_restore_point();
		tDispPageCtx.eOverlayId = DOI_ERROR;
		tDispPageCtx.bOverlayLock = true;
	}
	else
	{
		tDispPageCtx.eOverlayId = DOI_NONE;
		tDispPageCtx.bOverlayLock = false;
		if(tDispPageCtx.ePageId == DPI_ERROR && state == DS_WORK && b_disp_is_work_page(tDispPageCtx.ePrevPageId))
			page_id = tDispPageCtx.ePrevPageId;
	}

	if(page_id != tDispPageCtx.ePageId)
	{
		bDisp_PageSwitch(page_id, true);
		if(state == DS_WORK && b_disp_is_work_page(page_id))
			v_disp_home_restore_from_point();
	}
	else
		tDispPageCtx.usDirtyMask |= DDM_CONTENT;

	g_bDispPageDirty = true;
}

#if(boardKEY_EN)
/***********************************************************************************************************************
-----函数功能    显示页面按键处理
-----说明(备注)  先处理息屏唤醒, 再根据当前页面分发按键动作
-----传入参数    key_event:按键事件
-----输出参数    none
-----返回值      true:已处理 false:未处理
************************************************************************************************************************/
bool bDisp_PageHandleKey(KeyTriEvent_e key_event)
{
	if((tSysInfo.eDevState != DS_WORK) && (tSysInfo.eDevState != DS_ERR))
		return false;

	if(tDisp.bLight == false)
	{
		switch(key_event)
		{
			case KTE_ENTER_SHORT:
			case KTE_LEFT_SHORT:
			case KTE_UP_SHORT:
			case KTE_DOWN_SHORT:
			case KTE_RIGHT_SHORT:
				bDisp_Switch(ST_ON, false);
				tDispPageCtx.usDirtyMask |= DDM_FULL;
				g_bDispPageDirty = true;
				return true;

			default:
				return false;
		}
	}

	if(tDispPageCtx.bOverlayLock == true && key_event != KTE_ENTER_LONG && key_event != KTE_ENTER_SHORT)
		return false;

	if(tDispPageCtx.ePageId == DPI_ERROR)
	{
		if(key_event == KTE_ENTER_SHORT)
			return b_disp_toggle_alarm_buzzer();
		return false;
	}

	if(key_event == KTE_ENTER_LONG)
		return false;

	switch(tDispPageCtx.ePageId)
	{
		case DPI_HOME:
			switch(key_event)
			{
				case KTE_LEFT_SHORT:
					return b_disp_cycle_main_page(false);

				case KTE_RIGHT_SHORT:
					return b_disp_cycle_main_page(true);

				case KTE_UP_SHORT:
					return b_disp_select_home_module(false);

				case KTE_DOWN_SHORT:
					return b_disp_select_home_module(true);

				case KTE_ENTER_SHORT:
					return bDisp_PageSwitch(e_disp_home_module_page_id(tDispPageCtx.eHomeModule), false);

				case KTE_LEFT_LONG:
					tDispPageCtx.eHomeModule = DHM_LIGHT;
					tDispPageCtx.eHomeTab = DHT_LIGHT;
					tDispPageCtx.eFocusId = DFI_CARD_1;
					tDispPageCtx.ucFieldIndex = 0U;
					tDispPageCtx.ucTabItemIndex = 0U;
					v_disp_page_set_hint("HOME");
					return true;

				default:
					break;
			}
			break;

		case DPI_LIGHT:
		case DPI_HEAT:
		case DPI_WPUMP:
		case DPI_O2PUMP:
			switch(key_event)
			{
				case KTE_LEFT_SHORT:
					return b_disp_adjust_detail_current(false);

				case KTE_RIGHT_SHORT:
					return b_disp_adjust_detail_current(true);

				case KTE_UP_SHORT:
					return b_disp_focus_field(false);

				case KTE_DOWN_SHORT:
					return b_disp_focus_field(true);

				case KTE_ENTER_SHORT:
					return b_disp_adjust_detail_current(true);

				case KTE_LEFT_LONG:
					return b_disp_switch_main_page(DMP_HOME);

				default:
					break;
			}
			break;

		case DPI_SETTING:
			switch(key_event)
			{
				case KTE_LEFT_SHORT:
					if(tDispPageCtx.bEditing == true)
						return b_disp_adjust_setting_item((DispSettingItem_E)tDispPageCtx.ucFieldIndex, false);
					return b_disp_cycle_main_page(false);

				case KTE_RIGHT_SHORT:
					if(tDispPageCtx.bEditing == true)
						return b_disp_adjust_setting_item((DispSettingItem_E)tDispPageCtx.ucFieldIndex, true);
					return b_disp_cycle_main_page(true);

				case KTE_UP_SHORT:
					return b_disp_focus_field(false);

				case KTE_DOWN_SHORT:
					return b_disp_focus_field(true);

				case KTE_ENTER_SHORT:
					tDispPageCtx.bEditing = !tDispPageCtx.bEditing;
					v_disp_page_set_hint(tDispPageCtx.bEditing ? "EDIT CACHE" : "CACHE HOLD");
					return true;

				case KTE_LEFT_LONG:
					return b_disp_switch_main_page(DMP_HOME);

				case KTE_RIGHT_LONG:
					if(tDispPageCtx.tSettingCache.bRestoreDefault == true)
						v_disp_setting_cache_load_default();
					return b_disp_save_setting_cache();

				default:
					break;
			}
			break;

		case DPI_ADC:
			switch(key_event)
			{
				case KTE_LEFT_SHORT:
					return b_disp_adjust_adc_group(false);

				case KTE_RIGHT_SHORT:
					return b_disp_adjust_adc_group(true);

				case KTE_UP_SHORT:
					return b_disp_focus_field(false);

				case KTE_DOWN_SHORT:
					return b_disp_focus_field(true);

				case KTE_ENTER_SHORT:
					v_disp_page_set_hint("ADC FIELD");
					return true;

				case KTE_LEFT_LONG:
					return b_disp_switch_main_page(DMP_HOME);

				default:
					break;
			}
			break;

		default:
			break;
	}

	return false;
}
#endif

/***********************************************************************************************************************
 -----函数功能    Disp显示任务初始化
 -----说明(备注)  none
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
bool bDisp_TaskInit(void)
{
    v_disp_param_init();

	vDisp_IfaceInit(); // 初始化OLED硬件接口
	
	if(bDisp_QueueInit() == false)
		return false;

    #if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vDisp_Task,            // 任务函数
                (const char* )"DispTask",             // 任务名称
                (u16 ) dispTASK_STK_SIZE,             // 任务堆栈大小
                (void* )NULL,                         // 传递给任务函数的参数
                (UBaseType_t ) dispTASK_PRIO,         // 任务优先级
                (TaskHandle_t*)&tDispTaskHandler);    // 任务句柄
    #endif  //boardUSE_OS

    return true;
}

/***********************************************************************************************************************
 -----函数功能    设置显示设备运行状态
 -----说明(备注)  none
 -----传入参数    state: 设备状态
 -----输出参数    none
 -----返回值      true:操作成功   false:操作失败
 ************************************************************************************************************************/
bool bDisp_SetDevState(DevState_E state)
{
	if(tDisp.eDevState != state)
	{
		tDisp.eDevState = state;
		vDisp_PageSyncByState(state);
		if(tDisp.eDevState == DS_INIT)  //初始化
		{
			if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
				sMyPrint("bDispTask:显示任务状态为初始化\r\n");
		}
		else if(tDisp.eDevState == DS_CLOSING)  //关闭中
		{
			if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
				sMyPrint("bDispTask:显示任务状态为关闭中\r\n");
		}
		else if(tDisp.eDevState == DS_SHUT_DOWN)  //关闭
		{
			if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
				sMyPrint("bDispTask:显示任务状态为关闭\r\n");
		}
		else if(tDisp.eDevState == DS_ERR)  //错误
		{
			if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
				sMyPrint("bDispTask:显示任务状态为错误\r\n");
		}
		else if(tDisp.eDevState == DS_BOOTING)    //启动中
		{
			if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
				sMyPrint("bDispTask:显示任务状态为启动中\r\n");
		}
		else if(tDisp.eDevState == DS_WORK)    //工作
		{
			if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
				sMyPrint("bDispTask:显示任务状态为工作\r\n");
		}
		#if(boardENG_MODE_EN)
		else if(tDisp.eDevState == DS_ENG_MODE)  //工程模式
		{
			if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
				sMyPrint("bDispTask:显示任务状态为工程模式\r\n");
		}
		#endif //boardENG_MODE_EN
		else if(tDisp.eDevState == DS_UPDATA_MODE)    //升级模式
		{
			if(uPrint.tFlag.bDispTask || uPrint.tFlag.bImportant)
				sMyPrint("bDispTask:显示任务状态为升级模式\r\n");
		}
	}
	
	return true;
}

/***********************************************************************************************************************
 -----函数功能    tDisp显示任务
 -----说明(备注)  none
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vDisp_Task(void *pvParameters)
{
	static Task_T *tp_task = NULL;
	
	#if(boardUSE_OS)
	for(;;)
	#endif  //boardUSE_OS
	{
		if(tp_task == NULL)
		{
			if(tpDispTask != NULL)
				tp_task = tpDispTask;
			
			#if(boardUSE_OS)
			vTaskDelay(100);
			continue;
			#else
			return;
			#endif  //boardUSE_OS
		}
		
		if(tp_task->vp_func != NULL && tp_task->bNowRun == false)
			tp_task->vp_func(tp_task);
		else if(tp_task->vp_func == NULL || tp_task->bNowRun == true)
		{
			#if(boardUSE_OS)
			if(lwrb_get_full(&tp_task->tQueueBuff) == 0)
				ulTaskNotifyTake(pdFALSE, boardDISP_REFRESH_TMIE);
			#endif  //boardUSE_OS
			
			if(tp_task->bp_task_manage_func != NULL)
				tp_task->bp_task_manage_func(tp_task);

			if(tp_task->ucID == DTI_NULL &&
			   g_bDispPageDirty == true &&
			   tDisp.bLight == true &&
			   (tDisp.eDevState == DS_WORK || tDisp.eDevState == DS_ERR))
			{
				if(!bDisp_RenderUi())
				{
					/* U8G2模式下 bDisp_RenderUi 对 WORK/ERR 返回 false，
					   页面渲染已下沉到队列任务，需重新装载对应任务刷新 */
					switch(tDisp.eDevState)
					{
						case DS_WORK:
							cQueue_AddQueueTask(tp_task, DTI_WORK, 0, false);
							break;
						case DS_ERR:
							cQueue_AddQueueTask(tp_task, DTI_ERR, 0, false);
							break;
						default:
							break;
					}
				}
			}
		}
	}
}

/***********************************************************************************************************************
 -----函数功能    显示开关
 -----说明(备注)  none
 -----传入参数    type:类型   fore_en:强制打开
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
bool bDisp_Switch(SwitchType_E type, bool fore_en)
{
	switch(type)
	{
		case ST_ON:
			goto LoopOn;
		
		case ST_OFF:
			goto LoopOff;
		
		default:
		{
			if(tDisp.bLight == false)
			{
				LoopOn:
				if(tDisp.bLight == false)
				{
					vDisp_SetPower(true);
					vDisp_SetContrast(tAppMemParam.tDISP.ucHighLightValue);
					g_bDispPageDirty = true;
				}
				tDisp.bLight = true;
				
				//关闭息屏
				if(fore_en == true)
					tDisp.usAutoOffTime = 0;
				
				//更新显示时间
				if(tDisp.usAutoOffTime)
					tDisp.usAutoOffCnt =  tDisp.usAutoOffTime;
			}
			else 
			{
				LoopOff:
				if(tDisp.bLight)
				{
					vDisp_ClearBuffer();
					vDisp_Refresh();
					vDisp_SetPower(false);
				}
				tDisp.bLight = false;
				if(tDisp.usAutoOffTime)
					tDisp.usAutoOffCnt = tDisp.usAutoOffTime;
				g_bDispPageDirty = true;
			}
		}
		break;
	}
	
	return true;
}

/***********************************************************************************************************************
 -----函数功能    背光自动关闭计时
 -----说明(备注)  none
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vDisp_TickTimer(void) 
{
	//非工作状态下退出
	if(tSysInfo.eDevState != DS_WORK) 
		return;
	
	//非亮屏幕状态
	if(tDisp.bLight == false)   
		return;
	
	//-----自动关闭背光--------------------------------------   
	if(tDispPageCtx.usHintCnt > 0U)
	{
		tDispPageCtx.usHintCnt--;
		tDispPageCtx.usDirtyMask |= DDM_BOTTOM;
		g_bDispPageDirty = true;
	}

	if(tDisp.usAutoOffTime)
	{
		if(tDisp.usAutoOffCnt)
		{
			tDisp.usAutoOffCnt--;
			tDispPageCtx.usDirtyMask |= DDM_CONTENT;
			g_bDispPageDirty = true;
			if(tDisp.usAutoOffCnt == 0)
			{
				bDisp_Switch(ST_OFF, false);
				if(uPrint.tFlag.bDispTask|| uPrint.tFlag.bImportant)
					sMyPrint("Lcd_Task:倒计时结束,进入息屏 时间 = %dS\r\n",tDisp.usAutoOffTime);
			}
		}
	}
}

/***********************************************************************************************************************
-----函数功能    初始化显示记忆参数
-----说明(备注)  写入显示模块默认亮度和自动息屏时间
-----传入参数    p_disp_mem:显示记忆参数结构体指针
-----输出参数    none
-----返回值      true:设置成功 false:设置失败
************************************************************************************************************************/
bool bDisp_MemParamInit(DispMemParam_T* p_disp_mem)
{
	p_disp_mem->ucHighLightValue = boardDISP_HIGH_LIGHT_VALUE;
	p_disp_mem->ucLowLightValue = boardDISP_LOW_LIGHT_VALUE;
	p_disp_mem->usAutoOffTime = boardDISP_OFF_TIME;
	return true;
}

#if(boardLOW_POWER)
/***********************************************************************************************************************
-----函数功能    选择显示供电通路
-----说明(备注)  根据外部输入电源状态控制显示屏电池供电开关
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_dis_power_select( void )
{
	if(tDisp.bLight)
	{
		/*************************************电源有输入*********************************************************/
		if( tAdcSamp.usBMS_Vin >= boardBMS_MIN_VOLT )   
		{
			Disp_EN_OFF();          //关闭显示屏的电池供电
		}
		/*************************************没有电源输入*******************************************************/
		else
		{
			Disp_EN_ON();          //打开显示屏的电池供电
		}	
	}
	else 
	{
		Disp_EN_OFF();          //关闭显示屏的电池供电
	}
}


/***********************************************************************************************************************
-----函数功能    进入显示低功耗
-----说明(备注)  挂起显示任务以降低低功耗模式下的运行消耗
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vLcd_EnterLowPower(void)
{
	vTaskSuspend(tDispTaskHandler);
}

/***********************************************************************************************************************
-----函数功能    退出显示低功耗
-----说明(备注)  恢复显示任务运行
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vLcd_ExitLowPower(void)
{
	vTaskResume(tDispTaskHandler);
}
#endif //boardLOW_POWER

#endif //boardDISPLAY_EN


