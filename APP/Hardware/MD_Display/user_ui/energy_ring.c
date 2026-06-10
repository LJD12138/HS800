/*****************************************************************************************************************
 *                                                                                                                *
 *                                              能量环UI组件 - 实现文件                                          *
 *                                                                                                                *
 ******************************************************************************************************************/

/**
 * @file energy_ring.c
 * @brief Energy Ring UI Component - Implementation
 * @author User
 * @version 1.0
 * 
 * @details 能量环组件详细设计说明：
 * 
 * 一、设计目标
 * -------------
 * 1. 工业级性能：低内存占用、低CPU消耗、高渲染效率
 * 2. 极简架构：单对象设计，易于维护和调试
 * 3. 纯整数运算：避免浮点运算开销，适合嵌入式环境
 * 
 * 二、核心设计原理
 * -----------------
 * 1. 单对象架构：整个能量环由一个lv_obj对象承载
 * 2. 主绘制回调：在LV_EVENT_DRAW_MAIN事件中绘制所有segment
 * 3. 定时器驱动：lv_timer周期性递增active_seg并触发重绘
 * 4. 角度系统：0度位于3点钟方向，顺时针增加
 * 
 * 三、渲染优化策略
 * -----------------
 * 1. 单个 obj + LV_EVENT_DRAW_MAIN：所有 segment 在一次主绘制阶段完成
 * 2. 单个 lv_timer：每次仅做 active_seg++ 与 invalidate
 * 3. 不使用 Canvas/图片/旋转变换：避免大块 RAM 占用
 * 4. 不做浮点运算：角度全部使用整数
 * 5. 关闭 rounded：segment 两端保持直角，同时仍由 LVGL 软件渲染保持抗锯齿边缘
 * 6. 颜色策略仅区分 off/on/head 三档：视觉层次明确，但不会引入复杂 alpha blending
 * 
 * 四、颜色状态机
 * ---------------
 * - tSegOffColor  ：未点亮segment颜色（深色）
 * - tSegOnColor   ：已点亮segment颜色（主色）
 * - tSegHeadColor ：头部segment高亮颜色（亮色）
 * 
 * 五、扩展功能建议
 * -----------------
 * - 呼吸灯：在 timer 中周期调整 tSegOnColor/tSegHeadColor 的亮度，再 invalidate
 * - 扫光：将"已点亮判定"改为仅高亮一个移动窗口，而非 0..active_seg 全亮
 * - 拖尾：按 segment 与头部距离返回多级亮度，而不是单一 on/head 两级
 * - 渐变亮度：预先准备 4~8 档离散颜色表，按距离取色，避免运行时复杂混色
 */

#include "MD_Display/user_ui/energy_ring.h"

#if(boardDISPLAY_EN)
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/md_display_iface.h"  /* 包含dispTFT_WIDTH和dispTFT_HEIGHT的定义 */


//****************************************************局部宏定义初始化*********************************************//

/**
 * Energy Ring 关键参数配置说明：
 * 
 * 角度系统说明：
 * - LVGL角度定义：0度位于3点钟方向，顺时针增加
 * - 90度 = 6点钟方向
 * - 180度 = 9点钟方向  
 * - 270度 = 12点钟方向
 * 
 * 参数说明：
 * - SEG_COUNT      : 段数，段数越多越细腻，但每帧 draw_arc 次数也会增加
 * - START_ANGLE    : 起始角度（120度，位于8点钟方向附近）
 * - SWEEP_ANGLE    : 总扫描角度（300度，从120度到420度，形成一个弧形）
 * - GAP_ANGLE      : 相邻 segment 固定角度间隙（2度）
 * - RADIUS/WIDTH   : 外半径（88像素）与环宽（12像素），决定视觉粗细
 * - TIMER_MS       : 每次 active_seg++ 的周期（85毫秒）
 * - CENTER_Y_OFFSET: 中心Y轴偏移（-8像素），用于垂直位置微调
 */
#define ENERGY_RING_SEG_COUNT             20U     /* segment数量：20段 */
#define ENERGY_RING_START_ANGLE           120U    /* 起始角度：120度（8点钟方向附近） */
#define ENERGY_RING_SWEEP_ANGLE           300U    /* 扫描角度：300度 */
#define ENERGY_RING_GAP_ANGLE             1U      /* 段间隔：1度 */
#define ENERGY_RING_RADIUS                76      /* 外半径：80像素 */
#define ENERGY_RING_WIDTH                 9       /* 环宽：9像素 */
#define ENERGY_RING_TIMER_MS              400U     /* 动画周期：400毫秒 */
#define ENERGY_RING_CENTER_Y_OFFSET       (-18)   /* 中心Y偏移：-18像素（向上移动10像素） */

/* 内环装饰环默认参数 */
#define INNER_RING_SEG_COUNT             60U     /* 内环矩形数量：60个 */
#define INNER_RING_START_ANGLE           3U      /* 内环起始角度：3度，避开正水平轴 */
#define INNER_RING_RADIUS                62      /* 内环外沿半径：68像素 */
#define INNER_RING_SEG_LEN               6       /* 内环矩形长度：6像素 */
#define INNER_RING_SEG_WIDTH             2       /* 内环矩形宽度：2像素 */
#define INNER_RING_OUTER_GAP             6       /* 与外环保留的径向间距：6像素 */
#define INNER_RING_COLOR                 lv_color_hex(0x2992CE) /* 内环主色 */

//****************************************************局部函数定义************************************************//
static void energy_ring_draw_event(lv_event_t *tp_event);
static void energy_ring_delete_event(lv_event_t *tp_event);
static lv_color_t energy_ring_get_seg_color(const EnergyRing_T *tp_ring, u16 us_seg_index);
static void energy_ring_recalc(EnergyRing_T *p_ring);
static void energy_ring_reset_runtime(EnergyRing_T *p_ring);
static lv_obj_t *energy_ring_get_parent_screen(void);
static u16 energy_ring_normalize_angle(u16 us_angle);
static bool energy_ring_angle_in_span(u16 us_start_angle, u16 us_end_angle, u16 us_test_angle);
static lv_coord_t energy_ring_q15_mul_round(s32 sl_value, s32 sl_q15, u8 uc_extra_shift);
static lv_coord_t energy_ring_get_outer_extent(const EnergyRing_T *tp_ring);
static void energy_ring_get_object_area(lv_obj_t *tp_parent, const EnergyRing_T *tp_ring, lv_area_t *tp_area);
static void energy_ring_extend_area_with_point(lv_area_t *tp_area, lv_coord_t t_x, lv_coord_t t_y);
static bool energy_ring_get_seg_area(const EnergyRing_T *tp_ring, u16 us_seg_index, lv_area_t *tp_area);
static void energy_ring_invalidate_seg_range(EnergyRing_T *tp_ring, u16 us_old_active_seg, u16 us_new_active_seg);
static void energy_ring_draw_inner_ring(const EnergyRing_T *tp_ring, lv_layer_t *tp_layer, lv_point_t t_center,
                                        lv_coord_t t_outer_ring_radius);
