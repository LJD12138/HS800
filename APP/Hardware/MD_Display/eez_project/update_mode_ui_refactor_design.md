# HS800 APP 升级模式 UI 重构设计方案

> 文件：`APP/Hardware/MD_Display/eez_project/eez_project.eez-project`
> 日期：2026-06-24
> 作者：LJD
> 状态：待确认

---

## 一、现状分析

### 1.1 当前 Main_Update 屏幕（待重构）

- **布局**：垂直居中堆叠（标题→Spinner→百分比→状态消息→进度条→倒计时）
- **数据展示**：仅通过 `uca_update_msg` 多行文本打包显示部分数据，信息密度低、层次不清
- **进度指示**：Spinner（等待动画）+ Bar（进度条）双重指示，主次不分
- **全局变量**：`uca_update_progress`、`uca_update_countdown`、`uca_update_state`、`uca_update_msg`

### 1.2 数据源（`Update_T` 结构体）

```c
typedef struct {
    vu16  usRecFrameCnt;      // 当前接收帧数
    vu16  usTotalFrmValue;    // 总帧数
    vu16  usRecOverTimeCnt;   // 帧超时计数
    vu16  usLostOverTimeCnt;  // 接收超时计数
    UpdateObj_E   eObj;       // 升级对象 (Host/Console/BMS/MPPT/DCAC)
    ChannelType_E eChType;    // 通道类型 (None/Console/Print)
    ProtoType_E   eProtoType; // 协议类型 (None/Xmodem/Baiku)
} Update_T;
```

### 1.3 参考布局（BOOT `update_mode_ui.c`，仅参考结构不参考视觉）

- 左信息面板 + 右仪表盘分区布局
- 5 个独立容器：标题栏、信息面板、仪表盘、状态栏、倒计时栏
- 静态/动态分离，信息项标签+数值对齐

---

## 二、目标布局设计（320×240）

### 2.1 布局总览

```
┌─────────────────────────────────────────────────────┐ y=0
│              FIRMWARE UPDATE                         │ 标题区 (0,0,320,30)
├──────────────────┬──────────────────────────────────┤ y=32
│   OBJ            │                                  │
│   Host           │           ╭──────╮               │
│                  │           │  75  │               │
│   CHANNEL        │           │   %  │               │ 右侧仪表盘区
│   Print          │           ╰──────╯               │ (138,32,178,116)
│                  │                                   │ Arc圆环 + 百分比
│   PROTOCOL       ├──────────────────────────────────┤ y=150
│   Xmodem         │         Upgrading...             │ 状态栏 (138,150,178,28)
│                  │                                   │
│   FRAME          │                                   │
│   0768/1024      │                                   │
│                  │                                   │
│   OT CNT         │                                   │
│   000            │                                   │
├──────────────────┴──────────────────────────────────┤ y=204
│              Reboot in  10 s                        │ 倒计时区 (0,204,320,36)
└─────────────────────────────────────────────────────┘ y=240

  左信息面板               右仪表盘+状态栏
  (4,32,130,168)          (138,32,178,146)
```

### 2.2 容器/组件定义

| 区域 | 坐标 (x,y,w,h) | LVGL组件 | 说明 |
|------|----------------|----------|------|
| 标题区 | (0, 0, 320, 30) | Label | "FIRMWARE UPDATE"，font-26，白色，居中 |
| 左信息面板 | (4, 32, 130, 168) | Container | 深色背景容器，圆角 |
| 右仪表盘区 | (138, 32, 178, 116) | Container + Arc | 圆环进度仪表 |
| 状态栏 | (138, 150, 178, 28) | Label | 状态文字，颜色随状态变化 |
| 倒计时区 | (0, 204, 320, 36) | Label | "Reboot in Ns"，font-26，居中 |

> [!NOTE]
> 调整说明：左面板从 118 扩到 130（+12px），右面板相应从 190 减到 178（-12px），间距从 4 增到 8。值标签宽度从 110 增到 122（与左面板同步），为长字符串（如 "Console"/"Xmodem"）提供更宽的渲染区。

### 2.3 左信息面板内 5 行数据

每行采用 **标签简写（左，font-14 灰色）+ 数值右对齐（右，font-26 白色）** 并排布局，行高 30px。
通过简写标签和右对齐设计，可彻底避免大字号数值与标签在 118px 窄通道内水平重叠：

