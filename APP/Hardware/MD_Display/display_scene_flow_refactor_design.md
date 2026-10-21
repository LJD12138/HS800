# HS800 APP 显示框架「场景流程状态机 (Scene Manager)」改造设计方案

> 范围：`APP\Middlewares\UniDisplay`（通用件）+ `APP\Hardware\MD_Display`（项目件）
> 日期：2026-10-15
> 作者：LJD
> 状态：已实施（同日追加：① core 按域拆分为 uni_disp_scene/uni_disp_nav/uni_disp_power，core 只留引擎时序；② 清理 b_flow_done_closing 冗余完成条件与死代码 v_dis_power_select；③ 通用件去项目化——页面/场景枚举下放项目件 MDPageId_E/MDSceneId_E、overlay 页经 port 原语注入、port 移至 Hardware/MD_Display；④ 弹窗栈机制整体移除，见"评审关注点与决策"第 6 条后续注记）

---

## 一、现状分析

### 1.1 现有调度架构

UniDisplay 调度核心 `vDisp_EnginePoll()` 每帧执行 5 步：

```
1. 快照抓取   vDisp_DataCaptureSnapshot()
2. 状态路由   v_disp_route_by_dev_state()   ← 本次改造对象
3. 事件分发   页面 bOnEvent → 框架默认处理
4. 页面切换   vOnExit → vOnEnter → 强刷置位
5. 息屏门控渲染
```

步骤 2 的现状模型：**设备状态电平 → 边沿检测 → 直接选页**。

- 状态变化沿触发 `eDisp_MapDevStateToPage()`（`md_display_data.c:180`）得到目标页；
- `vDisp_RequestPage()` 覆盖目标页并**清空弹窗栈**（`uni_disp_core.c:102`）；
- `S_ucMappedDevState` 记录"已路由状态"，仅用于边沿判断（`uni_disp_core.c:384`）。

同时，部分页面在 `vOnUpdate` 内**自行导航**（BOOT 页等"动画 100% ∧ DS_WORK"后自请求 WORK 页，`md_display_page_boot.c:104`），与框架路由形成双头控制。

### 1.2 问题清单

| 编号 | 问题 | 代码定位 | 后果 |
|------|------|----------|------|
| P1 | **导航权威分裂**：框架边沿路由、BOOT 页自导航、FAULT 页事件退栈三套并存 | `uni_disp_core.c:371` / `md_display_page_boot.c:104` / `md_display_page_fault.c:107` | 新增页面须重新发明 gating 逻辑，行为不可预测 |
| P2 | **FAULT 页死出口**：退栈唯一依赖 `DISP_EVT_FAULT_CLEAR`，全工程无人 post（仅 `key_task.c:350` post 按键事件） | `md_display_page_fault.c:107`；`grep bDisp_PostEvent` 确认 | 故障清除后显示**卡死在 FAULT 页** |
| P3 | **边沿记录失真**：`S_ucMappedDevState` 仅在映射成功时更新；DS_WORK 无映射（`md_display_data.c:186` 已注释）→ 永不更新 | `uni_disp_core.c:384-392` | ① 故障清除回 DS_WORK 时路由不触发（P2 的根因之一）；② 二次进 DS_ERR 时边沿"消失"，**不再跳故障页** |
| P4 | **状态打断流程无恢复**：状态边沿一到即覆盖切页+清弹窗栈，页内进度（BOOT 动画 60%、CLOSING 保持计数）无保存、无恢复 | `uni_disp_core.c:391` | 用户感知"显示部分丢失"：开机动画被砍/重播、关机画面被提前切断 |
| P5 | **完成条件散落**：BOOT 等动画、CLOSING 数 1500ms、INIT 置就绪标志，各页自管；框架另有 `bDisp_PortIsSystemReady()` 锁 INIT 页特判补丁 | 各 `md_display_page_*.c` | 流程无统一声明处，约束靠页面自觉 |

### 1.3 根因

