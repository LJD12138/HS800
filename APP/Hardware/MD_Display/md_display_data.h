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

/* 系统级故障位 (镜像 tSysInfo.uErrCode.usCode, 位序与 SysErrCode_U 一致; 仅定义主界面图标消费位, 用到位再补) */
#define			DISP_SYSERR_BIT_OT						(1U << 0)	/* 过温 */
#define			DISP_SYSERR_BIT_OL						(1U << 4)	/* 过载 */

//****************************************************Types*********************************************************************//
/*----------------页面/场景 ID 集 (通用件 opaque 契约 DispPageId_E/DispScene_E=uint8_t 的本工程取值)----------------*/
typedef enum
{
	PAGE_ID_INIT = 0,		/* 初始化页面 (预留: 不显示) */
	PAGE_ID_BOOT,			/* 开机/自检页面 */
	PAGE_ID_WORK_MAIN1,		/* 主页1 (WORK 组入口页) */
	PAGE_ID_FAULT,			/* 故障告警专用页/弹窗 */
	PAGE_ID_ENG_MODE,		/* 工程师模式主菜单 */
	PAGE_ID_ENG_PV,			/* 工程师模式-参数查看 (页面栈二级页) */
	PAGE_ID_ENG_PV_DETAIL,	/* 工程师模式-参数查看详情 (页面栈三级页) */
	PAGE_ID_UPDATE,			/* 固件升级进度页 */
	PAGE_ID_CLOSING,		/* 关机动画/提示页 */
	PAGE_ID_SHUT_DOWN,		/* 关机状态页面 (预留: 不显示) */
	PAGE_ID_ENG_PARAM_SET,	/* 工程师模式-记忆参数设置 (页面栈二级页) */
	PAGE_ID_ENG_SYS_SET,	/* 工程师模式-系统设置 (页面栈二级页) */
	PAGE_ID_ENG_CONFIRM,	/* 工程师模式-确认对话框 (页面栈三级页) */
	PAGE_ID_MAX				/* 项目页面数哨兵: 必须小于 dispPAGE_REG_MAX, 勿删 */
}MDPageId_E;

typedef enum
{
	DISP_SCENE_NONE = 0,		/* 无映射: 保持当前场景 (DS_ERR/DS_LOST/未知状态) */
	DISP_SCENE_INIT,			/* 初始化场景 (单步): 系统就绪前常驻 INIT 页 (未就绪门控) */
	DISP_SCENE_POWER_ON,		/* 开机场景: BOOT → WORK */
	DISP_SCENE_POWER_OFF,		/* 关机场景: CLOSING → SHUT_DOWN */
	DISP_SCENE_UPDATE,			/* 升级场景 (单步) */
	DISP_SCENE_ENG,				/* 工程模式场景 (单步) */
	DISP_SCENE_MAX
}MDSceneId_E;

/* 统一显示数据原子快照结构体 (HS800 项目件) */
struct DispDataSnapshot_T
{
	uint8_t				ucDevState;				/* 设备运行状态 (调度依据, 见 eDisp_MapDevStateToScene) */
	uint8_t				ucSoc;					/* 电池剩余百分比 (0~100) */
	uint8_t				ucChgState;				/* 充电状态: 0=未充, 1=慢充, 2=快充 */
	uint16_t			usInPwr;				/* 当前总输入功率 (W) */
	uint16_t			usOutPwr;				/* 当前总输出功率 (W) */
	uint16_t			usChgFullTimeMin;		/* 充满剩余时间 (分钟) */
	uint16_t			usDisChgEmptyTimeMin;	/* 放空剩余时间 (分钟) */
	uint16_t			usPrimaryErrCode;		/* 主告警轮显错误码 (0 为无故障, 源自 usDisp_ErrCodeDisplay) */
	uint16_t			usSysErrCode;			/* 系统故障位图镜像 (tSysInfo.uErrCode.usCode) */
	uint32_t			ulPortActiveFlags;		/* 各端口稳态激活位图 */
	uint32_t			ulPortBlinkFlags;		/* 各端口错误态位图 (错误/保护态闪烁) */
	uint8_t				ucPvWorkMode;			/* MPPT 工作模式 (MpptWorkMode_E, PV 图标选源依据) */
	bool				bBmsChg;				/* BMS 充电中 (tBms.eWorkState == BWS_CHG, 剩余时间字段选择) */
	bool				bDisChgActive;			/* 任一输出端口放电中 (AC>=WORK/USB/DC/照明, AC 模式动画依据) */
	bool				bBatErr;				/* 电池硬件故障标志 */
	bool				bBatLock;				/* 电池保护锁定标志 */
};

//****************************************************Extern********************************************************************//
/* 快照获取 (仅显示任务调用): 项目持有快照缓冲, 框架只读指针, 不做栈拷贝 */
const DispDataSnapshot_T *pDisp_DataSnapshot(void);

/* 读取快照中的设备状态 (DispOps_T.ucGetDevState 实现) */
uint8_t ucDisp_DataGetDevState(const DispDataSnapshot_T *p_snapshot);

/* 快照捕获 (项目件内部: 页面首帧渲染前抓取一次完整快照) */
void vDisp_DataCaptureSnapshot(DispDataSnapshot_T *p_snapshot);

/* 息屏倒计时生效判定 (DispOps_T.bAutoOffArmed 实现) */
bool bDisp_DataAutoOffArmed(uint8_t uc_dev_state);

/*----------------场景流程数据钩子 (DispOps_T 装配)----------------*/
/* 设备状态 -> 场景 映射 (overlay 触发状态返回 dispSCENE_NONE, 由框架 overlay 电平评估处理) */
DispScene_E eDisp_MapDevStateToScene(uint8_t uc_dev_state);

/* 场景 -> 步骤表 查询 (返回表首地址, 步数经 p_num 回填; 无效场景返回 NULL) */
const DispFlowStep_T *pDisp_GetSceneSteps(DispScene_E e_scene, uint8_t *p_num);

/* 场景入口步选择 (处理"关机后再开机跳过 INIT""工程/升级退回直接进 WORK"等边界) */
uint8_t ucDisp_SceneEntryStep(DispScene_E e_scene, const DispDataSnapshot_T *p_snapshot);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DISPLAY_DATA_H_ */
