/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_core.h
 * Date    : 2026-09-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 调度核心对外接口 - 页面注册/场景调度/背光息屏/轮询引擎
 * -------------------------------------------------------
 * API 调用规约 (线程安全矩阵, 代码评审依据):
 * | 接口                     | 允许调用上下文            | 机制                     |
 * | bDisp_PostEvent          | 任意任务(不建议 ISR)      | 临界区入队+通知唤醒      |
 * | vDisp_RequestPage        | 任意任务                  | 目标页槽结构体临界区写   |
 * | vDisp_RequestPageEx      | 任意任务                  | 目标页槽结构体临界区写   |
 * | bDisp_PagePush           | 仅显示任务(页面回调内)    | 栈操作非原子, 单上下文   |
 * | bDisp_PagePop            | 仅显示任务(页面回调内)    | 栈操作非原子, 单上下文   |
 * | bDisp_PageCanPop         | 仅显示任务                | 单读(栈顶单写)           |
 * | ulDisp_GetNavParam       | 仅显示任务(vOnEnter 内)   | 单读(写者临界区)         |
 * | pDisp_GetPageUser        | 仅显示任务(页面回调内)    | 单读(切页单写)           |
 * | bDisp_Switch             | 任意任务(禁止 ISR)       | 内部状态单写+通知唤醒    |
 * | bDisp_IsBacklightOn      | 任意任务                  | 布尔量单读               |
 * | bDisp_IsDisplayActive    | 任意任务                  | 布尔量单读(背光或后端判定) |
 * | vDisp_TickTimer          | 任意任务/软件定时器(禁止硬件 ISR) | 息屏 1S 周期递减  |
 * | usDisp_EnginePoll        | 仅显示任务                | 单上下文                 |
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

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
/*----------------框架初始化与页面管理----------------*/
/* 框架核心初始化 (显示任务创建前调用; 注册项目操作集并复位各域) */
void vDisp_CoreInit(const DispOps_T *p_ops);

/* 页面静态注册 (显示任务创建前调用; ePageId 越界或重复注册忽略) */
void vDisp_PageRegister(const DispPage_T *p_page);

/* 页面表批量注册 (显示任务创建前调用; p_pages 为 const 页面指针数组) */
void vDisp_PageRegisterAll(const DispPage_T *const *p_pages, uint8_t uc_count);

/* 获取项目操作集 (框架内部/项目件可读; 未注册返回 NULL) */
const DispOps_T *pDisp_GetOps(void);

/* 读取快照中的设备状态 (经项目 ops 适配; 无适配返回 dispDEV_STATE_NONE) */
uint8_t ucDisp_DataDevState(const DispDataSnapshot_T *p_snapshot);

/*----------------框架内部适配 (供 nav/power/event 调用, 通用件不直接引用项目 port 头)----------------*/
/* 临界区进入/退出 (经 DispOps_T; 项目未提供时为空实现) */
void vDisp_CoreEnterCritical(void);
void vDisp_CoreExitCritical(void);

/* 默认帧刷新周期 ms (项目 ops 未提供时回退 dispFRAME_PERIOD_DEFAULT_MS) */
uint16_t usDisp_CoreDefaultFramePeriod(void);

/*----------------页面导航----------------*/
/* 请求切换页面 (任意任务上下文): 显式导航, 显示任务下一轮询生效, 无参等价 RequestPageEx(e,0) */
void vDisp_RequestPage(DispPageId_E e_new_page);

/* 带参请求切换页面 (任意任务上下文): 目标页 vOnEnter 内经 ulDisp_GetNavParam 读取 ul_param */
void vDisp_RequestPageEx(DispPageId_E e_new_page, uint32_t ul_param);

/*----------------页面栈 (页内多级钻取: 场景=流程权威, 栈=页内返回链)----------------*/
/* 压栈跳转 (仅显示任务上下文, 页面回调内调用): 当前页入栈, 带参请求目标页; 栈满返回 false */
bool bDisp_PagePush(DispPageId_E e_page, uint32_t ul_param);

/* 出栈返回 (仅显示任务上下文, 页面回调内调用): 以 ul_result 结束子页并请求栈顶父页; 空栈返回 false */
bool bDisp_PagePop(uint32_t ul_result);

/* 查询可否出栈 (仅显示任务上下文): 栈非空表示存在返回链, 供页面 vOnExit 决定粒度释放/箭头显隐 */
bool bDisp_PageCanPop(void);

/* 读取导航参数 (仅显示任务上下文, 页面 vOnEnter 内调用): 最近一次导航带入的参数 */
uint32_t ulDisp_GetNavParam(void);

/* 读取当前页面上下文 (仅显示任务上下文, 页面回调内调用): 当前页 DispPage_T.pUser, 无当前页/无上下文返回 NULL */
void *pDisp_GetPageUser(void);

/*----------------背光与息屏管理----------------*/
/* 背光开关 (任意任务上下文): 点亮时唤醒显示任务即时渲染; b_fore_en=true 表示常亮(清息屏倒计时) */
bool bDisp_Switch(SwitchType_E e_type, bool b_fore_en);

/* 查询背光状态 (任意任务上下文) */
bool bDisp_IsBacklightOn(void);

/* 查询显示是否处于活动状态 (背光或后端判定; 断码屏可恒 true) */
bool bDisp_IsDisplayActive(void);

/* 重置息屏倒计时 (任意任务上下文): 不点亮屏幕, 仅重置自动息屏计数 */
void vDisp_ResetAutoOffCountdown(void);

/* 显示 1S 定时节拍 (息屏倒计时递减; 由 timer_task 的 1S 重复定时器直接调用) */
void vDisp_TickTimer(void);

/*----------------调度引擎----------------*/
/* UniDisplay 调度轮询引擎 (仅显示任务调用):
 * 快照抓取 -> 场景调度 -> 事件分发 -> 页面切换 -> 息屏门控渲染;
 * 返回值: 建议下次轮询间隔 ms (当前页帧周期), 显示任务据此动态睡眠;
 * 事件/导航/背光请求经 vWakeTask 即时唤醒, 不受该间隔限制 */
uint16_t usDisp_EnginePoll(void);

/* 查询当前页面帧刷新周期 ms (任意任务上下文): 页面 usRefreshMs 声明值, 供任务等待节拍/帧折算定时使用 */
uint16_t usDisp_GetFramePeriod(void);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* UNI_DISP_CORE_H */