**模型错配**：系统状态是"电平语义"（任意时刻可读的当前值），显示需求是"流程语义"（带完成条件、最短展示时长、可抢占可恢复的顺序步骤）。用前者直接驱动后者，必然产生边沿丢失、覆盖打断、出口死路三类结构性缺陷，且只能靠页面内补丁（自导航、锁页特判）缓解，补丁又反过来加剧权威分裂（P1）。

---

## 二、改造目标

| 编号 | 目标 | 对应问题 |
|------|------|----------|
| G1 | 初始化流程走完才允许进入下一步（INIT → BOOT 不可跳过、不可被打断） | P4 / 用户诉求 1 |
| G2 | 开机后 BOOT 动画完整走完才进入 WORK，即使系统状态先到 DS_WORK | P4 / 用户诉求 2 |
| G3 | 状态边沿无权直接切页；打断（故障）后可恢复现场，显示不丢失 | P1/P4 / 用户诉求 3 |
| G4 | 故障页出入口闭环（进入/退出均由框架统一执行，不再依赖外部事件） | P2/P3 |
| G5 | 兼容性：`DispPage_T` 页面契约、页面注册表、事件队列、弹窗栈、背光息屏管理**全部保留**；改动收敛在调度层与项目件数据层 | 工程约束 |

---

## 三、总体设计

### 3.1 核心思想

把"选页权"从**系统状态**收回到显示任务自己的**显示流程状态机（Scene Manager）**：

1. **场景 (Scene) 显式声明**：流程收编为项目件场景表——每个场景 = 有序步骤表，每步 = 页面 + 完成条件 + 驻留上限；
2. **状态降级为输入**：系统状态只在两个场合起作用——① 场景边界：决定进入哪个场景；② 步骤完成条件的一部分（如 BOOT 步需要 DS_WORK）。场景内部，状态边沿无权直接切页；
3. **抢占 + 恢复**：FAULT 是唯一可打断任意步骤的 overlay 层，以 `DS_ERR` **电平语义**每帧评估（无边沿丢失），打断时保存（场景 / 步骤），清除后恢复现场。

### 3.2 术语

| 术语 | 定义 |
|------|------|
| 场景 (Scene) | 一段有始有终的显示流程，由有序步骤表构成，如 POWER_ON = INIT→BOOT→WORK |
| 步骤 (Step) | 场景内一页的驻留单元：页面 + 完成条件 `bDone` + 驻留上限 `usMaxHoldMs`；`bDone == NULL` 表示常驻步 |
| 驻留时间 (Dwell) | 当前步骤已持续毫秒数，调度器按帧周期累加，用于完成条件与超时兜底 |
| Overlay | 最高优先级抢占层（FAULT）：压入时保存场景/步骤现场，弹出时恢复 |

### 3.3 架构

```
                ┌────────────────────┐   ┌────────────────────┐
                │  系统状态 (快照/边沿) │   │  事件队列 (按键/请求) │   ← 仅作输入
                └─────────┬──────────┘   └─────────┬──────────┘
                          ▼                        ▼
                ┌───────────────────────────────────────────┐
                │      显示流程状态机 v_disp_flow_schedule     │   ← 唯一导航权威
                │  ① FAULT overlay 电平评估 (压入/弹出)        │
                │  ② 场景边界仲裁 eDisp_MapDevStateToScene     │
                │  ③ 步骤推进: bDone() 成立 或 超时兜底         │
                └───────────────────┬───────────────────────┘
                                    ▼ 按场景表推进
        ┌───────────────────────────┴────────────────────────────┐
        │  开机场景 POWER_ON          │  关机场景 POWER_OFF          │
        │  INIT 页                    │  CLOSING 页                 │
        │   完成: 就绪 ∧ 已开机        │   完成: 驻留 ≥ 1500ms        │
        │      ↓                      │      ↓                      │
        │  BOOT 页                    │  SHUT_DOWN 页                │
        │   完成: 动画100% ∧ DS_WORK   │   常驻                       │
        │   兜底: 超时 5000ms 强制步进  │                              │
        │      ↓                      └─────────────────────────────┘
        │  WORK 页 (常驻)              另有单步场景: UPDATE / ENG_MODE
        └───────────────────────────┘
                                    ▲ 抢占/恢复 (虚线)
                ┌───────────────────┴───────────────────────┐
                │  FAULT 弹层 (最高优先级)                     │
                │  压入: 保存 (场景/步骤) → 切 FAULT 页         │
                │  弹出: 恢复 (场景/步骤) → 回原步骤页面          │
                └───────────────────────────────────────────┘
```

