# EEZ Studio + UniDisplay 显示任务架构详解

> 参考实现：HS800 APP 工程 `APP\Hardware\MD_Display` + `APP\Middlewares\UniDisplay`。
> 本文档是 SKILL.md 的深度参考，含目录职责、设计依据、关键代码模式与移植清单。

## 1. 目录结构与职责

```
APP/
├── Middlewares/UniDisplay/               # 通用件（跨项目复用，不含任何业务）
│   ├── uni_disp_core.c/.h                # 调度引擎：页面注册表/状态路由/弹窗栈/背光息屏/EnginePoll
│   ├── uni_disp_page.h                   # 页面契约：DispPageId_E / DispPage_T / 三个数据钩子声明
│   ├── uni_disp_event.c/.h               # 事件总线：静态环形队列（DispEvent_E / DispEventMsg_T）
│   └── uni_disp_port.c/.h                # 移植层：临界区 + 任务唤醒钩子（唯一按平台适配的文件）
│
└── Hardware/MD_Display/                  # 项目件（HS800 业务，移植时按此骨架重建）
    ├── md_display_task.c/.h              # 任务外壳：cDisp_TaskInit 建任务/页面静态注册/记忆参数/低功耗
    ├── md_display_data.c/.h              # 数据快照池：DispDataSnapshot_T + 状态路由表 + 息屏判定
    ├── md_display_api.c/.h               # 显示 API：vDisp_Init/vDisp_LoadScreen/vDisp_UiRefresh
    ├── md_display_iface.c/.h             # 硬件接口：SPI(软/硬)+DMA、GPIO 宏、ST7789 写命令原语
    ├── lv_port_disp.c/.h                 # LVGL 显示驱动注册（双缓冲 + DMA + 信号量同步）
    ├── md_display_err.c                  # 错误码轮显逻辑（usDisp_ErrCodeDisplay）
    ├── md_display_page_init.c            # 初始化页（预留，不显示）
    ├── md_display_page_boot.c            # 开机步进动画页
    ├── md_display_page_work.c            # 主工作页（EEZ 屏 + user_ui 混合渲染的样板）
    ├── md_display_page_fault.c           # 故障弹窗页（Push/Pop 模式样板）
    ├── md_display_page_closing.c         # 关机动画页
    ├── md_display_page_shut_down.c       # 关机状态页（预留，不显示）
    ├── md_display_page_update.c          # 固件升级进度页
    ├── md_display_page_eng.c             # 工程模式页（纯 user_ui 动态创建/删除样板）
    │
    ├── eez_project/                      # EEZ Studio 设计源（人维护）
    │   ├── eez_project.eez-project       # EEZ Studio 工程文件（UI 设计源）
    │   ├── Image/                        # 图片资源（PNG 源）
    │   ├── fonts/                        # 字体源（TTF/OTF + 生成的 .c）
    │   ├── gen_eng_mode.js               # 复杂界面脚本生成 EEZ 工程的模式（Node）
    │   └── *.md                          # UI 重构设计文档
    │
    ├── eez_ui/                           # EEZ Studio 生成件（只读！构建即覆盖）
    │   ├── .eez-project-build            # 生成文件清单（manifest，判定哪些文件会被覆盖）
    │   ├── ui.c/.h                       # ui_init/ui_tick/loadScreen
    │   ├── screens.c/.h                  # 屏幕对象与控件树（objects_t 全局对象表）
    │   ├── vars.h                        # Native 变量 get/set 声明（生成）
    │   ├── vars.c                        # Native 变量实现（手写！不在清单内，唯一例外）
    │   ├── actions.h / structs.h         # 动作/结构（生成，本工程为空壳）
    │   ├── styles.c/.h, images.c/.h, fonts.h
    │   └── ui_font_*.c, ui_image_*.c     # 生成的字体/图片资源数组
    │
    └── user_ui/                          # 用户自绘件（手写，永不被覆盖）
        ├── main_1_ui.c/.h                # 主工作界面：EEZ 变量桥 + 设备图标/错误码/动画编排
        ├── energy_ring.c/.h              # 能量环组件（纯 LVGL 自绘）
        ├── img_breath.c/.h               # 电量呼吸动画组件（ManualTick 模式）
        └── eng_mode_ui.c/.h              # 工程模式 UI（Create/Tick/Delete 三段式）
```