static void energy_ring_timer_cb(lv_timer_t *tp_timer);
static bool energy_ring_create(EnergyRing_T *p_ring);
static u16 energy_ring_soc_to_seg_count(const EnergyRing_T *tp_ring, u8 uc_soc);

/***********************************************************************************************************************
-----函数功能    初始化能量环配置参数
-----说明(备注)  为能量环结构体设置默认配置参数，包括段数、角度、颜色等，并计算运行时参数
-----传入参数    p_ring:能量环结构体指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_init_cfg(EnergyRing_T *p_ring)
{
    if(p_ring == NULL)
        return;

    if(p_ring->usSegCount != 0U)
        return;

    p_ring->usSegCount = ENERGY_RING_SEG_COUNT;
    p_ring->usStartAngle = ENERGY_RING_START_ANGLE;
    p_ring->usSweepAngle = ENERGY_RING_SWEEP_ANGLE;
    p_ring->usGapAngle = ENERGY_RING_GAP_ANGLE;
    p_ring->usAnimPeriodMs = ENERGY_RING_TIMER_MS;
    p_ring->tRadius = ENERGY_RING_RADIUS;
    p_ring->tWidth = ENERGY_RING_WIDTH;
    p_ring->tCenterYOffset = ENERGY_RING_CENTER_Y_OFFSET;
    p_ring->tBgColor = lv_color_hex(0x050B12);
    p_ring->tSegOffColor = lv_color_hex(0x14304E);
    p_ring->tSegOnColor = lv_color_hex(0x298EC5);
    p_ring->tSegHeadColor = lv_color_hex(0x29A2E6);
    p_ring->bEnabled = false;

    /* 内环装饰环参数初始化 */
    p_ring->usInnerSegCount = INNER_RING_SEG_COUNT;
    p_ring->tInnerRadius = INNER_RING_RADIUS;
    p_ring->tInnerSegLen = INNER_RING_SEG_LEN;
    p_ring->tInnerSegWidth = INNER_RING_SEG_WIDTH;
    p_ring->tInnerSegColor = INNER_RING_COLOR;
    p_ring->bInnerRingEnabled = true;

    energy_ring_recalc(p_ring);
    energy_ring_reset_runtime(p_ring);
}

/***********************************************************************************************************************
-----函数功能    Q15定点乘法并四舍五入
-----说明(备注)  用于半径与三角函数值相乘，extra_shift可支持半像素对称偏移
-----传入参数    sl_value:整数值 sl_q15:Q15三角函数值 uc_extra_shift:额外右移位数
-----输出参数    none
-----返回值      四舍五入后的像素偏移值
************************************************************************************************************************/
static lv_coord_t energy_ring_q15_mul_round(s32 sl_value, s32 sl_q15, u8 uc_extra_shift)
{
    s32 sl_product;
    s32 sl_round;
    u8 uc_total_shift;

    uc_total_shift = (u8)(LV_TRIGO_SHIFT + uc_extra_shift);
    sl_product = sl_value * sl_q15;
    sl_round = (s32)(1L << (uc_total_shift - 1U));

    if(sl_product >= 0)
        sl_product += sl_round;
    else
        sl_product -= sl_round;

    return (lv_coord_t)(sl_product >> uc_total_shift);
}

/***********************************************************************************************************************
-----函数功能    绘制内环装饰环（优化抗锯齿版）
-----说明(备注)  将每个径向短条视为“带宽度的极短圆弧段”进行绘制。
-----            借用 LVGL 的圆弧抗锯齿算法彻底消除多线拼接导致的毛边与错位。
************************************************************************************************************************/
static void energy_ring_draw_inner_ring(const EnergyRing_T *tp_ring, lv_layer_t *tp_layer, lv_point_t t_center,
                                        lv_coord_t t_outer_ring_radius)
{
    u16 us_seg_index;                   /* 内环矩形索引 */
    u16 us_angle;                       /* 当前矩形中心角度 */
    lv_coord_t t_inner_outer_radius;   /* 内环外沿半径 */
    lv_coord_t t_max_outer_radius;     /* 内环允许的最大外沿半径 */
    u16 us_angle_step;                  /* 角度步进 */
    u16 us_half_width_angle;            /* 由条纹宽度折算出的半边角度 */
    lv_draw_arc_dsc_t t_arc_dsc;        /* 弧形绘制描述符 */

    /* 参数有效性检查 */
    if((tp_ring == NULL) || (tp_layer == NULL))
        return;

    /* 内环未使能时直接返回 */
    if(tp_ring->bInnerRingEnabled == false)
        return;

    /* 内环矩形数量为0时直接返回 */
    if(tp_ring->usInnerSegCount == 0U)
        return;

    /* 矩形长度或宽度非法时直接返回 */
    if((tp_ring->tInnerSegLen <= 0) || (tp_ring->tInnerSegWidth <= 0))
        return;

    /* 计算最大可用外半径 */
    t_max_outer_radius = (lv_coord_t)(t_outer_ring_radius - (tp_ring->tWidth / 2) - INNER_RING_OUTER_GAP); //
    t_inner_outer_radius = tp_ring->tInnerRadius; //
    if(t_inner_outer_radius > t_max_outer_radius)
        t_inner_outer_radius = t_max_outer_radius; //

    if(t_inner_outer_radius <= tp_ring->tInnerSegLen)
        return; //

    /* 计算角度步进 = 360 / 矩形数量 */
    us_angle_step = (u16)(360U / tp_ring->usInnerSegCount); //
    if(us_angle_step == 0U)
        return; //

    /* * 关键优化点：将条纹的物理像素宽度（tInnerSegWidth）粗略映射为圆弧的扫描角度。
     * 在当前 60~70 半径下，1度弧长约为 1 像素。
     * 如果想更精准，可以使用公式：角度 = Width * 180 / (PI * Radius)
     * 这里直接提供一个适合小尺寸屏幕的稳定映射：
     */
    us_half_width_angle = (u16)(tp_ring->tInnerSegWidth / 2);
    if(us_half_width_angle == 0U)
        us_half_width_angle = 1U; /* 确保至少有 1 度的宽度，防止变透明消失 */

    /* 初始化弧形绘制描述符，配置一次即可 */
    lv_draw_arc_dsc_init(&t_arc_dsc);
    t_arc_dsc.base.layer = tp_layer;
    t_arc_dsc.center = t_center;
    /* 以短条的中点作为圆弧绘制半径，宽度即为短条的长度 */
    t_arc_dsc.radius = (uint16_t)(t_inner_outer_radius - (tp_ring->tInnerSegLen / 2));
    t_arc_dsc.width = tp_ring->tInnerSegLen;
    t_arc_dsc.color = tp_ring->tInnerSegColor;
    t_arc_dsc.rounded = 0U;           /* 保持和外环一样的直角端面 */
    t_arc_dsc.opa = LV_OPA_COVER;

    /* 遍历所有内环小条 */
    for(us_seg_index = 0U; us_seg_index < tp_ring->usInnerSegCount; us_seg_index++)
    {
        /* 计算当前条纹的中心角度 */
        us_angle = (u16)(INNER_RING_START_ANGLE + (us_seg_index * us_angle_step)); //
        if(us_angle >= 360U)
            us_angle = (u16)(us_angle - 360U); //

        /* * 通过中心角度向两侧展开 us_half_width_angle 
         * 使得每个短条转变成一段超短的、朝向圆心的圆弧面
         */
        t_arc_dsc.start_angle = (u16)(us_angle >= us_half_width_angle ? (us_angle - us_half_width_angle) : (360U + us_angle - us_half_width_angle));
        t_arc_dsc.end_angle = (u16)(us_angle + us_half_width_angle);

        /* 调用 LVGL 的高效带抗锯齿圆弧绘制 */
        lv_draw_arc(tp_layer, &t_arc_dsc);
    }
}

