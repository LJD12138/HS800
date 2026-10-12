---
name: eez-uni-display
description: EEZ Studio + LVGL + UniDisplay 显示任务架构约束与开发流程。当用户要求开发、修改、分析嵌入式显示任务（EEZ Studio 生成 UI 骨架）、新增页面或自绘组件、处理显示任务与其它任务交互、或将该显示架构移植到其他项目时使用。Do not use for 纯 LVGL 原生工程（无 EEZ/UniDisplay 骨架）或非显示类需求。
---

# EEZ Studio + UniDisplay 显示任务架构

参考实现：HS800 APP 工程 `APP\Hardware\MD_Display` + `APP\Middlewares\UniDisplay`（BOOT 工程有同构独立副本）。

## 1. 架构总览

显示任务 = **UniDisplay 调度核心（通用件）+ 项目显示模块（项目件）+ EEZ Studio 生成 UI（只读件）+ 用户自绘 UI（手写件）** 四部分协作。

```
其他任务 (Sys/BMS/DCAC/MPPT/USB/Key/Update...)
   │  只允许: bDisp_PostEvent / vDisp_RequestPage / bDisp_Switch / bDisp_IsBacklightOn / vDisp_TickTimer
   ▼
显示任务外壳 xxx_display_task.c (FreeRTOS, 帧周期补偿等待)
   └─ vDisp_EnginePoll() 引擎 [Middlewares/UniDisplay, 通用件]
        1.快照抓取 → 2.状态路由 → 3.事件分发 → 4.切页生命周期 → 5.息屏门控渲染
             │              │                            │
      xxx_display_data.c   uni_disp_event.c         xxx_display_page_*.c
      (项目件: 快照+路由表)   (事件环形队列, 深度4)       (页面契约 DispPage_T 四回调)
             │                                            │
             └────────── 快照 p_data 逐层传入 ─────────────┤
                                                        │
                                            ┌────────────┴────────────┐
                                       eez_ui/ 只读              user_ui/ 手写
                                       (EEZ Studio 生成)         (自绘组件/动画)
   渲染出口(经 port 原语 vDisp_PortUiRefresh): ui_tick() → 自绘 ManualTick() → lv_timer_handler()
```

详细目录职责、设计依据、移植步骤见 [references/architecture.md](references/architecture.md)。

## 2. 硬性约束（红线，违反即返工）

1. **eez_ui/ 目录只读**：其内文件由 EEZ Studio 构建生成，再次构建会被整体覆盖。改 UI 只有两途径：
   - 改 EEZ Studio 工程（eez_project/）后重新构建生成；
   - 在 user_ui/ 用原生 LVGL API 写自绘代码。
   唯一例外：`eez_ui/vars.c`（Native 变量 get/set 实现，手写维护，不在 `.eez-project-build` 生成清单内）。
2. **LVGL 单上下文**：所有 LVGL API（含 `loadScreen`、控件操作）只能在显示任务上下文调用。其它任务一律经事件队列 / 页面请求 API 中转，禁止直接调页面回调和 LVGL。
3. **页面回调禁止阻塞**：vOnEnter/vOnUpdate/bOnEvent/vOnExit 在渲染主循环内执行，禁止 vTaskDelay、等待信号量、耗时循环。
4. **通用件零项目依赖**：UniDisplay 调度核心（core/page/event）禁止 include 项目头或引用项目符号；对项目的全部访问（快照定义、渲染出口、背光、息屏参数、就绪判定、提示音）必须经 `uni_disp_port.h` 的适配原语。移植新工程只改 port 两文件 + 项目件。
5. **快照消费模式**：页面渲染链（page 回调 → user_ui 函数）只读帧快照 `p_data`，禁止直读其它任务的全局变量（`tSysInfo`/`tBms` 等）；数据需求通过扩展快照字段解决。
6. **帧周期下限**：页面 `usRefreshMs` 低于 20ms 会被引擎钳位（`dispFRAME_PERIOD_MIN_MS`）。
7. **快照原子性**：抓快照必须临界区保护多字节字段（如 64 位故障字）；低频量可临界区外经访问函数直读（显示任务单读者）。

## 3. 线程安全矩阵（API 调用规约，代码评审依据）