## 2. 设计依据（为什么这么分）

| 设计决策 | 依据 |
| :--- | :--- |
| 通用件/项目件分离 | UniDisplay 只感知快照首字段 ucDevState（opaque 前向声明），换产品只改项目件；四个数据钩子是唯一契约面 |
| 快照模式 | 其它任务全局变量多线程裸读有撕裂风险；每帧临界区抓一次快照，路由与渲染共用一份数据，帧内绝对一致 |
| 事件队列而非直接调用 | 其它任务直接调页面回调会引入并发；静态环形队列 + 任务通知唤醒，生产者任意任务、消费者唯一显示任务 |
| 状态路由用变化沿 | 电平触发会与页内导航互踩（用户在工程模式翻页被状态路由拉回）；变化沿 + 无映射返回 PAGE_ID_MAX 维持当前页 |
| 弹窗栈与显式导航互斥 | RequestPage 清栈保证"导航即回主路径"；Push/Pop 幂等保护防重复故障事件把栈顶刷成故障页自身 |
| 帧周期补偿等待 | 固定 delay 会导致 帧周期+渲染时间 漂移；扣除本帧已耗时使实际帧率贴近页面声明值 |
| 息屏 2s 心跳 + 通知唤醒 | 息屏后无渲染需求，低频挂起 CPU 0%；事件入队/亮屏通过任务通知立即打断心跳 |
| 1S TickTimer 独立计时 | 息屏倒计时若按轮询次数折算，帧周期变化时计时不准；由系统 1S 定时器直调，绝对准确 |
| lv_tick_set_cb 复用 RTOS 节拍 | 免去 lv_tick_inc 喂值任务；configTICK_RATE_HZ=1000 时节拍即毫秒 |
| lv_timer_handler 单点调用 | LVGL 非线程安全；所有 LVGL 调用收敛到显示任务，天然免锁 |
| EEZ 生成与手写分离 | EEZ Studio 每次构建覆盖 eez_ui；user_ui 独立目录保证手写代码零丢失 |
| vOnEnter 先静态后重组件 | lv_refr_now 先推出静态框架，能量环等重组件随后创建，缩短用户切屏等待感知 |

## 3. 关键代码模式

### 3.1 任务主体（帧周期补偿等待）

```c
void vDisp_Task(void *pvParameters)
{
    TickType_t x_start, x_period, x_elapsed;

    for (;;)
    {
        x_start = xTaskGetTickCount();

        vDisp_EnginePoll();                 /* 快照→路由→事件→切页→渲染 */

        if (bDisp_IsBacklightOn() == false)
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));      /* 息屏低频心跳 */
        else
        {
            x_period  = pdMS_TO_TICKS(usDisp_GetFramePeriod());
            x_elapsed = xTaskGetTickCount() - x_start;
            ulTaskNotifyTake(pdTRUE, (x_elapsed < x_period) ? (x_period - x_elapsed) : 1U);
        }
    }
}
```

### 3.2 任务初始化顺序（顺序不可换）

```c
s8 cDisp_TaskInit(void)
{
    vDisp_CoreInit();                       /* 1. 引擎复位（息屏参数从记忆参数载入） */
    vDisp_IfaceInit();                      /* 2. 显示接口引脚/时钟 */
    vDisp_Init();                           /* 3. LVGL + 驱动 + EEZ ui_init（在 b_task_param_init 前，页面 OnEnter 依赖 LVGL 就绪） */
    if (!b_task_param_init())               /* 4. 页面静态注册 */
        return -1;
    xTaskCreate(...);                       /* 5. 创建显示任务 */
    vDisp_PortSetTaskHandle(tDispTaskHandler); /* 6. 绑定唤醒钩子 */
}
```

### 3.3 数据钩子实现（项目件三契约）

