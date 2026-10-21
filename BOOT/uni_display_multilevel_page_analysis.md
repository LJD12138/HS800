# HS800 UniDisplay 多级页面能力分析与优化方案

> 范围：`APP\Middlewares\UniDisplay`（通用件 6 文件）+ `APP\Hardware\MD_Display`（项目件 + port 适配层）
> 日期：2026-10-16　作者：LJD　状态：分析完成，方案待评审
> 结论先行：**框架原生为"单槽覆盖式导航"，多级页面栈支持为 0 级（仅单层 overlay 抢占恢复）**；推荐在 nav 域自研"页面栈 + 带参导航 + 框架化返回"（约 200 行，不破通用件零项目依赖红线），不建议引入开源框架（评估见 §4.2）。

---

## 1. 框架现状梳理

### 1.1 架构定位

显示框架 = UniDisplay 调度核心（通用件）+ 项目显示模块（项目件）+ EEZ Studio 生成 UI（只读件）+ 用户自绘 UI（手写件）。引擎五步时序：快照抓取 → 场景调度 → 事件分发 → 切页生命周期 → 息屏门控渲染（[uni_disp_core.c](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_core.c#L55-L100)）。

### 1.2 页面层级管理能力

| 机制 | 实现 | 层级能力 |
| :--- | :--- | :--- |
| 场景（Scene） | [md_display_data.c](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/md_display_data.c#L183-L198) 静态步骤表（INIT / POWER_ON=BOOT→WORK / POWER_OFF=CLOSING→SHUT_DOWN / UPDATE / ENG） | 一级流程分组；**步骤只进不退**（[uni_disp_scene.c](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_scene.c#L134-L142) `step++`），末步常驻 |
| 组内跳转 | [bDisp_PageGoto](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_nav.c#L137-L147)：校验 `eDisp_MapPageToScene` 归属一致，否则静默拒绝 | 同场景单跳，**无返回记忆**；当前 WORK 组仅 1 页（[eDisp_MapPageToScene](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/md_display_data.c#L316-L325)），能力空置 |
| overlay 抢占 | [uni_disp_scene.c](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_scene.c#L77-L102)：电平评估（DS_ERR→FAULT），保存/恢复（场景/步骤/页ID）三元组 | **单层**：保存槽仅一份（`S_eSavedScene/S_ucSavedStep/S_eSavedPage`），overlay 期间主流程冻结，不能嵌套 |
| 弹窗栈 | `vDisp_PushPage/vDisp_PopPage` 已于 2026-10-15 移除（零调用方，见 [display_scene_flow_refactor_design.md](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/display_scene_flow_refactor_design.md#L344-L348)，可从 git 历史 `355e765^` 恢复参考实现） | 已不存在 |

**核心事实：导航原语是单槽覆盖式**——[vDisp_RequestPage](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_nav.c#L72-L76) 仅写 `S_eTargetPageId`，连续两次请求后者覆盖前者，无栈、无历史、无参数。

### 1.3 路由跳转机制

- **唯一导航权威 = 场景调度器**（[uni_disp_scene.c](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_scene.c#L69-L145)），优先级：overlay 电平评估 → 场景边界仲裁（`ucDevState → Scene`，[映射表](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/md_display_data.c#L209-L226)）→ 步骤推进（步骤 `bDone` ∧ 步骤页 `bOnDone` 与合成，`usMaxHoldMs` 超时兜底）。
- **跨"页面组"跳转实际走设备状态**：如进工程模式是 [sys_queue_task.c:171](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Application/Sys/sys_queue_task.c#L171) `bSys_SetDevState(DS_ENG_MODE)` → 场景仲裁 → ENG 页。这是**状态驱动场景切换**，代价是整页销毁重建，不是页面压栈。
- 事件通道仅 4 个枚举（[uni_disp_event.h](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_event.h#L38-L42)），`DISP_EVT_REQ_PAGE` 已降级为调试后门；队列深 4、单帧最多处理 3 条。

### 1.4 组件复用规则

- 页面控制块 `G_tPageXxx` 为 `static const`（如 [G_tPageWorkMain1](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/md_display_page_work.c#L33-L41)），注册表为静态数组 `S_aptPages[PAGE_ID_MAX]`（[uni_disp_nav.c:25](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_nav.c#L25)）——**页面对象零动态内存，天然无泄漏**。
- EEZ 5 个 Screen 在 `ui_init()` 一次性全部创建（`create_screens()`），[loadScreen 用 `lv_scr_load_anim(..., auto_del=false)`](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/eez_ui/ui.c#L18-L22)——**所有屏幕对象树常驻 LVGL 内存池**，切屏不增内存。
- 重组件按页 enter 创建 / exit 删除：能量环（[energy_ring.c](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/user_ui/energy_ring.c#L871) `lv_obj_create` / [:914](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/user_ui/energy_ring.c#L914) `lv_obj_delete`）、ENG UI（[eng_mode_ui.c:1909](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/user_ui/eng_mode_ui.c#L1909) 创建 / [:1963](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/user_ui/eng_mode_ui.c#L1963) 删除）。
- user_ui 更新函数一律带 `const DispDataSnapshot_T *p_data` 入参 + 静态缓存比对，变化才写控件（减少脏区）。

### 1.5 内存占用控制逻辑

| 项 | 现状 | 位置 |
| :--- | :--- | :--- |
| 框架自身内存 | 注册表 `PAGE_ID_MAX×4B` + 事件队列 `4×8B` + 快照约 40B 栈分配 + 场景状态几字节，全部静态 | [uni_disp_nav.c:25-31](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_nav.c#L25-L31) |
| LVGL 堆 | 内置池 **31KB**（`LV_MEM_SIZE`），`LV_USE_OS=FreeRTOS` | [lv_conf.h:112](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/LVGL/lv_conf.h#L112)、[:160](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/LVGL/lv_conf.h#L160) |
| CPU 控制 | 帧周期钳位 ≥20ms；息屏 2s 心跳挂起（CPU 0%）；息屏门控渲染；背光关边沿补一帧清显存 | [uni_disp_nav.c:187-196](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_nav.c#L187-L196)、[md_display_task.c:154-165](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/md_display_task.c#L154-L165) |
| 内存监控 | 仅 ENG 页内 debug 打印 `lv_mem_monitor`（受打印开关控制），**框架无全局水位/泄漏防护** | [eng_mode_ui.c:1883-1885](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/user_ui/eng_mode_ui.c#L1883-L1885) |
| 板级参数 | 默认帧周期 50ms，息屏 180s | [board_config.h:327-331](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Application/board_config.h#L327-L331) |

### 1.6 原生多级页面栈支持边界（结论）

**0 级栈。** 框架原生只保证：单层 overlay 抢占恢复、场景内单向前进步骤流、同组无记忆单跳。不原生支持：push/pop 多级栈、back 返回语义、带参跳转、嵌套 overlay、页级缓存与恢复。

---

## 2. 典型场景现有表现验证（代码级走查结论）

| 场景 | 现有表现 | 判定 |
| :--- | :--- | :--- |
| Work 状态加载一级主页 | DS_WORK→POWER_ON 场景，入口步=1（[ucDisp_SceneEntryStep](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/md_display_data.c#L297-L306)），WORK_MAIN1 enter：加载 main_work → 快照首帧 → 能量环创建 → 点亮背光（[md_display_page_work.c:51-75](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/md_display_page_work.c#L51-L75)） | ✅ 正常 |
| 一级 → 二级 | 生产路径**不存在**。WORK 场景步骤表无二级页；`bDisp_PageGoto` 限同组且 WORK 组仅 1 页；去 ENG 只能改全局设备状态（整页销毁重建）。ENG 模式的"主菜单→PV/PS/SS/CFM 子页"是 **ENG 页内部**用 `v_page_show` 手撕的懒创建/删除 UI 状态机（[eng_mode_ui.c:1937](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/user_ui/eng_mode_ui.c#L1937)、[:1330-1394](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/user_ui/eng_mode_ui.c#L1330-L1394)），框架无感知 | ❌ 不支持 |
| 二级 → 三级 | 同上，框架无此能力；ENG 内部靠 `S_tState.ePage` 枚举切换模拟 | ❌ 不支持 |
| 多级返回 | 仅 overlay 弹出恢复现场（FAULT→回原页）；组内/跨级 back **不存在**，只能再改状态或再请求页（覆盖式） | ❌ 仅 overlay 单级 |
| 参数传递 | 导航 API 只携带 page_id；现有通信仅：帧快照（只读）、事件 `ul_param`、持久参数 `tAppMemParam`、页面 static。子页无法获知"从哪来/为何来" | ❌ 不支持 |
| 状态同步 | 帧快照临界区抓取、帧内一致，✅ 良好；但页面局部状态无保存/恢复契约：场景重入入口步重置（如 WORK↔BOOTING 往返回 WORK_MAIN1），ENG tab/菜单位置靠 static 隐式保留 | ⚠️ 半支持 |
| 页面销毁 | 契约为 OnExit 清理；ENG/能量环实现到位；但**框架无审计**，漏清理即 31KB 池泄漏直至耗尽；EEZ 屏幕永不销毁（常驻） | ⚠️ 约定靠自觉 |

---

## 3. 能力满足点与缺口

### 3.1 框架已满足的需求点

1. **状态驱动导航稳定闭环**：场景边界仲裁 + overlay 电平评估，故障进出由框架统一执行，无"卡死出口"类历史问题（2026-10-15 重构已解决）。
2. **页面生命周期契约清晰**：四回调 + 可选 `bOnDone`，线程安全矩阵明确，页面对象全静态零动态内存。
3. **背压与防护**：事件队列满丢弃返回 false；帧周期下限钳位防 CPU 打满；息屏渲染门控。
4. **内存敏感设计**：重组件懒创建/退出删除、EEZ 全屏常驻复用、user_ui 脏区比对。
5. **真实需求已被验证**：ENG 模式用约 1900 行手撕代码模拟了"多级页面+懒加载+删除"，正说明框架缺该能力、且需求紧迫。

### 3.2 能力缺口与潜在问题清单

| # | 缺口 | 影响 | 严重度 |
| :--- | :--- | :--- | :--- |
| G1 | **无页面栈**（push/pop 已移除且无替代） | ≥2 级钻取页面必须占用全局设备状态（场景切换），状态是稀缺资源，不能被 UI 层级滥用 | 高 |
| G2 | **无返回语义** | 多级流转后无法"回到来处"，只能重定位，用户体验与状态一致性差 | 高 |
| G3 | **无带参导航** | 二级页无法获知来源与上下文，只能靠全局变量打补丁 | 高 |
| G4 | **导航权威单一 vs 页内导航矛盾** | 场景调度随时可因状态变化抢走子页面（上下文丢失）；页内 drill-down 只能靠同组 `bDisp_PageGoto` 或调试后门 | 高 |
| G5 | **overlay 单层** | 故障抢占中无法再叠二级弹窗（如故障详情/确认框） | 中 |
| G6 | **页面局部状态无保存/恢复契约** | 场景重入/被打断后回到入口步，子页进度丢失（BOOT 动画被砍重播类问题在多级页下会放大） | 中 |
| G7 | **内存泄漏防护缺失** | 无全局水位监控/泄漏断言；单个页面 OnExit 漏清理 → 31KB 池耗尽 → LVGL 分配失败（表现为控件消失/花屏） | 中 |
| G8 | **性能余量收窄** | EEZ 5 屏已常驻 31KB 池；新增多屏/多级组件将进一步挤压；多级动画页全 20ms 跑满无分级 | 中 |
| G9 | **同页多实例不可重入** | 页面状态依赖文件级 static，无法支撑"同一页面模板多处打开" | 低 |

---

## 4. 优化方案

### 4.1 方案 A（推荐）：UniDisplay 框架内增强

设计原则：① 非破坏式扩展，既有 8 页 5 场景零改动；② 保持"场景=流程权威、页面栈=页内导航"二层模型（与 [设计文档 4.7](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/display_scene_flow_refactor_design.md#L344-L348) 职责划分一致）；③ 通用件零项目依赖红线不破，内存审计等 LVGL 相关能力经 port 原语下沉。

#### A1. 页面栈管理（uni_disp_nav 域扩展，约 80 行）

```c
#define dispPAGE_STACK_DEPTH    (4U)   /* 可配宏, 深度按产品最大钻取层级 +1 预留 */

typedef struct
{
    DispPageId_E    e_page;     /* 压栈时的当前页 */
    uint32_t        ul_param;   /* 返回该页时回传的参数槽 */
} DispNavEntry_T;

static DispNavEntry_T   S_atNavStack[dispPAGE_STACK_DEPTH];
static uint8_t          S_ucNavTop = 0;     /* 0=空栈 */

/* 导航参数槽: 最近一次 Push/RequestPageEx 带给目标页的参数 (单上下文安全) */
static uint32_t         S_ulNavParam = 0;

bool bDisp_PagePush(DispPageId_E e_page, uint32_t ul_param);   /* 当前页入栈 + 请求目标页 */
bool bDisp_PagePop(uint32_t ul_result);                        /* 出栈返回栈顶页, ul_result 回传父页 */
bool bDisp_PageReplace(DispPageId_E e_page, uint32_t ul_param);/* 同级跳转: 请求目标页, 栈不变 */
bool bDisp_PageCanPop(void);                                   /* 供返回箭头显隐 */
uint32_t ulDisp_GetNavParam(void);                             /* 页面 vOnEnter 内读取导航参数 */
```

语义与边界：
- `bDisp_PagePush`：栈满返回 false（调用方降格处理）；否则（`S_eCurrentPageId` 入栈，`ul_param` 写入参数槽）→ `vDisp_RequestPage(e_page)`。
- `bDisp_PagePop`：空栈返回 false（页面决定兜底）；出栈元素参数写入参数槽 → 请求出栈页。
- **清栈时机**：场景边界仲裁命中、overlay 压入时清空（恢复 2026-10-15 移除弹窗栈时的"抢占清栈"语义），防止跨场景残留。
- 线程约束：与 `bDisp_PageGoto` 一致——**仅显示任务上下文**（页面回调内）调用；栈操作含 RMW，不进临界区。

#### A2. 带参导航（目标槽结构体化）

```c
/* 目标页槽由单字节扩为结构体, 写路径加临界区 (原"单字节写原子"承诺在此升级为临界区写) */
static struct
{
    DispPageId_E    e_id;
    uint32_t        ul_param;
} S_tTarget;

void vDisp_RequestPageEx(DispPageId_E e_page, uint32_t ul_param);  /* 带参版 */
/* vDisp_RequestPage(e) 保持原签名, 等价 vDisp_RequestPageEx(e, 0) */
```

页面侧取参：`vOnEnter` 内调 `ulDisp_GetNavParam()`（非破坏式，不改 `DispPage_T` 契约）。

#### A3. 返回（back）框架化

- 新增事件 `DISP_EVT_BACK`（key 任务或页面产生）；框架默认处理：栈非空 → `bDisp_PagePop(NULL)`；空栈 → 透传（页面/场景兜底）。
- 分发顺序不变：页面 `bOnEvent` 优先消费（二级页可拦截做"取消确认"），未消费才走框架默认——与现两级分发兼容（[uni_disp_core.c:70-79](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_core.c#L70-L79)）。

#### A4. overlay 保持单层，二级弹窗走页面栈（不动场景核心）

overlay 机制维持现状；overlay 页内若需二级弹窗（FAULT→故障详情），用 `bDisp_PagePush` 实现。避免改动场景调度器的现场保存逻辑，风险最小。

#### A5. 内存自动回收与防护（经 port 原语，保持通用件零 LVGL 依赖）

```c
/* uni_disp_port.h 新增原语 (项目件实现, 通用件只调用) */
void vDisp_PortMemAudit(uint16_t us_free_min);  /* LVGL lv_mem_monitor 水位检查, 越限打印/计数 */
```

- 引擎每次 `bDisp_NavApplySwitch()` 返回 true（切页后）调用一次；水位低于阈值（建议 4KB）或碎片率 >50% → 项目侧打印 + 错误计数。
- 同步固化编码契约（review  checklist 增补）：页面 `vOnEnter` 创建的 LVGL 对象必须在 `vOnExit` 配对删除（现状 ENG/能量环已合规，写入 checklist 防回归）。

#### A6. 多级页面通信

- 结果回传：`bDisp_PagePop(ul_result)` 将子页结果写入导航参数槽，父页 `vOnEnter` 经 `ulDisp_GetNavParam()` 读取——**零新增全局变量**。
- 页间异步信号：复用现有事件队列（深度 4 对按键/通知够用）；快照字段扩展走既有数据钩子流程。

#### A7. 帧率分级（性能，二期可选）

列表/静态二级页声明 `usRefreshMs=100~200ms`（契约已支持）；亮屏无操作 30s 后引擎临时 4 倍帧周期（渲染门控已具备，仅改等待计算）。

### 4.2 方案 B：开源项目评估（结论：不引入，仅借鉴语义）

| 候选 | 页面栈 | 带参 | RTOS 安全 | 与本架构兼容性 | 结论 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| [LVGL lv_fragment](https://lvgl.github.io/open-docs/HTML/9.3/details/auxiliary-modules/fragment.html)（官方 9.x，类 Android Fragment/iOS UIViewController，Manager 内置栈） | ✅ push/pop | ✅ | LVGL 单上下文 | 基于 view 树 attach 到宿主，与 EEZ"全屏 Screen 常驻 + loadScreen"模型冲突；其栈语义与方案 A1 一致 | **借鉴语义，不引入代码** |
| [lvgl-nav-kit](https://components.espressif.com/components/txp666/lvgl-nav-kit/versions/1.2.5/readme)（页面栈/手势/过渡动画） | ✅ | ✅ | 依赖 ESP-IDF | 平台耦合 ESP32，STM32 + 现有 FreeRTOS 工程不适用 | 不适用 |
| [lv_menu](https://lvgl.io/docs/open/widgets/menu)（LVGL 官方多级菜单控件） | ✅ | ❌ | — | LVGL 9.3 已 **deprecated**，且是控件级菜单非页面栈 | 不采用 |
| EEZ Flow 动作脚本 | ❌ | 有限 | — | eez_ui 只读约束下无法扩展生成代码 | 维持现状 |
| GuiLite / Embedded Wizard / GUIX 等 | — | — | — | 与快照+场景架构理念冲突、体积与内存不可控（31KB 池） | 不采用 |

**评估结论**：LVGL 生态无可直接引入的轻量"页面栈 + 带参导航 + RTOS 安全"库；官方 lv_fragment 证明了"栈 + 生命周期"是通用正解，但其视图模型与 EEZ 生成代码不兼容。方案 A 自研约 200 行即可等效，且守住通用件零项目依赖红线。

### 4.3 落地步骤

| 阶段 | 内容 | 验证 |
| :--- | :--- | :--- |
| 1 ✅(2026-10-16) | nav 域：栈 + `vDisp_RequestPageEx` + 参数槽 + `DISP_EVT_BACK` 默认处理（清栈挂接场景调度 2 处） | IDE 诊断通过；Keil 编译待人工执行 |
| 2 ✅(2026-10-16) | **试点**：ENG 模式 `主菜单 →Push→ 参数查看(PAGE_ID_ENG_PV) →Push→ 详情(PAGE_ID_ENG_PV_DETAIL)` 三级链；键任务经 `ePendingNav` 队列转入显示任务执行栈原语；Pop 空栈兜底回主菜单（防 overlay 抢占清栈后无出口） | §5 验收 F1~F5 待真机 |
| 3 | port 原语 `vDisp_PortMemAudit` + 引擎挂接 + checklist 固化 | §5 验收 P3/S1 |
| 4 | （按需）overlay 页内二级弹窗、帧率分级 | 按产品需求排期 |

---

## 5. 优化后验收标准

### 5.1 功能要求

| 编号 | 条目 |
| :--- | :--- |
| F1 | WORK 主页 → 二级页 → 三级页 连续压栈 ≥3 级稳定，每级首帧上屏 ≤100ms（`lv_refr_now` 同步首帧） |
| F2 | back 逐层返回：栈非空必返回来处；栈空 back 有明确兜底（页面自定义或场景默认），不卡死、不误切 |
| F3 | 带参正确：子页 `vOnEnter` 经 `ulDisp_GetNavParam()` 读到 Push 参数；Pop 时子页结果回传父页 |
| F4 | 栈边界安全：栈满 Push 返回 false 不崩；空栈 Pop 返回 false；连续快速 back/.Push 1000 次无栈错乱 |
| F5 | 场景切换 / overlay 压入自动清栈；overlay 弹出后现场恢复正确（复用现有保存机制，回归通过） |
| F6 | 既有 8 页面 + 5 场景行为零回归（BOOT→WORK、关机、升级、工程模式、故障 overlay 全矩阵） |

### 5.2 性能指标

| 编号 | 条目 |
| :--- | :--- |
| P1 | 任一级动画页 ≥30FPS（33ms 帧周期下渲染耗时 ≤15ms） |
| P2 | 3 级全栈压入后 LVGL 池 free ≥8KB；无分配失败 |
| P3 | 连续 1000 次 push/pop 循环，free_size 抖动 <1KB（无泄漏、无碎片累积） |
| P4 | 息屏 CPU 0% 保持；亮屏常态 CPU 占比不高于优化前 |

### 5.3 兼容性要求

| 编号 | 条目 |
| :--- | :--- |
| C1 | 非破坏式扩展：`DispPage_T` 契约不变、既有页面零改动编译通过 |
| C2 | 通用件零项目依赖红线不破：新增能力全部在 nav/core 通用件内，LVGL 相关经 port 原语 |
| C3 | 新增公共 API 集中于 [uni_disp_core.h](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_core.h)，命名遵循 `bDisp_/vDisp_` 规范 |
| C4 | BOOT 工程（未集成 UniDisplay）不受影响；APP 编译零告警 |

---

## 附录 A：关键代码位置索引

| 主题 | 位置 |
| :--- | :--- |
| 单槽导航/注册表 | [uni_disp_nav.c](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_nav.c#L23-L31) |
| 组内跳转 | [uni_disp_nav.c#L137-147](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_nav.c#L137-L147) |
| 场景调度（overlay/边界/步进） | [uni_disp_scene.c#L69-145](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_scene.c#L69-L145) |
| 页面/步骤契约 | [uni_disp_page.h#L47-83](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_page.h#L47-L83) |
| 引擎五步时序 | [uni_disp_core.c#L55-100](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/UniDisplay/uni_disp_core.c#L55-L100) |
| 场景步骤表/状态映射 | [md_display_data.c#L183-226](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/md_display_data.c#L183-L226) |
| ENG 内部多级菜单（手撕栈的证据） | [eng_mode_ui.c#L1905-1968](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/user_ui/eng_mode_ui.c#L1905-L1968) |
| EEZ 屏幕常驻（auto_del=false） | [eez_ui/ui.c#L18-27](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/eez_ui/ui.c#L18-L27) |
| LVGL 内存池 31KB | [lv_conf.h#L108-112](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Middlewares/LVGL/lv_conf.h#L108-L112) |
| 弹窗栈移除注记 | [display_scene_flow_refactor_design.md#L344-348](file:///g:/1-Baiku_Projects/25-HS800/1.software/HS800/APP/Hardware/MD_Display/display_scene_flow_refactor_design.md#L344-L348) |
