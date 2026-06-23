# 工程模式 UI 系统性梳理与优化报告

> 关联规范：[engineering_mode_spec.md](./engineering_mode_spec.md)
> 涉及文件：
> - `APP/Hardware/MD_Display/user_ui/eng_mode_ui.c`（LVGL UI 实现）
> - `APP/Hardware/MD_Display/md_display_queue_task_eng.c`（显示队列任务）
> - `APP/Hardware/MD_Display/md_display_eng_mode.c`（记忆参数/亮度类型，未改动）
> - `APP/Hardware/Key/key_func_eng.c`（工程模式按键映射）

---

## 1. 现状梳理

### 1.1 界面结构

工程模式共 5 个页面，由 `eng_mode_ui.c` 统一管理（`EngModeState_T.ePage`）：

```
ENG_PAGE_MAIN_MENU  主菜单（3 项）
├── ENG_PAGE_PARAM_VIEW  参数查看（7 Tab：BMS/MPPT/DCAC/USB/DC/ADC/SYS，每页 8 行）
├── ENG_PAGE_PARAM_SET   记忆参数设置（7 Tab：SYS/LCD/BAT/MPPT/DCAC/USB/DC，可滚动列表）
└── ENG_PAGE_SYS_SET     系统设置（保存退出/恢复默认/固件升级）
    └── ENG_PAGE_CONFIRM 确认对话框（取消/确认）
```

### 1.2 按键映射（优化前）

| 按键事件 | 功能 | 说明 |
|:---|:---|:---|
| `AC_SHORT` | KeyUp | 上移 / 增值 |
| `DC_SHORT` | KeyDown | 下移 / 减值 |
| `POWER_SHORT` | KeyEnter | 确认 / 选择 |
| `LIGHT_SHORT` | KeyRight | 右切 Tab |
| `POWER_LONG` | KeyBack | 返回 / 退出 |
| —— | KeyLeft | **UI 层已实现但无按键触发，左切 Tab 不可达** |

### 1.3 视觉规范

屏幕 320×240 横屏，暗色主题，配色见规范文档第 1 节。标题栏 24px，底部 Tab 栏 18px。

---

## 2. 发现的问题

| 编号 | 类别 | 问题 | 影响 |
|:---:|:---|:---|:---|
| P1 | 交互缺陷 | `vEngMode_KeyLeft()` 已实现但 `key_func_eng.c` 无对应按键映射，左切 Tab 功能不可达 | 与规范“左右按键切换 Tab”不一致，用户只能单向循环切 Tab |
| P2 | 显示 Bug | FanCtrl（SYS Tab 参数 1）取值语句 `S_tState.ucPsItem == 1 ? tEngMode.cEngModeState : 0`，依赖当前是否选中该项，非选中时恒显示 0 | 风扇状态显示错误 |
| P3 | 安全隐患 | 确认对话框默认选中“确认(OK)”，对“恢复默认/固件升级”等危险操作易误触发 | 误操作风险 |
| P4 | 易用性 | 参数设置列表选中项变化时不自动滚动到可视区，长列表（DCAC 13 项）中选中项被遮挡 | 操作不便 |
| P5 | 一致性 | 进入“参数查看/参数设置”页面时标题栏仍显示 `ENG MODE`，不随页面变化 | 定位感差 |
| P6 | 代码质量 | `eng_mode_ui.c` 重复包含 `Dc/dc_task.h`、`Usb/usb_task.h` | 冗余 |
| P7 | 日志噪声 | `md_display_queue_task_eng.c` 中 case0/case1 调试日志无条件输出，case1 每 1s 打印一次 | 生产环境日志噪声 |

---

## 3. 优化前后对比

### 3.1 [P1] 补全左切 Tab 按键映射

**文件**：`key_func_eng.c`

优化前：仅 5 个按键映射，`vEngMode_KeyLeft()` 无触发源。

优化后：新增 `LIGHT_LONG -> KeyLeft` 映射，与 `LIGHT_SHORT -> KeyRight` 对称，实现双向 Tab 切换。

```c
// 新增映射表
u8 const KeyTriType_LeftBuff[ 2 ] = { KTE_LIGHT_LONG, KTE_FUN_NULL };   //左切Tab

// v_key_func_eng() 内新增分支
else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_LeftBuff, sizeof(KeyTriType_LeftBuff)) )
{
    vEngMode_KeyLeft();
    #if(boardBUZ_EN)
    bBuz_Tweet(SHORT_1);
    #endif
}
```

> 说明：`LIGHT_LONG` 在工程模式下未被占用（正常模式下的灯开关在工程模式不生效），可安全复用。

### 3.2 [P2] 修复 FanCtrl 风扇状态显示

**文件**：`eng_mode_ui.c` → `v_ps_get_value_str()`

优化前（Bug）：
```c
case 1: snprintf(pc_buf, uc_size, "%d",
    S_tState.ucPsItem == 1 ? tEngMode.cEngModeState : 0); break;
```
取值依赖“当前是否选中该项”，非选中时恒为 0。

优化后：直接读取风扇实际工作档位 `eFan_GetWorkMode()`，显示 OFF/ON/FULL，与参数查看页 SYS Tab 的风扇显示一致。
```c
case 1: /* FanCtrl: 显示风扇实际工作档位 */
{
#if(boardHEAT_MANAGE_EN)
    FanWorkMode_E e_fm = eFan_GetWorkMode();
    const char *p_fm = "OFF";
    if(e_fm == FWM_GEAR_FULL) p_fm = "FULL";
    else if(e_fm > FWM_OFF)   p_fm = "ON";
    snprintf(pc_buf, uc_size, "%s", p_fm);
#else
    snprintf(pc_buf, uc_size, "-");
#endif
}break;
```

