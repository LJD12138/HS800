# TaskQueue 物理时间戳超时机制设计规范与重构方案

| 属性 | 内容 |
| :--- | :--- |
| **文档版本** | V1.1.1 |
| **生效模块** | `Middlewares/TaskQueue` 及全工程队列任务应用层 |
| **编写日期** | 2026-09-24 |
| **责任作者** | LJD (291483914@qq.com) |
| **核心目标** | 解决任务通知（Task Notification）事件驱动唤醒下循环计数加速失真导致的误超时/误动作缺陷 |

---

## 1. 背景与现状问题诊断

### 1.1 历史背景
在早期版本中，系统各任务采用轮询节拍或固定 `vTaskDelay` 模型运行。每个任务函数的执行周期基本恒定（如 `sysTASK_INIT_CYCLE_TIME = 100ms`）。
历史代码普遍依赖简单的递增计数器来粗略估算流逝时间：
```c
// 步骤等待 5 秒
p_task->usStepWaitCnt++;
if (p_task->usStepWaitCnt > (5000 / sysTASK_INIT_CYCLE_TIME)) { ... }

// 任务超时 10 秒
p_task->usTaskWaitCnt++;
if (p_task->usTaskWaitCnt > (10000 / sysTASK_INIT_CYCLE_TIME)) { ... }
```
上述逻辑有效的前提是：**“每次循环耗时固定为 100ms，执行次数与物理时间成严格线性正比”**。

---

### 1.2 升级任务通知后的致命缺陷

随着 `TaskQueue` 升级为基于 FreeRTOS 任务通知（Task Notification）机制：
```c
#if (boardUSE_OS)
ulTaskNotifyTake(pdTRUE, sysTASK_INIT_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
#endif
```

#### 缺陷 1：异步事件唤醒导致“时钟加速”（超时极速触发）
* 当有新任务投递（`cQueue_AddQueueTask` / `cQueue_AddQueueTaskFromISR`）或其它事件触发 `xTaskNotifyGive(tSysTaskHandler)` 时，`ulTaskNotifyTake` 会**立即返回（实际耗时可能仅 0~1ms）**。
* 任务被唤醒后，主循环 `vSys_Task` 立即重新调度执行 `vQueue_TaskPoll`，导致当前任务函数被**高频重入**。
* **灾难性后果**：
  * **开机误关机**：在系统初始化期间，若外部通信、按键检测或 ADC 密集投递事件，`p_task->usTaskWaitCnt` 会在短短几百毫秒内累加超过 100 次（预期的 10 秒），**直接触发 `gpioASSIST_OPEN_OFF()` 导致整机开机异常断电**。
  * **充电激活提前误触发**：5 秒的防抖滤波时间在频繁唤醒下可能只持续几十毫秒就被触发。
  * **按键模式误触发**：3 秒长按进入工程/工厂模式的判定严重缩水，短按即被判定为长按。

#### 缺陷 2：内部混用延时导致“时钟减速”（超时被大幅拉长）
* 初始化步骤中存在 `vTaskDelay(100)`（参数初始化失败）和 `vTaskDelay(500)`（等待 ADC 就绪）。
* 发生延时时，单次循环实际耗时为 `500ms + 100ms = 600ms`，但 `usTaskWaitCnt` 仍然只自增了 1。
* 计数器与真实物理时间完全脱钩：**快的时候一次循环 1ms，慢的时候一次循环 600ms**。

---

## 2. 总体架构设计原则

为了在事件驱动和异步唤醒架构下实现精准、高可靠的超时控制，方案 2 采取**“中间件统一承载时间戳，业务层零心智负担”**的架构设计：