/***********************************************************************************************************************
-----函数功能    重新计算能量环渲染参数
-----说明(备注)  根据段数和间隙角度计算每段的跨度角度和步进角度，确保环形均匀分布
-----传入参数    p_ring:能量环结构体指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_recalc(EnergyRing_T *p_ring)
{
    u32 gap_total;          /* 所有间隔的总角度 */
    u32 active_span_total;  /* segment和间隔的总跨度 */
    u32 remain_angle;       /* 剩余角度(用于居中对齐) */

    if(p_ring == NULL)
        return;

    /* 防止segment数为0导致除零错误 */
    if(p_ring->usSegCount == 0U)
        p_ring->usSegCount = 1U;

    /* 计算间隔总角度 = (segment数-1) × 间隔角度 */
    gap_total = 0U;
    if(p_ring->usSegCount > 1U)
        gap_total = (u32)(p_ring->usSegCount - 1U) * p_ring->usGapAngle;

    /* 如果间隔总角度超过扫描角度，需要缩减间隔 */
    if(gap_total >= p_ring->usSweepAngle)
    {
        p_ring->usGapAngle = 1U;
        gap_total = (p_ring->usSegCount > 1U) ? (u32)(p_ring->usSegCount - 1U) : 0U;
    }

    /* 计算单个segment跨度角度 = (扫描角度-间隔总角度) / segment数 */
    p_ring->usSegSpanAngle = (u16)((p_ring->usSweepAngle - gap_total) / p_ring->usSegCount);
    if(p_ring->usSegSpanAngle == 0U)
        p_ring->usSegSpanAngle = 1U;

    /* 计算segment步进角度 = 跨度 + 间隔 */
    p_ring->usSegStepAngle = p_ring->usSegSpanAngle + p_ring->usGapAngle;

    /* 计算实际占用的总角度，用于居中对齐 */
    active_span_total = (u32)p_ring->usSegSpanAngle * p_ring->usSegCount + gap_total;
    remain_angle = (p_ring->usSweepAngle > active_span_total) ? (p_ring->usSweepAngle - active_span_total) : 0U;
    
    /* 设置实际渲染起始角度(带居中偏移) */
    p_ring->usRenderStartAngle = p_ring->usStartAngle + (u16)(remain_angle / 2U);
}

/***********************************************************************************************************************
-----函数功能    重置能量环运行状态
-----说明(备注)  将当前活动段索引重置为0，用于重新开始动画
-----传入参数    p_ring:能量环结构体指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_reset_runtime(EnergyRing_T *p_ring)
{
    if(p_ring == NULL)
        return;

    p_ring->usActiveSeg = 0U;
    p_ring->bAnimDirDec = false;
}

/***********************************************************************************************************************
-----函数功能    角度归一化
-----说明(备注)  将输入角度限制在 [0, 360) 范围内，通过循环减去360度消除多圈旋转
                  用于后续角度比较和区间判断，确保角度值始终处于有效范围
-----传入参数    us_angle:待归一化的角度值（可大于360）
-----输出参数    none
-----返回值      归一化后的角度值（0~359）
************************************************************************************************************************/
static u16 energy_ring_normalize_angle(u16 us_angle)
{
    while(us_angle >= 360U)
        us_angle = (u16)(us_angle - 360U);

    return us_angle;
}

/***********************************************************************************************************************
-----函数功能    判断角度是否在指定角度区间内
-----说明(备注)  判断 us_test_angle 是否位于 [us_start_angle, us_end_angle] 区间内
                  自动处理跨0度（360度回绕）的情况，例如区间[350, 10]会正确包含355度和5度
-----传入参数    us_start_angle:区间起始角度  us_end_angle:区间结束角度  us_test_angle:待测试角度
-----输出参数    none
-----返回值      true:角度在区间内  false:角度不在区间内
************************************************************************************************************************/
static bool energy_ring_angle_in_span(u16 us_start_angle, u16 us_end_angle, u16 us_test_angle)
{
    us_start_angle = energy_ring_normalize_angle(us_start_angle);
    us_end_angle = energy_ring_normalize_angle(us_end_angle);
    us_test_angle = energy_ring_normalize_angle(us_test_angle);

    if(us_start_angle <= us_end_angle)
        return (us_test_angle >= us_start_angle) && (us_test_angle <= us_end_angle);

    return (us_test_angle >= us_start_angle) || (us_test_angle <= us_end_angle);
}

/***********************************************************************************************************************
-----函数功能    计算能量环外侧包围半径
-----说明(备注)  根据能量环的外半径和环宽计算视觉包围范围（含2像素抗锯齿余量），用于确定对象区域大小
-----传入参数    tp_ring:能量环结构体指针
-----输出参数    none
-----返回值      能量环外侧包围半径（像素），结构体为NULL时返回0
************************************************************************************************************************/
static lv_coord_t energy_ring_get_outer_extent(const EnergyRing_T *tp_ring)
{
    if(tp_ring == NULL)
        return 0;

    return (lv_coord_t)(tp_ring->tRadius + tp_ring->tWidth + 2);
}

/***********************************************************************************************************************
-----函数功能    获取能量环挂载的父屏幕对象
-----说明(备注)  优先返回EEZ主屏对象(objects.main)，若不可用则回退到LVGL当前活动屏幕
                  确保能量环始终挂载到正确的屏幕，避免屏幕切换窗口期创建到错误screen
-----传入参数    none
-----输出参数    none
-----返回值      父屏幕lv_obj_t指针
************************************************************************************************************************/
static lv_obj_t *energy_ring_get_parent_screen(void)
{
    if(objects.main_work != NULL)
        return objects.main_work;

    return lv_screen_active();
}

