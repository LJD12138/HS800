/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_data.h
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 数据快照池 (项目件) - HS800 TFT/LVGL 字段集与状态路由
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DISPLAY_DATA_H_
#define MD_DISPLAY_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDISPLAY_EN)
#include "uni_disp_page.h"                     /* DispDataSnapshot_T 前向声明与数据钩子契约 */

//****************************************************Macros********************************************************************//
/* 端口激活/错误闪烁位掩码 (ulPortActiveFlags 与 ulPortBlinkFlags 使用同一位序) */
#define			DISP_PORT_BIT_USB_OUT					(1U << 0)
#define			DISP_PORT_BIT_DC_OUT					(1U << 1)
#define			DISP_PORT_BIT_AC_OUT					(1U << 2)
#define			DISP_PORT_BIT_AC_IN						(1U << 3)
#define			DISP_PORT_BIT_DC_IN						(1U << 4)
#define			DISP_PORT_BIT_LIGHT						(1U << 5)

/* 系统级故障位 (镜像 tSysInfo.uErrCode.usCode, 位序与 SysErrCode_U 一致) */
#define			DISP_SYSERR_BIT_OT						(1U << 0)	/* 过温 */
#define			DISP_SYSERR_BIT_UT						(1U << 1)	/* 低温 */
#define			DISP_SYSERR_BIT_OV						(1U << 2)	/* 过压 */
#define			DISP_SYSERR_BIT_UV						(1U << 3)	/* 欠压 */
#define			DISP_SYSERR_BIT_OL						(1U << 4)	/* 过载 */
#define			DISP_SYSERR_BIT_0_SOC					(1U << 5)	/* 0%SOC */
#define			DISP_SYSERR_BIT_CLOSE_FAULT				(1U << 6)	/* 关闭失败 */
#define			DISP_SYSERR_BIT_BOOT_FAULT				(1U << 7)	/* 开启失败 */
#define			DISP_SYSERR_BIT_DISCHG_OL				(1U << 8)	/* 放电过功率 */
#define			DISP_SYSERR_BIT_TASK_FAULT				(1U << 9)	/* 任务初始化/创建失败 */

//****************************************************Types*********************************************************************//
/* 统一显示数据原子快照结构体 (HS800 项目件) */
struct DispDataSnapshot_T
{
	uint8_t				ucDevState;				/* 设备运行状态 (状态路由依据, 见 eDisp_MapDevStateToPage) */
	uint8_t				ucSoc;					/* 电池剩余百分比 (0~100) */
	uint8_t				ucChgState;				/* 充电状态: 0=未充, 1=慢充, 2=快充 */
	uint16_t			usInPwr;				/* 当前总输入功率 (W) */
	uint16_t			usOutPwr;				/* 当前总输出功率 (W) */
	uint16_t			usChgFullTimeMin;		/* 充满剩余时间 (分钟) */
	uint16_t			usDisChgEmptyTimeMin;	/* 放空剩余时间 (分钟) */
	uint16_t			usPrimaryErrCode;		/* 主告警轮显错误码 (0 为无故障, 源自 usDisp_ErrCodeDisplay) */
	uint16_t			usSysErrCode;			/* 系统故障位图镜像 (tSysInfo.uErrCode.usCode) */
	uint32_t			ulPortActiveFlags;		/* 各端口稳态激活位图 */
	uint32_t			ulPortBlinkFlags;		/* 各端口错误态位图 */
	bool				bBatErr;				/* 电池硬件故障标志 */
	bool				bBatLock;				/* 电池保护锁定标志 */
};

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DISPLAY_DATA_H_ */