| 行 | 相对Y坐标 | 标签简写 | 默认数值内容 | 数据源 / 计算方式 | 刷新策略 |
|----|-----------|----------|--------------|-------------------|----------|
| 1 | 6 | "OBJ" | Host/Console/BMS/MPPT/DCAC | `eObj` | 值变化时 |
| 2 | 36 | "CH" | None/Console/Print | `eChType` | 值变化时 |
| 3 | 66 | "PROT" | None/Xmodem/Baiku | `eProtoType` | 值变化时 |
| 4 | 96 | "FRM" | 0768/1024 | `usRecFrameCnt` / `usTotalFrmValue` | 帧数变化时 |
| 5 | 126 | "TIMEOUT" | 360 | `UPDATE_TICK_TO_SEC(usLostOverTimeCnt)` (秒) | 秒数变化时 |

> [!NOTE]
> `usLostOverTimeCnt` 和 `usRecOverTimeCnt` 均为底层递减 tick 计时器（初始化为 `updateREC_LOST_OVERTIME = (360*1000)/boardREPET_TIMER_CYCLE_TMIE`，约 36000 递减到 0）。其中 `usLostOverTimeCnt` 为整体接收超时计时器，收到帧时重置；`usRecOverTimeCnt` 为帧间超时计时器。Row 5 选择显示 `usLostOverTimeCnt` 换算的剩余超时秒数（TIMEOUT），逻辑与 BOOT 对齐。
>
> 换算宏定义（避免硬编码 `/100`，适配不同定时器周期）：
> ```c
> #define UPDATE_TICK_TO_SEC(tick)  ((uint16_t)((tick) * boardREPET_TIMER_CYCLE_TMIE / 1000))
> ```

### 2.4 右仪表盘内布局

| 元素 | 坐标/参数 | 说明 |
|------|-----------|------|
| Arc 圆环 | center=(221,90), r=50, thickness=6 | 进度 0-100%，bg=暗色轨道，indicator=亮色 |
| 百分比数字 | 圆环中心，font-50，白色，X=126, Y=50, W=190, H=50, 居中 | 仅显示纯数字（如 "75"），**不包含** `%` 符号 |
| 百分号 | 静态 Label 或 Image，放在数字右侧 | 静态显示的 "%" 字符，避免与动态格式化冲突 |
| 状态文字 | (126,150,190,28)，font-26 | 状态文字+颜色随状态变化 |

### 2.5 字体方案

| 字体 | 用途 | 说明 |
|------|------|------|
| BarlowCondensed-Regular-50 | 百分比数字 | **现有**，仪表盘大数字 |
| BarlowCondensed-Regular-26 | 标题/数值/状态/倒计时 | **现有**，主要文字 |
| BarlowCondensed-Regular-14 | 面板标签 | **新增**，小号灰色标签 |

> 新增 14px 字体仅用于信息面板行标签，使标签与数值形成层次对比，参考 BOOT 布局中"标签小+数值大"的结构关系。

---

## 三、全局变量设计

### 3.1 新增变量

> [!WARNING]
> `uca_update_progress` 当前仅以 `__attribute__((weak))` 弱链接形式存在于 `md_display_queue_task_updata.c` 中，**不在** EEZ 生成的 `vars.c/vars.h` 中。本次重构需在 EEZ 工程中正式创建该变量，并移除弱链接兼容代码。

| 变量名 | 类型 | 用途 | 绑定方式 |
|--------|------|------|----------|
| `uca_update_progress` | string | 进度数字（如 "75"，**不含%**），绑定仪表盘数字 Label | tick 自动刷新 Label |
| `uca_update_obj` | string | 升级对象字符串 | tick 自动刷新 Label |
| `uca_update_channel` | string | 通道类型字符串 (如 "Console") | tick 自动刷新 Label |
| `uca_update_proto` | string | 协议类型字符串 (如 "Xmodem") | tick 自动刷新 Label |
| `uca_update_frame` | string | "NNNN/TTTT" 帧数格式化串 | tick 自动刷新 Label |
| `uca_update_timeout` | string | 剩余超时秒数 (如 "360") | tick 自动刷新 Label |

### 3.2 保留变量（复用）

| 变量名 | 类型 | 用途调整 |
|--------|------|----------|
| `uca_update_countdown` | string | 倒计时文字（如 "Reboot in 10s"） |
| `uca_update_state` | integer | UI状态（0:Waiting, 1:Upgrading, 2:Success, 3:Failed） |
| `uca_update_msg` | string | **改为状态文字**（如 "Upgrading..."），绑定状态栏 Label（原绑定 `uc_update_waiting_label` → 新绑定 `obj_status_label`） |

