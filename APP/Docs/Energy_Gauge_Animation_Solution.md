# 环形能量格动效方案设计

## 1. 项目环境分析

### 硬件资源限制
- **MCU**: GD32F30x 系列 (Cortex-M4)
- **RAM**: 有限资源 (具体大小需确认，假设 64KB-128KB)
- **显示屏**: 240x320 像素，16位 RGB565
- **LVGL内存池**: 20KB (`LV_MEM_SIZE = 20 * 1024U`)
- **刷新率**: 30fps (33ms 周期)
- **操作系统**: FreeRTOS

### 当前技术栈
- LVGL v9.4.0-dev
- 软件渲染引擎 (无GPU加速)
- EEZ Studio UI 设计工具

---

## 2. 方案设计思路

### 核心优化策略
1. **单一模板 + 旋转复制**: 只绘制一个能量格单元，通过旋转角度复制到各个位置
2. **Canvas局部绘制**: 使用LVGL Canvas对象，避免创建大量独立对象
3. **状态机控制**: 使用简单计数器控制点亮进度
4. **脏矩形优化**: 只刷新变化的区域

---

## 3. 推荐实现方案

### 方案A: 基于Canvas的高效实现 (推荐)

#### 原理
```
┌─────────────────────────────────────┐
│           外圆 (背景环)              │
│    ┌───────────────────────────┐    │
│    │      内圆 (镂空区域)       │    │
│    │                           │    │
│    │    ●  ●  ●  ●  ●  ●  ●   │    │
│    │    ↑                      │    │
│    │    只绘制一个能量格单元    │    │
│    │    通过旋转得到所有格子    │    │
│    └───────────────────────────┘    │
└─────────────────────────────────────┘
```

#### 实现步骤

**步骤1: 定义能量格参数**
```c
#define ENERGY_SEGMENTS       12      // 能量格数量
#define SEGMENT_WIDTH         8       // 单个格子宽度(像素)
#define SEGMENT_HEIGHT        20      // 单个格子高度(像素)
#define GAUGE_RADIUS          80      // 能量环半径
#define GAUGE_CENTER_X        120     // 圆心X坐标
#define GAUGE_CENTER_Y        160     // 圆心Y坐标
#define ANGLE_PER_SEGMENT     (360 / ENERGY_SEGMENTS)  // 每格角度
```

**步骤2: 创建Canvas缓冲区**
```c
// 最小化RAM占用: 只分配一个能量格单元的缓冲区
// RGB565格式: 2字节/像素
#define SEGMENT_BUF_SIZE  (SEGMENT_WIDTH * SEGMENT_HEIGHT * 2)

// 静态分配，避免动态内存碎片
static uint8_t segment_buf[SEGMENT_BUF_SIZE] __attribute__((aligned(4)));
static lv_obj_t *energy_canvas = NULL;
```

**步骤3: 绘制单个能量格模板**
```c
/**
 * @brief 绘制单个能量格单元
 * @param bright: true-亮色(已点亮), false-暗色(未点亮)
 */
static void draw_segment_template(bool bright)
{
    lv_color_t color = bright ? 
        lv_color_make(0, 255, 0) :   // 亮色: 绿色
        lv_color_make(0, 64, 0);      // 暗色: 深绿
    
    // 填充能量格形状 (圆角矩形)
    for(int y = 0; y < SEGMENT_HEIGHT; y++) {
        for(int x = 0; x < SEGMENT_WIDTH; x++) {
            lv_color_t *pixel = (lv_color_t*)(segment_buf + (y * SEGMENT_WIDTH + x) * 2);
            *pixel = color;
        }
    }
}
```