/***********************************************************************************************************************
-----函数功能    计算能量环对象的包围矩形区域
-----说明(备注)  根据父屏幕尺寸和能量环参数（外半径、环宽、Y偏移）计算能量环在屏幕上的包围盒
                  确定lv_obj对象的精确位置和大小，使能量环居中显示且最小化对象面积
-----传入参数    tp_parent:父屏幕对象指针  tp_ring:能量环结构体指针
-----输出参数    tp_area:输出的包围矩形区域
-----返回值      none
************************************************************************************************************************/
static void energy_ring_get_object_area(lv_obj_t *tp_parent, const EnergyRing_T *tp_ring, lv_area_t *tp_area)
{
    lv_area_t t_parent_coords;
    lv_coord_t t_extent;
    lv_coord_t t_center_x;
    lv_coord_t t_center_y;
    lv_coord_t t_parent_width;
    lv_coord_t t_parent_height;

    if((tp_ring == NULL) || (tp_area == NULL))
        return;

    t_extent = energy_ring_get_outer_extent(tp_ring);
    if(t_extent <= 0)
    {
        lv_area_set(tp_area, 0, 0, 0, 0);
        return;
    }

    if(tp_parent != NULL)
    {
        lv_obj_get_content_coords(tp_parent, &t_parent_coords);
        t_parent_width = (lv_coord_t)((t_parent_coords.x2 - t_parent_coords.x1) + 1);
        t_parent_height = (lv_coord_t)((t_parent_coords.y2 - t_parent_coords.y1) + 1);
        t_center_x = (lv_coord_t)(t_parent_width / 2);
        t_center_y = (lv_coord_t)((t_parent_height / 2) + tp_ring->tCenterYOffset);
    }
    else
    {
        t_center_x = (lv_coord_t)(dispTFT_WIDTH / 2);
        t_center_y = (lv_coord_t)((dispTFT_HEIGHT / 2) + tp_ring->tCenterYOffset);
    }

    lv_area_set(tp_area,
                (int32_t)(t_center_x - t_extent),
                (int32_t)(t_center_y - t_extent),
                (int32_t)(t_center_x + t_extent - 1),
                (int32_t)(t_center_y + t_extent - 1));
}

/***********************************************************************************************************************
-----函数功能    将点纳入包围区域
-----说明(备注)  将给定坐标点纳入矩形包围区域，自动扩展区域边界以包含该点
                  用于逐步计算segment的精确包围盒
-----传入参数    tp_area:当前包围区域指针  t_x:点的X坐标  t_y:点的Y坐标
-----输出参数    tp_area:扩展后的包围区域
-----返回值      none
************************************************************************************************************************/
static void energy_ring_extend_area_with_point(lv_area_t *tp_area, lv_coord_t t_x, lv_coord_t t_y)
{
    if(tp_area == NULL)
        return;

    if(t_x < tp_area->x1)
        tp_area->x1 = t_x;
    if(t_x > tp_area->x2)
        tp_area->x2 = t_x;
    if(t_y < tp_area->y1)
        tp_area->y1 = t_y;
    if(t_y > tp_area->y2)
        tp_area->y2 = t_y;
}

/***********************************************************************************************************************
-----函数功能    计算单个segment的包围矩形区域
-----说明(备注)  通过三角函数计算指定segment在圆弧上的精确包围盒，采样起始/中间/结束角度及
                  跨越的基点角度（0/90/180/270度），对内外半径各点取并集后扩展2像素余量
                  用于局部脏区域失效（invalidate），避免整环刷新
-----传入参数    tp_ring:能量环结构体指针  us_seg_index:segment索引（0起始）
-----输出参数    tp_area:输出的segment包围矩形
-----返回值      true:计算成功  false:参数无效或计算失败
************************************************************************************************************************/
static bool energy_ring_get_seg_area(const EnergyRing_T *tp_ring, u16 us_seg_index, lv_area_t *tp_area)
{
    static const u16 s_us_cardinal_angles[] = {0U, 90U, 180U, 270U};
    lv_area_t t_coords;
    lv_point_t t_center;
    lv_coord_t t_radius;
    lv_coord_t t_radius_x_max;
    lv_coord_t t_radius_y_max;
    lv_coord_t t_outer_radius;
    lv_coord_t t_inner_radius;
    lv_coord_t t_x;
    lv_coord_t t_y;
    s32 sl_cos;
    s32 sl_sin;
    u16 us_start_angle;
    u16 us_end_angle;
    u16 us_mid_angle;
    u16 us_angle_index;
    u16 us_angles[7];
    u8 uc_angle_count;
    u8 uc_index;

    if((tp_ring == NULL) || (tp_ring->pObj == NULL) || (tp_area == NULL))
        return false;

    if((tp_ring->usSegSpanAngle == 0U) || (us_seg_index >= tp_ring->usSegCount))
        return false;

    lv_obj_get_content_coords(tp_ring->pObj, &t_coords);

    t_center.x = (lv_coord_t)((t_coords.x1 + t_coords.x2) / 2);
    t_center.y = (lv_coord_t)((t_coords.y1 + t_coords.y2) / 2);

    t_radius_x_max = (lv_coord_t)(((t_coords.x2 - t_coords.x1) + 1) / 2 - tp_ring->tWidth);
    t_radius_y_max = (lv_coord_t)(((t_coords.y2 - t_coords.y1) + 1) / 2 - tp_ring->tWidth);
    t_radius = tp_ring->tRadius;

    if(t_radius > t_radius_x_max)
        t_radius = t_radius_x_max;
    if(t_radius > t_radius_y_max)
        t_radius = t_radius_y_max;

    if(t_radius <= tp_ring->tWidth)
        return false;

    t_outer_radius = (lv_coord_t)(t_radius + (tp_ring->tWidth / 2) + 2);
    t_inner_radius = (lv_coord_t)(t_radius - ((tp_ring->tWidth + 1) / 2) - 2);
    if(t_inner_radius < 0)
        t_inner_radius = 0;

    us_start_angle = (u16)(tp_ring->usRenderStartAngle + (u16)(us_seg_index * tp_ring->usSegStepAngle));
    us_end_angle = (u16)(us_start_angle + tp_ring->usSegSpanAngle);
    us_mid_angle = (u16)(us_start_angle + (tp_ring->usSegSpanAngle / 2U));

    uc_angle_count = 0U;
    us_angles[uc_angle_count++] = energy_ring_normalize_angle(us_start_angle);
    us_angles[uc_angle_count++] = energy_ring_normalize_angle(us_mid_angle);
    us_angles[uc_angle_count++] = energy_ring_normalize_angle(us_end_angle);

    for(us_angle_index = 0U; us_angle_index < (u16)(sizeof(s_us_cardinal_angles) / sizeof(s_us_cardinal_angles[0])); us_angle_index++)
    {
        if(energy_ring_angle_in_span(us_start_angle, us_end_angle, s_us_cardinal_angles[us_angle_index]))
            us_angles[uc_angle_count++] = s_us_cardinal_angles[us_angle_index];
    }

    sl_cos = lv_trigo_cos((int16_t)us_angles[0]);
    sl_sin = lv_trigo_sin((int16_t)us_angles[0]);
    t_x = (lv_coord_t)(t_center.x + energy_ring_q15_mul_round(t_outer_radius, sl_cos, 0U));
    t_y = (lv_coord_t)(t_center.y + energy_ring_q15_mul_round(t_outer_radius, sl_sin, 0U));
    lv_area_set(tp_area, t_x, t_y, t_x, t_y);

    for(uc_index = 0U; uc_index < uc_angle_count; uc_index++)
    {
        sl_cos = lv_trigo_cos((int16_t)us_angles[uc_index]);
        sl_sin = lv_trigo_sin((int16_t)us_angles[uc_index]);

        t_x = (lv_coord_t)(t_center.x + energy_ring_q15_mul_round(t_outer_radius, sl_cos, 0U));
        t_y = (lv_coord_t)(t_center.y + energy_ring_q15_mul_round(t_outer_radius, sl_sin, 0U));
        energy_ring_extend_area_with_point(tp_area, t_x, t_y);

        t_x = (lv_coord_t)(t_center.x + energy_ring_q15_mul_round(t_inner_radius, sl_cos, 0U));
        t_y = (lv_coord_t)(t_center.y + energy_ring_q15_mul_round(t_inner_radius, sl_sin, 0U));
        energy_ring_extend_area_with_point(tp_area, t_x, t_y);
    }

    lv_area_increase(tp_area, 2, 2);
    return true;
}