### 3.4 调度时序（改造后的 EnginePoll）

```
1. 快照抓取            vDisp_DataCaptureSnapshot()          (不变)
2. 场景调度            v_disp_flow_schedule(&t_snapshot)     (替换原 v_disp_route_by_dev_state)
3. 事件分发            页面 bOnEvent → 框架默认处理            (微调, 见 4.6)
4. 页面生命周期切换      vOnExit → vOnEnter → 强刷置位          (不变)
5. 息屏门控渲染                                              (不变)
```

---

## 四、详细设计

### 4.1 新增类型与项目件契约（`uni_disp_page.h`）

```c
/*----------------场景与步骤 (通用件契约)----------------*/
typedef enum
{
	DISP_SCENE_NONE = 0,		/* 无映射: 保持当前场景 (DS_ERR/DS_LOST/未知状态) */
	DISP_SCENE_POWER_ON,		/* 开机场景: INIT → BOOT → WORK */
	DISP_SCENE_POWER_OFF,		/* 关机场景: CLOSING → SHUT_DOWN */
	DISP_SCENE_UPDATE,			/* 升级场景 (单步) */
	DISP_SCENE_ENG,				/* 工程模式场景 (单步) */
	DISP_SCENE_MAX
}DispScene_E;

typedef struct
{
	DispPageId_E		ePage;			/* 本步骤展示的页面 */
	bool			(*bDone)(const DispDataSnapshot_T *p_snapshot, uint32_t ul_dwell_ms);	/* 完成条件, NULL=常驻步 */
	uint16_t			usMaxHoldMs;		/* 驻留上限 ms: 0=不限, 到期强制步进 (超时兜底) */
}DispFlowStep_T;

/*----------------项目件必须实现的三个数据钩子 (替代原 eDisp_MapDevStateToPage)----------------*/
/* 设备状态 -> 场景 映射 (DS_ERR 返回 DISP_SCENE_NONE, 由 overlay 以电平语义处理) */
DispScene_E eDisp_MapDevStateToScene(uint8_t uc_dev_state);

/* 场景 -> 步骤表 查询 (返回表首地址, 步数经 p_num 回填) */
const DispFlowStep_T *pDisp_GetSceneSteps(DispScene_E e_scene, uint8_t *p_num);

/* 场景入口步选择 (处理"关机后再开机跳过 INIT""工程/升级退回直接进 WORK"等边界) */
uint8_t ucDisp_SceneEntryStep(DispScene_E e_scene, const DispDataSnapshot_T *p_snapshot);
```

> [!NOTE]
> 原 `eDisp_MapDevStateToPage()`（`md_display_data.c:180`）**删除**，唯一调用方（旧路由函数）同批替换。`vDisp_DataCaptureSnapshot()` / `bDisp_DataAutoOffArmed()` 两个钩子保留不动。

### 4.2 场景表（项目件 `md_display_data.c`）

