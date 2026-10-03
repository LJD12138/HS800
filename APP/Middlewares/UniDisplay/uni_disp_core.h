/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_core.h
 * Date    : 2026-09-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 调度核心对外接口 - 页面注册/路由调度/弹窗栈/背光息屏/轮询引擎/适配器原语
 * -------------------------------------------------------
 * API 调用规约 (线程安全矩阵, 代码评审依据):
 * | 接口                     | 允许调用上下文            | 机制                     |
 * | bDisp_PostEvent          | 任意任务(不建议 ISR)      | 临界区入队+通知唤醒      |
 * | vDisp_RequestPage        | 任意任务                  | 目标页单字节写, 原子     |
 * | bDisp_Switch             | 任意任务                  | 内部状态单写+通知唤醒    |
 * | bDisp_IsBacklightOn      | 任意任务                  | 布量单读                 |
 * | vDisp_TickTimer          | 任意任务/定时器           | 息屏 1S 周期递减         |
 * | vDisp_PushPage/PopPage   | 仅显示任务                | 单上下文                 |
 * | vDisp_EnginePoll         | 仅显示任务                | 单上下文                 |
 * | usDisp_GetFramePeriod    | 任意任务                  | 单读(切页单写)           |
 * | 页面四回调               | 仅显示任务(框架调用)      | 单上下文                 |
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef UNI_DISP_CORE_H
#define UNI_DISP_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDISPLAY_EN)
#include "uni_disp_page.h"
#include "uni_disp_port.h"

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
/*----------------框架初始化与页面管理----------------*/
/* 框架核心初始化 (显示任务创建前调用; 息屏参数复位) */
void vDisp_CoreInit(void);

/* 页面静态注册 (显示任务创建前调用; ePageId 越界或重复注册忽略) */
void vDisp_PageRegister(const DispPage_T *p_page);

/*----------------页面导航----------------*/
/* 请求切换页面 (任意任务上下文): 显式导航语义, 清空弹窗栈, 显示任务下一轮询生效 */
void vDisp_RequestPage(DispPageId_E e_new_page);

/* 弹窗式压栈 (仅显示任务上下文): 幂等保护, 目标页是当前页或已在栈中时忽略 */
void vDisp_PushPage(DispPageId_E e_popup_page);

/* 弹窗退栈返回 (仅显示任务上下文): 只退一级 */
void vDisp_PopPage(void);

/*----------------背光与息屏管理----------------*/
/* 背光开关 (任意任务上下文): 点亮时唤醒显示任务即时渲染; b_fore_en=true 表示常亮(清息屏倒计时) */
bool bDisp_Switch(SwitchType_E e_type, bool b_fore_en);

/* 查询背光状态 (任意任务上下文) */
bool bDisp_IsBacklightOn(void);

/* 重置息屏倒计时 (任意任务上下文): 不点亮屏幕, 仅重置自动息屏计数 */
void vDisp_ResetAutoOffCountdown(void);

/* 显示 1S 定时节拍 (息屏倒计时递减; 由 timer_task 的 1S 重复定时器直接调用) */
void vDisp_TickTimer(void);

/*----------------调度引擎----------------*/
/* UniDisplay 调度轮询引擎 (仅显示任务调用):
 * 快照抓取 -> 状态路由 -> 事件分发 -> 页面切换 -> 息屏门控渲染 */
void vDisp_EnginePoll(void);

/* 查询当前页面帧刷新周期 ms (任意任务上下文): 页面 usRefreshMs 声明值, 供任务等待节拍/帧折算定时使用 */
uint16_t usDisp_GetFramePeriod(void);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* UNI_DISP_CORE_H */