/***********************************************************************************************************************
-----函数功能    局部失效指定范围的segment区域
-----说明(备注)  计算新旧active_seg之间变化的segment范围，逐个计算包围盒并调用lv_obj_invalidate_area
                  实现局部重绘，避免整环刷新带来的性能开销
-----传入参数    tp_ring:能量环结构体指针  us_old_active_seg:变化前的活动段数  us_new_active_seg:变化后的活动段数
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_invalidate_seg_range(EnergyRing_T *tp_ring, u16 us_old_active_seg, u16 us_new_active_seg)
{
    lv_area_t t_seg_area;
    u16 us_start_seg;
    u16 us_end_seg;
    u16 us_seg;

    if((tp_ring == NULL) || (tp_ring->pObj == NULL))
        return;

    if(us_old_active_seg > tp_ring->usSegCount)
        us_old_active_seg = tp_ring->usSegCount;
    if(us_new_active_seg > tp_ring->usSegCount)
        us_new_active_seg = tp_ring->usSegCount;

    if(us_old_active_seg == us_new_active_seg)
        return;

    if((us_old_active_seg == 0U) || (us_new_active_seg == 0U))
        us_start_seg = 1U;
    else if(us_old_active_seg < us_new_active_seg)
        us_start_seg = us_old_active_seg;
    else
        us_start_seg = us_new_active_seg;

    us_end_seg = (us_old_active_seg > us_new_active_seg) ? us_old_active_seg : us_new_active_seg;
    if(us_end_seg == 0U)
        return;

    for(us_seg = us_start_seg; us_seg <= us_end_seg; us_seg++)
    {
        if(energy_ring_get_seg_area(tp_ring, (u16)(us_seg - 1U), &t_seg_area))
            lv_obj_invalidate_area(tp_ring->pObj, &t_seg_area);
    }
}

/***********************************************************************************************************************
-----函数功能    能量环定时器回调函数
-----说明(备注)  定时器周期性调用，递增活动段索引并触发LVGL对象重绘，实现动画效果
-----传入参数    tp_timer:LVGL定时器指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_timer_cb(lv_timer_t *tp_timer)
{
    EnergyRing_T *tp_ring;  /* 能量环上下文指针 */
    u16 us_soc_base_seg;    /* SOC基点对应的segment数量 */
    u16 us_old_active_seg;  /* 变化前的segment数量 */

    /* 参数有效性检查 */
    if(tp_timer == NULL)
        return;

    /* 从定时器用户数据获取能量环上下文 */
    tp_ring = (EnergyRing_T *)lv_timer_get_user_data(tp_timer);
    if(tp_ring == NULL)
        return;

    /* 检查使能状态和对象有效性，未使能或对象无效时直接返回 */
    if((tp_ring->bEnabled == false) || (tp_ring->pObj == NULL))
        return;

    us_old_active_seg = tp_ring->usActiveSeg;

    /* 充电模式下执行充电动画 */
    if(tp_ring->eSocMode == ENERGY_RING_SOC_CHARGING)
    {
        /* 获取SOC对应segment数 */
        us_soc_base_seg = energy_ring_soc_to_seg_count(tp_ring, tp_ring->ucSocValue);
        
        if(tp_ring->bAnimDirDec)
        {
            /* 递减阶段：逐步减少活动segment数 */
            if(tp_ring->usActiveSeg > us_soc_base_seg)
            {
                tp_ring->usActiveSeg--;
            }
            else
            {
                tp_ring->usActiveSeg = us_soc_base_seg;
            }
            
            /* 到达或低于起始SOC对应的segment数时，切换为递增阶段 */
            if(tp_ring->usActiveSeg <= us_soc_base_seg)
            {
                tp_ring->bAnimDirDec = false;
            }
        }
        else
        {
            /* 递增阶段：逐步增加活动segment数 */
            tp_ring->usActiveSeg++;
            
            /* 到达或超过总segment数时，切换为递减阶段 */
            if(tp_ring->usActiveSeg >= tp_ring->usSegCount)
            {
                tp_ring->usActiveSeg = tp_ring->usSegCount;
                tp_ring->bAnimDirDec = true;
            }
        }
    }

    /* 仅重绘发生变化的segment区域，避免整环反复刷新 */
    energy_ring_invalidate_seg_range(tp_ring, us_old_active_seg, tp_ring->usActiveSeg);
}

