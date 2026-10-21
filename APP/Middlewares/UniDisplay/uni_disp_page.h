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
 * 4. vOnExit  : 退出页面, 清理标志/保存临时参数;
 * 5. bOnDone  : (可选) 页面内在流程完成自报 (如开机动画走满), 场景步骤推进时与步骤 bDone 与合成 (NULL=无自完成语义)。
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
/* 通用哨兵与容量上限 (与项目枚举解耦: 项目页面 ID 必须小于 dispPAGE_REG_MAX) */
#define			dispPAGE_NONE						(0xFFU)		/* 无效页面 ID */
#ifndef			dispPAGE_REG_MAX
#define			dispPAGE_REG_MAX					(16U)		/* 页面注册表容量 (项目 board 头可按产品页面数覆盖) */
#endif
#ifndef			dispPAGE_STACK_DEPTH
#define			dispPAGE_STACK_DEPTH				(4U)		/* 页面栈深度 (项目 board 头可按钻取层级覆盖) */
#endif

/*----------------框架功能裁剪 (断码屏极简形态: 项目 board 头置 0, 裁掉对应域的 RAM/Flash 开销)----------------*/
#ifndef			boardDISP_SCENE_EN
#define			boardDISP_SCENE_EN					(1)			/* 场景状态机使能: 0=无场景/overlay 调度, 导航全由项目显式 RequestPage 驱动 */
#endif
#ifndef			boardDISP_NAV_STACK_EN
#define			boardDISP_NAV_STACK_EN				(1)			/* 页面栈使能: 0=裁掉钻取返回链, Push/Pop/CanPop 恒返回 false */
#endif
#define			dispSCENE_NONE						(0x00U)		/* 无场景/自由页 */
#define			dispDEV_STATE_NONE					(0xFFU)		/* 无设备状态 */
#define			dispFRAME_PERIOD_DEFAULT_MS			(50U)		/* 默认帧周期 ms (项目 ops 未提供 usDefaultFramePeriodMs 时回退) */
#define			dispFRAME_PERIOD_MIN_MS				(20U)		/* 页面帧周期下限 ms */

//****************************************************Types*********************************************************************//
/* 数据快照类型 (前向声明, opaque): 完整字段集由项目件定义
 * (HS800: Hardware/MD_Display/md_display_data.h)。
 * 框架仅感知类型存在与首字段 ucDevState (状态路由依据), 其余字段对框架透明 */
typedef struct DispDataSnapshot_T DispDataSnapshot_T;
/* 统一页面 ID 类型 (通用件 opaque): 具体取值由项目件枚举定义, 页 ID 必须小于 dispPAGE_REG_MAX
 * (HS800: Hardware/MD_Display/md_display_data.h 的 MDPageId_E, 值 0 保留) */
typedef uint8_t DispPageId_E;

/* 页面控制块定义 */
typedef struct DispPage_T DispPage_T;
struct DispPage_T
{
	DispPageId_E		ePageId;			/* 页面唯一 ID */

    /* 1. 进入页面回调: 初始化显存/加载 LVGL Screen/重置本地页面变量 */
	void				(*vOnEnter)(void);

	/* 2. 退出页面回调: 清除特定标志/保存页面临时参数 */
	void				(*vOnExit)(void);

    /* 3. 帧刷新回调: 周期调用 (周期由页面 usRefreshMs 声明), 根据 Snapshot 渲染;
	 *    p_data 可为 NULL (项目未提供快照或快照未就绪), 页面须判空后渲染 */
	void				(*vOnUpdate)(const DispDataSnapshot_T *p_data, bool b_force);

    /* 4. 异步事件回调: 处理按键、告警; 返回 true 表示消费, false 表示透传框架默认处理 */
	bool				(*bOnEvent)(DispEvent_E e_event, uint32_t ul_param);

	/* 5. (可选) 本页内在流程完成判定: 页面自报完成 (如开机动画走满);
	 * 场景步骤推进时与步骤 bDone 与合成 (各自 NULL 视为满足, 均 NULL 为常驻步); NULL=无自完成语义 */
	bool				(*bOnDone)(void);

    /* 页面帧刷新周期 ms: 0=使用框架默认周期 dispFRAME_PERIOD_DEFAULT_MS; 低于下限时钳位到 dispFRAME_PERIOD_MIN_MS */
	uint16_t			usRefreshMs;