```
+-------------------------------------------------------------------------+
|                              业务应用层                                  |
|  (sys_queue_task_init.c / bms_queue_task.c / dcac_queue_task.c ...)     |
|                                                                         |
|   bQueue_IsStepTimeoutMs(task, 5000)      bQueue_IsTaskTimeoutMs(task, 10000)
+------------------------------------+------------------------------------+
                                     | 调用 API（无感知物理时钟实现）
                                     v
+------------------------------------+------------------------------------+
|                         TaskQueue 中间件层                               |
|                  (queue_task.h / queue_task.c)                          |
|                                                                         |
|  cQueue_GotoStep()          vQueue_ResetTaskState()                     |
|  -> 自动更新 ulStepStartTick   -> 自动更新 ulTaskStartTick                  |
|                                                                         |
|  [Task_T 结构体扩展]                                                     |
|  - ulTaskStartTick (4字节) : 任务装载物理时间戳                            |
|  - ulStepStartTick (4字节) : 步骤进入物理时间戳                            |
+------------------------------------+------------------------------------+
                                     | 驱动层时基
                                     v
+------------------------------------+------------------------------------+
|                FreeRTOS 内核 / 硬件底层 SysTick                           |
|                      xTaskGetTickCount()                                |
+-------------------------------------------------------------------------+
```

### 关键设计约束
1. **绝对保护上位机调试协议（`TaskDebug_T`）**：
   `print_prot_frame.c` 中通过 `memcpy(&t_frame.tTask, (u8*)tpSysTask, sizeof(TaskDebug_T))` 截取前 10 字节上传。**新扩展的时间戳成员绝对不能插入到前 10 字节中，必须放在其后，确保内存布局 100% 兼容。**
2. **4 字节自然对齐保证**：
   结构体字段需保证 32 位自然对齐，避免 ARM Cortex-M 架构下的非对齐访问与隐蔽填充洞。
3. **防回绕（Wrap-around）时间差计算**：
   利用无符号整数溢出回绕特性，直接使用 `(xTaskGetTickCount() - ulStartTick) >= timeout_ticks`，天然免疫 FreeRTOS 滴答计数器回绕问题。
4. **生命周期自动托管**：
   步骤切换（`cQueue_GotoStep`）与任务装载（`vQueue_ResetTaskState`）自动刷新对应时间戳，业务层无需手动记录或维护静态变量。

---

## 3. 数据结构修改规范

### 3.1 `Task_T` 结构体扩展 (`queue_task.h`)

在 `queue_task.h` 中对 `struct Task_T` 进行扩展，保留原有 10 字节调试字段，并在 `#if (boardUSE_OS)` 下新增时间戳字段：

```c
/* 任务控制块结构体 */
struct Task_T
{
    /* ========= 前 10 字节：严格与上位机 TaskDebug_T 内存镜像保持一致 ========= */
    vu8                 ucID;                   /* 当前任务ID (Offset 0) */
    vu8                 ucStep;                 /* 当前步骤 (Offset 1) */
    vu16                usInParam;              /* 函数参数 (Offset 2~3) */
    vu16                usStepWaitCnt;          /* 步骤等待次数 (保留兼容, Offset 4~5) */
    vu16                usStepRepeatCnt;        /* 步骤重复次数 (Offset 6~7) */
    vu16                usTaskWaitCnt;          /* 任务等待次数 (保留兼容, Offset 8~9) */
    /* ========================================================================= */

    bool                bNowRun;                /* 立刻执行 (Offset 10) */
    u8                  ucPad;                  /* 填充1字节，确保4字节自然对齐 (Offset 11) */
    bpTaskManageFunc    bp_task_manage_func;    /* 任务调度函数 (Offset 12~15) */
    vpFunc              vp_func;                /* 任务函数 (Offset 16~19) */
    vpAddTaskReturnFunc vp_return_func;         /* 事件回调函数 (Offset 20~23) */

    #if (boardUSE_OS)
    TaskHandle_t        tTaskHandler;           /* 绑定的 FreeRTOS 任务句柄 (Offset 24~27) */
    uint32_t            ulTaskStartTick;        /* 任务装载绝对起始时间戳 (Offset 28~31) */
    uint32_t            ulStepStartTick;        /* 步骤进入绝对起始时间戳 (Offset 32~35) */
    #endif  /* boardUSE_OS */

    lwrb_t              tQueueBuff;             /* 任务队列缓存器 */
    lwrb_t              tReplyBuff;             /* 回复缓存器 */
    u8                  uac_buff[];             /* 柔性数组缓冲区 */
};
```

---

## 4. 中间件核心函数重构

### 4.1 自动打标机制

#### (1) 步骤切换时刷新步骤时间戳 (`cQueue_GotoStep`)
在 `queue_task.c` 的 `cQueue_GotoStep` 函数临界区内，加入步骤时间戳自动刷新：