| 接口 | 允许调用上下文 | 机制 |
| :--- | :--- | :--- |
| bDisp_PostEvent | 任意任务（不建议 ISR） | 临界区入队 + 任务通知唤醒 |
| vDisp_RequestPage | 任意任务 | 目标页单字节写，原子 |
| bDisp_Switch | 任意任务（禁止 ISR） | 内部状态单写 + 通知唤醒 |
| bDisp_IsBacklightOn | 任意任务 | 布尔量单读 |
| vDisp_TickTimer | 任意任务/软件定时器（禁止硬件 ISR） | 息屏 1S 周期递减 |
| vDisp_PushPage / vDisp_PopPage | 仅显示任务 | 单上下文 |
| vDisp_EnginePoll | 仅显示任务 | 单上下文 |
| usDisp_GetFramePeriod | 任意任务 | 单读（切页单写） |
| 页面四回调 | 仅显示任务（框架调用） | 单上下文 |

## 4. 移植层适配原语（uni_disp_port，项目耦合唯一汇聚点）

| 原语 | 作用 | HS800 实现 |
| :--- | :--- | :--- |
| vDisp_PortUiRefresh | 渲染输出一轮 | 转调 vDisp_UiRefresh（ui_tick→自绘→lv_timer_handler） |
| vDisp_PortSetBacklight | 背光硬件开关 | 转调接口层 vDisp_SetBacklight |
| usDisp_PortGetAutoOffTime | 持久化息屏时间（0=常亮） | 读 tAppMemParam.tDISP.usAutoOffTime |
| bDisp_PortIsSystemReady | 系统就绪判定（false 锁 INIT 页） | 读 tSysInfo.uInit.tFinish.bIF_DispTask |
| vDisp_PortBeepOnLightOn | 常亮亮屏提示钩子 | bBuz_Tweet(LONG_1)（boardBUZ_EN 内裁剪） |
| vDisp_PortSetTaskHandle / vDisp_PortWakeTask | 任务唤醒钩子（OS 环境） | FreeRTOS 任务通知；裸机为空宏 |

port.h 同时引入项目快照定义头（工程适配点：引擎需栈分配快照并感知首字段 `ucDevState`）。

## 5. 页面契约（DispPage_T）

```c
const DispPage_T G_tPageXxx =
{
    .ePageId     = PAGE_ID_XXX,
    .vOnEnter    = v_page_xxx_enter,    /* 进页: 加载 EEZ Screen/重置页面变量/启动重组件 */
    .vOnExit     = v_page_xxx_exit,     /* 退页: 停动画/清标志/存临时参数 */
    .vOnUpdate   = v_page_xxx_update,   /* 帧刷新: 依据 Snapshot 渲染; 周期=usRefreshMs */
    .bOnEvent    = b_page_xxx_event,    /* 事件: 返回 true=已消费, false=透传框架默认 */
    .usRefreshMs = 20,                  /* 0=框架默认(boardDISP_REFRESH_TIME), 下限20ms钳位 */
};
```

生命周期要点：
- **vOnEnter**：① 若活动屏不对先 `vDisp_LoadScreen()`；② 需要首帧渲染数据时本地调 `vDisp_DataCaptureSnapshot(&t_snapshot)` 抓一份快照；③ 先刷新静态框架（`lv_refr_now(NULL)` 同步首帧，缩短切屏感知）；④ 再创建重组件（能量环/动画）；⑤ 需要亮屏时 `bDisp_Switch(ST_ON, false)`。
- **vOnUpdate(p_data, b_force)**：只读快照渲染，把 `p_data` 逐层传给 user_ui 更新函数；自绘更新内部用静态缓存比对，变化才写控件（减少脏区）。
- **bOnEvent**：页面专属按键/故障逻辑返回 true；通用行为（按键重置息屏、故障弹窗）透传 return false。消费按键事件时需自行重置息屏倒计时。
- **vOnExit**：释放/停止本页动画与组件，复位"已初始化"标志。

## 6. 引擎轮询时序（vDisp_EnginePoll，优先级从高到低）

