/***********************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G2404-3/APP/Hardware/MD_Display
 * File    : md_display_api.h
 * Date    : 2026-03-19
 * Author  : LJD(291483914@qq.com)
 * Description: MD Display API Header File - Optimized version based on M5004-3
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
************************************************************************************************************************/
#ifndef MD_DISPLAY_API_H
#define MD_DISPLAY_API_H


#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================includes====================================*/
#include "board_config.h"

#if(boardDISPLAY_EN)

/* ==========================================macros======================================*/
//OLED面板参数, 供直驱接口和U8g2适配层共用
#define     OLED_CMD                        0U
#define     OLED_DATA                       1U
#define     OLED_WIDTH_PIXELS               128U
#define     OLED_HEIGHT_PIXELS              64U
#define     OLED_PAGE_COUNT                 8U
#define     OLED_SPI_TIMEOUT                0x0000FFFFUL

/* ==========================================globals=====================================*/


/* ==========================================types=======================================*/
//页面渲染共享快照, 在统一渲染入口开始时刷新一次
typedef struct
{
	s16 sWaterTemp;
	s16 sWaterTemp1;
	s16 sWaterTemp2;
	s16 sBoardTemp5V;
	s16 sBoardTemp12V;
	u16 usInitProgress;
	u16 usErrCode;
	u16 usAutoOffCnt;
	u16 usLightAdc;
	u16 us12VVolt;
	u16 usVinVolt;
	u16 usVinCurrMa;
	u16 usVinPowerW;
	u16 usHeatCurrMa;
	u16 usPumpCurrMa;
	u16 usO2CurrMa;
	u16 usLightPowerW;
	u16 usHeatPowerW;
	u16 usPumpPowerW;
	u16 usO2PowerW;
	u16 usLightCurrMa;
	u16 usLightCtrlPower;
	u16 usLightWarm;
	u16 usLightBlue;
	u16 usLightGreen;
	u16 usLightRed;
	u16 usO2PumpSpeed;
	u16 usPumpSpeed;
	u16 usFanMode;
	s16 sHeatTargetTemp;
	s16 sFanTempStart;
	s16 sFanTempFull;
	u16 usHeatPwm;
	u16 usFanPwm;
	u8 ucHeatEnable;
	u8 ucFanEnable;
	u8 ucLightMode;
	u8 ucLightRgbMode;
	u8 ucLightDevState;
	u8 ucUpgradePercent;
	u8 ucBuzOff;
	u8 ucForceClose;
	const char *pcLightMode;
	const char *pcPumpMode;
	const char *pcO2PumpMode;
	const char *pcHeatMode;
	const char *pcUpgradeStage;
	const char *pcUpgradeNote;
	const char *pcErrModule;
	const char *pcErrMode;
	const char *pcErrAction;
	const char *pcAlarmDesc;
}DispUiSnapshot_T;


/* ==========================================extern======================================*/

#if(dispUSE_U8G2 == 0)
void vDisp_OledDrawPixel(u8 x, u8 y, bool on);
void vDisp_OledDrawHLine(u8 x, u8 y, u8 len, bool on);
void vDisp_OledDrawVLine(u8 x, u8 y, u8 len, bool on);
void vDisp_OledDrawChar6x8(u8 x, u8 y, char c);
void vDisp_OledDrawString6x8(u8 x, u8 y, const char *str);
void vDisp_OledFillBuffer(u8 value);
#endif  // dispUSE_U8G2 == 0

//显示驱动与页面渲染接口
void vDisp_Init(void);
void vDisp_Refresh(void);
void vDisp_SetPower(bool on);
void vDisp_SetContrast(u8 value);
void vDisp_ClearBuffer(void);
void vDisp_UiTest(void);
bool bDisp_RenderUi(void);
void vDisp_ClearRegion(u8 x, u8 y, u8 w, u8 h);
void vDisp_DrawPageFrame(const char *title);
void vDisp_DrawFullTopBar(const char *title, const char *tag);
void vDisp_DrawStatusTag(const char *label);
void vDisp_DrawHintLine(void);
void vDisp_UpdateUiSnapshot(void);
const DispUiSnapshot_T *ptDisp_GetUiSnapshot(void);

#ifdef __cplusplus
}
#endif

#endif //boardDISPLAY_EN

#endif  //MD_DISPLAY_API_H