**步骤4: 旋转绘制到Canvas**
```c
/**
 * @brief 将能量格旋转绘制到指定位置
 * @param canvas: LVGL Canvas对象
 * @param segment_index: 能量格索引 (0 ~ ENERGY_SEGMENTS-1)
 * @param bright: 是否点亮
 */
static void draw_segment_rotated(lv_obj_t *canvas, uint8_t segment_index, bool bright)
{
    // 计算旋转角度
    int32_t angle = segment_index * ANGLE_PER_SEGMENT;
    
    // 计算能量格中心位置 (极坐标转直角坐标)
    float rad = angle * 3.14159f / 180.0f;
    int16_t cx = GAUGE_CENTER_X + (int16_t)(GAUGE_RADIUS * cosf(rad));
    int16_t cy = GAUGE_CENTER_Y + (int16_t)(GAUGE_RADIUS * sinf(rad));
    
    // 绘制模板到Canvas (带旋转)
    lv_draw_image_dsc_t draw_dsc;
    lv_draw_image_dsc_init(&draw_dsc);
    
    // 使用LVGL的图片旋转功能
    draw_dsc.src = segment_buf;
    draw_dsc.pivot.x = SEGMENT_WIDTH / 2;
    draw_dsc.pivot.y = SEGMENT_HEIGHT / 2;
    draw_dsc.rotation = angle * 10;  // LVGL角度单位: 0.1度
    
    // 局部刷新，避免全屏重绘
    lv_area_t area;
    area.x1 = cx - SEGMENT_WIDTH;
    area.y1 = cy - SEGMENT_HEIGHT;
    area.x2 = cx + SEGMENT_WIDTH;
    area.y2 = cy + SEGMENT_HEIGHT;
    
    lv_canvas_draw_buf(canvas, &area, &draw_dsc);
}
```

**步骤5: 动画定时器回调**
```c
static uint8_t current_segment = 0;
static bool animation_running = true;
static lv_timer_t *energy_timer = NULL;

/**
 * @brief 能量格动画定时器回调
 * @param timer: LVGL定时器指针
 */
static void energy_gauge_timer_cb(lv_timer_t *timer)
{
    if(!animation_running) return;
    
    if(current_segment < ENERGY_SEGMENTS) {
        // 点亮当前能量格
        draw_segment_rotated(energy_canvas, current_segment, true);
        current_segment++;
        
        // 局部刷新Canvas
        lv_obj_invalidate(energy_canvas);
    } else {
        // 动画完成，可选择停止或循环
        animation_running = false;
        
        // 可选: 触发动画完成回调
        // on_energy_animation_complete();
    }
}

/**
 * @brief 启动能量格动画
 * @param interval_ms: 每格点亮间隔(毫秒)
 */
void start_energy_animation(uint32_t interval_ms)
{
    current_segment = 0;
    animation_running = true;
    
    // 先绘制所有暗色能量格
    for(int i = 0; i < ENERGY_SEGMENTS; i++) {
        draw_segment_rotated(energy_canvas, i, false);
    }
    
    // 创建定时器
    if(energy_timer != NULL) {
        lv_timer_del(energy_timer);
    }
    energy_timer = lv_timer_create(energy_gauge_timer_cb, interval_ms, NULL);
}
```

---

### 方案B: 基于多个lv_obj的简化方案

如果Canvas实现过于复杂，可以使用多个小对象:

