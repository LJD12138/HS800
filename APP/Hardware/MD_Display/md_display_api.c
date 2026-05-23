/*******************************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\2-User_Projects\3-SmartFishJar\1.software\SmartFishJar\APP\Hardware\MD_Display
 * File    : md_display_api.c
 * Date    : 2026-04-28 15:59:09
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.驱动IC为SSD1106，使用SPI接口，分辨率128x64，单色显示。需要实现基本的像素点控制、字符显示、图形绘制等功能。
 * 2.优化显示性能，减少刷新时间，提升用户体验。
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_api.h"

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_iface.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_queue_task.h"

#include "app_info.h"

#include "Sys/sys_task.h"

#if(boardADC_EN)
#include "Adc/adc_task.h"
#endif

#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif

#if(boardWATER_PUMP_EN)
#include "Pump/pump_task.h"
#endif

#if(boardO2PUMP_EN)
#include "O2Pump/o2pump_task.h"
#endif

#if(boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_hm_task.h"
#include "MD_HeatManage/md_hm_iface.h"
#endif

#include "MD_Display/icon_bitmaps.h"

#if(boardUPDATA)
#include "Sys/sys_queue_task_updata.h"
#endif

#include <stdio.h>
#include <string.h>

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS


//****************************************************Macros*******************************************************************//
#define DISP_HEAT_TARGET_TEMP_C               25
#define DISP_HEAT_STRONG_TEMP_C               22



//****************************************************Parameter Initialization************************************************//
#if(dispUSE_U8G2 == 0)
static u8 s_oled_gram[OLED_PAGE_COUNT][OLED_WIDTH_PIXELS];
#else
//页面渲染数据缓存, 在bDisp_RenderUi开始时刷新
static DispUiSnapshot_T s_tDispUiSnapshot;
static unsigned char u8g_logo_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x20, 0x04, 0x08, 0x06, 0x00, 0x00, 0x00, 0x80, 0xFF, 0x0F, 0x7C, 0xDF, 0x07, 0x00, 0x00, 0x00,
    0xA0, 0xFF, 0x08, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x10, 0x04, 0x51, 0x04, 0x00, 0x00, 0x00, 0x90, 0x00, 0x88,
    0x1F, 0x00, 0x00, 0x00, 0x40, 0xDB, 0x16, 0x04, 0x51, 0x00, 0x00, 0x00, 0x00, 0x98, 0x04, 0xBF, 0x10, 0x00, 0x00,
    0x00, 0x40, 0xDB, 0x76, 0x64, 0x9F, 0x03, 0x00, 0x00, 0x00, 0x98, 0x24, 0x88, 0x10, 0x00, 0x00, 0x00, 0x40, 0xDB,
    0x76, 0x44, 0x01, 0x04, 0x00, 0x00, 0x00, 0x94, 0x22, 0x88, 0x10, 0x00, 0x00, 0x00, 0x40, 0xDB, 0x76, 0x44, 0x41,
    0x04, 0x00, 0x00, 0x00, 0x90, 0xFE, 0x98, 0x10, 0x00, 0x00, 0x00, 0x40, 0xDB, 0x76, 0x7C, 0xC1, 0x07, 0x00, 0x00,
    0x00, 0x90, 0x23, 0x88, 0x17, 0x00, 0x00, 0x00, 0x40, 0xDB, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x2A,
    0x8C, 0x10, 0x00, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x2A, 0x8A, 0x10, 0x00,
    0x00, 0x00, 0x80, 0xFF, 0x0F, 0xFC, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x50, 0x22, 0x8B, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x22, 0x88, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x22, 0x88, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x32, 0x8E, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF,
    0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF, 0x1F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x01,
    0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x07, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x1C, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x38, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x0E,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x1C, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x07, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x8C, 0x31, 0xC6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xC0, 0x18, 0x63, 0x8C, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x81, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xE2, 0x01, 0x00, 0xFC, 0x27, 0x9F, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x13, 0x00, 0x00, 0x40, 0xF8, 0xE0, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x12, 0x0C,
    0x00, 0x40, 0x20, 0x5F, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0xB8, 0x01, 0xE0, 0xE1,
    0xF4, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE2, 0x48, 0x02, 0x50, 0xA3, 0x84, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x49, 0x02, 0x48, 0x96, 0x9E, 0x39, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x49, 0x02, 0x44, 0x90, 0xE6, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x49, 0x02, 0x40, 0x48, 0x82, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xF7,
    0x48, 0x02, 0x40, 0x68, 0x9D, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF,
    0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xF3, 0xFF, 0xFC, 0x3F, 0xFF, 0xCF, 0xFF, 0x3F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif  //dispUSE_U8G2 == 1

//****************************************************Function Declaration****************************************************//
static void v_disp_clear_region(u8 x, u8 y, u8 w, u8 h);
static void v_disp_draw_page_frame(const char *title);
static void v_disp_draw_full_top_bar(const char *title, const char *tag);
static void v_disp_draw_status_tag(const char *label);
static void v_disp_draw_hint_line(void);
static u16 us_disp_text_hash(const char *text);
static void v_disp_make_home_line(char *dst, u8 dst_size, const char *src, u8 width_chars, bool highlight, u8 field_key);
static void v_disp_draw_home_line(u8 x, u8 y, u8 w, const char *text, bool selected, bool highlight, u8 field_key);
static void v_disp_collect_snapshot(void);
static s16 s_disp_get_water_temp(void);
static u16 us_disp_get_init_progress(void);
static u8 uc_disp_get_updata_percent(void);
static const char *pc_disp_light_mode(void);
static const char *pc_disp_pump_mode(void);
static const char *pc_disp_o2pump_mode(void);
static const char *pc_disp_heat_mode(void);
static const char *pc_disp_updata_stage(void);
static const char *pc_disp_updata_note(void);
static const char *pc_disp_err_module(u16 code);
static const char *pc_disp_err_mode(u16 code);
static const char *pc_disp_err_action(u16 code);
static const char *pc_disp_alarm_desc(u16 code);
static const char *pc_disp_fan_mode(void);
static const char *pc_disp_light_white_state(void);
static const char *pc_disp_light_rgb_state(void);
static const char *pc_disp_short_mode(const char *mode);
static void v_disp_draw_home_head_value(u8 x, u8 y, u8 w, const char *label, const char *value, bool selected);
static const unsigned char *pc_disp_tab_icon(DispHomeTabId_E tab_id);
static void v_disp_draw_home_item_block(u8 x, u8 y, u8 w, const char *label, const char *value, bool selected, bool highlight, u8 field_key);
static void v_disp_draw_home_tab_card(u8 x, u8 y, u8 w, u8 h, DispHomeTabId_E tab_id, bool selected, bool active);
static void v_disp_draw_home_tab_body(u8 x, u8 y, u8 w, u8 h, DispHomeTabId_E tab_id, bool selected, bool active);