### 3.3 直接操作对象（不走变量，显示任务中直接调用 LVGL API）

| 对象 | 操作 | 说明 |
|------|------|------|
| Arc 圆环 | `lv_arc_set_value()` | 进度值 0-100，带动画 |
| 状态栏 Label | `lv_obj_set_style_text_color()` | 状态颜色切换 |
| Spinner（等待时） | 显示/隐藏切换 | 等待状态显示旋转动画 |

---

## 四、状态机设计

### 4.1 状态定义

| 状态值 | 状态名 | 状态文字 | 颜色 | Arc表现 | Spinner | 动画 |
|--------|--------|----------|------|---------|---------|------|
| 0 | WAITING | "Waiting for update..." | 灰色 #AAAAAA | 隐藏 | 显示（旋转） | 文字省略号 200ms 切换 |
| 1 | UPGRADING | "Upgrading..." | 白色 #FFFFFF | 显示，0-100%进度动画 | 隐藏 | Arc 平滑增长 |
| 2 | SUCCESS | "Update Complete!" | 绿色 #4CAF50 | 显示，100%静止 | 隐藏 | 无 |
| 3 | FAILED | "Update Failed!" | 红色 #F44336 | 显示，保持当前值静止 | 隐藏 | 无 |

### 4.2 状态转换条件

```
WAITING ──(usTotalFrmValue>0 && usRecFrameCnt>0)──→ UPGRADING
WAITING ──(180s超时)──→ FAILED
UPGRADING ──(percent>=100)──→ SUCCESS
UPGRADING ──(15s无新帧)──→ FAILED
SUCCESS/FAILED ──(倒计时归零)──→ 重启
```

### 4.3 动画效果

| 场景 | 动画方式 | 实现 |
|------|----------|------|
| 进度增长 | Arc 值平滑过渡 | `lv_arc_set_value()` + LVGL 内置 anim |
| 等待旋转 | 独立 Spinner 组件 | WAITING 状态显示 `uc_update_spinner`（LVGL 原生 lv_spinner），隐藏 Arc；其他状态隐藏 Spinner |
| 状态切换 | 文字颜色淡入 | `lv_obj_set_style_text_color()` 直接设置 |
| 屏幕载入 | 淡入过渡 | `loadScreen()` 已内置 `lv_scr_load_anim(..., LV_SCR_LOAD_ANIM_FADE_IN, 200, ...)`，无需额外处理 |
| 等待动画点 | 文字省略号变化 | "Waiting." → "Waiting.." → "Waiting..." 三状态 200ms 循环切换 |

---

## 五、EEZ Studio 工程修改清单

### 5.1 `.eez-project` 文件修改

| 修改项 | 内容 |
|--------|------|
| 新增字体 | BarlowCondensed-Regular-14（size=14，同 ttf 源文件） |
| 新增全局变量 | `uca_update_progress`、`uca_update_obj`、`uca_update_channel`、`uca_update_proto`、`uca_update_frame`、`uca_update_timeout`（6个 string） |
| 重构 Main_Update 屏幕 | 替换现有组件为左侧面板+右侧仪表盘布局 |

### 5.2 Main_Update 屏幕新组件树

> [!IMPORTANT]
> **注意**：容器内部组件必须配置为**相对父容器的相对坐标**（Row 5 的 Y 若误用绝对值会超出容器底部边缘导致显示不全）。
> 所有 Value 标签（`obj_*_value`）的文本对齐方式（Align）在 EEZ Studio 中必须设置为**右对齐（Right Align）**，以规避重叠。

