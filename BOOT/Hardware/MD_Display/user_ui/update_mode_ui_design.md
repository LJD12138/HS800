# HS800 BOOT 升级模式 UI 实现文档 — 左信息面板 + 右仪表盘风格

> 文件：`Hardware/MD_Display/user_ui/update_mode_ui.c`
> 日期：2026-06-23
> 作者：LJD
> 状态：已实现

---

## 一、视觉风格

**左信息面板 + 右仪表盘**：左侧竖向信息面板展示通道/协议/帧数等调试数据，右侧大圆环进度仪表作为视觉焦点，状态文字与倒计时置于底部。整体类似汽车仪表盘布局，信息密度高且层次分明。

### 与旧 UI 的核心差异

| 对比项 | 旧 UI | 当前实现 |
|--------|-------|----------|
| 布局结构 | 上下堆叠（圆环→状态→胶囊条→信息→倒计时） | 左右分区（信息面板 \| 仪表盘） |
| 视觉焦点 | 圆环和胶囊条并列，无主次 | 圆环为唯一主视觉焦点，放大居右 |
| 信息呈现 | 信息项零散分布底部 | 左侧面板集中展示，标签+数值对齐 |
| 进度指示 | 圆环+胶囊条双重 | 仅圆环（去掉胶囊条，减少视觉干扰） |
| 背景层次 | 纯色背景无分区 | 5个独立容器，深蓝面板区分层次 |
| 倒计时 | 整行动态刷新 | "Exit in "和"s"静态，仅数字动态 |

---

## 二、界面布局设计（320×240）

### 2.1 布局总览

```
┌─────────────────────────────────────────────────────┐ y=0
│              （顶部5px遮挡区）                        │
├─────────────────────────────────────────────────────┤ y=10
│                FIRMWARE UPDATE                       │ 标题栏容器 (0,10,320,34)
│             (2x字体, 居中, y=12)                      │
├──────────────────┬──────────────────────────────────┤ y=43
│▓   CHANNEL    ▓ │                                  │
│▓   Print      ▓ │              ╭──────╮             │
│▓               ▓ │              │  75  │             │ 仪表盘容器
│▓   PROTOCOL   ▓ │              │   %  │             │ (126,43,188,126)
│▓   Xmodem     ▓ │              ╰──────╯             │ center=(220,106)
│▓               ▓ │                                   │ r=44
│▓   FRAME      ▓ ├──────────────────────────────────┤ y=173
│▓   0768/1024  ▓ │            Upgrading...           │ 状态栏容器
│▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│                                   │ (126,173,188,22)
├──────────────────┴──────────────────────────────────┤ y=198
│              Exit in  45 s                          │ 倒计时栏容器
│         ("Exit in "和"s"静态, 数字动态)              │ (0,198,320,37)
└─────────────────────────────────────────────────────┘ y=240

  信息面板容器         仪表盘+状态栏容器
  (6,43,112,152)      (126,43,188,22)
```

### 2.2 容器定义

| 容器 | 坐标 (x,y,w,h) | 背景色 | 说明 |
|------|----------------|--------|------|
| 标题栏 | (0, 10, 320, 34) | 0x01A5 中蓝 | 全宽横条 |
| 信息面板 | (6, 43, 112, 152) | 0x0145 深蓝 | 左侧竖向面板 |
| 仪表盘 | (126, 43, 188, 126) | 0x0145 深蓝 | 右侧圆环区 |
| 状态栏 | (126, 173, 188, 22) | 0x0145 深蓝 | 状态文字+动画 |
| 倒计时栏 | (0, 198, 320, 37) | 0x01A5 中蓝 | 全宽横条 |

### 2.3 信息面板内布局

字体8x16：scale=1高度16px，scale=2高度32px。三行均匀分布，坐标基于容器起始Y相对计算。

| 行 | 标签(y) | 数值(y) | 字体 | 内容 |
|----|---------|---------|------|------|
| 通道 | "CHANNEL" (49) | "Print" (67) | 标签1x灰色, 数值2x白色 | 动态：eChType |
| 协议 | "PROTOCOL" (105) | "Xmodem" (123) | 标签1x灰色, 数值2x白色 | 动态：eProtoType |
| 帧数 | "FRAME" (161) | "0768/1024" (179) | 标签1x灰色, 数值1x白色 | 半静态"/TTTT" + 动态"NNNN" |