### 3.3 [P3] 确认对话框默认选项安全化

**文件**：`eng_mode_ui.c` → `vEngMode_KeyEnter()` (ENG_PAGE_SYS_SET 分支)

优化前：所有操作均默认选中“确认”。
```c
S_tState.ucConfirmSel = 1;  /* 默认选中确认 */
```

优化后：保存退出(非破坏性)默认“确认”，恢复默认/固件升级(破坏性)默认“取消”。
```c
/* 保存退出(SAVE&EXIT)默认选中确认, 重置/升级等危险操作默认选中取消, 防止误触发 */
S_tState.ucConfirmSel = (S_tState.ucSsSel == 0) ? 1 : 0;
```

### 3.4 [P4] 参数列表选中项自动滚动

**文件**：`eng_mode_ui.c` → `v_ps_update_selection()`

优化前：选中项超出可视区时不滚动，长列表需盲操作。

优化后：选中项变化后调用 `lv_obj_scroll_to_view()` 自动滚入可视区。
```c
/* 选中项自动滚动到可视区域, 避免长列表中选中项被遮挡 */
if(S_tState.ucPsItem < uc_cnt && S_tObjs.p_ps_items[S_tState.ucPsItem] != NULL)
{
    lv_obj_scroll_to_view(S_tObjs.p_ps_items[S_tState.ucPsItem], LV_ANIM_ON);
}
```

### 3.5 [P5] 标题栏随页面上下文更新

**文件**：`eng_mode_ui.c` → `v_page_show()`

优化前：进入参数查看/参数设置页面时标题栏保持 `ENG MODE`。

优化后：分别显示 `PARAM VIEW` / `PARAM SET`，与主菜单/系统设置页一致，提升页面定位感。

### 3.6 [P6] 清理重复包含

**文件**：`eng_mode_ui.c`

优化前：`Dc/dc_task.h`、`Usb/usb_task.h` 各被包含两次。
优化后：删除文件末尾重复的 `#if(boardDC_EN)/#if(boardUSB_EN)` 包含块。

### 3.7 [P7] 调试日志受控输出

**文件**：`md_display_queue_task_eng.c`

优化前：case0 进入日志、case0 渲染日志、case1 周期日志（每 1s）均无条件 `sMyPrint`。

优化后：全部由 `uPrint.tFlag.bDispTask` 调试开关控制，与超时日志（`log_w`）的受控方式一致，生产环境无噪声。
```c
if(uPrint.tFlag.bDispTask)
    sMyPrint("DispEng: case 1 running, active_scr = %p\r\n", lv_screen_active());
```

---

## 4. 交互流程对照（优化后）

```
主菜单
  AC_SHORT/DC_SHORT : 上下选择
  POWER_SHORT       : 进入子页面
  POWER_LONG        : 退出工程模式

参数查看 / 参数设置
  LIGHT_SHORT : 右切 Tab   LIGHT_LONG : 左切 Tab   (新增双向切换)
  AC_SHORT    : 上移/增值   DC_SHORT  : 下移/减值
  POWER_SHORT : 确认/下一参数
  POWER_LONG  : 返回主菜单

系统设置
  AC_SHORT/DC_SHORT : 上下选择
  POWER_SHORT       : 弹出确认框 (危险操作默认聚焦“取消”)
  POWER_LONG        : 返回主菜单

确认对话框
  AC_SHORT/DC_SHORT : 切换 取消/确认
  POWER_SHORT       : 执行选中项
  POWER_LONG        : 返回系统设置
```

---

## 5. 未改动项与后续建议

| 项 | 说明 | 建议 |
|:---|:---|:---|
| 温度单位 | 实现使用 `C`，规范要求 `°C` | 后续统一为 `°C`，需确认 Barlow/Montserrat 字体含度数符号且源文件 UTF-8 编码稳定 |
| 参数查看布局 | 实现为“每行 1 项（标签+值）”，规范为“每行 2 项（左右双列）” | 屏幕宽度受限，当前单列布局可读性良好；如需双列需重构 `v_pv_update_data` 与行对象数组，建议评估后再改 |
| UI 创建期内存监控日志 | `vEngMode_UiCreate` 内 6 处 `lv_mem_monitor` 日志 | 为开发期内存诊断保留；如需静默可引入 `print_task.h` 后用 `bDispTask` 包裹 |
| 后台 `v_sys_queue_task_eng.c` | `EMS_SET` 分支保存/重置逻辑被注释（goto shut_down） | 当前由 UI 层 `vEngMode_KeyEnter` 直接执行保存/重置，后台逻辑未启用，二者并存需注意避免重复执行 |

---

## 6. 验证

- 三个被修改文件 VS Code 诊断均无报错/警告。
- 改动均为增量式，未改变现有函数签名与全局变量，保持与 `md_display_eng_mode.c`、`sys_queue_task_eng.c` 后端接口兼容。
- 建议上机验证：① 左切 Tab 可用；② FanCtrl 显示随风扇状态变化；③ DCAC 13 项列表上下选择时自动滚动；④ 恢复默认/固件升级确认框默认聚焦“取消”；⑤ 各页面标题栏文本正确。