```
Screen: main_update (320×240)
├── obj_title_label        Label  "FIRMWARE UPDATE"  font-26 白色 居中 (0, 0, 320, 30)
├── obj_info_panel         Container (4, 32, 118, 168)  深色背景 圆角
│   ├── obj_obj_label      Label  "OBJ"      font-14 灰色相对 (8, 6)
│   ├── obj_obj_value      Label  uca_update_obj       font-26 右对齐 (x=4, y=2, w=110)
│   ├── obj_ch_label       Label  "CH"       font-14 灰色相对 (8, 36)
│   ├── obj_ch_value       Label  uca_update_channel   font-26 右对齐 (x=4, y=32, w=110)
│   ├── obj_proto_label    Label  "PROT"     font-14 灰色相对 (8, 66)
│   ├── obj_proto_value    Label  uca_update_proto     font-26 右对齐 (x=4, y=62, w=110)
│   ├── obj_frm_label      Label  "FRM"      font-14 灰色相对 (8, 96)
│   ├── obj_frm_value      Label  uca_update_frame     font-26 右对齐 (x=4, y=92, w=110)
│   ├── obj_to_label       Label  "TIMEOUT"  font-14 灰色相对 (8, 126)
│   └── obj_to_value       Label  uca_update_timeout   font-26 右对齐 (x=4, y=122, w=110)
├── obj_gauge_panel        Container (126, 32, 190, 116)  深色背景
│   ├── obj_progress_arc   Arc    进度圆环 0-100  center(221,90) r=50
│   │                      初始化: lv_arc_set_range(arc, 0, 100); lv_arc_set_value(arc, 0)
│   ├── obj_progress_label Label  uca_update_progress  font-50 白色 居中 (x=35, y=33, w=100)
│   ├── obj_pct_label      Label  "%" 字符   font-26 白色相对 (135, 45)
│   └── uc_update_spinner  Spinner 等待动画  相对 (45, 8, 100, 100)  WAITING显示/其他隐藏
├── obj_status_label       Label  uca_update_msg  font-26 居中 (126, 150, 190, 28)
└── obj_countdown_label    Label  uca_update_countdown  font-26 居中 (0, 204, 320, 36)
```

> [!IMPORTANT]
> **objects_t 结构体更新**：需在 `screens.h` 的 `objects_t` 中新增以下成员：
> `obj_info_panel`、`obj_gauge_panel`、`obj_progress_arc`、`obj_progress_label`、`obj_pct_label`、`obj_status_label`、`obj_title_label`、`obj_obj_label`、`obj_obj_value`、`obj_ch_label`、`obj_ch_value`、`obj_proto_label`、`obj_proto_value`、`obj_frm_label`、`obj_frm_value`、`obj_to_label`、`obj_to_value`。
> 移除旧成员：`uc_update_bar`、`uc_update_waiting_label`（`uc_update_spinner` 保留）。
> 移除旧成员 `obj4`、`obj5`、`obj6`（由上述语义化命名替代）。

### 5.3 显示任务修改（`md_display_queue_task_updata.c`）

| 修改项 | 内容 |
|--------|------|
| 新增数据格式化 | `eObj`/`eChType`/`eProtoType` 枚举转字符串，设置 5 个新变量 |
| Arc 进度驱动 | `lv_arc_set_value(objects.obj_progress_arc, percent)` 替代 `lv_bar_set_value()`；初始化时调用 `lv_arc_set_range(arc, 0, 100)` |
| Spinner 显示/隐藏 | WAITING 状态 `lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN)` + `lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN)`；其他状态反之 |
| 状态文字驱动 | 设置 `uca_update_msg` 为状态文字（不再是多行打包消息） |
| 等待动画分频 | WAITING 状态下周期切换 "Waiting." / "Waiting.." / "Waiting..."。由于运行频率为 33ms/步，声明静态局部变量计数器 `s_uc_anim_cnt`，用分频 `(s_uc_anim_cnt / 6) % 3` 过滤出 ~200ms 的动画步进（三状态循环）。 |
| 移除旧逻辑 | 移除 `v_format_update_msg()` 多行打包逻辑，改为独立变量 |
| 兼容性弱定义 | 移除 `uca_update_progress` 弱链接（EEZ 重新生成后不再需要） |

---

## 六、数据映射表

| 数据源 | 转换 | 目标变量/对象 | 刷新触发 |
|--------|------|---------------|----------|
| `tUpdate.eObj` | enum→"Host"/"Console"/... | `uca_update_obj` → obj_obj_value | eObj 变化 |
| `tUpdate.eChType` | enum→"None"/"Console"/"Print" | `uca_update_channel` → obj_ch_value | eChType 变化 |
| `tUpdate.eProtoType` | enum→"None"/"Xmodem"/"Baiku" | `uca_update_proto` → obj_proto_value | eProtoType 变化 |
| `tUpdate.usRecFrameCnt` + `usTotalFrmValue` | `snprintf("%04u/%04u")` | `uca_update_frame` → obj_frm_value | 帧数变化 |
| `tUpdate.usLostOverTimeCnt` | `UPDATE_TICK_TO_SEC()` 换算秒并 `snprintf("%03u")` | `uca_update_timeout` → obj_to_value | 秒数变化 |
| `usRecFrameCnt*100/usTotalFrmValue` | 计算 0-100 纯数值，不拼接 `%` | Arc 直接设值 + `uca_update_progress`（绑 `obj_progress_label`） | 进度变化 |
| 状态机 | enum→状态文字 | `uca_update_msg` → obj_status_label | 状态变化 |
| 内部倒计时 | `snprintf("Reboot in %us")` | `uca_update_countdown` → obj_countdown_label | 秒数变化 |