```c
/* 快照抓取：临界区保护多字节字段；低频量可临界区外经访问函数直读（显示任务单读者） */
void vDisp_DataCaptureSnapshot(DispDataSnapshot_T *p_snapshot)
{
    mainENTER_CRITICAL();
    p_snapshot->ucDevState = (uint8_t)tSysInfo.eDevState;   /* 首字段=路由依据，框架唯一感知 */
    /* ...其余项目字段... */
    mainEXIT_CRITICAL();
}

/* 息屏策略：哪些设备状态参与自动息屏 */
bool bDisp_DataAutoOffArmed(uint8_t uc_dev_state) { return (uc_dev_state == DS_WORK); }

/* 状态→页面映射：返回 PAGE_ID_MAX 表示维持当前页 */
DispPageId_E eDisp_MapDevStateToPage(uint8_t uc_dev_state)
{
    switch ((DevState_E)uc_dev_state)
    {
        case DS_BOOTING:   return PAGE_ID_BOOT;
        case DS_ERR:       return PAGE_ID_FAULT;
        /* ... */
        default:           return PAGE_ID_MAX;
    }
}
```

### 3.4 EEZ 变量桥（user_ui 写数据，EEZ Flow 自动刷）

```c
/* user_ui 侧：业务值 → EEZ Native 变量（字符串） */
void vDisp_UpdateDevParam(void)
{
    char cStr[10];
    snprintf(cStr, sizeof(cStr), "%d", tSysInfo.usOutPwr);
    set_var_uca_out_pwr_value(cStr);        /* ui_tick() 内 EEZ Flow 比对刷新绑定控件 */
}

/* EEZ 做不了的效果（可见性/闪烁/换图）直接操作生成的对象表 */
v_disp_set_icon_visible(objects.b_dev_ac_out_state, b_show);       /* hidden flag */
lv_image_set_src(objects.b_dev_pv_state, &img_icon_dc);            /* 动态换源 */
```

### 3.5 脏区友好更新（静态缓存比对）

```c
static int8_t s_last_dev_usb_state = -1;
if (s_last_dev_usb_state != b_show || b_force)
{
    s_last_dev_usb_state = b_show;
    lv_obj_add_flag/clear_flag(objects.b_dev_usb_state, LV_OBJ_FLAG_HIDDEN);
}
```

### 3.6 页面两种集成形态

| 形态 | 样板 | 适用 |
| :--- | :--- | :--- |
| EEZ 屏 + user_ui 覆盖件 | md_display_page_work.c + main_1_ui.c | 常驻业务主界面：EEZ 出静态布局，自绘出动画/动态图标 |
| 纯 user_ui 动态全屏 | md_display_page_eng.c + eng_mode_ui.c（UiCreate/UiTick/UiDelete） | 临时性复杂界面：进页创建、退页整树删除 |

### 3.7 故障弹窗闭环（框架默认处理 + 页面消费）

```
故障发生: 其它任务 bDisp_PostEvent(DISP_EVT_FAULT_POPUP, err)
  → 页面未消费 → 框架默认 vDisp_PushPage(PAGE_ID_FAULT)   (压栈, 幂等)
故障全清: bDisp_PostEvent(DISP_EVT_FAULT_CLEAR, 0)
  → fault 页 bOnEvent 消费 → vDisp_PopPage() 退栈回工作页
设备状态变化沿路由始终最高优先级（清栈），保证严重故障覆盖弹窗
```

## 4. 板级配置（board_config 显示相关）

| 宏 | 作用 |
| :--- | :--- |
| boardDISPLAY_EN | 显示总开关（所有显示文件 #if 包裹） |
| boardDISP_REFRESH_TIME | 框架默认帧周期（TFT 33ms / 断码屏 500ms） |
| boardDISP_HIGH/LOW_LIGHT_VALUE | 记忆参数默认高/低亮值 |
| boardDISP_OFF_TIME | 默认自动息屏时间（0=常亮） |
| boardUSE_OS | OS/裸机（影响等待原语与移植层实现） |
| boardUPDATE / boardENG_MODE_EN | 升级页/工程页条件编译 |
| LV_USE_ST7789 | LVGL 内置 ST7789 驱动/本地表驱动驱动二选一 |