1. **快照抓取**：临界区一次性拷贝全局状态 → 帧内一致，路由与渲染共用。
2. **设备状态路由**：状态变化沿触发（非电平），经项目件 `eDisp_MapDevStateToPage()` 映射，显式导航（清弹窗栈）；系统未就绪（port 判定）时锁定 INIT 页。
3. **事件分发**：逐条出队，先给当前页 bOnEvent，未消费走框架默认（故障弹窗/清除、REQ_PAGE、按键重置息屏）。队列深度 4，单帧最多 3 条——事件风暴天然受限于队列容量，无需额外帧预算。
4. **切页生命周期**：目标页 ≠ 当前页时 OnExit → OnEnter → 加载页面帧周期 → 强刷置位。
5. **息屏门控渲染**：亮屏或强刷才渲染；渲染出口固定三步 `ui_tick()` → 自绘动画 ManualTick → `lv_timer_handler()`（经 vDisp_PortUiRefresh）。

导航语义：
- `vDisp_RequestPage`：显式导航，**清空弹窗栈**；
- `vDisp_PushPage`：弹窗压栈（深度 4，幂等保护，仅显示任务）；
- `vDisp_PopPage`：只退一级，空栈回 WORK 页。

## 7. 任务节拍（FreeRTOS）

- 亮屏：等待 = 页面帧周期 − 本帧已耗渲染时间（耗时补偿，已超时立刻进下一帧）；
- 息屏：2 秒低频心跳挂起（CPU 0%），靠任务通知提前唤醒；
- 息屏倒计时由外部 1S 定时器调 `vDisp_TickTimer()` 独立递减，与轮询次数解耦；仅 `bDisp_DataAutoOffArmed()` 判定通过的状态（HS800 为 DS_WORK）参与递减；
- LVGL 时基：`lv_tick_set_cb(xTaskGetTickCount)` 复用 FreeRTOS 节拍，不用 lv_tick_inc 喂值。

## 8. 快照数据模式（项目件核心契约）

- `DispDataSnapshot_T` 首字段必须是 `ucDevState`（框架 opaque 契约），其余字段项目自定义；
- 数据需求三原则：多字节/一致性敏感字段在临界区内捕获；低频量临界区外经访问函数直读（单读者安全）；端口状态聚合为位图（激活位图稳态点亮 + 闪烁位图错误态，错误/保护态优先判闪烁）；
- 端口图标显隐统一由 `v_disp_apply_port_flags(p_data)` 从位图刷新（500ms 统一闪烁相位，多端口故障同步闪烁），不再按端口分散计时；
- user_ui 对外更新函数一律带 `const DispDataSnapshot_T *p_data` 入参并做 NULL 防御（如 `vDisp_UpdateDevParam(p_data)` / `bDisp_Main1DataUpdate(p_data)`）。

## 9. EEZ Studio 数据桥（变量绑定模式）

- EEZ 工程 Native 变量 → 生成 `vars.h` 的 `get_var_xxx/set_var_xxx` 声明 → 用户在 `vars.c` 手写实现（strncpy 带边界）。
- 数据流：页面/user_ui `set_var_xxx(字符串)` → `ui_tick()` 内 EEZ Flow 自动比对刷新绑定控件。
- EEZ Flow 做不了的效果（图标闪烁、可见性切换、复杂动画）在 user_ui 中直接操作 `objects.xxx` 控件对象实现。
- `ui_init()` 只能执行一次（G_bUiInitialized 防重，防 EEZ 屏幕对象重复创建）。

## 10. 开发 SOP

### A. 改 UI 布局/外观/加图片字体 → EEZ Studio
1. 打开 `eez_project/eez_project.eez-project`，修改设计（图片放 `eez_project/Image/`，字体放 `eez_project/fonts/`）；
2. EEZ Studio Build 生成到 `eez_ui/`（以 `.eez-project-build` 清单为准）；
3. 新增 Native 变量时补 `vars.c` 手写实现；
4. 将工程中生成的 `eez_ui` 文件加入构建工程编译。
   复杂界面可用 Node 脚本直接生成 EEZ 工程结构（参考 `eez_project/gen_eng_mode.js` 模式）。

### B. 新增自绘组件/动画 → user_ui/
1. 在 `user_ui/` 新建组件 c/h（对齐 c-code-templates 模板）；
2. 组件接口三件套：Init/SetMode(或 Update)/Stop(+ManualTick 若有动画)；
3. 数据一律从 `p_data` 快照入参获取，禁止直读业务全局变量；
4. 动画不依赖 LVGL 定时器时提供 `vXxx_ManualTick()`，在 `vDisp_UiRefresh()` 中统一驱动（保持单上下文）；
5. 页面 vOnEnter 启动、vOnExit 停止并复位初始化标志。