```c
/*----------------各步骤完成条件----------------*/
/* INIT 步: 显示任务就绪 ∧ 状态已进入开机方向 */
static bool b_flow_done_init(const DispDataSnapshot_T *p_snapshot, uint32_t ul_dwell_ms)
{
	(void)ul_dwell_ms;
	return (bDisp_PortIsSystemReady() && (p_snapshot->ucDevState >= DS_BOOTING));
}

/* BOOT 步: 动画走满 ∧ 系统已工作 (动画进度由 BOOT 页提供查询接口) */
static bool b_flow_done_boot(const DispDataSnapshot_T *p_snapshot, uint32_t ul_dwell_ms)
{
	(void)ul_dwell_ms;
	return (bDisp_BootAnimDone() && (p_snapshot->ucDevState == DS_WORK));
}

/* CLOSING 步: 驻留满 1500ms (保持时长上收调度器, 页面只负责渲染) */
static bool b_flow_done_closing(const DispDataSnapshot_T *p_snapshot, uint32_t ul_dwell_ms)
{
	(void)p_snapshot;
	return (ul_dwell_ms >= 1500U);
}

/*----------------场景步骤表----------------*/
static const DispFlowStep_T s_tScenePowerOn[] =
{
	{PAGE_ID_INIT,		b_flow_done_init,		0},			/* 0: 就绪 ∧ 已开机 */
	{PAGE_ID_BOOT,		b_flow_done_boot,		5000},		/* 1: 动画完 ∧ DS_WORK, 5s 兜底 */
	{PAGE_ID_WORK,		NULL,					0},			/* 2: 常驻 */
};

static const DispFlowStep_T s_tScenePowerOff[] =
{
	{PAGE_ID_CLOSING,	b_flow_done_closing,	0},			/* 0: 保持 1500ms */
	{PAGE_ID_SHUT_DOWN,	NULL,					0},			/* 1: 常驻 */
};

static const DispFlowStep_T s_tSceneUpdate[] = { {PAGE_ID_UPDATE,   NULL, 0} };
static const DispFlowStep_T s_tSceneEng[]    = { {PAGE_ID_ENG_MODE, NULL, 0} };
```

状态 → 场景映射（替代原状态 → 页面映射）：

```c
DispScene_E eDisp_MapDevStateToScene(uint8_t uc_dev_state)
{
	switch ((DevState_E)uc_dev_state)
	{
		case DS_INIT:
		case DS_BOOTING:
		case DS_WORK:		return DISP_SCENE_POWER_ON;
		case DS_CLOSING:
		case DS_SHUT_DOWN:	return DISP_SCENE_POWER_OFF;
		case DS_UPDATE_MODE:	return DISP_SCENE_UPDATE;
		case DS_ENG_MODE:	return DISP_SCENE_ENG;
		default:		return DISP_SCENE_NONE;		/* DS_ERR 由 overlay 电平处理; DS_LOST/未知保持 */
	}
}

/* 场景入口步: 关机后再开机不重走 INIT; ENG/UPDATE 退回直接进 WORK, 不重播开机动画 */
uint8_t ucDisp_SceneEntryStep(DispScene_E e_scene, const DispDataSnapshot_T *p_snapshot)
{
	if (e_scene == DISP_SCENE_POWER_ON)
	{
		if (bDisp_PortIsSystemReady() && (p_snapshot->ucDevState >= DS_BOOTING))
			return (p_snapshot->ucDevState == DS_BOOTING) ? 1U : 2U;	/* BOOT 步 / WORK 步 */
	}
	return 0U;		/* 默认从首步进入 */
}
```

### 4.3 调度器 `v_disp_flow_schedule()`（`uni_disp_core.c`）

替换原 `v_disp_route_by_dev_state()`。核心状态量：

```c
static DispScene_E	S_eScene			= DISP_SCENE_NONE;	/* 当前场景 */
static uint8_t		S_ucStep			= 0;				/* 当前步骤索引 */
static uint32_t		S_ulStepDwellMs		= 0;				/* 当前步骤驻留毫秒数 (按帧周期累加) */
static bool			S_bOverlayOn		= false;				/* FAULT 弹层激活标志 */
static DispScene_E	S_eSavedScene		= DISP_SCENE_NONE;	/* overlay 现场: 场景 */
static uint8_t		S_ucSavedStep		= 0;				/* overlay 现场: 步骤 */
```

调度逻辑（仅显示任务上下文，每帧一次）：