```c
#define ENERGY_SEGMENTS  12
static lv_obj_t *segments[ENERGY_SEGMENTS];

/**
 * @brief 创建能量格UI (简化版)
 */
void create_energy_gauge_ui(void)
{
    lv_obj_t *parent = lv_scr_act();
    
    for(int i = 0; i < ENERGY_SEGMENTS; i++) {
        // 创建能量格对象
        segments[i] = lv_obj_create(parent);
        lv_obj_set_size(segments[i], SEGMENT_WIDTH, SEGMENT_HEIGHT);
        
        // 设置样式 (未点亮状态)
        lv_obj_set_style_bg_color(segments[i], lv_color_make(0, 64, 0), 0);
        lv_obj_set_style_radius(segments[i], 3, 0);
        lv_obj_set_style_border_width(segments[i], 0, 0);
        
        // 计算位置 (极坐标)
        float angle = i * (2 * 3.14159f / ENERGY_SEGMENTS);
        int16_t x = GAUGE_CENTER_X + (int16_t)(GAUGE_RADIUS * cosf(angle)) - SEGMENT_WIDTH/2;
        int16_t y = GAUGE_CENTER_Y + (int16_t)(GAUGE_RADIUS * sinf(angle)) - SEGMENT_HEIGHT/2;
        
        lv_obj_set_pos(segments[i], x, y);
        
        // 设置旋转角度
        lv_obj_set_style_transform_rotation(segments[i], i * ANGLE_PER_SEGMENT * 10, 0);
    }
}

/**
 * @brief 点亮指定能量格
 * @param index: 能量格索引
 */
void light_up_segment(uint8_t index)
{
    if(index >= ENERGY_SEGMENTS) return;
    
    // 切换到亮色
    lv_obj_set_style_bg_color(segments[index], lv_color_make(0, 255, 0), 0);
    
    // 可选: 添加发光效果
    lv_obj_set_style_shadow_width(segments[index], 10, 0);
    lv_obj_set_style_shadow_color(segments[index], lv_color_make(0, 255, 0), 0);
}
```

---

## 4. 内存优化技巧

### 4.1 减少Canvas缓冲区大小
```c
// 如果能量格较小，可以使用更小的Canvas
// 例如: 8x20像素 = 320字节，非常节省RAM
```

### 4.2 使用1位色深
```c
// 如果只需两种颜色，可使用LV_COLOR_FORMAT_L1
// 内存占用减少16倍
#define LV_COLOR_FORMAT_L1  1
```

### 4.3 共享缓冲区
```c
// 亮色和暗色模板可以共享同一个缓冲区
// 通过修改像素值切换状态
static void update_segment_brightness(bool bright)
{
    lv_color_t color = bright ? 
        lv_color_make(0, 255, 0) : 
        lv_color_make(0, 64, 0);
    
    // 批量更新像素 (使用memset或DMA)
    uint16_t color565 = lv_color_to_u16(color);
    uint16_t *pixels = (uint16_t*)segment_buf;
    
    for(int i = 0; i < SEGMENT_WIDTH * SEGMENT_HEIGHT; i++) {
        pixels[i] = color565;
    }
}
```

---

## 5. 性能对比

| 方案 | RAM占用 | CPU占用 | 实现复杂度 | 推荐指数 |
|------|---------|---------|------------|----------|
| Canvas方案 | ~400字节 | 低 | 中等 | ????? |
| 多对象方案 | ~2KB | 中等 | 简单 | ???? |
| 全Canvas绘制 | ~150KB | 高 | 复杂 | ?? |

---

## 6. 集成到现有项目

### 6.1 在EEZ Studio中创建Canvas
```c
// 在screens.h中添加
lv_obj_t * create_energy_gauge_screen(void);

// 在screens.c中实现
lv_obj_t * create_energy_gauge_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    
    // 创建Canvas
    energy_canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(energy_canvas, segment_buf, 
                        SEGMENT_WIDTH, SEGMENT_HEIGHT, 
                        LV_COLOR_FORMAT_RGB565);
    
    // 设置Canvas位置和大小
    lv_obj_set_size(energy_canvas, 240, 320);
    lv_obj_center(energy_canvas);
    
    return screen;
}
```

### 6.2 在显示任务中调用
```c
// 在md_display_queue_task_work.c中
void vDisp_EnterEnergyGaugeMode(void)
{
    // 切换到能量格屏幕
    lv_scr_load(create_energy_gauge_screen());
    
    // 启动动画 (每200ms点亮一格)
    start_energy_animation(200);
}

void vDisp_ExitEnergyGaugeMode(void)
{
    // 停止动画
    if(energy_timer != NULL) {
        lv_timer_del(energy_timer);
        energy_timer = NULL;
    }
}
```