### 2.4 仪表盘内布局

| 元素 | 坐标 | 参数 |
|------|------|------|
| 进度圆环 | center=(220,106), r=44, thickness=7 | 活跃色0x06FF, 非活跃0x2126, bg=0x0145 |
| 进度数字 | x=176, y=86, w=72px | 3x字体, 白色, 固定3位`%3u` |
| 百分号 | x=248, y=94 | 2x字体, 白色, 静态定位 |

### 2.5 状态栏内布局

| 元素 | 坐标 | 参数 |
|------|------|------|
| 状态文字 | x=220居中, y=176 | 1x字体, 状态色, 半静态 |
| 动画点 | 状态文字后3字符 | 1x字体, 状态色, 200ms/帧 |

### 2.6 倒计时栏内布局

| 元素 | 坐标 | 参数 | 类型 |
|------|------|------|------|
| "Exit in " | x=112, y=208 | 1x字体, 暗灰0x8410 | 静态 |
| 数字 | x=176, y=208, w=24px | 1x字体, 暗灰, 固定3位`%3u` | 动态 |
| "s" | x=200, y=208 | 1x字体, 暗灰 | 静态 |

### 2.7 颜色方案（RGB565）

| 用途 | 颜色值 | 宏名 | 说明 |
|------|--------|------|------|
| 主背景 | 0x0000 | UI_COLOR_BG | 纯黑 |
| 容器背景 | 0x0145 | UI_COLOR_CONTAINER | 深蓝（面板/仪表盘/状态栏） |
| 横条背景 | 0x01A5 | UI_COLOR_CONTAINER_BAR | 中蓝（标题栏/倒计时栏） |
| 活跃进度 | 0x06FF | UI_COLOR_ACTIVE | 亮青（圆环进度） |
| 圆环非活跃 | 0x2126 | UI_COLOR_CIRCLE_INACT | 深灰 |
| 白色文字 | 0xFFFF | UI_COLOR_WHITE | 数值/标题/百分号 |
| 灰色标签 | 0xAD55 | UI_COLOR_GRAY | 面板标签/等待状态 |
| 暗灰 | 0x8410 | UI_COLOR_DIM | 倒计时文字 |
| 警告色 | 0xFD20 | UI_COLOR_WARN | 橙红（取消/超时/错误） |
| 成功色 | 0x07E0 | UI_COLOR_SUCCESS | 绿色（成功状态） |

---

## 三、静态/动态分离设计

### 3.1 分离原则

每个UI元素拆分为静态部分（不变）和动态部分（变化时刷新），动态部分用固定宽度格式（`%3u`/`%04u`）保证静态部分位置不变。

### 3.2 分离清单

| 元素 | 静态部分 | 动态部分 | 动态刷新区域 |
|------|----------|----------|-------------|
| 帧数 | "/1024" (半静态) | "0768" | 32px (4字符×8px) |
| 进度数字 | "%" (静态) | "75" | 72px (3字符×8px×3倍) |
| 状态文字 | "Upgrading..." (半静态) | 动画点"..." | 24px (3字符×8px) |
| 倒计时 | "Exit in " + "s" (静态) | "45" | 24px (3字符×8px) |

### 3.3 圆环百分号特殊处理

`DrawProgressCircle` 的 `bg_color` 参数会填充圆环**内部所有像素**，覆盖已绘制的文字。因此 `%` 不能仅作为静态绘制，必须在 `v_ui_update_gauge` 中圆环绘制**之后**重新绘制：

```c
static void v_ui_update_gauge(const UiSnapshot_T *snap)
{
    // 1. 绘制圆环（会覆盖内部所有内容）
    vDisp_DrawProgressCircle(..., UI_COLOR_CONTAINER);

    // 2. 清除并绘制数字（固定3位）
    vDisp_DrawFillRect(UI_GAUGE_NUM_X, UI_GAUGE_NUM_Y, UI_GAUGE_NUM_W, 50, UI_COLOR_CONTAINER);
    vDisp_DrawText(UI_GAUGE_NUM_X, UI_GAUGE_NUM_Y, buf, ...);

    // 3. 圆环之后重新绘制"%"（必须在此位置）
    vDisp_DrawText(UI_GAUGE_PCT_X, UI_GAUGE_NUM_Y + 8, "%", ...);
}
```