```c
static void v_disp_flow_schedule(const DispDataSnapshot_T *p_snapshot)
{
	/*----------a. FAULT overlay 电平评估 (每帧判定, 天然无边沿丢失)----------*/
	if (p_snapshot->ucDevState == DS_ERR)
	{
		if (!S_bOverlayOn)						/* 压入: 保存现场 */
		{
			S_eSavedScene = S_eScene;
			S_ucSavedStep = S_ucStep;
			S_bOverlayOn  = true;
			S_ucPopupDepth = 0;					/* 抢占清弹窗栈 (与原语义一致) */
			v_disp_set_target(PAGE_ID_FAULT);
		}
		return;									/* overlay 期间主流程暂停 (步进/驻留计时冻结) */
	}
	if (S_bOverlayOn)							/* 弹出: 恢复现场 (策略A, 见 4.4) */
	{
		S_bOverlayOn = false;
		S_eScene    = S_eSavedScene;
		S_ucStep    = S_ucSavedStep;
		S_ulStepDwellMs = 0;
		v_disp_set_target(v_disp_step_page(S_eScene, S_ucStep));
	}

	/*----------b. 场景边界仲裁: 状态仅在场景边界决定进入哪个场景----------*/
	e_target = eDisp_MapDevStateToScene(p_snapshot->ucDevState);
	if ((e_target != DISP_SCENE_NONE) && (e_target != S_eScene))
	{
		S_eScene        = e_target;
		S_ucStep       = ucDisp_SceneEntryStep(S_eScene, p_snapshot);
		S_ulStepDwellMs = 0;
		S_ucPopupDepth = 0;						/* 场景切换清弹窗栈 */
		v_disp_set_target(v_disp_step_page(S_eScene, S_ucStep));
		return;
	}

	/*----------c. 步骤推进: 完成条件成立 或 超时兜底----------*/
	p_step = pDisp_GetSceneSteps(S_eScene, &uc_num);		/* 当前步骤控制块 */
	if (p_step != NULL)
	{
		bool b_step_done = (p_step[S_ucStep].bDone != NULL) &&
			p_step[S_ucStep].bDone(p_snapshot, S_ulStepDwellMs);

		if ((b_step_done) ||
			((p_step[S_ucStep].usMaxHoldMs != 0U) && (S_ulStepDwellMs >= p_step[S_ucStep].usMaxHoldMs)))
		{
			if (S_ucStep < (uint8_t)(uc_num - 1U))		/* 步进, 越界则驻留末步 (常驻) */
				S_ucStep++;
			S_ulStepDwellMs = 0;
			v_disp_set_target(v_disp_step_page(S_eScene, S_ucStep));
		}
		S_ulStepDwellMs += usDisp_GetFramePeriod();		/* 驻留计时 (精度 ±1 帧) */
	}
}
```

> [!NOTE]
> `v_disp_step_page(scene, step)`：内部经 `pDisp_GetSceneSteps()` 查步骤表取 `ePage`，越界返回 `PAGE_ID_MAX`（维持当前页）。原 `bDisp_PortIsSystemReady()` 锁 INIT 页的框架特判（`uni_disp_core.c:375`）**删除**——该职责已由 INIT 步完成条件表达；`bDisp_PortIsSystemReady()` 本体保留（项目件完成条件使用）。

### 4.4 FAULT overlay 抢占与恢复

| 属性 | 设计 |
|------|------|
| 压入条件 | `ucDevState == DS_ERR`（电平，每帧评估；不再依赖 `DISP_EVT_FAULT_POPUP` 事件） |
| 压入动作 | 保存（场景/步骤）→ 清弹窗栈 → 切 FAULT 页 |
| 弹出条件 | `ucDevState != DS_ERR`（含恢复为 DS_BOOTING/DS_WORK/DS_CLOSING 等任意非故障态） |
| 弹出动作 | 恢复（场景/步骤）→ 切回该步骤页面，驻留计时清零 |
| 计时语义 | overlay 激活期间主流程驻留计时冻结（BOOT 步 5s 兜底不会因故障占用而误触发） |
| overlay 期间 | 场景仲裁与步进跳过；FAULT 页正常接收事件与渲染；息屏倒计时策略沿用 `bDisp_DataAutoOffArmed()`（DS_ERR 不参与递减，现状语义不变） |

