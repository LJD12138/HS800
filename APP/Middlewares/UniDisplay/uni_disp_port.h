/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_port.h
 * Date    : 2026-09-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 移植层 - 临界区宏/任务唤醒钩子/项目适配原语 (工程唯一需要按项目/平台适配的文件)
 * -------------------------------------------------------
 * 移植说明:
 * 1. FreeRTOS 环境: 提供任务级临界区与任务通知唤醒, 等待原语由任务主体(通知+超时)实现;
 * 2. 裸机环境: 临界区为空实现, 唤醒为空操作;
 * 3. 项目适配: 本文件引入项目快照定义并声明适配原语, 调度核心(core/page/event)经由本层访问
 *    项目资源, 移植新工程时除本文件与 uni_disp_port.c 外的 UniDisplay 文件零修改;
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
#include "MD_Display/md_display_data.h"		/* 工程适配点: 项目快照定义 (调度引擎需栈分配快照并访问首字段 ucDevState) */

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

//****************************************************Macros********************************************************************//

/* 板级显示参数转接 (框架唯一允许引用 board_config 显示参数的位置, 其余通用文件一律使用别名) */
#define			dispFRAME_PERIOD_MIN_MS					(20U)	/* 页面帧周期下限 ms (50MHz SPI 下全屏传输约25ms, 动画页按20ms/50FPS目标) */

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
/*----------------项目适配原语 (由 uni_disp_port.c 转调项目件实现, 调度核心经此访问项目资源)----------------*/
/* 执行一轮 UI 刷新 (项目件实现: EEZ ui_tick/自绘动画/lv_timer_handler) */
void vDisp_PortUiRefresh(void);

/* 背光硬件开关 (项目件实现) */
void vDisp_PortSetBacklight(bool on);

/* 获取持久化自动息屏时间 (0=常亮, 项目件实现) */
uint16_t usDisp_PortGetAutoOffTime(void);

/* 系统初始化就绪判定 (false 时框架锁定 INIT 页, 项目件实现) */
bool bDisp_PortIsSystemReady(void);

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