---

## 四、数据项映射

| 需求项 | 数据源 | 显示位置 | 转换 | 刷新触发 |
|--------|--------|----------|------|----------|
| usRecFrameCnt | `tUpdate.usRecFrameCnt` | 左面板第3行动态部分 | `%04u` | 帧数变化 |
| eChType | `tUpdate.eChType` | 左面板第1行 | enum→string | 通道变化 |
| eProtoType | `tUpdate.eProtoType` | 左面板第2行 | enum→string | 协议变化 |
| usTotalFrmValue | `tUpdate.usTotalFrmValue` | 左面板第3行半静态 | `/%04u` | 总帧变化 |
| 升级进度 | `usRecFrameCnt*100/usTotalFrmValue` | 右侧圆环+数字 | `%3u` 计算0-100 | 进度变化 |
| eAppState | `tBootMemParam.tParam.eAppState` | 右侧状态文字 | enum→状态文字 | 状态变化 |
| 退出倒计时 | `tUpdate.tpProtoRx->usLostOverTimeCnt/100` | 底部数字 | 10ms→秒 `%3u` | 秒数变化 |

---

## 五、架构设计

### 5.1 函数分层

```
vDisp_UpdateModeUi()                  ← 对外接口（显示任务50ms调用）
  ├── v_ui_collect_snapshot()         ← 采集数据快照
  ├── v_ui_draw_static()              ← L0: 标题+容器背景+标签+"%"+倒计时静态文字（仅首次）
  ├── v_ui_update_info_static()       ← L1: 通道/协议/总帧数（低频变化时）
  ├── v_ui_update_info_frame_cnt()    ← L1: 接收帧数数字（高频变化时，仅32px区域）
  ├── v_ui_update_gauge()             ← L2: 圆环+数字+"%"（进度变化时）
  ├── v_ui_update_status_text()       ← L1: 状态文字全量重绘（状态变化时）
  ├── v_ui_update_status_dots()       ← L1: 动画点3字符（200ms/帧，仅24px区域）
  └── v_ui_update_countdown()         ← L1: 倒计时数字（秒数变化时，仅24px区域）
```

### 5.2 数据快照机制

```c
typedef struct {
    uint8_t      progress;        // 进度 0-100
    uint16_t     rec_frame_cnt;   // 接收帧数
    uint16_t     total_frame;     // 总帧数
    uint16_t     countdown_sec;   // 倒计时秒数
    UiState_E    ui_state;        // UI状态
    const char  *p_status_str;    // 状态文字
    uint16_t     status_color;    // 状态颜色
    const char  *p_ch_str;        // 通道字符串
    const char  *p_proto_str;     // 协议字符串
} UiSnapshot_T;
```

每次刷新开始一次性采集，后续绘制从快照读取，保证一致性。

### 5.3 变化检测

每个动态元素保留 `static` 上次值，仅在变化时刷新：

```c
// L1半静态: 通道/协议/总帧数 — 任一变化才刷新
if (snap.p_ch_str != s_pc_prev_ch ||
    snap.p_proto_str != s_pc_prev_proto ||
    snap.total_frame != s_us_prev_total ||
    s_b_first_run)

// L1高频: 接收帧数 — 仅帧数变化才刷新32px数字区域
if (snap.rec_frame_cnt != s_us_prev_frm || s_b_first_run)

// L2: 仪表盘 — 进度变化才刷新
if (snap.progress != s_uc_prev_progress || s_b_first_run)

// L1: 状态文字 — 仅状态变化才全量重绘
if (snap.p_status_str != s_pc_prev_status || s_b_first_run)

// L1: 动画点 — 仅等待/擦除状态，每200ms刷新24px区域
if (b_need_anim && anim_idx > 0)

// L1: 倒计时数字 — 仅秒数变化才刷新24px数字区域
if (snap.countdown_sec != s_us_prev_countdown || s_b_first_run)
```

### 5.4 等待动画

状态文字与动画点分离：
- `v_ui_update_status_text()` — 状态变化时全量重绘（含3字符动画点占位）
- `v_ui_update_status_dots()` — 每200ms仅刷新尾部3字符动画点