/***********************************************************************************************************************
-----函数功能    获取能量环段颜色
-----说明(备注)  根据当前活动段索引和段位置返回对应的颜色，实现能量环的渐变显示效果
-----传入参数    tp_ring:能量环结构体指针 us_seg_index:段索引
-----输出参数    none
-----返回值      对应段的颜色值
************************************************************************************************************************/
static lv_color_t energy_ring_get_seg_color(const EnergyRing_T *tp_ring, u16 us_seg_index)
{
    /* 参数有效性检查 */
    if(tp_ring == NULL)
        return lv_color_black();

    /* 当active_seg为0时，所有segment都处于熄灭状态 */
    if(tp_ring->usActiveSeg == 0U)
        return tp_ring->tSegOffColor;

    /* 当segment索引+1 < active_seg时，该segment已点亮 */
    if((u16)(us_seg_index + 1U) < tp_ring->usActiveSeg)
        return tp_ring->tSegOnColor;

    /* 当segment索引+1 == active_seg时，该segment是当前头部(最新点亮的) */
    if((u16)(us_seg_index + 1U) == tp_ring->usActiveSeg)
        return tp_ring->tSegHeadColor;

    /* 其他情况：segment尚未点亮 */
    return tp_ring->tSegOffColor;
}

/***********************************************************************************************************************
-----函数功能    能量环绘制事件回调函数
-----说明(备注)  处理LVGL对象的LV_EVENT_DRAW_MAIN事件，绘制所有能量环段，实现环形动画的视觉效果
-----传入参数    tp_event:LVGL事件指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_draw_event(lv_event_t *tp_event)
{
    EnergyRing_T *tp_ring;          /* 能量环上下文指针 */
    lv_obj_t *tp_obj;               /* LVGL对象指针 */
    lv_layer_t *tp_layer;           /* LVGL绘制层指针 */
    lv_area_t t_coords;             /* 对象内容区域坐标 */
    lv_draw_arc_dsc_t t_arc_dsc;    /* 弧形绘制描述符 */
    lv_point_t t_center;            /* 圆心坐标 */
    lv_coord_t t_radius;            /* 实际渲染半径 */
    lv_coord_t t_radius_x_max;      /* X方向最大可用半径 */
    lv_coord_t t_radius_y_max;      /* Y方向最大可用半径 */
    u16 us_seg_index;               /* segment索引 */
    u16 us_start_angle;             /* 当前segment起始角度 */

    /* 只处理主绘制事件 */
    if(lv_event_get_code(tp_event) != LV_EVENT_DRAW_MAIN)
        return;

    /* 获取能量环上下文 */
    tp_ring = (EnergyRing_T *)lv_event_get_user_data(tp_event);
    if(tp_ring == NULL)
        return;

    /* 验证当前目标对象与能量环对象一致 */
    tp_obj = lv_event_get_current_target(tp_event);
    if(tp_obj != tp_ring->pObj)
        return;

    /* 获取LVGL绘制层 */
    tp_layer = lv_event_get_layer(tp_event);
    if(tp_layer == NULL)
        return;

    /* 获取对象内容区域坐标 */
    lv_obj_get_content_coords(tp_obj, &t_coords);

    /* 计算圆心坐标：对象已按目标中心定位，这里直接取对象中心 */
    t_center.x = (lv_coord_t)((t_coords.x1 + t_coords.x2) / 2);
    t_center.y = (lv_coord_t)((t_coords.y1 + t_coords.y2) / 2);

    /* 计算X和Y方向的最大可用半径（考虑环宽） */
    t_radius_x_max = (lv_coord_t)(((t_coords.x2 - t_coords.x1) + 1) / 2 - tp_ring->tWidth);
    t_radius_y_max = (lv_coord_t)(((t_coords.y2 - t_coords.y1) + 1) / 2 - tp_ring->tWidth);
    t_radius = tp_ring->tRadius;

    /* 限制半径不超过最大值，防止超出对象边界 */
    if(t_radius > t_radius_x_max)
        t_radius = t_radius_x_max;
    if(t_radius > t_radius_y_max)
        t_radius = t_radius_y_max;

    /* 验证半径有效性：半径必须大于环宽，且segment跨度不能为0 */
    if((t_radius <= tp_ring->tWidth) || (tp_ring->usSegSpanAngle == 0U))
        return;

    /* 初始化弧形绘制描述符 */
    lv_draw_arc_dsc_init(&t_arc_dsc);
    t_arc_dsc.base.layer = tp_layer;
    t_arc_dsc.center = t_center;
    t_arc_dsc.radius = (uint16_t)t_radius;
    t_arc_dsc.width = tp_ring->tWidth;
    t_arc_dsc.rounded = 0U;           /* 关闭圆角，保持直角边缘 */
    t_arc_dsc.opa = LV_OPA_COVER;     /* 完全不透明 */

    /* 设置起始角度 */
    us_start_angle = tp_ring->usRenderStartAngle;

    /* 遍历所有segment，逐个绘制弧形 */
    for(us_seg_index = 0U; us_seg_index < tp_ring->usSegCount; us_seg_index++)
    {
        /* 获取当前segment颜色 */
        t_arc_dsc.color = energy_ring_get_seg_color(tp_ring, us_seg_index);
        
        /* 设置弧形角度范围 */
        t_arc_dsc.start_angle = us_start_angle;
        t_arc_dsc.end_angle = (u16)(us_start_angle + tp_ring->usSegSpanAngle);
        
        /* 绘制弧形segment */
        lv_draw_arc(tp_layer, &t_arc_dsc);
        
        /* 更新下一个segment的起始角度 */
        us_start_angle = (u16)(us_start_angle + tp_ring->usSegStepAngle);
    }

    /* 绘制内环装饰环 */
    energy_ring_draw_inner_ring(tp_ring, tp_layer, t_center, t_radius);
}

/***********************************************************************************************************************
-----函数功能    能量环删除事件回调函数
-----说明(备注)  处理LVGL对象的LV_EVENT_DELETE事件，清理定时器和运行状态，确保资源正确释放
-----传入参数    tp_event:LVGL事件指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_delete_event(lv_event_t *tp_event)
{
    EnergyRing_T *tp_ring;  /* 能量环上下文指针 */

    /* 只处理删除事件 */
    if(lv_event_get_code(tp_event) != LV_EVENT_DELETE)
        return;

    /* 获取能量环上下文 */
    tp_ring = (EnergyRing_T *)lv_event_get_user_data(tp_event);
    if(tp_ring == NULL)
        return;

    /* 验证当前目标对象与能量环对象一致 */
    if(lv_event_get_current_target(tp_event) != tp_ring->pObj)
        return;

    /* 重置对象指针和状态 */
    tp_ring->pObj = NULL;
    tp_ring->bEnabled = false;
    tp_ring->usActiveSeg = 0U;

    /* 删除动画定时器 */
    if(tp_ring->pTimer != NULL)
    {
        lv_timer_delete(tp_ring->pTimer);
        tp_ring->pTimer = NULL;
    }
}