---

## 七、与 BOOT 设计的对照

| 对比项 | BOOT 实现 | 本方案 APP 实现 |
|--------|-----------|----------------|
| 布局结构 | 左信息面板+右仪表盘 | **相同**（参考） |
| 信息面板行数 | 3行（CHANNEL/PROTOCOL/FRAME） | 5行（+OBJ+OT CNT），覆盖全部要求数据 |
| 仪表盘组件 | 自绘 `DrawSegmentedRing` | LVGL Arc Widget（原生组件） |
| 进度指示 | 圆环+数字 | **相同**（Arc+数字+百分号） |
| 状态动画 | 自绘旋转点 | LVGL 文字省略号切换 / Spinner |
| 静态/动态分离 | 是（static 变量检测） | 是（EEZ tick 自动 diff + 显示任务变化检测） |
| 字体/颜色 | 8x16位图字体 / RGB565 | BarlowCondensed / LVGL hex 色值（**不参考**） |

---

## 八、风险与对策

| 风险 | 对策 |
|------|------|
| 相对坐标混淆为绝对坐标导致底端溢出裁剪 | 明确 EEZ 容器子项为相对 Y 坐标（Row 1-5 相对 Y 递增为 6, 36, 66, 96, 126，底端位于 152 像素，小于容器高 168） |
| 大数字与标签水平空间不足导致重叠 | 1. 缩短标签字数（OBJ, CH, PROT, FRM, TIMEOUT）<br>2. 右对齐值标签：设置宽 110 像素且 Right Align，令长字符自动左延，保留安全间距 |
| 进度条数字带 `%` 与静态 `%` 图标造成双字符冲突 | `uca_update_progress` 只格式化纯数字，排除 `%` 后缀 |
| `usLostOverTimeCnt` 周期变动高频消耗 CPU | 不直接渲染 tick 倒计时，使用 `UPDATE_TICK_TO_SEC()` 宏换算为秒数后渲染，保持一秒仅变更一次 |
| 新增 14px 字体增加 Flash 占用 | BarlowCondensed-14 仅含 ASCII 子集，约 3-5KB |
| Arc 动画在低性能 MCU 上卡顿 | 使用 `lv_arc_set_value()` 无动画模式，或限制 anim 时间 |
| EEZ 重新生成代码覆盖手动修改 | 所有动态逻辑放在 `md_display_queue_task_updata.c`，不修改生成代码 |
| `objects.obj_progress_arc` 访问时机 | 显示任务 step 1 中判空后操作 |
| 弱链接变量冲突 | EEZ 重新生成后移除 `md_display_queue_task_updata.c` 中的弱链接定义 |

---

## 九、实施步骤

1. **修改 `.eez-project` 文件**：新增 font-14、6 个全局变量、重构 Main_Update 屏幕组件树（EEZ Studio GUI 操作，同步更新工程文件）
2. **手动更新生成的源文件**（若不使用 EEZ Studio 重新生成）：直接修改 `eez_ui/` 下的 `vars.c/h`、`screens.c/h`、`fonts.h`、新增 `ui_font_barlow_condensed_regular_14.c`
3. **修改 `md_display_queue_task_updata.c`**：适配新变量、Arc 驱动、Spinner 显示/隐藏、状态文字、等待动画
4. **移除弱链接兼容代码**：清理旧的 `uca_update_progress` 弱定义
5. **编译验证**：确保无编译错误

---

## 十、验收标准

- [ ] 左信息面板显示 OBJ / CH / PROT / FRM / TIMEOUT 五项数据并右对齐排布，无重叠和截断
- [ ] 右侧 Arc 圆环进度+数字百分比（纯数字，百分号静态）作为视觉焦点
- [ ] 正确显示 `usLostOverTimeCnt` 秒数倒计时、`usRecFrameCnt`、`usTotalFrmValue`、`eObj`、`eChType`、`eProtoType`、升级进度
- [ ] 进度增长有平滑动画效果
- [ ] 等待状态有旋转/省略号分频动画（200ms步进）
- [ ] 状态切换有颜色反馈（灰→白→绿/红）
- [ ] 屏幕载入有淡入过渡
- [ ] 倒计时正确显示并归零后触发重启
- [ ] 320×240 分辨率下布局不溢出
- [ ] 新增 font-14 字体正常渲染
- [ ] EEZ 重新生成代码后无编译错误