## 5. 移植到新项目的完整清单

1. **拷贝通用件**：`Middlewares/UniDisplay/` 四文件原样复制，不改一行。
2. **适配移植层** `uni_disp_port.c/h`：
   - FreeRTOS：任务通知唤醒（vTaskNotifyGiveFromTask / ulTaskNotifyTake）；临界区用工程统一 mainENTER_CRITICAL；
   - 裸机：唤醒空宏，临界区按平台关中断。
3. **重建项目件**（按第 1 节骨架，替换 "md_display" 前缀为项目前缀）：
   - [ ] iface：引脚/时钟/SPI/DMA 宏 + 写命令原语（vDisp_WriteCommand/WriteData8/16/WriteBuffer）；
   - [ ] lv_port_disp：LVGL 显示驱动注册、缓冲策略（双缓冲 DMA、FlushDone 信号量）；
   - [ ] api：vDisp_Init（lv_init + tick 回调 + 驱动注册 + ST7789 初始化命令表 + ui_init 防重）、vDisp_LoadScreen、vDisp_UiRefresh（ui_tick → 自绘 ManualTick → lv_timer_handler）；
   - [ ] data：快照结构体字段集 + 位掩码宏 + 三数据钩子 + 状态路由表；
   - [ ] task：DispMemParam_T 记忆参数、任务创建、页面注册、vDisp_TickTimer 挂接、低功耗挂起/恢复；
   - [ ] err：错误码轮显（多故障轮播 + 切换间隙返回 0 配合闪烁）；
   - [ ] page_*：按契约逐页实现。
4. **EEZ Studio**：新建工程（320x240 横屏示例），设计 Screen，配 Native 变量，构建生成 eez_ui，手写 vars.c，把 `.eez-project-build` 清单内文件加入构建。
5. **板级配置**：第 4 节宏表。
6. **系统挂接**：
   - [ ] 初始化序列调 cDisp_TaskInit()（注意在 LVGL 依赖的外设时钟之后）；
   - [ ] 1S 定时器回调挂 vDisp_TickTimer()；
   - [ ] 系统初始化完成置 tSysInfo.uInit.tFinish.bIF_DispTask=1（解除 INIT 页锁定）；
   - [ ] 按键任务投递 DISP_EVT_KEY_SHORT/LONG；
   - [ ] 故障管理投递 FAULT_POPUP/FAULT_CLEAR。
7. **验证**：
   - [ ] 上电 INIT→BOOT→WORK 路由正确；
   - [ ] 亮屏帧率贴近页面声明（GPIO 翻转或 RTOS 运行时统计）；
   - [ ] 息屏后显示任务 CPU 占用归零；
   - [ ] 事件风暴（队列满）不死机，bDisp_PostEvent 返回 false 有日志；
   - [ ] 弹窗进出栈幂等（重复故障事件不卡死）。

## 6. 已知限制与注意

- `vDisp_ReqUiRefresh()` 在 HS800 中为空实现（被 EnginePoll 周期渲染取代），新项目勿依赖它做即时刷新，用 bDisp_PostEvent(DISP_EVT_REQ_PAGE, ...) 或事件驱动。
- `v_disp_fill_color()`/`vDisp_ClearScreen()` 部分路径已注释停用（HS800 全走 LVGL 渲染）；纯命令屏直写需求需自行恢复。
- `b_page_*_event` 返回 true 后事件不再走框架默认——消费按键事件时记得自行重置息屏倒计时（vDisp_ResetAutoOffCountdown）。
- EEZ `loadScreen` 使用 `lv_scr_load_anim(..., LV_SCR_LOAD_ANIM_FADE_IN, 0, 0, false)`，旧屏不删除；EEZ 屏幕对象为全局持久，勿在 user_ui 中删除 EEZ 生成的屏幕根对象。
- 快照 `usPrimaryErrCode` 在临界区外读取（依赖单读者 + 访问函数内部保护），新增字段沿用"多字节临界区内、低频量走访问函数"的分层原则。