```c
s8 cQueue_GotoStep(Task_T* task, u8 toStep)
{
    s8 c_result = 0;
    // ... 原有逻辑 ...
    
    mainENTER_CRITICAL();
    
    task->usStepWaitCnt = 0;
    task->usStepRepeatCnt = 0;
    
    #if (boardUSE_OS)
    task->ulStepStartTick = xTaskGetTickCount();  /* 自动刷新步骤时间戳 */
    #endif  /* boardUSE_OS */
    
    switch (toStep)
    {
        // ... 原有步骤变更代码 ...
    }
    
    // ...
}
```

#### (2) 任务装载时统一初始化时间戳 (`vQueue_ResetTaskState`)
在 `queue_task.h` 的内联函数 `vQueue_ResetTaskState` 中加入任务与步骤双时间戳刷新：

```c
__STATIC_INLINE void vQueue_ResetTaskState(Task_T *task)
{
    if (task != NULL)
    {
        task->bNowRun         = false;
        task->ucStep          = 0;
        task->usTaskWaitCnt   = 0;
        task->usStepWaitCnt   = 0;
        task->usStepRepeatCnt = 0;
        
        #if (boardUSE_OS)
        uint32_t ul_now = xTaskGetTickCount();
        task->ulTaskStartTick = ul_now;
        task->ulStepStartTick = ul_now;
        #endif  /* boardUSE_OS */
    }
}
```

---

### 4.2 统一超时判断 API 规范 (`queue_task.h`)

在 `queue_task.h` 中提供高性能的内联超时检查函数，天然隔离 OS 与非 OS 平台。

> **替代策略（V1.1.0 定稿）**：新 API **直接替代**旧计数版 API，不保留旧内容：
> - `bQueue_IsStepTimeout()`（计数版步骤超时）——**已删除**，仅有的 2 处调用点（`md_mppt_queue_task_set_chg_pwr.c` / `md_dcac_queue_task_update.c`）已迁移至 `bQueue_IsStepTimeoutMs()`；
> - `vQueue_ResetStepCounters()`（无任何调用方）——**已删除**；
> - `bQueue_IsStepRetryOver()` / `usStepRepeatCnt` 重试计数——**保留**（次数语义而非时间语义，无新 API 替代，BMS 开关重试 / DCAC 升级重发等继续使用）；
> - `usStepWaitCnt` / `usTaskWaitCnt` 字段——**保留**（上位机 `TaskDebug_T` 10 字节镜像兼容），业务层计数用法已全部清除，仅框架内部（`cQueue_GotoStep` / `vQueue_ResetTaskState`）清零维护。

```c
/***********************************************************************************************************************
 * 函数功能    : 步骤级物理时间超时判定
 * 说明(备注)  : 基于真实 Tick 差值判断当前步骤耗时是否超过指定毫秒数；完全免疫异步唤醒与重入
 * 传入参数    : task: 任务控制块指针; timeout_ms: 超时门限（毫秒）
 * 输出参数    : 无
 * 返回值      : true: 已超时; false: 未超时
 ************************************************************************************************************************/
__STATIC_INLINE bool bQueue_IsStepTimeoutMs(const Task_T *task, uint32_t timeout_ms)
{
    if (task == NULL)
        return false;

    #if (boardUSE_OS)
    return ((xTaskGetTickCount() - task->ulStepStartTick) >= pdMS_TO_TICKS(timeout_ms));
    #else
    return false;
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 任务级物理时间超时判定
 * 说明(备注)  : 基于真实 Tick 差值判断从任务装载至今的总耗时是否超过指定毫秒数
 * 传入参数    : task: 任务控制块指针; timeout_ms: 超时门限（毫秒）
 * 输出参数    : 无
 * 返回值      : true: 已超时; false: 未超时
 ************************************************************************************************************************/
__STATIC_INLINE bool bQueue_IsTaskTimeoutMs(const Task_T *task, uint32_t timeout_ms)
{
    if (task == NULL)
        return false;

    #if (boardUSE_OS)
    return ((xTaskGetTickCount() - task->ulTaskStartTick) >= pdMS_TO_TICKS(timeout_ms));
    #else
    return false;
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 手动重置/刷新步骤时间戳
 * 说明(备注)  : 用于步骤内部某些事件（如按键长按重新计时、通信收到握手包重置等待）需要从此刻重新计时
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void vQueue_RefreshStepTick(Task_T *task)
{
    #if (boardUSE_OS)
    if (task != NULL)
        task->ulStepStartTick = xTaskGetTickCount();
    #else
    (void)task;
    #endif  /* boardUSE_OS */
}
```

