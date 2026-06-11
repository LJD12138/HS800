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
#define TIM_MAX    5999U   // 99.9H
#define TIM_MAX1   59999U  // 999H
#define POWER_MAX  9999U
#define SOC_MAX    188U
#define O_ERR      0xFFFFU


/* ==========================================globals=====================================*/


/* ==========================================types=======================================*/


/* ==========================================extern======================================*/
//设备状态
void Display_IconWifi(void);
void Display_IconBL(void);
void Display_IconFan(void);
void Display_IconBuz(void);
void Display_IconLight(void);
void Display_IconUsbOut(void);
void Display_IconDcOut(void);
void Display_IconAcOut(void);
void Display_IconAcIn(void);
void Display_IconDcIn(void);
void Display_IconPvIn(void);
void Display_IconParaIn(void);
void Display_IconExpCap(void);
void Display_IconMultiPack(void);
void Display_IconWirelessCharge(void);
void Display_GRID(void);

//符号
void Display_IconUpdate(void);
void Display_IconTimeH(void);
void Display_IconTimeM(void);
void Display_IconClock(void);
void Display_IconTimeDot(void);
void Display_IconKwhDot(void);
void Display_IconKwh(void);
void Display_IconSave(void);
void Display_IconTest(void);

//错误和保护
void Display_IconSysErr(void);
void Display_IconBatLow(void);
void Display_IconOT(void);
void Display_IconOutOT(void);
void Display_IconUT(void);
void Display_IconOL(void);
void Display_IconBmsOT(void);

//控制指令
void Display_SetStandbyMode(void);
void Display_SetRunMode(void);
void Display_ClearData(void);
void Display_RefreshData(void);

//数据显示
void Display_NoShowAll(void);
void Display_ShowAll(void);
void Display_ShowON(void);
void Display_ShowOFF(void);
void Display_ForeverShow(void);
void Display_Time(u16 min);
void Display_TimRoll(u8 tim);
void Display_BatChgRoll(u8 soc);
void Display_BAT(u8 cds,  u8 soc);
void Display_Soc(u8 soc);
void Display_InPwr(u16 power);
void Display_OutPwr(u16 power);
void Display_ShowErrCode(u32 list);
void Display_Cap(u16 num);
void Display_IconExpCapNum(u8 num);

//特殊模式
void Display_OutNum(s16 num);
void Display_UpdateProgress(u16 frame_num, u16 rec_frame_num);
void Display_UpdateState(u8 obj, u8 proto, u8 num);
void Display_UpdateTime(u16 min);
void Display_UpdateAnimation(void);





void HT1621_Init(void);

#endif  // boardDISPLAY_EN

#ifdef __cplusplus
}
#endif

#endif  //MD_DISPLAY_API_H