/***********************************************************************************************************************
-----函数功能    创建能量环LVGL对象
-----说明(备注)  在当前活动屏幕上创建能量环对象，配置样式和事件回调，创建动画定时器
-----传入参数    p_ring:能量环结构体指针
-----输出参数    none
-----返回值      true-创建成功 false-创建失败
************************************************************************************************************************/
static bool energy_ring_create(EnergyRing_T *p_ring)
{
    lv_obj_t *tp_screen;  /* 当前活动屏幕指针 */
    lv_area_t t_obj_area; /* 能量环对象包围盒 */

    /* 参数有效性检查 */
    if(p_ring == NULL)
        return false;

    /* 初始化配置参数（如果尚未初始化） */
    energy_ring_init_cfg(p_ring);

    /* 检查LVGL是否已初始化 */
    if(!lv_is_initialized())
        return false;

    /* 如果对象已存在，直接返回成功 */
    if(p_ring->pObj != NULL)
        return true;

    /* 优先挂载到 EEZ 主屏，避免在屏幕切换窗口期创建到错误 screen */
    tp_screen = energy_ring_get_parent_screen();
    if(tp_screen == NULL)
        return false;

    /* 在活动屏幕上创建LVGL对象 */
    p_ring->pObj = lv_obj_create(tp_screen);
    if(p_ring->pObj == NULL)
        return false;

    /* 配置对象位置和大小：仅覆盖能量环实际绘制区域 */
    energy_ring_get_object_area(tp_screen, p_ring, &t_obj_area);
    lv_obj_set_pos(p_ring->pObj, t_obj_area.x1, t_obj_area.y1);
    lv_obj_set_size(p_ring->pObj, lv_area_get_width(&t_obj_area), lv_area_get_height(&t_obj_area));
    
    /* 移除滚动和点击功能，作为纯显示组件 */
    lv_obj_remove_flag(p_ring->pObj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(p_ring->pObj, LV_OBJ_FLAG_CLICKABLE);
    
    /* 背景保持透明，避免对象包围盒跟随界面局部刷新一起重画 */
    lv_obj_set_style_bg_color(p_ring->pObj, p_ring->tBgColor, 0);
    lv_obj_set_style_bg_opa(p_ring->pObj, LV_OPA_TRANSP, 0);
    
    /* 移除边框和圆角，保持简洁 */
    lv_obj_set_style_border_width(p_ring->pObj, 0, 0);
    lv_obj_set_style_radius(p_ring->pObj, 0, 0);
    
    /* 移除内边距，确保能量环占满整个对象区域 */
    lv_obj_set_style_pad_left(p_ring->pObj, 0, 0);
    lv_obj_set_style_pad_right(p_ring->pObj, 0, 0);
    lv_obj_set_style_pad_top(p_ring->pObj, 0, 0);
    lv_obj_set_style_pad_bottom(p_ring->pObj, 0, 0);
    
    /* 将能量环移动到最底层，其他控件可以放置在其上方 */
    lv_obj_move_background(p_ring->pObj);
    
    /* 注册绘制事件回调：在LV_EVENT_DRAW_MAIN中绘制所有segment */
    lv_obj_add_event_cb(p_ring->pObj, energy_ring_draw_event, LV_EVENT_DRAW_MAIN, p_ring);
    
    /* 注册删除事件回调：在LV_EVENT_DELETE中清理资源 */
    lv_obj_add_event_cb(p_ring->pObj, energy_ring_delete_event, LV_EVENT_DELETE, p_ring);

    /* 创建动画定时器 */
    if(p_ring->pTimer == NULL)
    {
        p_ring->pTimer = lv_timer_create(energy_ring_timer_cb, p_ring->usAnimPeriodMs, p_ring);
        if(p_ring->pTimer == NULL)
        {
            /* 定时器创建失败，删除对象并返回失败 */
            lv_obj_delete(p_ring->pObj);
            p_ring->pObj = NULL;
            return false;
        }
    }

    /* 初始状态：暂停定时器，等待enable调用 */
    lv_timer_pause(p_ring->pTimer);

    return true;
}

/***********************************************************************************************************************
-----函数功能    设置能量环使能状态
-----说明(备注)  控制能量环动画的启停，启用时恢复定时器并显示对象，禁用时暂停定时器并隐藏对象
-----传入参数    p_ring:能量环结构体指针 enable:true-启用动画 false-禁用动画
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_set_enable(EnergyRing_T *p_ring, bool enable)
{
    bool b_hidden_before;
    bool b_hidden_after;

    /* 参数有效性检查 */
    if(p_ring == NULL)
        return;

    /* 对象必须已创建才能设置使能状态 */
    if(p_ring->pObj == NULL)
        return;

    /* 更新使能标志 */
    p_ring->bEnabled = enable;
    b_hidden_before = lv_obj_has_flag(p_ring->pObj, LV_OBJ_FLAG_HIDDEN);

    /* 控制定时器：启用时恢复并重置，禁用时暂停 */
    if(p_ring->pTimer != NULL)
    {
        /* 设置定时器周期 */
        lv_timer_set_period(p_ring->pTimer, p_ring->usAnimPeriodMs);
        if(enable)
        {
            /* 启用动画：恢复定时器并重置计时 */
            lv_timer_resume(p_ring->pTimer);
            lv_timer_reset(p_ring->pTimer);
        }
        else
        {
            /* 禁用动画：暂停定时器 */
            lv_timer_pause(p_ring->pTimer);
        }
    }

    /* 控制对象可见性：启用时显示，禁用时隐藏 */
    if(enable)
        lv_obj_remove_flag(p_ring->pObj, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(p_ring->pObj, LV_OBJ_FLAG_HIDDEN);

    b_hidden_after = lv_obj_has_flag(p_ring->pObj, LV_OBJ_FLAG_HIDDEN);

    /* 仅在可见性切换时请求整对象刷新 */
    if(b_hidden_before != b_hidden_after)
        lv_obj_invalidate(p_ring->pObj);
}

/***********************************************************************************************************************
-----函数功能    删除能量环对象
-----说明(备注)  停止动画并删除能量环定时器和LVGL对象，释放相关资源
-----传入参数    p_ring:能量环结构体指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_delete(EnergyRing_T *p_ring)
{
    lv_obj_t *tp_obj;  /* 临时保存对象指针 */

    /* 参数有效性检查 */
    if(p_ring == NULL)
        return;

    /* 禁用动画 */
    p_ring->bEnabled = false;

    /* 删除动画定时器 */
    if(p_ring->pTimer != NULL)
    {
        lv_timer_delete(p_ring->pTimer);
        p_ring->pTimer = NULL;
    }

    /* 保存对象指针后清空结构体中的指针 */
    tp_obj = p_ring->pObj;
    p_ring->pObj = NULL;

    /* 删除LVGL对象 */
    if(tp_obj != NULL)
        lv_obj_delete(tp_obj);

    /* 重置运行状态 */
    p_ring->usActiveSeg = 0U;
}