    /* 页面上下文 (opaque, 框架不解释): 页面模板复用时挂数据源/状态块,
	 * 页面回调内经 pDisp_GetPageUser 读取 (仅当前页有效); NULL=无上下文 */
	void				*pUser;
};

/*----------------场景与步骤 (通用件契约)----------------*/
/* 场景: 一段有始有终的显示流程, 由有序步骤表构成, 如 POWER_ON = BOOT → WORK */
/* 场景 ID 类型 (通用件 opaque): 具体取值由项目件枚举定义, 0 保留为 dispSCENE_NONE
 * (HS800: Hardware/MD_Display/md_display_data.h 的 MDSceneId_E, 值 0 保留) */
typedef uint8_t DispScene_E;

/* 步骤: 场景内一页的驻留单元 (页面 + 完成条件 + 驻留上限) */
typedef struct
{
	DispPageId_E		ePage;			/* 本步骤展示的页面 */
	bool			(*bDone)(const DispDataSnapshot_T *p_snapshot, uint32_t ul_dwell_ms);	/* 完成条件, NULL=常驻步 */
	uint16_t			usMaxHoldMs;		/* 驻留上限 ms: 0=不限, 到期强制步进 (超时兜底) */
}DispFlowStep_T;

/*----------------项目适配操作集 (框架唯一项目耦合点; 未赋值的函数指针使用框架默认行为)----------------*/
typedef struct
{
	/* 数据 (pGetSnapshot 必选; ucGetDevState 缺省返回 dispDEV_STATE_NONE) */
	const DispDataSnapshot_T *(*pGetSnapshot)(void);
	uint8_t				(*ucGetDevState)(const DispDataSnapshot_T *p_snapshot);

	/* 流程策略 (断码屏可只实现映射与步骤表, 其余 NULL) */
	DispScene_E			(*eMapDevStateToScene)(uint8_t uc_dev_state);
	const DispFlowStep_T *(*pGetSceneSteps)(DispScene_E e_scene, uint8_t *p_num);
	uint8_t				(*ucSceneEntryStep)(DispScene_E e_scene, const DispDataSnapshot_T *p_snapshot);
	bool				(*bAutoOffArmed)(uint8_t uc_dev_state);

	/* 渲染后端与节拍 (vFlush 必选; 其余可 NULL) */
	void				(*vFlush)(void);
	void				(*vPostSwitchAudit)(void);		/* 页面切换完成后审计 (如 LVGL 内存水位); NULL=跳过 */
	void				(*vOnPageSwitch)(DispPageId_E e_from, DispPageId_E e_to);	/* 切页钩子 (OnExit 后/OnEnter 前, TFT/LVGL 挂切屏动画; 断码屏 NULL) */
	void				(*vBacklight)(bool on);
	bool				(*bDisplayActive)(void);		/* NULL=跟随背光; 断码屏可恒 true */
	uint32_t			(*ulGetTickMs)(void);			/* NULL=按帧周期累计驻留时间 */

	/* 线程与帧周期适配 (可选; NULL/0 使用框架默认, 通用件不依赖项目 port 头) */
	void				(*vEnterCritical)(void);		/* 临界区进入; NULL=空实现 */
	void				(*vExitCritical)(void);			/* 临界区退出; NULL=空实现 */
	uint16_t			usDefaultFramePeriodMs;			/* 默认帧刷新周期 ms; 0=dispFRAME_PERIOD_DEFAULT_MS */

	/* 可选策略 */
	bool				(*bOverlayActive)(const DispDataSnapshot_T *p_snapshot);
	DispPageId_E		(*eOverlayPage)(void);
	uint16_t			(*usGetAutoOffTime)(void);
	void				(*vWakeTask)(void);
	void				(*vBeepOnLightOn)(void);
}DispOps_T;

/*----------------项目侧数据钩子 (实现在项目件, 声明归项目数据头, 通用件不感知)----------------*/
/* 快照捕获/自动息屏判定/场景映射等钩子由项目实现并经 DispOps_T 注入;
 * 通用件只持有快照指针与 ops, 不引用任何项目符号或快照字段 */

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* UNI_DISP_PAGE_H */