**恢复策略**（评审拍板项）：

- **策略 A（默认，本次实施）**：恢复时页面生命周期完整重走（FAULT `vOnExit` → 原页 `vOnEnter`）。BOOT 页 `vOnEnter` 会重置动画进度 → 故障打断开机动画后，清除时**重播一次**开机动画。实现零改动、页面契约不变。
- **策略 B（演进预留）**：`DispPage_T` 增加可选 `vOnResume` 回调，恢复时跳过 `vOnEnter` 直接续播（页面把进度存于 static 变量、`vOnEnter` 判断"恢复进入"不重置）。可作为后续迭代。

### 4.5 超时兜底

每步 `usMaxHoldMs` 到期强制步进，防止"系统卡在 DS_BOOTING 但动画已满"之类流程死锁：

| 步骤 | 兜底值 | 兜底后去向 | 说明 |
|------|--------|-----------|------|
| INIT | 0（不限） | — | 等待用户开机，驻留不限时是产品语义 |
| BOOT | 5000ms | 强制进 WORK 步 | 动画实际 ~1.1s（33ms×34 帧），5s 为系统启动留足余量；超时后 WORK 页如实显示当前数据 |
| CLOSING | 完成条件即 1500ms | 进 SHUT_DOWN 步 | 保持时长本身即完成条件 |
| WORK / SHUT_DOWN / UPDATE / ENG | 0（常驻） | — | 仅被场景仲裁或 overlay 接管 |

### 4.6 事件总线兼容（`uni_disp_event.h` / `v_disp_event_default`）

| 事件 | 处置 |
|------|------|
| `DISP_EVT_KEY_SHORT / KEY_LONG` | 不变（页面消费 → 默认重置息屏倒计时） |
| `DISP_EVT_FAULT_POPUP / FAULT_CLEAR` | **删除定义与默认处理分支**（全工程无生产者；overlay 已闭环覆盖其语义。若评审倾向保留扩展点，可保留枚举但标注"保留未用"） |
| `DISP_EVT_REQ_PAGE` | 保留定义；语义降级为**调试后门**（显式页切换，清栈）。生产路径不再使用——升级/工程模式均由状态 → 场景仲裁接管。默认处理分支保留 |

### 4.7 弹窗栈职责划分

- **弹窗栈（`S_aePopupStack` / `vDisp_PushPage` / `vDisp_PopPage`）**：保留，服务于**场景内**真正的 UI 弹窗（如工程模式内子菜单）。`vDisp_PopPage()` 空栈时 fallback `PAGE_ID_WORK` 的行为**删除**（该兜底掩盖流程错误）。
  - **后续注记（2026-10-15）**：弹窗栈实际长期零调用方（工程模式子菜单未落地），机制已整体移除；故障抢占由 FAULT overlay 接管，如未来确有场景内弹窗需求可从 git 历史恢复。
- **FAULT overlay**：使用**独立现场保存字段**（`S_eSavedScene/S_ucSavedStep`），不复用弹窗栈——两者生命周期与语义不同（overlay 是流程级抢占，弹窗栈是页内导航），混用会互相清空。

---

## 五、文件变更影响面