/***********************************************************************************************************************
-----函数功能    启动能量环演示动画
-----说明(备注)  创建能量环对象，重置运行状态并启用动画
-----传入参数    p_ring:能量环结构体指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void energy_ring_demo_start(EnergyRing_T *p_ring)
{
    /* 参数有效性检查 */
    if(p_ring == NULL)
        return;

    /* 创建能量环对象，失败时直接返回 */
    if(energy_ring_create(p_ring) == false)
        return;

    /* 重置运行状态：active_seg归零 */
    energy_ring_reset_runtime(p_ring);
    
    /* 启用动画：显示对象并启动定时器 */
    energy_ring_set_enable(p_ring, true);
}

/***********************************************************************************************************************
-----函数功能    启动能量环动画
-----说明(备注)  初始化能量环结构体并启动能量环演示动画
-----传入参数    p_ring:能量环结构体指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void EnergyRing_Start(EnergyRing_T *p_ring)
{
    /* 参数有效性检查 */
    if(p_ring == NULL)
        return;

    /* 清空结构体，确保从干净状态启动 */
    memset(p_ring, 0, sizeof(EnergyRing_T));
    
    /* 创建能量环对象 */
    if(energy_ring_create(p_ring) == false)
        return;
    
    // /* 设置初始SOC为50%，充电状态为true，启动充电动画显示 */
    // EnergyRing_UpdateSoc(p_ring, 50, true);
}

/***********************************************************************************************************************
-----函数功能    停止能量环动画
-----说明(备注)  删除能量环对象并释放相关资源
-----传入参数    p_ring:能量环结构体指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void EnergyRing_Stop(EnergyRing_T *p_ring)
{
    /* 删除能量环对象并释放所有资源 */
    energy_ring_delete(p_ring);
}

/***********************************************************************************************************************
-----函数功能    暂停能量环动画
-----说明(备注)  禁用能量环显示，保持对象但停止动画刷新
-----传入参数    p_ring:能量环结构体指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void EnergyRing_Pause(EnergyRing_T *p_ring)
{
    /* 禁用能量环动画：暂停定时器并隐藏对象 */
    energy_ring_set_enable(p_ring, false);
}

/***********************************************************************************************************************
-----函数功能    将SOC值转换为对应的segment数量
-----说明(备注)  根据SOC百分比计算需要点亮的segment数量
-----传入参数    tp_ring:能量环结构体指针 uc_soc:SOC值(0-100)
-----输出参数    none
-----返回值      对应的segment数量
************************************************************************************************************************/
static u16 energy_ring_soc_to_seg_count(const EnergyRing_T *tp_ring, u8 uc_soc)
{
    u16 us_soc_seg_count;  /* SOC对应的segment数量 */

    /* 参数有效性检查 */
    if(tp_ring == NULL)
        return 0U;

    /* SOC值范围限制 */
    if(uc_soc > 100U)
        uc_soc = 100U;

    /* 计算SOC对应的segment数量：segment总数 * SOC / 100 */
    us_soc_seg_count = (u16)((u32)tp_ring->usSegCount * uc_soc / 100U);

    /* 确保结果不超过segment总数 */
    if(us_soc_seg_count > tp_ring->usSegCount)
        us_soc_seg_count = tp_ring->usSegCount;

    return us_soc_seg_count;
}

/***********************************************************************************************************************
-----函数功能    更新能量环SOC显示状态
-----说明(备注)  根据SOC值和充电状态更新能量环显示
-----传入参数    p_ring:能量环结构体指针 uc_soc:SOC值(0-100) b_charging:充电状态
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void EnergyRing_UpdateSoc(EnergyRing_T *p_ring, u8 uc_soc, bool b_charging)
{
    u16 us_soc_seg_count;  /* SOC对应的segment数量 */
    u16 us_old_active_seg; /* 变化前的segment数量 */
    bool b_was_hidden;     /* 更新前是否隐藏 */
    EnergyRing_SocMode_T e_old_mode; /* 更新前的显示模式 */

    /* 参数有效性检查 */
    if(p_ring == NULL)
        return;

    /* 确保对象已创建 */
    if(p_ring->pObj == NULL)
    {
        /* 如果对象未创建，先创建对象 */
        if(energy_ring_create(p_ring) == false)
            return;
    }

    /* 限制SOC值范围 */
    if(uc_soc > 100U)
        uc_soc = 100U;

    us_old_active_seg = p_ring->usActiveSeg;
    b_was_hidden = lv_obj_has_flag(p_ring->pObj, LV_OBJ_FLAG_HIDDEN);
    e_old_mode = p_ring->eSocMode;

    /* 更新SOC相关参数 */
    p_ring->ucSocValue = uc_soc;
    p_ring->bCharging = b_charging;

    /* 根据充电状态设置显示模式 */
    if(b_charging)
    {
        /* 充电模式下为动态 */
        p_ring->eSocMode = ENERGY_RING_SOC_CHARGING;
        
        /* 计算SOC对应的segment数 */
        us_soc_seg_count = energy_ring_soc_to_seg_count(p_ring, uc_soc);
        
        /* 缓存active_seg为SOC对应的segment数 */
        p_ring->usActiveSeg = us_soc_seg_count;
        
        /* 重置动画方向为递增 */
        p_ring->bAnimDirDec = false;
        
        /* 使能定时器 */
        energy_ring_set_enable(p_ring, true);
    }
    else
    {
        /* 静态模式：显示固定电量 */
        p_ring->eSocMode = ENERGY_RING_SOC_STATIC;
        
        /* 计算SOC对应的segment数量 */
        us_soc_seg_count = energy_ring_soc_to_seg_count(p_ring, uc_soc);
        
        /* 设置active_seg为SOC对应的segment数量 */
        p_ring->usActiveSeg = us_soc_seg_count;

        /* 静态模式仍保持组件可见，仅关闭动画 */
        p_ring->bEnabled = true;
        
        /* 禁用动画定时器（静态显示不需要动画） */
        if(p_ring->pTimer != NULL)
            lv_timer_pause(p_ring->pTimer);
        
        /* 确保对象可见 */
        lv_obj_remove_flag(p_ring->pObj, LV_OBJ_FLAG_HIDDEN);
    }

    /* 首次显示或模式切换时整环刷新；常规SOC变化只刷新受影响segment */
    if(b_was_hidden || (e_old_mode != p_ring->eSocMode))
        lv_obj_invalidate(p_ring->pObj);
    else
        energy_ring_invalidate_seg_range(p_ring, us_old_active_seg, p_ring->usActiveSeg);
}
#endif  //boardDISPLAY_EN
