/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display\user_ui
 * File    : eng_mode_ui.h
 * Date    : 2026-06-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : 工程模式LVGL UI - 主菜单/参数查看/记忆参数设置/系统设置
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef ENG_MODE_UI_H
#define ENG_MODE_UI_H


#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if(boardENG_MODE_EN && boardDISPLAY_EN)

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//

/* 工程模式页面枚举 */
typedef enum
{
	ENG_PAGE_MAIN_MENU = 0,	/* 主菜单 */
	ENG_PAGE_PARAM_VIEW,	/* 参数查看 */
	ENG_PAGE_PARAM_SET,		/* 记忆参数设置 */
	ENG_PAGE_SYS_SET,		/* 系统设置 */
	ENG_PAGE_CONFIRM,		/* 确认对话框 */
	ENG_PAGE_PV_DETAIL,		/* 参数查看详情 (框架页面栈管理的页内子级, 无独立LVGL容器体系) */
	ENG_PAGE_MAX
}EngModePage_E;

//****************************************************Globals*******************************************************************//


//****************************************************Extern********************************************************************//
void vEngMode_UiCreate(void);
void vEngMode_UiDelete(void);
void vEngMode_UiTick(void);
bool bEngMode_IsExitReq(void);

/* 页内子级粒度生命周期 (框架页面控制块调用) */
void vEngMode_PageEnter(EngModePage_E e_page);

/* 框架页 vOnExit 统一入口: 有返回链时仅释放本页; 无返回链(离场景/弹栈到根)时整树释放 */
void vEngMode_PageLeave(EngModePage_E e_page);

/* 参数查看详情页 UI (三级页面栈试点) */
void vEngPvDetail_UiCreate(uint32_t ul_arg);	/* ul_arg 低 8 位: PV Tab 索引 */
void vEngPvDetail_UiDelete(void);

void vEngMode_KeyUp(void);
void vEngMode_KeyDown(void);
void vEngMode_KeyEnter(void);
void vEngMode_KeyBack(void);
void vEngMode_KeyLeft(void);
void vEngMode_KeyRight(void);

#endif  /* boardENG_MODE_EN && boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* ENG_MODE_UI_H */