---

## 7. 扩展功能

### 7.1 渐变色效果
```c
static void draw_segment_gradient(bool bright, uint8_t progress)
{
    // progress: 0-100, 控制渐变程度
    lv_color_t color_start = bright ? 
        lv_color_make(0, 255, 0) : 
        lv_color_make(0, 64, 0);
    lv_color_t color_end = lv_color_make(0, 32, 0);
    
    // 线性插值计算渐变色
    for(int y = 0; y < SEGMENT_HEIGHT; y++) {
        uint8_t ratio = (y * 100) / SEGMENT_HEIGHT;
        lv_color_t color = lv_color_mix(color_start, color_end, ratio);
        
        for(int x = 0; x < SEGMENT_WIDTH; x++) {
            // 绘制像素...
        }
    }
}
```

### 7.2 脉冲呼吸效果
```c
static void pulse_animation_cb(lv_timer_t *timer)
{
    static uint8_t pulse_phase = 0;
    static bool pulse_dir = true;
    
    // 计算脉冲亮度 (0-255)
    uint8_t brightness = pulse_dir ? 
        (pulse_phase * 4) : 
        (255 - pulse_phase * 4);
    
    // 更新所有已点亮能量格的亮度
    for(int i = 0; i < current_segment; i++) {
        update_segment_brightness_with_alpha(i, brightness);
    }
    
    pulse_phase = (pulse_phase + 1) % 64;
}
```

### 7.3 环形进度条叠加
```c
// 在能量格外层绘制弧形进度条
static void draw_arc_progress(uint8_t progress)
{
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    
    arc_dsc.color = lv_color_make(0, 200, 255);
    arc_dsc.width = 4;
    arc_dsc.start_angle = 270;  // 12点钟方向
    arc_dsc.end_angle = 270 + (progress * 360 / 100);
    
    lv_canvas_draw_arc(energy_canvas, 
                      GAUGE_CENTER_X, GAUGE_CENTER_Y, 
                      GAUGE_RADIUS + 15, 
                      &arc_dsc);
}
```

---

## 8. 调试建议

### 8.1 性能监控
```c
// 在动画回调中添加性能统计
static void energy_gauge_timer_cb(lv_timer_t *timer)
{
    uint32_t start_tick = lv_tick_get();
    
    // ... 绘制逻辑 ...
    
    uint32_t elapsed = lv_tick_get() - start_tick;
    if(elapsed > 10) {  // 超过10ms告警
        LV_LOG_WARN("Energy gauge draw took %d ms", elapsed);
    }
}
```

### 8.2 内存监控
```c
// 定期检查LVGL内存使用
void check_lvgl_memory(void)
{
    lv_mem_monitor_t monitor;
    lv_mem_monitor(&monitor);
    
    LV_LOG_INFO("LVGL Memory: %d/%d bytes used (%d%%)", 
                monitor.total_size - monitor.free_size,
                monitor.total_size,
                (monitor.total_size - monitor.free_size) * 100 / monitor.total_size);
}
```

---

## 9. 总结

### 推荐方案
**方案A (Canvas方案)** 是最优选择，原因：
1. **RAM占用极低**: 仅需~400字节缓冲区
2. **CPU效率高**: 旋转计算简单，无需浮点运算(可用查表法)
3. **灵活性强**: 可轻松扩展渐变、脉冲等效果
4. **代码复用**: 模板绘制一次，多次复用

### 实现优先级
1. **Phase 1**: 基础Canvas方案 (1-2天)
2. **Phase 2**: 添加渐变色效果 (0.5天)
3. **Phase 3**: 添加脉冲呼吸效果 (0.5天)
4. **Phase 4**: 性能优化和调试 (0.5天)

### 预期效果
- 动画流畅度: 30fps (与系统刷新率同步)
- 内存占用: < 1KB
- CPU占用: < 5% (单次绘制 < 2ms)