```c
static const char *s_dot_anim[4] = { "   ", ".  ", ".. ", "..." };
uint8_t anim_idx = (s_uc_anim_frame >> 2) & 0x03;  // 4帧分频=200ms

// 仅在 WAITING/ERASING 状态显示动画
bool b_need_anim = (snap.ui_state == UI_STATE_WAITING || snap.ui_state == UI_STATE_ERASING);
```

---

## 六、状态机设计

### 6.1 状态枚举

```c
typedef enum {
    UI_STATE_IDLE = 0,      // 空闲（协议未选择）
    UI_STATE_WAITING,       // 等待升级开始
    UI_STATE_ERASING,       // 擦除Flash中
    UI_STATE_UPGRADING,     // 升级中
    UI_STATE_SUCCESS,       // 升级成功
    UI_STATE_CANCELLED,     // 升级取消
    UI_STATE_TIMEOUT,       // 升级超时
    UI_STATE_ERROR,         // 升级错误
} UiState_E;
```

### 6.2 状态判定优先级

```
1. 取消：协议状态 == CANCEL              → UI_STATE_CANCELLED  (橙红)
2. 超时：tpProtoRx->usLostOverTimeCnt==0
         && eAppState != AS_FINISH      → UI_STATE_TIMEOUT    (橙红)
3. 成功：eAppState == AS_FINISH          → UI_STATE_SUCCESS    (绿色)
4. 升级中：progress > 0 && progress < 100 → UI_STATE_UPGRADING  (白色)
5. 擦除：eAppState==AS_ERASE && progress==0
         && 协议已启动                    → UI_STATE_ERASING    (白色, 动画)
6. 等待：eAppState==AS_ERASE && progress==0 → UI_STATE_WAITING  (灰色, 动画)
7. 默认                                   → UI_STATE_IDLE       (灰色)
```

### 6.3 状态文字与颜色映射

| 状态 | 文字 | 颜色 | 动画 |
|------|------|------|------|
| IDLE | "Idle" | 灰色 | 无 |
| WAITING | "Waiting..." | 灰色 | 旋转点 |
| ERASING | "Erasing Flash..." | 白色 | 旋转点 |
| UPGRADING | "Upgrading..." | 白色 | 无 |
| SUCCESS | "Upgrade Success!" | 绿色 | 无 |
| CANCELLED | "Upgrade Cancelled!" | 橙红 | 无 |
| TIMEOUT | "Upgrade Timeout!" | 橙红 | 无 |

---

## 七、性能优化

### 7.1 刷新分级

| 级别 | 更新频率 | 内容 | 触发条件 |
|------|----------|------|----------|
| L0 静态 | 仅初始化 | 标题、容器背景、标签、"%"/"Exit in "/"s" | `s_b_first_run` |
| L1 半静态 | 低频变化 | 通道/协议/总帧数、状态文字 | 值变化 |
| L1 高频 | 高频变化 | 接收帧数(32px)、倒计时数字(24px)、动画点(24px) | 值变化/200ms |
| L2 | 进度变化 | 圆环+数字+"%" | `progress` 变化 |

### 7.2 刷新区域对比（优化前→优化后）

| 元素 | 优化前刷新区域 | 优化后刷新区域 | 缩减 |
|------|---------------|---------------|------|
| 帧数 | 96px (NNNN/TTTT) | 32px (NNNN) | 67% |
| 进度数字 | 88px (NNN%) | 72px (NNN) | 18% |
| 状态动画 | 188px (整条状态栏) | 24px (动画点) | 87% |
| 倒计时 | 96px (Exit in NNNs) | 24px (NNN) | 75% |

### 7.3 CPU 占用估算

| 场景 | 操作 | 预估 CPU |
|------|------|----------|
| 空闲周期（无变化） | 仅快照采集 | <0.1% |
| 帧数变化周期 | 32px数字重绘 | <0.5% |
| 进度变化周期 | 圆环+数字+"%"重绘 | 3~5% |
| 状态变化周期 | 状态文字重绘 | 1~2% |
| 动画帧周期 | 24px动画点重绘 | <0.5% |
| **峰值** | 进度+帧数同时变化 | **<8%** |

满足 ≤15% 要求。

### 7.4 优化措施