/***********************************************************************************************************************
-----函数功能    采集显示页面快照
-----说明(备注)  统一读取各模块实时数据, 避免绘制过程中数据变化导致页面不一致
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_collect_snapshot(void)
{
    memset(&s_tDispUiSnapshot, 0, sizeof(s_tDispUiSnapshot));

    s_tDispUiSnapshot.sWaterTemp = s_disp_get_water_temp();
    s_tDispUiSnapshot.usInitProgress = us_disp_get_init_progress();
    s_tDispUiSnapshot.ucUpgradePercent = uc_disp_get_updata_percent();
    s_tDispUiSnapshot.usErrCode = usDisp_ErrCodeDisplay();
    s_tDispUiSnapshot.usAutoOffCnt = tDisp.usAutoOffCnt;
    s_tDispUiSnapshot.ucBuzOff = tAppMemParam.tSYS.bBuzSwitchOff ? 1U : 0U;
    s_tDispUiSnapshot.ucForceClose = tSysInfo.uPerm.tPerm.bForceClose ? 1U : 0U;
    s_tDispUiSnapshot.pcLightMode = pc_disp_light_mode();
    s_tDispUiSnapshot.pcPumpMode = pc_disp_pump_mode();
    s_tDispUiSnapshot.pcO2PumpMode = pc_disp_o2pump_mode();
    s_tDispUiSnapshot.pcHeatMode = pc_disp_heat_mode();
    s_tDispUiSnapshot.pcUpgradeStage = pc_disp_updata_stage();
    s_tDispUiSnapshot.pcUpgradeNote = pc_disp_updata_note();
    s_tDispUiSnapshot.pcErrModule = pc_disp_err_module(s_tDispUiSnapshot.usErrCode);
    s_tDispUiSnapshot.pcErrMode = pc_disp_err_mode(s_tDispUiSnapshot.usErrCode);
    s_tDispUiSnapshot.pcErrAction = pc_disp_err_action(s_tDispUiSnapshot.usErrCode);
    s_tDispUiSnapshot.pcAlarmDesc = pc_disp_alarm_desc(s_tDispUiSnapshot.usErrCode);

    #if(boardADC_EN)
    s_tDispUiSnapshot.sWaterTemp1 = tAdcSamp.sWaterTemp1;
    s_tDispUiSnapshot.sWaterTemp2 = tAdcSamp.sWaterTemp2;
    s_tDispUiSnapshot.sBoardTemp5V = tAdcSamp.s5VTemp;
    s_tDispUiSnapshot.sBoardTemp12V = tAdcSamp.s12VTemp;
    s_tDispUiSnapshot.usLightAdc = tAdcSamp.usLightRes;
    s_tDispUiSnapshot.us12VVolt = tAdcSamp.us12VVolt;
    s_tDispUiSnapshot.usVinVolt = tAdcSamp.usVinVolt;
    s_tDispUiSnapshot.usVinCurrMa = (u16)(tAdcSamp.fVinCurr * 1000.0f);
    s_tDispUiSnapshot.usLightCurrMa = (u16)(tAdcSamp.fLightCurr * 1000.0f);
    s_tDispUiSnapshot.usHeatCurrMa = (u16)(tAdcSamp.fHeatCurr * 1000.0f);
    s_tDispUiSnapshot.usPumpCurrMa = (u16)(tAdcSamp.fPumpCurr * 1000.0f);
    s_tDispUiSnapshot.usO2CurrMa = (u16)(tAdcSamp.fO2Curr * 1000.0f);
    s_tDispUiSnapshot.usVinPowerW = (u16)(((u32)tAdcSamp.usVinVolt * s_tDispUiSnapshot.usVinCurrMa + 5000U) / 10000U);
    s_tDispUiSnapshot.usLightPowerW = (u16)(((u32)tAdcSamp.us12VVolt * s_tDispUiSnapshot.usLightCurrMa + 5000U) / 10000U);
    s_tDispUiSnapshot.usHeatPowerW = (u16)(((u32)tAdcSamp.us12VVolt * s_tDispUiSnapshot.usHeatCurrMa + 5000U) / 10000U);
    s_tDispUiSnapshot.usPumpPowerW = (u16)(((u32)tAdcSamp.us12VVolt * s_tDispUiSnapshot.usPumpCurrMa + 5000U) / 10000U);
    s_tDispUiSnapshot.usO2PowerW = (u16)(((u32)tAdcSamp.us12VVolt * s_tDispUiSnapshot.usO2CurrMa + 5000U) / 10000U);
    #endif

    #if(boardLIGHT_EN)
    s_tDispUiSnapshot.usLightWarm = tLight.usWarm;
    s_tDispUiSnapshot.usLightCtrlPower = tLight.usPower;
    s_tDispUiSnapshot.usLightBlue = tLight.usBlue;
    s_tDispUiSnapshot.usLightGreen = tLight.usGreen;
    s_tDispUiSnapshot.usLightRed = tLight.usRed;
    s_tDispUiSnapshot.ucLightMode = (u8)tLight.eLampMode;
    s_tDispUiSnapshot.ucLightRgbMode = (u8)tLight.eRGBMode;
    s_tDispUiSnapshot.ucLightDevState = (u8)tLight.eDevState;
    #endif

    #if(boardHEAT_MANAGE_EN)
    s_tDispUiSnapshot.sHeatTargetTemp = tHM.sHeatTargetTemp;
    s_tDispUiSnapshot.sFanTempStart = tHM.sFanTempStart;
    s_tDispUiSnapshot.sFanTempFull = tHM.sFanTempFull;
    s_tDispUiSnapshot.usHeatPwm = (u16)usHM_GetDevPwm(HM_OBJ_HEAT);
    s_tDispUiSnapshot.usFanPwm = (u16)usHM_GetDevPwm(HM_OBJ_FAN);
    s_tDispUiSnapshot.ucHeatEnable = tHM.bHeatEnable ? 1U : 0U;
    s_tDispUiSnapshot.ucFanEnable = tHM.bFanEnable ? 1U : 0U;
    #endif

    #if(boardO2PUMP_EN)
    s_tDispUiSnapshot.usO2PumpSpeed = tO2Pump.usSpeed;
    #endif

    #if(boardWATER_PUMP_EN)
    s_tDispUiSnapshot.usPumpSpeed = tPump.usSpeed;
    #endif
}

/***********************************************************************************************************************
-----函数功能    获取显示用水温
-----说明(备注)  优先使用两路水温平均值, ADC无数据时使用系统最高温度
-----传入参数    none
-----输出参数    none
-----返回值      水温值
************************************************************************************************************************/
static s16 s_disp_get_water_temp(void)
{
    #if(boardADC_EN && boardWATER_TEMP_EN)
    if((tAdcSamp.sWaterTemp1 != 0) || (tAdcSamp.sWaterTemp2 != 0))
        return (s16)((tAdcSamp.sWaterTemp1 + tAdcSamp.sWaterTemp2) / 2);
    #endif

    return tSysInfo.sMaxTemp;
}

/***********************************************************************************************************************
-----函数功能    获取初始化进度
-----说明(备注)  根据系统初始化完成位计算百分比, 用于启动/初始化进度条
-----传入参数    none
-----输出参数    none
-----返回值      初始化进度百分比
************************************************************************************************************************/
static u16 us_disp_get_init_progress(void)
{
    u16 state = tSysInfo.uInit.State;
    u16 done_count = 0;
    u16 total_count = 15;

    while(state)
    {
        done_count += (u16)(state & 0x0001U);
        state >>= 1;
    }

    return (u16)((done_count * 100U) / total_count);
}

/***********************************************************************************************************************
-----函数功能    获取升级进度
-----说明(备注)  根据升级接收帧数计算百分比, 未获得总帧数前返回阶段占位进度
-----传入参数    none
-----输出参数    none
-----返回值      升级进度百分比
************************************************************************************************************************/
static u8 uc_disp_get_updata_percent(void)
{
    #if(boardUPDATA)
    if(tUpdata.usTotalFrmValue > 0)
    {
        u32 percent = (u32)tUpdata.usRecFrameCnt * 100U;
        percent /= tUpdata.usTotalFrmValue;
        if(percent > 100U)
            percent = 100U;
        return (u8)percent;
    }

    if(tUpdata.eProtoType != PT_NULL)
        return 15U;
    if(tUpdata.eChType != CT_NULL)
        return 8U;
    #endif

    return 0U;
}