### C. 新增页面
1. `uni_disp_page.h` 的 `DispPageId_E` 增加枚举（PAGE_ID_MAX 哨兵保持最后）；
2. EEZ Studio 加对应 Screen 并重新生成（或纯 user_ui 全屏组件）；
3. 新建 `xxx_display_page_xxx.c`：定义 `G_tPageXxx` 四回调（见第 5 节契约）；
4. `xxx_display_task.h` 导出声明，`b_task_param_init()` 中 `vDisp_PageRegister(&G_tPageXxx)` 注册；
5. 项目件路由表 `eDisp_MapDevStateToPage()` 增加状态映射（或经 `bDisp_PostEvent(DISP_EVT_REQ_PAGE, 页ID)` 请求）；
6. 条件编译页面用 `#if (boardXXX_EN)` 包裹注册与导出。

### D. 其它任务驱动显示
- 推事件：`bDisp_PostEvent(DISP_EVT_KEY_SHORT, key_id)` / `DISP_EVT_FAULT_POPUP` / `DISP_EVT_FAULT_CLEAR`；
- 请求切页：`vDisp_RequestPage(PAGE_ID_XXX)`（任意任务安全）；
- 亮/息屏：`bDisp_Switch(ST_ON/ST_OFF/ST_TOGGLE, b_fore_en)`；
- 禁止：直接调页面回调、直接摸 LVGL 控件、直接写 UI 状态、在 ISR 中调 bDisp_Switch/vDisp_TickTimer。

### E. 移植到新项目
1. 整目录拷贝 `Middlewares/UniDisplay/` 四文件；
2. 只改 port 两文件：`uni_disp_port.h`（引入新项目快照头、按需调整原语声明）、`uni_disp_port.c`（实现 6 类原语，转调新项目件）——core/page/event 三文件零修改；
3. 新建项目件目录（如 `Hardware/XX_Display/`），按模板实现：
   - `xx_display_iface.c/h`：引脚/SPI/控制器原语；
   - `lv_port_disp.c/h` + `xx_display_api.c/h`：LVGL 驱动注册、vDisp_Init/vDisp_UiRefresh；
   - `xx_display_data.c/h`：`DispDataSnapshot_T` 项目字段 + 三个数据钩子（CaptureSnapshot/AutoOffArmed/MapDevStateToPage）；
   - `xx_display_task.c/h`：任务外壳（创建任务、注册页面、绑定唤醒钩子）；
   - `xx_display_page_*.c`：各页面（注意 BOOT 工程是同构精简副本，独立演化）；
4. EEZ Studio 新建工程生成 `eez_ui/`，手写 `vars.c`；
5. board_config 配置：`boardDISPLAY_EN`、`boardDISP_REFRESH_TIME`、亮度、息屏时间等；
6. 定时任务挂 1S 节拍 `vDisp_TickTimer()`；系统初始化完成置就绪标志（经 bDisp_PortIsSystemReady 判定）解除 INIT 页锁定。

详细移植清单与设计依据见 [references/architecture.md](references/architecture.md)。

## 11. 交付前检查清单

- [ ] 未修改 `eez_ui/` 任何文件（vars.c 除外）？
- [ ] 未修改 core/page/event 三文件（项目耦合只进 port）？
- [ ] 渲染链只读快照 p_data，无直读其它任务全局变量？
- [ ] 新页面已注册且路由表/事件路径可达？
- [ ] 页面回调内无阻塞调用；vOnExit 释放动画资源？
- [ ] 自绘更新有静态缓存比对，避免每帧全量 set 产生脏区？
- [ ] usRefreshMs ≥ 20ms；息屏场景任务确实挂起？
- [ ] 字符串操作用 snprintf/strncpy（无 sprintf/strcpy）？
- [ ] 命名遵循工程规范（vDisp_/bDisp_/G_tPage 导出、static 小写下划线内部、快照字段带类型前缀）？
- [ ] 文件头注释、段落横幅、#endif 条件注释符合 c-code-templates？