> **V1.1.0 补充**：实施中发现原稿遗漏"任务级时间戳外部刷新"场景——工程模式 60s 无操作自动关机需要用户操作续命（`vEng_RefreshEngModeTime()`）。为此新增：

```c
/***********************************************************************************************************************
 * 函数功能    : 手动刷新任务起始时间戳
 * 说明(备注)  : 用于任务执行期间事件（用户操作、通信交互等）需要重置任务级总超时的场景（如无操作自动关机续命）
 * 传入参数    : task: 任务控制块指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
__STATIC_INLINE void vQueue_RefreshTaskTick(Task_T *task)
{
    #if (boardUSE_OS)
    if (task != NULL)
        task->ulTaskStartTick = xTaskGetTickCount();
    #else
    (void)task;
    #endif  /* boardUSE_OS */
}
```

> **V1.1.1 决策记录（条件驱动复合 API 方案——不采纳）**：曾评估新增 `bQueue_IsCondHeldMs`（条件断开自动重置计时）/ `bQueue_IsStepIdleMs`（活动自动续命）复合 API，以规避业务层忘记调用 `vQueue_RefreshStepTick` 的风险。评估结论：**隐式副作用（Is 前缀查询函数内部刷新时间戳）与"条件变化期间每轮必须调用"的隐式约束，可能引入比"忘记重置"更隐蔽的时序 bug（漏调导致时间戳陈旧、条件恢复后立即误判超时），收益不抵风险，不采纳**。业务层保持显式 `vQueue_RefreshStepTick` / `vQueue_RefreshTaskTick` 刷新。

---

## 5. 业务应用层改造示范 (`sys_queue_task_init.c`)

以出现缺陷的 `sys_queue_task_init.c` 为例，改造前后的代码对比：

### 5.1 步骤 3 充电激活等待判定改造
```diff
  #if (boardADC_EN)
  else if ((tAdcSamp.usSysInVolt > (tAppMemParam.tSYS.usMinOpenVolt / 2))
      #if (boardBMS_EN)
               && (tBms.eDevState == DS_LOST)
      #endif  //boardBMS_EN
  )
  {
      //充电激活
-     p_task->usStepWaitCnt++;
-     if (p_task->usStepWaitCnt > (5000 / sysTASK_INIT_CYCLE_TIME))
+     if (bQueue_IsStepTimeoutMs(p_task, 5000))
      {
          bSys_ChgWakeUp(SO_MPPT);
          cQueue_GotoStep(p_task, STEP_NEXT);
      }
      else
          break;
  }
  #endif  //boardADC_EN
  else
  {
-     p_task->usStepWaitCnt = 0;
+     vQueue_RefreshStepTick(p_task);  // 电压不满足时重置计时起点
      break;
  }
```

### 5.2 步骤 4 工程模式 / 工厂模式按键长按 3 秒判定改造
```diff
  if (s_uc_tri_type != 2)
  {
      s_uc_tri_type = 2;
-     p_task->usStepWaitCnt = 0;
+     vQueue_RefreshStepTick(p_task);  // 触发类型变化，重置长按起始时间
  }
- p_task->usStepWaitCnt++;
  // ...
- if (p_task->usStepWaitCnt > (3000 / sysTASK_INIT_CYCLE_TIME))
+ if (bQueue_IsStepTimeoutMs(p_task, 3000))
  {
      if (s_uc_tri_type == 2)  // 工厂模式
      {
          // ...
      }
  }
```

### 5.3 尾部 10 秒总开机超时与退出改造
```diff
+ // 步骤已全部完成，无需进行超时检查与多余的100ms等待，直接返回
+ if (p_task->ucStep >= STEP_END)
+     return;

  //初始化等待10S,超时强制关机退出
- p_task->usTaskWaitCnt++;
- if (p_task->usTaskWaitCnt > (10000 / sysTASK_INIT_CYCLE_TIME))
+ if (bQueue_IsTaskTimeoutMs(p_task, 10000))
  {
      gpioASSIST_OPEN_OFF();

      #if (boardBMS_EN)
      cBms_Switch(SO_KEY, ST_OFF, false);
      #endif  //boardBMS_EN
  }

  #if (boardUSE_OS)
  ulTaskNotifyTake(pdTRUE, sysTASK_INIT_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
  #endif  //boardUSE_OS
```