- 5个独立容器背景仅首次绘制，永不重绘
- 帧数"/TTTT"半静态，仅"NNNN"动态刷新32px
- 进度"%"静态定位，仅数字动态刷新72px（固定3位`%3u`）
- 状态文字半静态，动画点仅刷新24px
- 倒计时"Exit in "/"s"静态，仅数字动态刷新24px（固定3位`%3u`）
- 圆环仅在 `progress` 变化时重绘
- 数据快照一次性采集，避免多次访问共享变量

---

## 八、安全设计

### 8.1 空指针安全

`tUpdate.tpProtoRx` 在协议未初始化时为 `NULL`：

```c
static uint16_t us_get_countdown_sec(void)
{
    if (tUpdate.tpProtoRx == NULL)
        return 0;
    return tUpdate.tpProtoRx->usLostOverTimeCnt / 100;
}
```

### 8.2 除零保护

```c
snap->progress = 0;
if (tUpdate.usTotalFrmValue > 0)
{
    snap->progress = (uint32_t)tUpdate.usRecFrameCnt * 100 / tUpdate.usTotalFrmValue;
    if (snap->progress > 100) snap->progress = 100;
}
```

### 8.3 线程安全

`tUpdate` 为 `volatile` 修饰，GD32 上 16 位对齐访问原子。UI 先快照到局部变量，避免显示过程中值变化导致不一致。不使用临界区，避免影响升级实时性。

### 8.4 栈空间约束

显示任务栈 256×4 = 1KB，局部缓冲区控制在 40 字节内（`buf[40]`），复用缓冲区。

### 8.5 圆环覆盖处理

`DrawProgressCircle` 的 `bg_color` 填充圆环内部所有像素，会覆盖已绘制文字。`%` 必须在圆环绘制之后重新绘制（见 3.3 节）。

---

## 九、文件修改清单

| 文件 | 修改内容 |
|------|----------|
| `update_mode_ui.c` | 完全重写，容器化布局 + 静态/动态分离 |
| `update_mode_ui.h` | 无需改动（接口不变） |
| `md_display_task.c` | `bDisp_Switch()` 中移除标题绘制代码，清屏色改为 `0x0000` |

---

## 十、风险与对策

| 风险 | 对策 |
|------|------|
| `tpProtoRx` 为 NULL 时崩溃 | 倒计时函数判空返回 0 |
| `usTotalFrmValue == 0` 除零 | progress 计算前判 `> 0` |
| 栈空间不足 | 局部缓冲区 ≤40 字节，复用 |
| 显示与升级任务竞争 | volatile + 快照，不用临界区 |
| 动画帧计数器溢出 | uint8_t 自然回绕，位运算不受影响 |
| 帧数显示宽度变化导致残留 | 用 `%04u` 固定宽度 + 清背景 |
| 圆环覆盖"%" | `%` 在圆环绘制之后重新绘制 |
| 顶部5px遮挡 | 所有Y坐标偏移+5，标题栏从y=10开始 |

---

## 十一、验收标准

- [x] 5个独立容器：标题栏、信息面板、仪表盘、状态栏、倒计时栏
- [x] 左侧信息面板显示 CHANNEL / PROTOCOL / FRAME 三项
- [x] 右侧大圆环进度+数字百分比作为视觉焦点
- [x] 显示 `usRecFrameCnt`、`eChType`、`eProtoType`、进度、`eAppState` 五项数据
- [x] 倒计时基于 `tUpdate.tpProtoRx->usLostOverTimeCnt` 计算
- [x] 静态内容（标题/容器背景/标签/"%"/"Exit in "/"s"）仅初始化时绘制
- [x] 帧数"/TTTT"半静态，仅"NNNN"动态刷新32px
- [x] 进度"%"静态定位，仅数字动态刷新72px
- [x] 状态文字半静态，动画点仅刷新24px
- [x] 倒计时"Exit in "/"s"静态，仅数字动态刷新24px
- [x] 等待/擦除状态有旋转点动画（200ms/帧）
- [x] 包含成功(绿)/取消(橙)/超时(橙)状态显示
- [x] `tpProtoRx == NULL` 时不崩溃
- [x] 顶部5px遮挡区已处理（Y偏移+5）
- [x] CPU 占用峰值 < 15%
- [x] 320×240 分辨率下布局不溢出