| 文件 | 变更类型 | 内容 |
|------|----------|------|
| `Middlewares/UniDisplay/uni_disp_page.h` | 修改 | 新增 `DispScene_E` / `DispFlowStep_T`；三个新钩子声明；删除 `eDisp_MapDevStateToPage` 声明 |
| `Middlewares/UniDisplay/uni_disp_core.c` | 修改 | `v_disp_route_by_dev_state` → `v_disp_flow_schedule`；删除 `S_ucMappedDevState`、锁 INIT 特判；新增场景/步骤/overlay 状态量；事件默认处理按 4.6 调整 |
| `Middlewares/UniDisplay/uni_disp_event.h` | 修改 | `DISP_EVT_FAULT_POPUP / FAULT_CLEAR` 枚举删除（或标注废弃，评审定） |
| `Hardware/MD_Display/md_display_data.c` | 修改 | 删除 `eDisp_MapDevStateToPage`；新增场景表、三个钩子实现（状态→场景 / 场景→步骤表 / 入口步选择） |
| `Hardware/MD_Display/md_display_data.h` | 修改 | 步骤完成条件相关声明（如 `bDisp_BootAnimDone`）视放置位置同步 |
| `Hardware/MD_Display/md_display_page_boot.c` | 修改 | ① 删除 `v_page_boot_update` 内自导航 `vDisp_RequestPage(PAGE_ID_WORK)`；② 新增进度查询接口 `bDisp_BootAnimDone()`；③ 动画计数逻辑不变 |
| `Hardware/MD_Display/md_display_page_fault.c` | 修改 | 删除 `DISP_EVT_FAULT_CLEAR` 退栈处理（overlay 接管出入口） |
| `Hardware/MD_Display/md_display_page_closing.c` | 修改 | 1500ms 保持计数上收调度器：`v_page_closing_update` 仅渲染，删除自计数与自关背光（背光关闭移交 SHUT_DOWN 页 `vOnEnter`，现状 `md_display_page_shut_down.c:48` 已有 `ST_OFF`，无重复） |
| `Hardware/MD_Display/md_display_page_init.c` / `_work.c` / `_shut_down.c` / `_eng.c` / `_update.c` | 不动 | 现状职责（驱动初始化、就绪标志、背光管理）保留 |
| `Middlewares/UniDisplay/uni_disp_port.c` / `uni_disp_event.c` / `md_display_task.c` / `md_display_iface.c` / `md_display_api.c` | 不动 | 接口与任务壳不变 |

> [!NOTE]
> `vDisp_RequestPage` 对外接口保留（调试语义），生产代码调用方经 grep 确认仅 BOOT 页一处（本次删除）。`bDisp_Switch` 全工程 29 处调用不受影响。

---

## 六、实施阶段

| 阶段 | 内容 | 交付物 / 验收 |
|------|------|--------------|
| **阶段 0（可选止血）** | 若框架落地前需先修复"卡故障页"：FAULT 页 `vOnUpdate` 检测 `ucDevState != DS_ERR` 持续 2s → 自请求 WORK 页。标注 `TEMP`，阶段 2 清理 | 补丁仅一处，随时可撤 |
| **阶段 1（核心落地）** | 4.1 契约 → 4.2 场景表 → 4.3 调度器 → 4.6/4.7 事件与弹窗栈调整，一次性切换；用宏 `dispSCENE_FSM_EN`（=1 新调度器 / =0 旧边沿路由）双路并存，可一键回退 | 编译通过；测试矩阵 T1~T6 通过 |
| **阶段 2（验证与清理）** | 全矩阵回归；固化 `dispSCENE_FSM_EN=1`，删除旧路由函数、`S_ucMappedDevState`、阶段 0 补丁 | T1~T12 全通过；死代码清除 |
| **阶段 3（演进，可选）** | 恢复策略 B（`vOnResume` 续播）；`DISP_EVT_REQ_PAGE` 调试通道清理；故障弹层扩展（非 DS_ERR 状态下的告警轮显升级为弹窗，`usPrimaryErrCode` 为触发源） | 按需排期 |

---

## 七、验证计划（测试矩阵）

