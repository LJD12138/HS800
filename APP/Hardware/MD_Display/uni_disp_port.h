/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : uni_disp_port.h
 * Date    : 2026-09-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 项目适配实现 - 临界区/tick/任务唤醒/背光等项目原语 (由项目件 DispOps_T 引用)
 * -------------------------------------------------------
 * 移植说明:
 * 1. FreeRTOS 环境: 提供任务级临界区与任务通知唤醒, 等待原语由任务主体(通知+超时)实现;
 * 2. 裸机环境: 临界区为空实现, 唤醒为空操作;
 * 3. 项目适配: 通用件只经 DispOps_T 访问项目资源; 本文件提供 ops 所需的项目原语实现,
 *    移植新工程时通用件(core/nav/scene/power/page/event)零修改;
 * 4. 本文件不含任何业务逻辑, 业务实现位于 uni_disp_port.c (转调项目件)。
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef UNI_DISP_PORT_H
#define UNI_DISP_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDISPLAY_EN)
#include "uni_disp_page.h"		/* 快照前向声明 / DispPageId_E / 通用哨兵 (项目快照结构不再进入通用层) */

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

//****************************************************Macros********************************************************************//



//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
/*----------------项目适配原语 (由 uni_disp_port.c 转调项目件实现, 调度核心经此访问项目资源)----------------*/
/* 临界区进入/退出 (绑定 DispOps_T.vEnterCritical/vExitCritical) */
void vDisp_PortEnterCritical(void);
void vDisp_PortExitCritical(void);

/* 执行一轮 UI 刷新 (项目件实现: EEZ ui_tick/自绘动画/lv_timer_handler) */
void vDisp_PortUiRefresh(void);

/* 切页后内存审计 (绑定 DispOps_T.vPostSwitchAudit; LVGL 水位/碎片率) */
void vDisp_PortMemAudit(void);

/* 背光硬件开关 (项目件实现) */
void vDisp_PortSetBacklight(bool on);

/* 获取持久化自动息屏时间 (0=常亮, 项目件实现) */
uint16_t usDisp_PortGetAutoOffTime(void);

/* 获取毫秒时基 (场景驻留计时; 无时基项目返回 0, 框架退回按帧周期累计) */
uint32_t ulDisp_PortGetTickMs(void);

/* 系统初始化就绪判定 (INIT 步完成条件输入: 未就绪保持 INIT 步, 项目件实现) */
bool bDisp_PortIsSystemReady(void);

/* overlay 激活判定 (项目件实现: 场景调度每帧电平评估压入/弹出的触发条件, 可基于任意快照字段扩展, 如 ucDevState==DS_ERR) */
bool bDisp_PortIsOverlayActive(const DispDataSnapshot_T *p_snapshot);

/* 故障 overlay 页 ID (项目件指定; 场景调度 overlay 压入时使用) */
DispPageId_E eDisp_PortGetOverlayPage(void);

/* 常亮亮屏提示钩子 (项目件实现, 如蜂鸣提示音; 无需求可空实现) */
void vDisp_PortBeepOnLightOn(void);

#if (boardUSE_OS)
/* 绑定显示任务句柄 (显示任务创建后调用一次, 供唤醒钩子使用) */
void vDisp_PortSetTaskHandle(TaskHandle_t t_handle);

/* 唤醒显示任务: 事件入队/背光点亮后调用, 使 EnginePoll 提前执行 */
void vDisp_PortWakeTask(void);
#else
#define			vDisp_PortSetTaskHandle(h_handle)		((void)0)
#define			vDisp_PortWakeTask()					((void)0)
#endif  //boardUSE_OS

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* UNI_DISP_PORT_H */
