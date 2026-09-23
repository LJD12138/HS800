/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display\user_ui
 * File    : energy_ring.h
 * Date    : 2026-09-22
 * Author  : LJD(291483914@qq.com)
 * Desc    : Energy Ring UI Component - Header
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

/**
 * @file energy_ring.h
 * @brief Energy Ring UI Component - Header
 * @author User
 * @version 1.0
 */

#ifndef __ENERGY_RING_H
#define __ENERGY_RING_H


//****************************************************Includes******************************************************************//
#include "main.h"

#if(boardDISPLAY_EN)
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Types*********************************************************************//
/* Energy Ring SOC显示模式 */
typedef enum
{
	ENERGY_RING_SOC_STATIC = 0,	/* 静态显示模式：根据SOC显示固定电量 */
	ENERGY_RING_SOC_CHARGING	/* 充电模式：显示充电动画 */
}EnergyRing_SocMode_T;

/* Energy Ring 控制结构体 */
typedef struct
{
	lv_obj_t			* pObj;
	lv_timer_t			* pTimer;
	u16					usSegCount;
	u16					usActiveSeg;
	u16					usStartAngle;
	u16					usSweepAngle;
	u16					usGapAngle;
	u16					usRenderStartAngle;
	u16					usSegSpanAngle;
	u16					usSegStepAngle;
	u16					usAnimPeriodMs;
	lv_coord_t			tRadius;
	lv_coord_t			tWidth;
	lv_coord_t			tCenterYOffset;
	bool				bEnabled;
	lv_color_t			tBgColor;
	lv_color_t			tSegOffColor;
	lv_color_t			tSegOnColor;
	lv_color_t			tSegHeadColor;
    
    /* 内环装饰环参数 */
	u16					usInnerSegCount;	/* 内环矩形数量 */
	lv_coord_t			tInnerRadius;		/* 内环半径 */
	lv_coord_t			tInnerSegLen;		/* 内环矩形长度 */
	lv_coord_t			tInnerSegWidth;		/* 内环矩形宽度 */
	lv_color_t			tInnerSegColor;		/* 内环矩形颜色 */
	bool				bInnerRingEnabled;	/* 内环使能标志 */
    
    /* SOC显示相关参数 */
	u8					ucSocValue;			/* SOC值(0-100) */
	bool				bCharging;			/* 充电状态标志 */
	EnergyRing_SocMode_T	eSocMode;			/* SOC显示模式 */
	bool				bAnimDirDec;		/* 动画递减方向标志：true-递减，false-递增 */
}EnergyRing_T;

//****************************************************Extern********************************************************************//

void EnergyRing_Start(EnergyRing_T *p_ring);
void EnergyRing_Stop(EnergyRing_T *p_ring);
void EnergyRing_Pause(EnergyRing_T *p_ring);
void EnergyRing_UpdateSoc(EnergyRing_T *p_ring, u8 uc_soc, bool b_charging);

#ifdef __cplusplus
}
#endif

#endif  //boardDISPLAY_EN

#endif /* __ENERGY_RING_H */