| 编号 | 前置 | 操作 / 场景 | 预期 |
|------|------|------------|------|
| T1 | 冷上电 | 上电后不按开机键 | 息屏停留 INIT 步，不亮屏、不切页 |
| T2 | T1 | 按开机键 → DS_BOOTING → DS_WORK | BOOT 动画完整走满（~1.1s）后切 WORK；系统先到 DS_WORK 也不提前切 |
| T3 | BOOT 动画期间 | 制造瞬时故障（DS_ERR）后清除 | 故障期间显示 FAULT 页；清除后重播 BOOT 动画（策略 A）→ WORK，**不再卡故障页** |
| T4 | WORK 中 | 制造故障后清除 | FAULT 页即时弹出；清除后**直接回 WORK 页**（无开机动画） |
| T5 | T4 之后 | **二次**制造故障 | FAULT 页**再次正常弹出**（复现 P3 边沿丢失 bug，改造后应修复） |
| T6 | WORK 中 | 长按关机 → DS_CLOSING → DS_SHUT_DOWN | CLOSING 画面保持完整 1500ms（不被提前切断）→ SHUT_DOWN 息屏 |
| T7 | 关机态 | 再按开机键 → DS_BOOTING | 直接进 BOOT 步（**不重走 INIT**），动画正常 |
| T8 | WORK 中 | 进入/退出升级模式（DS_UPDATE_MODE） | UPDATE 页接管；退出回 POWER_ON **WORK 步**（不重播开机动画） |
| T9 | WORK 中 | 进入/退出工程模式（DS_ENG_MODE） | 同 T8 |
| T10 | 任意 | 人为让状态停留 DS_BOOTING | BOOT 步 5s 超时兜底，强制进 WORK 步，不死锁 |
| T11 | WORK 亮屏 | 按键交互、倒计时息屏、背光常亮切换 | 与现状一致（`bDisp_Switch` 通路未动） |
| T12 | WORK 中欠压 | UV/0%SOC 触发 `sys_queue_task_err` 10s 倒计时关机链路 | 显示跟随 CLOSING → SHUT_DOWN，全程无卡页无重播 |

观测手段：调试器 watch `S_eScene / S_ucStep / S_ulStepDwellMs / S_bOverlayOn`；`uPrint` 调试打印场景切换沿。

---

## 八、风险与回退

| 编号 | 风险 | 缓解 |
|------|------|------|
| R1 | 双路并存期间（阶段 1）新旧逻辑混淆 | 宏 `dispSCENE_FSM_EN` 全局唯一入口分叉；阶段 2 尽快清理，窗口期控制在一次迭代内 |
| R2 | 策略 A 重播开机动画的 UX 争议（仅"开机动画期间被故障打断"这一罕见场景） | 评审确认；策略 B 作为演进预案 |
| R3 | CLOSING 保持计数上收后，页面与调度器职责边界变化 | 步骤表即文档，`b_flow_done_closing` 单行可读；页面退化为纯渲染 |
| R4 | `DispFlowStep_T` 完成条件函数误访问非快照数据导致帧间不一致 | 约束：`bDone` 仅允许读入参快照与页面自身 static 进度（`bDisp_BootAnimDone`），禁止直读全局 |
| R5 | 帧周期累加的驻留计时精度（±1 帧 ≈ ±33ms） | 对 1500ms/5000ms 量级无影响；不接受亚帧级条件的步骤 |

**回退方案**：`dispSCENE_FSM_EN = 0` 一键回到旧边沿路由（旧函数体阶段 2 前保留）。

---

## 九、评审关注点（请评审时拍板）

1. **BOOT 超时兜底去向**：超时后强制进 WORK（当前提案），还是转 FAULT 页提示"启动失败"？
2. **恢复策略 A/B**：本次按策略 A（重播）实施、策略 B（续播）演进，是否认可？
3. **事件枚举清理**：`DISP_EVT_FAULT_POPUP / FAULT_CLEAR` 直接删除，还是保留枚举标注"预留"？
4. **场景表放置**：并入 `md_display_data.c`（提案，与"数据与路由"职责一致），还是新建 `md_display_flow.c`？
5. **CLOSING 1500ms 计数上收**调度器（提案）还是保留页面自计数？
6. **overlay 与弹窗栈分离**（提案）还是复用弹窗栈实现 overlay？