---

## 6. 兼容性与性能影响评估

1. **上位机通信协议兼容性（100% 兼容）**：
   由于 `Task_T` 的前 10 字节（`ucID` 到 `usTaskWaitCnt`）严格保持原状，`TaskDebug_T` 的 `memcpy` 二进制镜像完全一致，上位机工具无需做任何适配。
2. **RAM 内存开销（极微小）**：
   每个 `Task_T` 控制块增加 2 个 `uint32_t`（8 字节）。全工程核心队列任务总计约 8 个（Sys, BMS, DCAC, MPPT, USB, Print, DC, HeatManage），整机额外 RAM 开销仅 **64 字节**，对于 STM32/高规格 MCU 而言可忽略不计。
3. **CPU 性能消耗（显著降低）**：
   * 原有每次循环都需要执行整数除法门限或自增比对；
   * 新方案为简单的单周期整数减法比对（`TickNow - StartTick >= Limit`），效率极高。

---

## 7. 实施路线 (Rollout Checklist) — 已全量完成

- [x] **第一步**：修改 `APP/Middlewares/TaskQueue/queue_task.h`，扩展 `Task_T` 结构体（`ulTaskStartTick`/`ulStepStartTick` 落位 Offset 28~35），删除 `bQueue_IsStepTimeout` / `vQueue_ResetStepCounters`，新增 `bQueue_IsStepTimeoutMs` / `bQueue_IsTaskTimeoutMs` / `vQueue_RefreshStepTick` / `vQueue_RefreshTaskTick` 四个内联 API。
- [x] **第二步**：修改 `APP/Middlewares/TaskQueue/queue_task.c`，`cQueue_GotoStep` 临界区内自动刷新 `ulStepStartTick`；`vQueue_ResetTaskState` 刷新双时间戳。
- [x] **第三步**：改造 `APP/Application/Sys/sys_queue_task_init.c`，消除 10s 误关机与 5s 激活时钟漂移隐患。
- [x] **第四步（升级为全量迁移）**：APP 工程全部队列任务文件一次迁移完成：
  * **Sys 系列 9 个**：init / booting / closing / err / eng / reset / shut_down / update / update_err（含长短按判定重构为时间戳 + 静态按下标志）；
  * **Usb/Dc/Print 系列 7 个**：usb init/closing/booting、dc booting、print reply_run_log/reply_cali/reply_app_info；
  * **MPPT 3 个 + DCAC 6 个 + BMS 7 个**：尾部任务级超时全部迁移；DCAC 升级流程 6 处计数清零改为 `vQueue_RefreshStepTick()`（重发后重新计超时窗）；BMS run_log 超时门限改为 ms 变量（5s/10s/30s 分档）；`sysTASK_UPDATE_ERR_*` 宏量纲统一为 ms。
- [ ] **第五步（暂缓，BOOT 暂不同步）**：`BOOT\Middlewares\TaskQueue` 为独立同源副本且已升级任务通知架构，同类计数失真缺陷同样存在；本次决策暂不同步，两份中间件暂时分叉，后续单独排期处理。

### 7.1 迁移后语义映射速查

| 旧写法（计数） | 新写法（物理时间） |
| :--- | :--- |
| `p->usStepWaitCnt++; if (cnt >= N/cycle)` | `bQueue_IsStepTimeoutMs(p_task, N_ms)` |
| `p->usTaskWaitCnt++; if (cnt > N/cycle)` | `bQueue_IsTaskTimeoutMs(p_task, N_ms)` |
| `p->usStepWaitCnt = 0`（条件不满足重新计时） | `vQueue_RefreshStepTick(p_task)` |
| `p->usTaskWaitCnt = 0`（外部续命，如工程模式操作刷新） | `vQueue_RefreshTaskTick(p_task)` |
| `p->usStepRepeatCnt++`（重试计数） | **保持不变**（次数语义） |