/***********************************************************************************************************************
-----函数功能    获取照明模式文本
-----说明(备注)  将照明工作模式转换成OLED短文本
-----传入参数    none
-----输出参数    none
-----返回值      模式文本指针
************************************************************************************************************************/
static const char *pc_disp_light_mode(void)
{
    #if(boardLIGHT_EN)
    switch(tLight.eLampMode)
    {
        case LWM_LOW: return "LOW";
        case LWM_HALF: return "HALF";
        case LWM_FULL: return "FULL";
        case LWM_AUTO: return "AUTO";
        case LWM_SOS: return "SOS";
        case LWM_TWINKLE: return "TWKL";
        default: break;
    }
    #endif

    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    获取水泵模式文本
-----说明(备注)  将水泵工作模式转换成OLED短文本
-----传入参数    none
-----输出参数    none
-----返回值      模式文本指针
************************************************************************************************************************/
static const char *pc_disp_pump_mode(void)
{
    #if(boardWATER_PUMP_EN)
    switch(tPump.eMode)
    {
        case PUMP_LOW: return "LOW";
        case PUMP_MID: return "MID";
        case PUMP_HIGH: return "HIGH";
        case PUMP_MAX: return "MAX";
        default: break;
    }
    #endif

    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    获取氧气泵模式文本
-----说明(备注)  将氧气泵工作模式转换成OLED短文本
-----传入参数    none
-----输出参数    none
-----返回值      模式文本指针
************************************************************************************************************************/
static const char *pc_disp_o2pump_mode(void)
{
    #if(boardO2PUMP_EN)
    switch(tO2Pump.eMode)
    {
        case O2PUMP_LOW: return "LOW";
        case O2PUMP_MID: return "MID";
        case O2PUMP_HIGH: return "HIGH";
        case O2PUMP_MAX: return "MAX";
        default: break;
    }
    #endif

    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    获取温控模式文本
-----说明(备注)  根据水温和强制加热状态生成温控短文本
-----传入参数    none
-----输出参数    none
-----返回值      模式文本指针
************************************************************************************************************************/
static const char *pc_disp_heat_mode(void)
{
	#if(boardHEAT_MANAGE_EN)
    if(bHM_IsForceOn())
		return "MAN";
	#endif

    if(s_disp_get_water_temp() < DISP_HEAT_STRONG_TEMP_C)
        return "HIGH";
    if(s_disp_get_water_temp() < DISP_HEAT_TARGET_TEMP_C)
        return "KEEP";
    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    获取风扇模式文本
-----说明(备注)  将风扇PWM值转换成OLED短文本（无级调速）
-----传入参数    none
-----输出参数    none
-----返回值      模式文本指针
************************************************************************************************************************/
static const char *pc_disp_fan_mode(void)
{
    const char *pc_mode = "OFF";
    u16 us_fan_pwm = usHM_GetDevPwm(HM_OBJ_FAN);

    #if(boardHEAT_MANAGE_EN)
    if(us_fan_pwm == 0)
        pc_mode = "OFF";
    else
        pc_mode = (us_fan_pwm >= hmPWM_MAX_VALUE) ? "FULL" : "AUTO";
    #endif

    return pc_mode;
}

/***********************************************************************************************************************
-----函数功能    获取白光状态文本
-----说明(备注)  将白光状态转换成主页短文本
-----传入参数    none
-----输出参数    none
-----返回值      状态文本指针
************************************************************************************************************************/
static const char *pc_disp_light_white_state(void)
{
    #if(boardLIGHT_EN)
    switch(tLight.eLampMode)
    {
        case LWM_LOW: return "LOW";
        case LWM_HALF: return "DIM";
        case LWM_FULL: return "FUL";
        case LWM_AUTO: return "AUT";
        case LWM_SOS: return "SOS";
        case LWM_TWINKLE: return "TWK";
        default: break;
    }
    #endif

    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    获取RGB状态文本
-----说明(备注)  将RGB灯状态转换成主页短文本
-----传入参数    none
-----输出参数    none
-----返回值      状态文本指针
************************************************************************************************************************/
static const char *pc_disp_light_rgb_state(void)
{
    #if(boardLIGHT_EN)
    if(tLight.eLampMode == LWM_SOS)
        return "SOS";
    if(tLight.eLampMode == LWM_TWINKLE)
        return "TWK";
    if(tLight.usBlue > 0U || tLight.usGreen > 0U || tLight.usRed > 0U)
        return "ON";
    #endif

    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    缩短模式文本
-----说明(备注)  将较长模式文本转换成三字符显示文本
-----传入参数    mode:原模式文本
-----输出参数    none
-----返回值      短文本指针
************************************************************************************************************************/
static const char *pc_disp_short_mode(const char *mode)
{
    if(mode == NULL)
        return "OFF";
    if(strcmp(mode, "OFF") == 0)
        return "OFF";
    if(strcmp(mode, "HALF") == 0)
        return "DIM";
    if(strcmp(mode, "FULL") == 0)
        return "FUL";
    if(strcmp(mode, "LOW") == 0)
        return "LOW";
    if(strcmp(mode, "MID") == 0)
        return "MID";
    if(strcmp(mode, "HIGH") == 0)
        return "HIG";
    if(strcmp(mode, "MAX") == 0)
        return "MAX";
    if(strcmp(mode, "KEEP") == 0)
        return "KEP";
    if(strcmp(mode, "MAN") == 0)
        return "MAN";
    if(strcmp(mode, "TWKL") == 0)
        return "TWK";
    if(strcmp(mode, "SOS") == 0)
        return "SOS";
    return mode;
}

/***********************************************************************************************************************
-----函数功能    获取主页标签图标
-----说明(备注)  根据主页标签ID选择对应16x16图标
-----传入参数    tab_id:主页标签ID
-----输出参数    none
-----返回值      图标数据指针
************************************************************************************************************************/
static const unsigned char *pc_disp_tab_icon(DispHomeTabId_E tab_id)
{
    switch(tab_id)
    {
        case DHT_LIGHT: return icon_light_16x16;
        case DHT_HEAT: return icon_heat_16x16;
        case DHT_WPUMP: return icon_wpump_16x16;
        case DHT_O2PUMP: return icon_o2pump_16x16;
        case DHT_SETTING: return icon_setting_16x16;
        case DHT_ADC: return icon_adc_16x16;
        default: break;
    }

    return icon_light_16x16;
}

/***********************************************************************************************************************
-----函数功能    获取升级阶段文本
-----说明(备注)  根据升级任务状态生成升级阶段显示文本
-----传入参数    none
-----输出参数    none
-----返回值      阶段文本指针
************************************************************************************************************************/
static const char *pc_disp_updata_stage(void)
{
    #if(boardUPDATA)
    if(tUpdata.eChType == CT_NULL)
        return "WAIT LINK";
    if(tUpdata.eProtoType == PT_NULL)
        return "SET PROTO";
    if(tUpdata.usRecFrameCnt == 0)
        return "HANDSHAKE";
    if(tUpdata.usTotalFrmValue > 0 && tUpdata.usRecFrameCnt >= tUpdata.usTotalFrmValue)
        return "VERIFY";
    return "WRITE APP";
    #else
    return "LOCKED";
    #endif
}

/***********************************************************************************************************************
-----函数功能    获取升级提示文本
-----说明(备注)  根据升级目标生成底部提示文本
-----传入参数    none
-----输出参数    none
-----返回值      提示文本指针
************************************************************************************************************************/
static const char *pc_disp_updata_note(void)
{
    #if(boardUPDATA)
    if(tUpdata.eObj == UO_BMS)
        return "TARGET BMS";
    if(tUpdata.eObj == UO_CONSOLE)
        return "TARGET APP";
    #endif

    return "KEEP POWER";
}

/***********************************************************************************************************************
-----函数功能    获取错误模块文本
-----说明(备注)  根据错误码区间生成故障模块文本
-----传入参数    code:错误码
-----输出参数    none
-----返回值      模块文本指针
************************************************************************************************************************/
static const char *pc_disp_err_module(u16 code)
{
    if(code <= 4)
        return "SYSTEM";
    if(code >= 10 && code <= 35)
        return "BMS";
    if(code >= 40 && code <= 49)
        return "USB";
    if(code >= 50 && code <= 59)
        return "DC";
    if(code >= 60)
        return "CTRL";
    return "DEVICE";
}

/***********************************************************************************************************************
-----函数功能    获取错误等级文本
-----说明(备注)  根据错误码生成告警等级文本
-----传入参数    code:错误码
-----输出参数    none
-----返回值      等级文本指针
************************************************************************************************************************/
static const char *pc_disp_err_mode(u16 code)
{
    if(code == 0 || code == 2 || code == 4)
        return "HIGH";
    if(code == 1 || code == 3)
        return "LOW";
    if(code >= 10)
        return "FAULT";
    return "WARN";
}

/***********************************************************************************************************************
-----函数功能    获取错误处理文本
-----说明(备注)  根据错误码生成建议处理动作文本
-----传入参数    code:错误码
-----输出参数    none
-----返回值      处理文本指针
************************************************************************************************************************/
static const char *pc_disp_err_action(u16 code)
{
    switch(code)
    {
        case 0: return "CHECK COOL";
        case 1: return "WARM UNIT";
        case 2: return "CHECK VIN";
        case 3: return "CHARGE NOW";
        case 4: return "REDUCE LOAD";
        default: return "CHECK LOOP";
    }
}

/***********************************************************************************************************************
-----函数功能    获取告警描述文本
-----说明(备注)  根据错误码生成告警描述文本
-----传入参数    code:错误码
-----输出参数    none
-----返回值      描述文本指针
************************************************************************************************************************/
static const char *pc_disp_alarm_desc(u16 code)
{
    switch(code)
    {
        case 0: return "SYSTEM OVER TEMP";
        case 1: return "SYSTEM UNDER TEMP";
        case 2: return "INPUT OVER VOLT";
        case 3: return "INPUT UNDER VOLT";
        case 4: return "OUTPUT OVER LOAD";
        case 10: return "CELL OVER VOLT";
        case 11: return "CELL UNDER VOLT";
        case 12: return "BMS ENV OVER T";
        case 13: return "BMS ENV UNDER T";
        default: return "CHECK ACTIVE FAULT";
    }
}

/***********************************************************************************************************************
-----函数功能    清除指定显示区域
-----说明(备注)  清除矩形区域后恢复默认绘制颜色
-----传入参数    x/y/w/h:区域坐标和尺寸
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_clear_region(u8 x, u8 y, u8 w, u8 h)
{
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_DrawBox(&u8g2, x, y, w, h);
    u8g2_SetDrawColor(&u8g2, 1);
}

/***********************************************************************************************************************
-----函数功能    绘制通用页面框架
-----说明(备注)  根据脏标志刷新标题、内容和底部区域
-----传入参数    title:标题
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_draw_page_frame(const char *title)
{
    vu16 dirty_mask = tDispPageCtx.usDirtyMask;

    if(dirty_mask == DDM_NONE)
        dirty_mask = DDM_FULL;

    if(dirty_mask == DDM_FULL || (dirty_mask & DDM_LAYOUT))
    {
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
        u8g2_DrawBox(&u8g2, 0, 0, OLED_WIDTH_PIXELS, 9);
        u8g2_SetDrawColor(&u8g2, 0);
        u8g2_DrawStr(&u8g2, 2, 7, title);
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawLine(&u8g2, 0, 10, OLED_WIDTH_PIXELS - 1U, 10);
        u8g2_DrawLine(&u8g2, 0, 47, OLED_WIDTH_PIXELS - 1U, 47);
        return;
    }

    if(dirty_mask & DDM_HEADER)
    {
        v_disp_clear_region(0, 0, OLED_WIDTH_PIXELS, 10);
        u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
        u8g2_DrawBox(&u8g2, 0, 0, OLED_WIDTH_PIXELS, 9);
        u8g2_SetDrawColor(&u8g2, 0);
        u8g2_DrawStr(&u8g2, 2, 7, title);
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawLine(&u8g2, 0, 10, OLED_WIDTH_PIXELS - 1U, 10);
    }

    if(dirty_mask & DDM_CONTENT)
        v_disp_clear_region(0, 11, OLED_WIDTH_PIXELS, 36);

    if(dirty_mask & DDM_BOTTOM)
    {
        v_disp_clear_region(0, 48, OLED_WIDTH_PIXELS, 16);
        u8g2_DrawLine(&u8g2, 0, 47, OLED_WIDTH_PIXELS - 1U, 47);
    }
}

/***********************************************************************************************************************
-----函数功能    绘制状态标签
-----说明(备注)  在页面右上角绘制小型状态标签
-----传入参数    label:标签文本
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_draw_status_tag(const char *label)
{
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawRFrame(&u8g2, 92, 1, 34, 8, 2);
    u8g2_DrawStr(&u8g2, 96, 7, label);
}

/***********************************************************************************************************************
-----函数功能    绘制全宽顶部状态栏
-----说明(备注)  用于详情页顶部反色标题栏显示
-----传入参数    title:标题  tag:右侧标签
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_draw_full_top_bar(const char *title, const char *tag)
{
    u8 title_len = (u8)((title != NULL) ? strlen(title) : 0U);
    u8 tag_len = (u8)((tag != NULL) ? strlen(tag) : 0U);

    v_disp_clear_region(0, 0, OLED_WIDTH_PIXELS, 9);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawBox(&u8g2, 0, 0, OLED_WIDTH_PIXELS, 9);
    u8g2_SetDrawColor(&u8g2, 0);
    if(title_len > 0U)
        u8g2_DrawStr(&u8g2, 2, 7, title);
    if(tag_len > 0U)
        u8g2_DrawStr(&u8g2, (u8)(OLED_WIDTH_PIXELS - (tag_len * 6U) - 2U), 7, tag);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawLine(&u8g2, 0, 9, OLED_WIDTH_PIXELS - 1U, 9);
}

/***********************************************************************************************************************
-----函数功能    绘制底部提示文本
-----说明(备注)  提示计数未清零时显示临时操作提示
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_draw_hint_line(void)
{
    if(tDispPageCtx.usHintCnt > 0U && tDispPageCtx.acHint[0] != '\0')
    {
        u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
        u8g2_DrawStr(&u8g2, 52, 61, tDispPageCtx.acHint);
    }
}

/***********************************************************************************************************************
-----函数功能    计算文本哈希
-----说明(备注)  用于判断滚动文本内容是否发生变化
-----传入参数    text:字符串指针
-----输出参数    none
-----返回值      文本哈希值
************************************************************************************************************************/
static u16 us_disp_text_hash(const char *text)
{
    u16 hash = 0x811CU;

    while(text != NULL && *text != '\0')
    {
        hash ^= (u8)(*text);
        hash = (u16)(hash * 167U);
        text++;
    }

    return hash;
}

/***********************************************************************************************************************
-----函数功能    生成主页卡片文本行
-----说明(备注)  普通项超长截断, 选中项超长按滚动状态截取
-----传入参数    dst:输出缓存  src:源文本  width_chars:显示宽度  highlight:是否高亮  field_key:字段ID
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_make_home_line(char *dst, u8 dst_size, const char *src, u8 width_chars, bool highlight, u8 field_key)
{
    u8 len;
    u8 copy_len;
    u16 hash;

    if(dst == NULL || dst_size == 0U)
        return;

    dst[0] = '\0';
    if(src == NULL)
        return;

    len = (u8)strlen(src);
    if(width_chars == 0U)
        width_chars = 1U;

    if(len <= width_chars)
    {
        strncpy(dst, src, dst_size - 1U);
        dst[dst_size - 1U] = '\0';
        if(highlight == true && tDispPageCtx.tTextScroll.ucFieldKey == field_key)
        {
            tDispPageCtx.tTextScroll.ucOffset = 0U;
            tDispPageCtx.tTextScroll.ucHoldCnt = 0U;
            tDispPageCtx.tTextScroll.ucMaxOffset = 0U;
            tDispPageCtx.tTextScroll.usTextHash = 0U;
        }
        return;
    }

    if(highlight == false)
    {
        copy_len = (width_chars < (u8)(dst_size - 1U)) ? width_chars : (u8)(dst_size - 1U);
        if(copy_len == 0U)
            return;
        strncpy(dst, src, copy_len);
        dst[copy_len] = '\0';
        if(copy_len >= 2U)
            dst[copy_len - 1U] = '>';
        return;
    }

    hash = us_disp_text_hash(src);
    if(tDispPageCtx.tTextScroll.ucFieldKey != field_key || tDispPageCtx.tTextScroll.usTextHash != hash)
    {
        tDispPageCtx.tTextScroll.ucFieldKey = field_key;
        tDispPageCtx.tTextScroll.usTextHash = hash;
        tDispPageCtx.tTextScroll.ucOffset = 0U;
        tDispPageCtx.tTextScroll.ucHoldCnt = 0U;
    }

    tDispPageCtx.tTextScroll.ucMaxOffset = (u8)(len - width_chars);
    copy_len = width_chars;
    if(copy_len > (u8)(dst_size - 1U))
        copy_len = (u8)(dst_size - 1U);

    strncpy(dst, &src[tDispPageCtx.tTextScroll.ucOffset], copy_len);
    dst[copy_len] = '\0';
}

/***********************************************************************************************************************
-----函数功能    绘制主页文本行
-----说明(备注)  按选中和高亮状态绘制主页卡片中的滚动文本行
-----传入参数    x/y/w:区域参数  text:文本  selected:选中状态  highlight:高亮状态  field_key:字段键值
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_draw_home_line(u8 x, u8 y, u8 w, const char *text, bool selected, bool highlight, u8 field_key)
{
    char line_buf[24];
    u8 width_chars = (u8)(w / 6U);

    v_disp_make_home_line(line_buf, sizeof(line_buf), text, width_chars, highlight, field_key);

    if(highlight == true)
    {
        u8g2_SetDrawColor(&u8g2, selected == true ? 1 : 0);
        u8g2_DrawBox(&u8g2, x, (u8)(y - 8U), w, 10U);
        u8g2_SetDrawColor(&u8g2, selected == true ? 0 : 1);
    }
    else if(selected == true)
        u8g2_SetDrawColor(&u8g2, 0);
    else
        u8g2_SetDrawColor(&u8g2, 1);

    u8g2_DrawStr(&u8g2, x + 1U, y, line_buf);
}

/***********************************************************************************************************************
-----函数功能    绘制主页头部数值
-----说明(备注)  绘制卡片头部标题和右对齐数值
-----传入参数    x/y/w:区域参数  label:标题  value:数值  selected:选中状态
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_draw_home_head_value(u8 x, u8 y, u8 w, const char *label, const char *value, bool selected)
{
    u8 value_len = (u8)((value != NULL) ? strlen(value) : 0U);
    u8 value_x = (u8)(x + w - 1U - (value_len * 6U));

    if(selected == true)
        u8g2_SetDrawColor(&u8g2, 0);
    else
        u8g2_SetDrawColor(&u8g2, 1);

    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    if(label != NULL)
        u8g2_DrawStr(&u8g2, x + 1U, y + 7U, label);

    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    if(value != NULL)
        u8g2_DrawStr(&u8g2, value_x, y + 15U, value);

    u8g2_SetDrawColor(&u8g2, 1);
}

/***********************************************************************************************************************
-----函数功能    绘制主页项目块
-----说明(备注)  绘制主页标签内单个项目的名称和值
-----传入参数    x/y/w:区域参数  label:名称  value:数值  selected:选中状态  highlight:高亮状态  field_key:字段键值
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_draw_home_item_block(u8 x, u8 y, u8 w, const char *label, const char *value, bool selected, bool highlight, u8 field_key)
{
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    if(selected == true)
        u8g2_SetDrawColor(&u8g2, 0);
    else
        u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawStr(&u8g2, x + 1U, y + 7U, label);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    v_disp_draw_home_line(x, (u8)(y + 15U), w, value, selected, highlight, field_key);
    u8g2_SetDrawColor(&u8g2, 1);
}

/***********************************************************************************************************************
-----函数功能    绘制主页标签卡片内容
-----说明(备注)  根据标签类型绘制对应模块的状态和可调项目
-----传入参数    x/y/w/h:区域参数  tab_id:标签ID  selected:是否选中  active:是否激活
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_draw_home_tab_body(u8 x, u8 y, u8 w, u8 h, DispHomeTabId_E tab_id, bool selected, bool active)
{
    char line0[24];
    char line1[24];
    char line2[24];
    char line3[24];
    u8 visible_start = 0U;
    bool focus_0;
    bool focus_1;
    u8 text_w = (w > 4U) ? (u8)(w - 4U) : w;
    (void)h;

    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);

    if(tab_id == DHT_SETTING)
        visible_start = (u8)((tDispPageCtx.ucTabItemIndex / 2U) * 2U);

    focus_0 = (active == true && tDispPageCtx.ucTabItemIndex == visible_start);
    focus_1 = (active == true && tDispPageCtx.ucTabItemIndex == (u8)(visible_start + 1U));

    switch(tab_id)
    {
        case DHT_LIGHT:
            sprintf(line1, "%u", s_tDispUiSnapshot.usLightCurrMa / 100U);
            strcpy(line2, pc_disp_light_white_state());
            strcpy(line3, pc_disp_light_rgb_state());
            v_disp_draw_home_item_block(x + 1U, y + 17U, text_w, "WHT", line2, selected, focus_0, 0x10U);
            v_disp_draw_home_item_block(x + 1U, y + 33U, text_w, "RGB", line3, selected, focus_1, 0x11U);
            sprintf(line0, "I");
            v_disp_draw_home_head_value(x + 17U, y + 1U, 13U, line0, line1, selected);
            return;

        case DHT_HEAT:
            sprintf(line0, "%dC", s_tDispUiSnapshot.sWaterTemp);
            strcpy(line1, bHM_IsForceOn() ? "MAN" : "AUTO");
            strcpy(line2, pc_disp_short_mode(pc_disp_fan_mode()));
            v_disp_draw_home_item_block(x + 1U, y + 17U, text_w, "HE", line1, selected, focus_0, 0x20U);
            v_disp_draw_home_item_block(x + 1U, y + 33U, text_w, "FN", line2, selected, focus_1, 0x21U);
            v_disp_draw_home_head_value(x + 17U, y + 1U, 13U, "T", line0, selected);
            return;

        case DHT_WPUMP:
            sprintf(line0, "%u", s_tDispUiSnapshot.usPumpSpeed);
            strcpy(line1, pc_disp_short_mode(s_tDispUiSnapshot.pcPumpMode));
            v_disp_draw_home_item_block(x + 1U, y + 17U, text_w, "MD", line1, selected, focus_0, 0x30U);
            v_disp_draw_home_item_block(x + 1U, y + 33U, text_w, "ST", line2, selected, false, 0x31U);
            v_disp_draw_home_head_value(x + 17U, y + 1U, 13U, "S", line0, selected);
            return;

        case DHT_O2PUMP:
            sprintf(line0, "%u", s_tDispUiSnapshot.usO2PumpSpeed);
            strcpy(line1, pc_disp_short_mode(s_tDispUiSnapshot.pcO2PumpMode));
            v_disp_draw_home_item_block(x + 1U, y + 17U, text_w, "MD", line1, selected, focus_0, 0x40U);
            v_disp_draw_home_item_block(x + 1U, y + 33U, text_w, "ST", line2, selected, false, 0x41U);
            v_disp_draw_home_head_value(x + 17U, y + 1U, 13U, "S", line0, selected);
            return;

        case DHT_SETTING:
            sprintf(line0, "%u-%u", visible_start + 1U, visible_start + 2U);
            switch(visible_start)
            {
                case 0U:
                    sprintf(line1, "%u", tDispPageCtx.tSettingCache.usSleepTime);
                    break;

                default:
                    sprintf(line1, "%u", tDispPageCtx.tSettingCache.ucHighLightValue);
                    break;
            }
            v_disp_draw_home_item_block(x + 1U, y + 17U, text_w, visible_start == 0U ? "SLP" : "HBL", line1, selected, focus_0, (u8)(0x50U + visible_start));
            v_disp_draw_home_item_block(x + 1U, y + 33U, text_w, visible_start == 0U ? "BUZ" : "LBL", line2, selected, focus_1, (u8)(0x51U + visible_start));
            if(tDispPageCtx.tSettingCache.bDirty)
                strcpy(line3, "DIRTY");
            else if(tDispPageCtx.bEditing)
                strcpy(line3, "EDIT");
            else
                strcpy(line3, "CACHE");
            v_disp_draw_home_head_value(x + 17U, y + 1U, 13U, "CF", line0, selected);
            v_disp_draw_home_line(x + 1U, y + 55U, text_w, line3, selected, false, 0x54U);
            return;

        case DHT_ADC:
            sprintf(line0, "G%u", tDispPageCtx.ucAdcGroupIndex + 1U);
            switch(tDispPageCtx.ucAdcGroupIndex)
            {
                case 0U:
                    sprintf(line1, "%dC", s_tDispUiSnapshot.sWaterTemp1);
                    break;

                case 1U:
                    sprintf(line1, "%u.%uV", s_tDispUiSnapshot.us12VVolt / 10U, s_tDispUiSnapshot.us12VVolt % 10U);
                    break;

                default:
                    sprintf(line1, "%u", s_tDispUiSnapshot.usLightAdc / 10U);
                    break;
            }
            v_disp_draw_home_item_block(x + 1U, y + 17U, text_w,
                                        tDispPageCtx.ucAdcGroupIndex == 0U ? "W1" : (tDispPageCtx.ucAdcGroupIndex == 1U ? "VIN" : "LUX"),
                                        line1, selected, focus_0, (u8)(0x60U + (tDispPageCtx.ucAdcGroupIndex * 2U)));
            v_disp_draw_home_item_block(x + 1U, y + 33U, text_w,
                                        tDispPageCtx.ucAdcGroupIndex == 0U ? "W2" : (tDispPageCtx.ucAdcGroupIndex == 1U ? "CUR" : "TMP"),
                                        line2, selected, focus_1, (u8)(0x61U + (tDispPageCtx.ucAdcGroupIndex * 2U)));
            v_disp_draw_home_head_value(x + 17U, y + 1U, 13U, "AD", line0, selected);
            sprintf(line3, "%u/3", tDispPageCtx.ucAdcGroupIndex + 1U);
            v_disp_draw_home_line(x + 1U, y + 55U, text_w, line3, selected, false, 0x66U);
            return;

        default:
            break;
    }

    u8g2_SetDrawColor(&u8g2, 1);
}

/***********************************************************************************************************************
-----函数功能    绘制主页标签卡片
-----说明(备注)  绘制卡片边框、图标、内容和激活焦点框
-----传入参数    x/y/w/h:区域参数  tab_id:标签ID  selected:是否选中  active:是否激活
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_draw_home_tab_card(u8 x, u8 y, u8 w, u8 h, DispHomeTabId_E tab_id, bool selected, bool active)
{
    const unsigned char *icon = pc_disp_tab_icon(tab_id);

    if(selected == true)
        u8g2_DrawRBox(&u8g2, x, y, w, h, 2);
    else
        u8g2_DrawRFrame(&u8g2, x, y, w, h, 2);

    if(selected == true)
        u8g2_SetDrawColor(&u8g2, 0);

    u8g2_DrawLine(&u8g2, x, y + 16U, (u8)(x + w - 1U), y + 16U);
    u8g2_DrawLine(&u8g2, x, y + 32U, (u8)(x + w - 1U), y + 32U);
    u8g2_DrawLine(&u8g2, x, y + 48U, (u8)(x + w - 1U), y + 48U);
    u8g2_DrawLine(&u8g2, x + 16U, y + 1U, x + 16U, y + 15U);
    u8g2_DrawXBMP(&u8g2, x + 1U, y + 1U, 16, 16, icon);

    v_disp_draw_home_tab_body(x + 1U, y + 1U, (u8)(w - 2U), (u8)(h - 2U), tab_id, selected, active);

    u8g2_SetDrawColor(&u8g2, 1);
    if(active == true)
        u8g2_DrawFrame(&u8g2, x + 1U, y + 1U, (u8)(w - 2U), (u8)(h - 2U));
}

/***********************************************************************************************************************
-----函数功能    获取主页模块标签
-----说明(备注)  根据主页模块ID返回四宫格标题文本
-----传入参数    module:主页模块
-----输出参数    none
-----返回值      标签文本指针
************************************************************************************************************************/
#if(dispUSE_U8G2 == 0)
/***********************************************************************************************************************
-----函数功能    写OLED命令
-----说明(备注)  通过底层接口向OLED发送单字节命令
-----传入参数    cmd:OLED命令字节
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void oled_write_cmd(u8 cmd)
{
    vDisp_OledWriteByte(&cmd, 1U, OLED_CMD);
}

/***********************************************************************************************************************
-----函数功能    获取5x7字符字模
-----说明(备注)  返回软件GRAM模式下字符绘制使用的5x7点阵数据
-----传入参数    c:字符
-----输出参数    none
-----返回值      字模数据指针
************************************************************************************************************************/
static const u8 *oled_font_5x7(char c)
{
    static const u8 f_space[5] = {0x00,0x00,0x00,0x00,0x00};
    static const u8 f_percent[5] = {0x62,0x64,0x08,0x13,0x23};
    static const u8 f_minus[5] = {0x08,0x08,0x08,0x08,0x08};
    static const u8 f_colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const u8 f_slash[5] = {0x20,0x10,0x08,0x04,0x02};

    static const u8 d0[5] = {0x3E,0x51,0x49,0x45,0x3E};
    static const u8 d1[5] = {0x00,0x42,0x7F,0x40,0x00};
    static const u8 d2[5] = {0x62,0x51,0x49,0x49,0x46};
    static const u8 d3[5] = {0x22,0x41,0x49,0x49,0x36};
    static const u8 d4[5] = {0x18,0x14,0x12,0x7F,0x10};
    static const u8 d5[5] = {0x2F,0x49,0x49,0x49,0x31};
    static const u8 d6[5] = {0x3E,0x49,0x49,0x49,0x32};
    static const u8 d7[5] = {0x01,0x71,0x09,0x05,0x03};
    static const u8 d8[5] = {0x36,0x49,0x49,0x49,0x36};
    static const u8 d9[5] = {0x26,0x49,0x49,0x49,0x3E};

    static const u8 A[5] = {0x7E,0x11,0x11,0x11,0x7E};
    static const u8 B[5] = {0x7F,0x49,0x49,0x49,0x36};
    static const u8 C[5] = {0x3E,0x41,0x41,0x41,0x22};
    static const u8 D[5] = {0x7F,0x41,0x41,0x22,0x1C};
    static const u8 E[5] = {0x7F,0x49,0x49,0x49,0x41};
    static const u8 F[5] = {0x7F,0x09,0x09,0x09,0x01};
    static const u8 G[5] = {0x3E,0x41,0x49,0x49,0x3A};
    static const u8 H[5] = {0x7F,0x08,0x08,0x08,0x7F};
    static const u8 I[5] = {0x00,0x41,0x7F,0x41,0x00};
    static const u8 K[5] = {0x7F,0x08,0x14,0x22,0x41};
    static const u8 L[5] = {0x7F,0x40,0x40,0x40,0x40};
    static const u8 M[5] = {0x7F,0x02,0x0C,0x02,0x7F};
    static const u8 N[5] = {0x7F,0x04,0x08,0x10,0x7F};
    static const u8 O[5] = {0x3E,0x41,0x41,0x41,0x3E};
    static const u8 P[5] = {0x7F,0x09,0x09,0x09,0x06};
    static const u8 R[5] = {0x7F,0x09,0x19,0x29,0x46};
    static const u8 S[5] = {0x46,0x49,0x49,0x49,0x31};
    static const u8 T[5] = {0x01,0x01,0x7F,0x01,0x01};
    static const u8 U[5] = {0x3F,0x40,0x40,0x40,0x3F};
    static const u8 V[5] = {0x1F,0x20,0x40,0x20,0x1F};
    static const u8 W[5] = {0x7F,0x20,0x18,0x20,0x7F};
    static const u8 Y[5] = {0x07,0x08,0x70,0x08,0x07};

    switch(c)
    {
        case '0': return d0;
        case '1': return d1;
        case '2': return d2;
        case '3': return d3;
        case '4': return d4;
        case '5': return d5;
        case '6': return d6;
        case '7': return d7;
        case '8': return d8;
        case '9': return d9;
        case 'A': return A;
        case 'B': return B;
        case 'C': return C;
        case 'D': return D;
        case 'E': return E;
        case 'F': return F;
        case 'G': return G;
        case 'H': return H;
        case 'I': return I;
        case 'K': return K;
        case 'L': return L;
        case 'M': return M;
        case 'N': return N;
        case 'O': return O;
        case 'P': return P;
        case 'R': return R;
        case 'S': return S;
        case 'T': return T;
        case 'U': return U;
        case 'V': return V;
        case 'W': return W;
        case 'Y': return Y;
        case '%': return f_percent;
        case '-': return f_minus;
        case ':': return f_colon;
        case '/': return f_slash;
        case ' ': return f_space;
        default:  return f_space;
    }
}
#else
/***********************************************************************************************************************
-----函数功能    U8g2字节发送回调
-----说明(备注)  把U8g2发送消息转接到本模块SPI发送接口
-----传入参数    u8x8/msg/arg_int/arg_ptr:U8g2回调参数
-----输出参数    none
-----返回值      1:处理成功  0:未处理
************************************************************************************************************************/
uint8_t u8x8_byte_4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
	{
		/*通过SPI发送arg_int个字节数据*/
		case U8X8_MSG_BYTE_SEND:
		{
			vDisp_SpiSendByte((const u8 *)arg_ptr, arg_int);
		}
		break;

		/*初始化函数*/
		case U8X8_MSG_BYTE_INIT:
		break;

		/*设置DC引脚,表明发送的是数据还是命令*/
		case U8X8_MSG_BYTE_SET_DC:
		{
			if (arg_int) {
				dispOLED_DC_H();
			} else {
				dispOLED_DC_L();
            }
		}
		break;

		case U8X8_MSG_BYTE_START_TRANSFER:
			u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
			u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
			break;
		case U8X8_MSG_BYTE_END_TRANSFER:
			u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
			u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
			break;
		default:
			return 0;
    }
    return 1;
}

/***********************************************************************************************************************
-----函数功能    U8g2 GPIO和延时回调
-----说明(备注)  处理复位、片选、DC控制和毫秒延时消息
-----传入参数    u8x8/msg/arg_int/arg_ptr:U8g2回调参数
-----输出参数    none
-----返回值      1:处理成功
************************************************************************************************************************/
uint8_t u8x8_stm32_gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int,
                                  U8X8_UNUSED void *arg_ptr)
{
    switch (msg)
	{
		/*delay和GPIO的初始化，在main中已经初始化完成了*/
		case U8X8_MSG_GPIO_AND_DELAY_INIT:
			break;

			/*延时函数*/
		case U8X8_MSG_DELAY_MILLI:
			vTaskDelay(arg_int);    // 调用谁mcu系统延时函数
			break;

		/*片选信号*/  // 由于只有一个SPI设备，所以片选信号在初始化时已经设置为常有效
		case U8X8_MSG_GPIO_CS:
		{
			if (arg_int) {
				dispOLED_NSS_H();
			} else {
				dispOLED_NSS_L();
			}
		}
		break;

		/*设置DC引脚,表明发送的是数据还是命令*/
		case U8X8_MSG_GPIO_DC:
		{
			if (arg_int) {
				dispOLED_DC_H();
			} else {
				dispOLED_DC_L();
			}
		}
		break;

		/*复位信号*/
		case U8X8_MSG_GPIO_RESET:
		{
			if (arg_int) {
				dispOLED_RES_H();
			} else {
				dispOLED_RES_L();
			}
		}
		break;
    }
    return 1;
}

/***********************************************************************************************************************
-----函数功能    U8g2显示初始化
-----说明(备注)  配置SH1106驱动、显示缓存和上电状态
-----传入参数    u8g2:显示对象指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_disp_u8g2_init(u8g2_t *u8g2)
{
    uint8_t tile_buf_height;
    uint8_t *buf;

    u8g2_SetupDisplay(u8g2, u8x8_d_sh1106_128x64_noname, u8x8_cad_001,
                      u8x8_byte_4wire_hw_spi, u8x8_stm32_gpio_and_delay);
    buf = u8g2_m_16_8_f(&tile_buf_height);
    u8g2_SetupBuffer(u8g2, buf, tile_buf_height, u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
}
#endif  //dispUSE_U8G2






#if(dispUSE_U8G2 == 0)
/***********************************************************************************************************************
-----函数功能    设置单个OLED像素点
-----说明(备注)  根据坐标写入像素状态, 越界坐标直接忽略
-----传入参数    x:列坐标  y:行坐标  on:像素状态
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_OledDrawPixel(u8 x, u8 y, bool on)
{
    u8 page;
    u8 bit;

    if(x >= OLED_WIDTH_PIXELS || y >= OLED_HEIGHT_PIXELS)
        return;

    page = (u8)(y >> 3);
    bit = (u8)(1U << (y & 0x07U));

    if(on)
        s_oled_gram[page][x] |= bit;
    else
        s_oled_gram[page][x] &= (u8)(~bit);
}

/***********************************************************************************************************************
-----函数功能    绘制OLED水平线
-----说明(备注)  从起始坐标开始，连续绘制指定长度的水平线。
-----传入参数    x:起点列坐标  y:行坐标  len:线长  on:像素状态
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_OledDrawHLine(u8 x, u8 y, u8 len, bool on)
{
    u8 i;
    for(i = 0; i < len; i++)
        vDisp_OledDrawPixel((u8)(x + i), y, on);
}

/***********************************************************************************************************************
-----函数功能    绘制OLED竖直线
-----说明(备注)  从起始坐标开始，连续绘制指定长度的竖直线。
-----传入参数    x:列坐标  y:起点行坐标  len:线长  on:像素状态
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_OledDrawVLine(u8 x, u8 y, u8 len, bool on)
{
    u8 i;
    for(i = 0; i < len; i++)
        vDisp_OledDrawPixel(x, (u8)(y + i), on);
}

/***********************************************************************************************************************
-----函数功能    绘制6x8字符
-----说明(备注)  使用内置5x7字模显示单个字符，字符宽度按6列处理，便于字符间隔控制。
-----传入参数    x:起点列坐标  y:起点行坐标  c:待显示字符
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_OledDrawChar6x8(u8 x, u8 y, char c)
{
    u8 col;
    u8 row;

    for(col = 0; col < 5U; col++)
    {
        u8 bits = glyph[col];
        for(row = 0; row < 7U; row++)
        {
            vDisp_OledDrawPixel((u8)(x + col), (u8)(y + row), ((bits & 0x01U) != 0U));
            bits >>= 1;
        }
    }

    for(row = 0; row < 7U; row++)
        vDisp_OledDrawPixel((u8)(x + 5U), (u8)(y + row), false);
}

/***********************************************************************************************************************
-----函数功能    绘制6x8字符串
-----说明(备注)  逐字符调用字体绘制接口显示字符串，超出显示区域时自动停止。
-----传入参数    x:起点列坐标  y:起点行坐标  str:字符串指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_OledDrawString6x8(u8 x, u8 y, const char *str)
{
    while((*str) != '\0')
    {
        vDisp_OledDrawChar6x8(x, y, *str);
        x = (u8)(x + 6U);
        if(x > 122U)
            break;
        str++;
    }
}

/***********************************************************************************************************************
-----函数功能    填充OLED显存缓存
-----说明(备注)  用指定数据批量填充GRAM缓存，常用于全亮/全灭测试。
-----传入参数    value:填充值
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_OledFillBuffer(u8 value)
{
    u8 x;
    u8 page;
    for(page = 0; page < OLED_PAGE_COUNT; page++)
    {
        for(x = 0; x < OLED_WIDTH_PIXELS; x++)
            s_oled_gram[page][x] = value;
    }
}
#endif  //dispUSE_U8G2  == 0


/***********************************************************************************************************************
-----函数功能    显示驱动初始化
-----说明(备注)  。
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_Init(void)
{
    #if(dispUSE_U8G2 == 1)
	v_disp_u8g2_init(&u8g2);
	u8g2_SetContrast(&u8g2, tAppMemParam.tDISP.ucHighLightValue);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
    #else
    oled_write_cmd(0xAE);
    oled_write_cmd(0x02);
    oled_write_cmd(0x10);
    oled_write_cmd(0x40);
    oled_write_cmd(0xB0);
    oled_write_cmd(0x81);
    oled_write_cmd(0xCF);
    oled_write_cmd(0xA1);
    oled_write_cmd(0xA6);
    oled_write_cmd(0xA8);
    oled_write_cmd(0x3F);
    oled_write_cmd(0xAD);
    oled_write_cmd(0x8B);
    oled_write_cmd(0x33);
    oled_write_cmd(0xC8);
    oled_write_cmd(0xD3);
    oled_write_cmd(0x00);
    oled_write_cmd(0xD5);
    oled_write_cmd(0x80);
    oled_write_cmd(0xD9);
    oled_write_cmd(0x1F);
    oled_write_cmd(0xDA);
    oled_write_cmd(0x12);
    oled_write_cmd(0xDB);
    oled_write_cmd(0x40);

    vDisp_OledClearBuffer();
    vDisp_OledRefresh();
    vDisp_OledSetPower(true);
    #endif  //dispUSE_U8G2

    g_bDispPageDirty = false;
}

/***********************************************************************************************************************
-----函数功能    设置显示对比度
-----说明(备注)  使用记忆参数中的亮度值更新 OLED 对比度。
-----传入参数    value: 对比度/亮度值
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_SetContrast(u8 value)
{
    #if(dispUSE_U8G2 == 1)
    u8g2_SetContrast(&u8g2, value);
    #else
    oled_write_cmd(0x81);
    oled_write_cmd(value);
    #endif
}

/***********************************************************************************************************************
-----函数功能    清空显存缓存
-----说明(备注)  将GRAM缓存全部清零，等待后续刷新到屏幕。
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_ClearBuffer(void)
{
    #if(dispUSE_U8G2 == 1)
    u8g2_ClearBuffer(&u8g2);
    #else
    for(int page = 0; page < OLED_PAGE_COUNT; page++)
    {
        for(int x = 0; x < OLED_WIDTH_PIXELS; x++)
            s_oled_gram[page][x] = 0x00U;
    }
    #endif  //dispUSE_U8G2
}

/***********************************************************************************************************************
-----函数功能    刷新显示内容
-----说明(备注)  将显存缓存逐页写入OLED面板。
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_Refresh(void)
{
    #if(dispUSE_U8G2 == 1)
    u8g2_SendBuffer(&u8g2);
    #else
    for(int page = 0; page < OLED_PAGE_COUNT; page++)
    {
        oled_write_cmd((u8)(0xB0U + page));
        oled_write_cmd(0x02);
        oled_write_cmd(0x10);
        vDisp_OledWriteByte(&s_oled_gram[page][0], OLED_WIDTH_PIXELS, OLED_DATA);
    }
    #endif  //dispUSE_U8G2
}

/***********************************************************************************************************************
-----函数功能    设置电源状态
-----说明(备注)  通过指令控制OLED显示开关。
-----传入参数    on:true开显示 false关显示
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_SetPower(bool on)
{
    #if(dispUSE_U8G2 == 1)
    if(on)
        u8g2_SetPowerSave(&u8g2, 0);
    else
        u8g2_SetPowerSave(&u8g2, 1);
    #else
    if(on)
        oled_write_cmd(0xAF);
    else
        oled_write_cmd(0xAE);
    #endif  //dispUSE_U8G2
}

/***********************************************************************************************************************
-----函数功能    显示Hello World测试页
-----说明(备注)  清屏后绘制边框与测试字符串，用于验证OLED任务链路和字符显示是否正常。
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_UiTest(void)
{
    // U8g2显示测试页
    #if(dispUSE_U8G2 == 1)
    dispOLED_RES_H();  	  // 显示器复位拉高
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawXBM(&u8g2, 0, 0, OLED_WIDTH_PIXELS, OLED_HEIGHT_PIXELS, u8g_logo_bits);
    u8g2_SendBuffer(&u8g2);
    #else
    vDisp_OledClearBuffer();
	vDisp_OledDrawHLine(0U, 0U, OLED_WIDTH_PIXELS, true);
	vDisp_OledDrawHLine(0U, (u8)(OLED_HEIGHT_PIXELS - 1U), OLED_WIDTH_PIXELS, true);
	vDisp_OledDrawVLine(0U, 0U, OLED_HEIGHT_PIXELS, true);
	vDisp_OledDrawVLine((u8)(OLED_WIDTH_PIXELS - 1U), 0U, OLED_HEIGHT_PIXELS, true);
	vDisp_OledDrawString6x8(31, 28, "HELLO WORLD");
	vDisp_OledRefresh();
    #endif  //dispUSE_U8G2
}

/***********************************************************************************************************************
-----函数功能    获取显示快照指针
-----说明(备注)  返回最近一次渲染采集的数据快照
-----传入参数    none
-----输出参数    none
-----返回值      快照结构体常量指针
************************************************************************************************************************/
const DispUiSnapshot_T *ptDisp_GetUiSnapshot(void)
{
    return &s_tDispUiSnapshot;
}

/***********************************************************************************************************************
-----函数功能    刷新显示快照(对外接口)
-----说明(备注)  供状态队列任务在独立渲染前主动刷新快照
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_UpdateUiSnapshot(void)
{
    v_disp_collect_snapshot();
}

/***********************************************************************************************************************
-----函数功能    绘制页面框架(对外接口)
-----说明(备注)  调用内部页面框架绘制函数, 供队列任务直接复用
-----传入参数    title:标题
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_DrawPageFrame(const char *title)
{
    v_disp_draw_page_frame(title);
}

/***********************************************************************************************************************
-----函数功能    绘制状态标签(对外接口)
-----说明(备注)  调用内部状态标签绘制函数
-----传入参数    label:标签文本
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_DrawStatusTag(const char *label)
{
    v_disp_draw_status_tag(label);
}

/***********************************************************************************************************************
-----函数功能    清除显示区域(对外接口)
-----说明(备注)  调用内部区域清除函数
-----传入参数    x/y/w/h:区域坐标和尺寸
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_ClearRegion(u8 x, u8 y, u8 w, u8 h)
{
    v_disp_clear_region(x, y, w, h);
}

/***********************************************************************************************************************
-----函数功能    渲染当前显示页面
-----说明(备注)  采集快照、分发页面绘制、刷新屏幕并清除脏标志
-----传入参数    none
-----输出参数    none
-----返回值      true:已执行渲染 false:未执行渲染
************************************************************************************************************************/
bool bDisp_RenderUi(void)
{
    #if(dispUSE_U8G2 == 1)
    if(tDisp.eDevState == DS_WORK)
        return false;

    v_disp_collect_snapshot();

    // WORK 页面渲染已下沉到 work 队列任务, 这里不再分发 WORK 相关页面
    return false;
    #else
    vDisp_UiTest();
    #endif

    tDispPageCtx.usDirtyMask = DDM_NONE;
    g_bDispPageDirty = false;
    return true;
}

#endif  // boardDISPLAY_EN





