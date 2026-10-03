/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_page.h
 * Date    : 2026-09-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 页面契约 - 页面控制块与生命周期回调定义 (通用件, 不含业务字段)
 * -------------------------------------------------------
 * 生命周期 (所有回调仅在显示任务上下文被框架调用):
 * 1. vOnEnter : 进入页面, 初始化显存/加载 Screen/重置页面局部变量;
 * 2. vOnUpdate: 帧刷新, 周期由页面 usRefreshMs 声明(0=默认: 断码屏 500ms/TFT 33ms), 依据 Snapshot 渲染;
 * 3. bOnEvent : 异步事件, 返回 true 表示消费, false 表示透传框架默认处理;
 * 4. vOnExit  : 退出页面, 清理标志/保存临时参数。
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef UNI_DISP_PAGE_H
#define UNI_DISP_PAGE_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "uni_disp_event.h"

#if (boardDISPLAY_EN)

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//
/* 数据快照类型 (前向声明, opaque): 完整字段集由项目件定义
 * (P3601: Hardware/MD_Display/md_display_data.h)。
 * 框架仅感知类型存在与首字段 ucDevState (状态路由依据), 其余字段对框架透明 */
typedef struct DispDataSnapshot_T DispDataSnapshot_T;
/* 统一页面枚举 */
typedef enum
{
	PAGE_ID_INIT = 0,	/* 初始化页面 (预留: 不显示) */
	PAGE_ID_BOOT,		/* 开机/自检页面 */
	PAGE_ID_WORK,		/* 正常运行主页面 */
	PAGE_ID_FAULT,		/* 故障告警专用页/弹窗 */
	PAGE_ID_ENG_MODE,	/* 工程师模式/校准页 */
	PAGE_ID_UPDATE,		/* 固件升级进度页 */
	PAGE_ID_CLOSING,	/* 关机动画/提示页 */
	PAGE_ID_SHUT_DOWN,	/* 关机状态页面 (预留: 不显示) */
	PAGE_ID_MAX			/* 无映射哨兵, 勿删 */
}DispPageId_E;

/* 页面控制块定义 */
typedef struct DispPage_T DispPage_T;
struct DispPage_T
{
	DispPageId_E		ePageId;			/* 页面唯一 ID */

    /* 1. 进入页面回调: 初始化显存/加载 LVGL Screen/重置本地页面变量 */
	void				(*vOnEnter)(void);

	/* 2. 退出页面回调: 清除特定标志/保存页面临时参数 */
	void				(*vOnExit)(void);

    /* 3. 帧刷新回调: 周期调用(断码屏 500ms, TFT 33ms), 根据 Snapshot 渲染 */
	void				(*vOnUpdate)(const DispDataSnapshot_T *p_data, bool b_force);

    /* 4. 异步事件回调: 处理按键、告警; 返回 true 表示消费, false 表示透传框架默认处理 */
	bool				(*bOnEvent)(DispEvent_E e_event, uint32_t ul_param);

    /* 页面帧刷新周期 ms: 0=使用框架默认 boardDISP_REFRESH_TIME; 低于下限时钳位到 dispFRAME_PERIOD_MIN_MS */
	uint16_t			usRefreshMs;
};

/*----------------项目侧必须实现的三个数据钩子 (实现在项目件, 如 md_display_data.c)----------------*/
/* 获取当前全局数据快照 (仅显示任务调用, 每帧一次; 项目件内部临界区保护) */
void vDisp_DataCaptureSnapshot(DispDataSnapshot_T *p_snapshot);

/* 息屏倒计时生效判定 (项目策略: 如仅工作状态参与倒计时递减) */
bool bDisp_DataAutoOffArmed(uint8_t uc_dev_state);

/* 设备状态 -> 页面 映射表 (项目件, 按产品状态机实现)
 * 返回 PAGE_ID_MAX 表示该状态无页面映射 (维持当前页) */
DispPageId_E eDisp_MapDevStateToPage(uint8_t uc_dev_state);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* UNI_DISP_PAGE_H */
