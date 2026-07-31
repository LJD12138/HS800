/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS800\BOOT\Hardware\MD_Display\user_ui
 * File    : update_mode_ui.c
 * Date    : 2026-06-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 升级模式UI界面显示实现 — 左信息面板 + 右仪表盘风格
 * -------------------------------------------------------
 * todo    :
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/user_ui/update_mode_ui.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
#include "Print/print_task.h"
#include "boot_info.h"
#include "Update/update_main.h"
#include <stdio.h>
#include <string.h>

#if (boardDISPLAY_EN)
//****************************************************Macros*******************************************************************//

/* 屏幕尺寸 */
#define UI_SCREEN_W             dispTFT_WIDTH               // 320
#define UI_SCREEN_H             dispTFT_HEIGHT              // 240
#define UI_CENTER_X             (UI_SCREEN_W / 2)           // 160

/* ====== 容器布局 — 每个UI区域用独立背景块包裹 ======
 * 顶部5像素被屏幕遮挡，所有Y坐标统一偏移+5 */

/* 标题栏容器 */
#define UI_TITLE_BOX_X          0
#define UI_TITLE_BOX_Y          10                           /* 顶部预留5px遮挡区 */
#define UI_TITLE_BOX_W          UI_SCREEN_W
#define UI_TITLE_BOX_H          34
#define UI_TITLE_Y              (UI_TITLE_BOX_Y + 2)        /* 7 */
#define UI_TITLE_SCALE          2
#define UI_TITLE_STR            "FIRMWARE UPDATE"

/* 左侧信息面板容器 */
#define UI_INFO_BOX_X           6
#define UI_INFO_BOX_Y           43                          /* 标题栏下方+4间距 */
#define UI_INFO_BOX_W           112
#define UI_INFO_BOX_H           152
#define UI_INFO_TEXT_X          (UI_INFO_BOX_X + 8)         /* 14 */

/* 面板内标签与数值行坐标
 * 字体8x16：scale=1高度16px，scale=2高度32px
 * 三行均匀分布在容器内 */
#define UI_ROW_CH_LABEL_Y       (UI_INFO_BOX_Y + 6)         /* 49 */
#define UI_ROW_CH_VALUE_Y       (UI_ROW_CH_LABEL_Y + 18)    /* 67 */
#define UI_ROW_PROTO_LABEL_Y    (UI_ROW_CH_VALUE_Y + 38)    /* 105 */
#define UI_ROW_PROTO_VALUE_Y    (UI_ROW_PROTO_LABEL_Y + 18) /* 123 */
#define UI_ROW_FRM_LABEL_Y      (UI_ROW_PROTO_VALUE_Y + 38) /* 161 */
#define UI_ROW_FRM_VALUE_Y      (UI_ROW_FRM_LABEL_Y + 18)   /* 179 */

#define UI_LABEL_SCALE          1
#define UI_VALUE_SCALE_BIG      2
#define UI_VALUE_SCALE_SM       1

/* 帧数显示分离定位："NNNN/TTTT"
 * 动态部分(接收帧数): 4字符32px,  半静态部分("/TTTT"): 5字符40px */
#define UI_FRM_DYNAMIC_W        (4 * UI_FONT_CHAR_W)               /* 32px */
#define UI_FRM_STATIC_X         (UI_INFO_TEXT_X + UI_FRM_DYNAMIC_W) /* 分隔符/起始X */

/* 右侧仪表盘容器 */
#define UI_GAUGE_BOX_X          126
#define UI_GAUGE_BOX_Y          43
#define UI_GAUGE_BOX_W          188
#define UI_GAUGE_BOX_H          126

#define UI_GAUGE_CX             (UI_GAUGE_BOX_X + UI_GAUGE_BOX_W / 2)   /* 220 */
#define UI_GAUGE_CY             (UI_GAUGE_BOX_Y + UI_GAUGE_BOX_H / 2)   /* 106 */
#define UI_GAUGE_R              44
#define UI_GAUGE_THICKNESS      7
#define UI_GAUGE_SEGMENTS       20                          /* 圆环分段数 */
#define UI_GAUGE_GAP_ANGLE      3                           /* 段间间隙角度（度），r=44时约2.3px可见 */
#define UI_GAUGE_NUM_Y          (UI_GAUGE_CY - 14)          /* 90 */
#define UI_GAUGE_NUM_SCALE      2
#define UI_GAUGE_PCT_SCALE      2
/* 进度数字固定3位(%3u)，"%"静态定位
 * 数字3位*8*2=48px + "%"1位*8*2=16px = 64px，居中于圆环 */
#define UI_GAUGE_NUM_W          (3 * UI_FONT_CHAR_W * UI_GAUGE_NUM_SCALE)  /* 48px */
#define UI_GAUGE_TOTAL_W        (UI_GAUGE_NUM_W + UI_FONT_CHAR_W * UI_GAUGE_PCT_SCALE) /* 64px */
#define UI_GAUGE_NUM_X          (UI_GAUGE_CX - UI_GAUGE_TOTAL_W / 2 - 2)  /* 186 */
#define UI_GAUGE_PCT_X          (UI_GAUGE_NUM_X + UI_GAUGE_NUM_W)         /* 234 */

/* 状态栏容器 */
#define UI_STATUS_BOX_X         126
#define UI_STATUS_BOX_Y         173
#define UI_STATUS_BOX_W         188
#define UI_STATUS_BOX_H         22
#define UI_STATUS_Y             (UI_STATUS_BOX_Y + 3)       /* 176 */
#define UI_STATUS_SCALE         1

/* 倒计时栏容器 */
#define UI_COUNTDOWN_BOX_X      0
#define UI_COUNTDOWN_BOX_Y      198
#define UI_COUNTDOWN_BOX_W      UI_SCREEN_W
#define UI_COUNTDOWN_BOX_H      37
#define UI_COUNTDOWN_Y          (UI_COUNTDOWN_BOX_Y + 10)   /* 208 */
#define UI_COUNTDOWN_SCALE      1
/* 倒计时静态文字定位："Exit in "(8字符64px) + 数字(3字符24px) + "s"(1字符8px) = 96px */
#define UI_COUNTDOWN_TOTAL_W    96
#define UI_COUNTDOWN_START_X    (UI_CENTER_X - UI_COUNTDOWN_TOTAL_W / 2)  /* 112 */
#define UI_COUNTDOWN_NUM_X      (UI_COUNTDOWN_START_X + 8 * UI_FONT_CHAR_W) /* 176，数字起始X */

/* 字体宽度（8x16字体，scale=1时每字符8像素宽） */
#define UI_FONT_CHAR_W          8

/* 颜色定义 (RGB565) */
#define UI_COLOR_BG             0x0000                      // 主背景（纯黑）
#define UI_COLOR_CONTAINER      0x0145                      // 容器背景（深蓝）
#define UI_COLOR_CONTAINER_BAR  0x01A5                      // 标题/倒计时栏背景（中蓝）
#define UI_COLOR_ACTIVE         0x07FF                      // 活跃进度（亮青）
#define UI_COLOR_CIRCLE_HEAD    0x07FF                      // 圆环头部段高亮（亮青白）
#define UI_COLOR_CIRCLE_INACT   0x12D2                      // 圆环非活跃（深灰，配合深蓝底色形成精致轨道线效果）
#define UI_COLOR_WHITE          0xFFFF                      // 白色
#define UI_COLOR_GRAY           0xAD55                      // 灰色标签
#define UI_COLOR_DIM            0x8410                      // 暗灰
#define UI_COLOR_WARN           0xFD20                      // 警告橙红
#define UI_COLOR_SUCCESS        0x07E0                      // 成功绿

//****************************************************Types*******************************************************************//

/* UI状态枚举 */
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

/* 数据快照 — 每次刷新开始一次性采集 */
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

//****************************************************Function Declaration****************************************************//
static void v_ui_draw_static(void);
static void v_ui_update_info_static(const UiSnapshot_T *snap);
static void v_ui_update_info_frame_cnt(const UiSnapshot_T *snap);
static void v_ui_update_gauge(const UiSnapshot_T *snap, bool b_force);
static void v_ui_update_status_text(const UiSnapshot_T *snap);
static void v_ui_update_status_dots(const UiSnapshot_T *snap, uint8_t anim_idx);
static void v_ui_update_countdown(const UiSnapshot_T *snap);
static void v_ui_collect_snapshot(UiSnapshot_T *snap);
static uint8_t uc_ui_determine_state(UiSnapshot_T *snap);
static uint16_t us_get_countdown_sec(void);
static const char *pc_ch_type_str(ChannelType_E ch);
static const char *pc_proto_type_str(ProtoType_E proto);
static void v_draw_centered_text(uint16_t x_center, uint16_t y, const char *str,
                                 uint16_t color, uint16_t bg, uint8_t scale);


/*****************************************************************************************************************
 * 函数功能    : 通道类型枚举转字符串
 *****************************************************************************************************************/
static const char *pc_ch_type_str(ChannelType_E ch)
{
    switch (ch)
    {
        case CT_NULL:    return "None";
        case CT_CONSOLE: return "Console";
        case CT_PRINT:   return "Print";
        case CT_WIFI:    return "WiFi";
        default:         return "Unknown";
    }
}

/*****************************************************************************************************************
 * 函数功能    : 协议类型枚举转字符串
 *****************************************************************************************************************/
static const char *pc_proto_type_str(ProtoType_E proto)
{
    switch (proto)
    {
        case PT_NULL:   return "None";
        case PT_XMODEM: return "Xmodem";
        case PT_BAIKU:  return "Baiku";
        default:        return "Unknown";
    }
}

/*****************************************************************************************************************
 * 函数功能    : 获取退出倒计时秒数（基于 tpProtoRx->usLostOverTimeCnt）
 * 说明(备注)  : usLostOverTimeCnt 以 usTaskCycleTime(=boardREPET_TIMER_CYCLE_TMIE=100ms) 递减，秒数 = cnt / 10
 *               tpProtoRx 可能为 NULL，需判空
 *****************************************************************************************************************/
static uint16_t us_get_countdown_sec(void)
{
    if (tUpdate.tpProtoRx == NULL)
        return 0;
    return tUpdate.tpProtoRx->usLostOverTimeCnt / 100;
}

/*****************************************************************************************************************
 * 函数功能    : 绘制居中文字
 * 说明(备注)  : 根据字符串长度和字体倍数计算居中X坐标
 *****************************************************************************************************************/
static void v_draw_centered_text(uint16_t x_center, uint16_t y, const char *str,
                                 uint16_t color, uint16_t bg, uint8_t scale)
{
    uint16_t text_w = strlen(str) * UI_FONT_CHAR_W * scale;
    uint16_t x = x_center - text_w / 2;
    vDisp_DrawText(x, y, str, color, bg, scale);
}

/*****************************************************************************************************************
 * 函数功能    : 判断当前UI状态
 * 说明(备注)  : 按优先级判定，返回状态并填充快照中的状态文字和颜色
 * 返回值      : 动画帧索引（0~3），非动画状态返回0
 *****************************************************************************************************************/
static uint8_t uc_ui_determine_state(UiSnapshot_T *snap)
{
    bool b_cancelled = false;
    bool b_timeout = false;

    /* 检测取消 */
    if (tUpdate.eProtoType == PT_XMODEM && tXmodem.eState == XMODEM_STATE_CANCEL)
        b_cancelled = true;
    else if (tUpdate.eProtoType == PT_BAIKU && tBaiKuProto.eState == BAIKU_STATE_CANCEL)
        b_cancelled = true;

    /* 检测超时：倒计时归零且未成功 */
    if (snap->countdown_sec == 0 && tBootMemParam.tParam.eAppState != AS_FINISH)
        b_timeout = true;

    if (b_cancelled)
    {
        snap->ui_state  = UI_STATE_CANCELLED;
        snap->p_status_str  = "Upgrade Cancelled!";
        snap->status_color  = UI_COLOR_WARN;
        return 0;
    }

    if (b_timeout)
    {
        snap->ui_state  = UI_STATE_TIMEOUT;
        snap->p_status_str  = "Upgrade Timeout!";
        snap->status_color  = UI_COLOR_WARN;
        return 0;
    }

    if (tBootMemParam.tParam.eAppState == AS_FINISH)
    {
        snap->ui_state  = UI_STATE_SUCCESS;
        snap->p_status_str  = "Upgrade Success!";
        snap->status_color  = UI_COLOR_SUCCESS;
        return 0;
    }

    if (tBootMemParam.tParam.eAppState == AS_ERASE)
    {
        bool b_proto_started = (tUpdate.eProtoType == PT_XMODEM &&
                                tXmodem.eState != XMODEM_STATE_IDLE &&
                                tXmodem.eState != XMODEM_STATE_STANDBY) ||
                               (tUpdate.eProtoType == PT_BAIKU &&
                                tBaiKuProto.eState != BAIKU_STATE_IDLE &&
                                tBaiKuProto.eState != BAIKU_STATE_STANDBY);

        if (snap->progress > 0 && snap->progress < 100)
        {
            snap->ui_state  = UI_STATE_UPGRADING;
            snap->p_status_str  = "Upgrading...";
            snap->status_color  = UI_COLOR_WHITE;
            return 0;
        }
        else if (snap->progress == 0 && b_proto_started)
        {
            snap->ui_state  = UI_STATE_ERASING;
            snap->p_status_str  = "Erasing Flash...";
            snap->status_color  = UI_COLOR_WHITE;
            return 1;   /* 动画 */
        }
        else
        {
            snap->ui_state  = UI_STATE_WAITING;
            snap->p_status_str  = "Waiting...";
            snap->status_color  = UI_COLOR_GRAY;
            return 1;   /* 动画 */
        }
    }

    snap->ui_state  = UI_STATE_IDLE;
    snap->p_status_str  = "Idle";
    snap->status_color  = UI_COLOR_GRAY;
    return 0;
}

/*****************************************************************************************************************
 * 函数功能    : 采集数据快照
 *****************************************************************************************************************/
static void v_ui_collect_snapshot(UiSnapshot_T *snap)
{
    /* 进度计算（除零保护） */
    snap->progress = 0;
    if (tUpdate.usTotalFrmValue > 0)
    {
        snap->progress = (uint32_t)tUpdate.usRecFrameCnt * 100 / tUpdate.usTotalFrmValue;
        if (snap->progress > 100) snap->progress = 100;
    }

    snap->rec_frame_cnt  = tUpdate.usRecFrameCnt;
    snap->total_frame    = tUpdate.usTotalFrmValue;
    snap->countdown_sec  = us_get_countdown_sec();
    snap->p_ch_str       = pc_ch_type_str(tUpdate.eChType);
    snap->p_proto_str    = pc_proto_type_str(tUpdate.eProtoType);

    /* 状态判定填充 p_status_str 和 status_color */
    uc_ui_determine_state(snap);
}

/*****************************************************************************************************************
 * 函数功能    : 绘制静态内容（仅首次运行时调用）
 * 说明(备注)  : 标题、面板背景、面板内标签 — 这些内容在升级过程中不变
 *****************************************************************************************************************/
static void v_ui_draw_static(void)
{
    /* 全屏清屏 */
    vDisp_DrawFillRect(0, 0, UI_SCREEN_W, UI_SCREEN_H, UI_COLOR_BG);

    /* 绘制各容器背景块 */
    vDisp_DrawFillRect(UI_TITLE_BOX_X, UI_TITLE_BOX_Y, UI_TITLE_BOX_W, UI_TITLE_BOX_H, UI_COLOR_CONTAINER_BAR);
    vDisp_DrawFillRect(UI_INFO_BOX_X, UI_INFO_BOX_Y, UI_INFO_BOX_W, UI_INFO_BOX_H, UI_COLOR_CONTAINER);
    vDisp_DrawFillRect(UI_GAUGE_BOX_X, UI_GAUGE_BOX_Y, UI_GAUGE_BOX_W, UI_GAUGE_BOX_H, UI_COLOR_CONTAINER);
    vDisp_DrawFillRect(UI_STATUS_BOX_X, UI_STATUS_BOX_Y, UI_STATUS_BOX_W, UI_STATUS_BOX_H, UI_COLOR_CONTAINER);
    vDisp_DrawFillRect(UI_COUNTDOWN_BOX_X, UI_COUNTDOWN_BOX_Y, UI_COUNTDOWN_BOX_W, UI_COUNTDOWN_BOX_H, UI_COLOR_CONTAINER_BAR);

    /* 标题 */
    v_draw_centered_text(UI_CENTER_X, UI_TITLE_Y, UI_TITLE_STR, UI_COLOR_WHITE, UI_COLOR_CONTAINER_BAR, UI_TITLE_SCALE);

    /* 面板内标签（静态） */
    vDisp_DrawText(UI_INFO_TEXT_X, UI_ROW_CH_LABEL_Y,    "CHANNEL",  UI_COLOR_GRAY, UI_COLOR_CONTAINER, UI_LABEL_SCALE);
    vDisp_DrawText(UI_INFO_TEXT_X, UI_ROW_PROTO_LABEL_Y, "PROTOCOL", UI_COLOR_GRAY, UI_COLOR_CONTAINER, UI_LABEL_SCALE);
    vDisp_DrawText(UI_INFO_TEXT_X, UI_ROW_FRM_LABEL_Y,   "FRAME",    UI_COLOR_GRAY, UI_COLOR_CONTAINER, UI_LABEL_SCALE);

    /* 进度百分号会在更新仪表盘时与数字动态合并且居中绘制 */

    /* 倒计时静态文字 "Exit in " 和 "s"，数字区域留空由动态函数刷新 */
    vDisp_DrawText(UI_COUNTDOWN_START_X, UI_COUNTDOWN_Y, "Exit in ", UI_COLOR_DIM, UI_COLOR_CONTAINER_BAR, UI_COUNTDOWN_SCALE);
    vDisp_DrawText(UI_COUNTDOWN_NUM_X + 3 * UI_FONT_CHAR_W, UI_COUNTDOWN_Y, "s", UI_COLOR_DIM, UI_COLOR_CONTAINER_BAR, UI_COUNTDOWN_SCALE);
}

/*****************************************************************************************************************
 * 函数功能    : 更新左侧信息面板 — 通道/协议/帧数总帧（半静态部分）
 * 说明(备注)  : 通道、协议、"/总帧数" 变化频率低，仅在值变化时刷新
 *****************************************************************************************************************/
static void v_ui_update_info_static(const UiSnapshot_T *snap)
{
    char buf[16];

    /* 通道值 (scale=2, 高度32px) */
    vDisp_DrawFillRect(UI_INFO_TEXT_X, UI_ROW_CH_VALUE_Y, UI_INFO_BOX_W - 16, 32, UI_COLOR_CONTAINER);
    vDisp_DrawText(UI_INFO_TEXT_X, UI_ROW_CH_VALUE_Y, snap->p_ch_str, UI_COLOR_WHITE, UI_COLOR_CONTAINER, UI_VALUE_SCALE_BIG);

    /* 协议值 (scale=2, 高度32px) */
    vDisp_DrawFillRect(UI_INFO_TEXT_X, UI_ROW_PROTO_VALUE_Y, UI_INFO_BOX_W - 16, 32, UI_COLOR_CONTAINER);
    vDisp_DrawText(UI_INFO_TEXT_X, UI_ROW_PROTO_VALUE_Y, snap->p_proto_str, UI_COLOR_WHITE, UI_COLOR_CONTAINER, UI_VALUE_SCALE_BIG);

    /* 帧数半静态部分 "/TTTT" (scale=1) */
    snprintf(buf, sizeof(buf), "/%04u", snap->total_frame);
    vDisp_DrawFillRect(UI_FRM_STATIC_X, UI_ROW_FRM_VALUE_Y, 5 * UI_FONT_CHAR_W, 16, UI_COLOR_CONTAINER);
    vDisp_DrawText(UI_FRM_STATIC_X, UI_ROW_FRM_VALUE_Y, buf, UI_COLOR_WHITE, UI_COLOR_CONTAINER, UI_VALUE_SCALE_SM);
}

/*****************************************************************************************************************
 * 函数功能    : 更新左侧信息面板 — 接收帧数（高频动态部分）
 * 说明(备注)  : 仅刷新 "NNNN" 数字区域，不触碰 "/TTTT" 半静态部分
 *****************************************************************************************************************/
static void v_ui_update_info_frame_cnt(const UiSnapshot_T *snap)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%04u", snap->rec_frame_cnt);

    vDisp_DrawFillRect(UI_INFO_TEXT_X, UI_ROW_FRM_VALUE_Y, UI_FRM_DYNAMIC_W, 16, UI_COLOR_CONTAINER);
    vDisp_DrawText(UI_INFO_TEXT_X, UI_ROW_FRM_VALUE_Y, buf, UI_COLOR_WHITE, UI_COLOR_CONTAINER, UI_VALUE_SCALE_SM);
}

/*****************************************************************************************************************
 * 函数功能    : 更新右侧仪表盘（圆环+进度数字）
 * 说明(备注)  : 圆环仅段数变化时重绘（5%粒度，20段数圆环）；数字每次进度变化刷新（1%粒度）
 *               进度数字与百分号动态合并且居中绘制，解决固定宽度导致小数字偏右不对称问题
 *****************************************************************************************************************/
static void v_ui_update_gauge(const UiSnapshot_T *snap, bool b_force)
{
    static uint8_t s_uc_prev_seg = 0xFF;
    uint8_t seg_count = (uint8_t)((uint32_t)snap->progress * UI_GAUGE_SEGMENTS / 100);

    /* L2: 圆环 — 仅段数变化或强制刷新时重绘（5%粒度，减少SPI传输） */
    if (seg_count != s_uc_prev_seg || b_force)
    {
        vDisp_DrawSegmentedRing(UI_GAUGE_CX, UI_GAUGE_CY, UI_GAUGE_R, UI_GAUGE_THICKNESS,
                                seg_count, UI_GAUGE_SEGMENTS, UI_GAUGE_GAP_ANGLE,
                                UI_COLOR_ACTIVE, UI_COLOR_CIRCLE_HEAD, UI_COLOR_CIRCLE_INACT,
                                UI_COLOR_CONTAINER);
        s_uc_prev_seg = seg_count;
    }

    /* L2: 进度数字 — 每次进度变化都刷新（动态居中绘制进度数字与百分号） */
    char buf[16];
    snprintf(buf, sizeof(buf), "%u%%", snap->progress);
    
    /* 擦除之前的文本范围（使用最大可能宽度 4字符 * 8px * scale=2 = 64px, 加些边距用 72px 擦除） */
    vDisp_DrawFillRect(UI_GAUGE_CX - 36, UI_GAUGE_NUM_Y, 72, 32, UI_COLOR_CONTAINER);
    v_draw_centered_text(UI_GAUGE_CX, UI_GAUGE_NUM_Y, buf, UI_COLOR_WHITE, UI_COLOR_CONTAINER, UI_GAUGE_NUM_SCALE);
}

/*****************************************************************************************************************
 * 函数功能    : 更新右侧状态文字（状态变化时全量重绘）
 * 说明(备注)  : 仅在状态文字变化时调用，重绘整个状态区域
 *****************************************************************************************************************/
static void v_ui_update_status_text(const UiSnapshot_T *snap)
{
    /* 清除状态容器区域并居中绘制状态文字+动画点占位 */
    char buf[40];
    snprintf(buf, sizeof(buf), "%s   ", snap->p_status_str);  /* 固定追加3字符动画点空间 */
    vDisp_DrawFillRect(UI_STATUS_BOX_X, UI_STATUS_BOX_Y, UI_STATUS_BOX_W, UI_STATUS_BOX_H, UI_COLOR_CONTAINER);
    v_draw_centered_text(UI_GAUGE_CX, UI_STATUS_Y, buf, snap->status_color, UI_COLOR_CONTAINER, UI_STATUS_SCALE);
}

/*****************************************************************************************************************
 * 函数功能    : 更新右侧状态动画点（仅刷新尾部3字符）
 * 说明(备注)  : 状态文字不变，仅刷新动画点区域，1s/帧
 *****************************************************************************************************************/
static void v_ui_update_status_dots(const UiSnapshot_T *snap, uint8_t anim_idx)
{
    static const char *s_dot_anim[4] = { "   ", ".  ", ".. ", "..." };

    /* 计算动画点起始X：状态文字宽度 + 居中偏移
     * 整体 = 状态文字 + 3字符动画点，居中于 UI_GAUGE_CX */
    uint16_t status_w = strlen(snap->p_status_str) * UI_FONT_CHAR_W;
    uint16_t total_w  = status_w + 3 * UI_FONT_CHAR_W;
    uint16_t dots_x   = UI_GAUGE_CX - total_w / 2 + status_w;

    /* 仅清除并重绘3字符动画点区域 */
    vDisp_DrawFillRect(dots_x, UI_STATUS_Y, 3 * UI_FONT_CHAR_W, 16, UI_COLOR_CONTAINER);
    vDisp_DrawText(dots_x, UI_STATUS_Y, s_dot_anim[anim_idx], snap->status_color, UI_COLOR_CONTAINER, UI_STATUS_SCALE);
}

/*****************************************************************************************************************
 * 函数功能    : 更新底部倒计时
 *****************************************************************************************************************/
static void v_ui_update_countdown(const UiSnapshot_T *snap)
{
    /* 倒计时格式 "Exit in NNNs"，"Exit in " 和 "s" 为静态，仅数字动态
     * 数字用3位固定宽度，保证"s"位置不变，静态部分无需重绘 */

    char num_buf[8];
    snprintf(num_buf, sizeof(num_buf), "%3u", snap->countdown_sec);  /* 固定3位宽度 */

    uint16_t num_w = 3 * UI_FONT_CHAR_W;        /* 数字固定占3字符 = 24px */
    uint16_t num_x = UI_COUNTDOWN_NUM_X;        /* 数字起始X，静态计算 */

    /* 仅清除并重绘数字区域 */
    vDisp_DrawFillRect(num_x, UI_COUNTDOWN_Y, num_w, 16, UI_COLOR_CONTAINER_BAR);
    vDisp_DrawText(num_x, UI_COUNTDOWN_Y, num_buf, UI_COLOR_DIM, UI_COLOR_CONTAINER_BAR, UI_COUNTDOWN_SCALE);
}

/*****************************************************************************************************************
 * 函数功能    : 更新升级模式UI界面显示内容
 * 说明(备注)  : 左信息面板+右仪表盘布局
 *               L0静态内容仅首次绘制；L1动态内容值变化才刷新；L2进度变化才刷新
 *               动画帧1s切换，CPU占用峰值<8%
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 *****************************************************************************************************************/
void vDisp_UpdateModeUi(void)
{
    static bool s_b_first_run = true;
    static uint8_t s_uc_anim_frame = 0;

    /* 变化检测缓存 */
    static uint8_t s_uc_prev_progress = 0xFF;
    static const char *s_pc_prev_status = NULL;
    static const char *s_pc_prev_ch = NULL;
    static const char *s_pc_prev_proto = NULL;
    static uint16_t s_us_prev_frm = 0xFFFF;
    static uint16_t s_us_prev_total = 0xFFFF;
    static uint16_t s_us_prev_countdown = 0xFFFF;

    /* 背光关闭时不刷新，标记首次运行以便重新点亮时全量刷新 */
    if (tDisp.bLight == false)
    {
        s_b_first_run = true;
        return;
    }

    /* 采集数据快照 */
    UiSnapshot_T snap;
    v_ui_collect_snapshot(&snap);

    /* 动画帧索引：每20个显示周期(1s)切换一次 */
    uint8_t anim_idx = 0;
    bool b_need_anim = (snap.ui_state == UI_STATE_WAITING || snap.ui_state == UI_STATE_ERASING);
    if (b_need_anim)
    {
        anim_idx = (s_uc_anim_frame / 20) & 0x03;
    }
    s_uc_anim_frame++;
    if (s_uc_anim_frame >= 80)
    {
        s_uc_anim_frame = 0;
    }

    /* L0: 静态内容（仅首次） */
    if (s_b_first_run)
    {
        v_ui_draw_static();
    }

    /* L1: 左侧信息面板 — 通道/协议/总帧数（低频，任一变化才刷新） */
    if (snap.p_ch_str != s_pc_prev_ch ||
        snap.p_proto_str != s_pc_prev_proto ||
        snap.total_frame != s_us_prev_total ||
        s_b_first_run)
    {
        v_ui_update_info_static(&snap);
        s_pc_prev_ch = snap.p_ch_str;
        s_pc_prev_proto = snap.p_proto_str;
        s_us_prev_total = snap.total_frame;
    }

    /* L1: 左侧信息面板 — 接收帧数（高频，仅帧数变化才刷新数字区域） */
    if (snap.rec_frame_cnt != s_us_prev_frm || s_b_first_run)
    {
        v_ui_update_info_frame_cnt(&snap);
        s_us_prev_frm = snap.rec_frame_cnt;
    }

    /* L2: 右侧仪表盘（进度变化才刷新，圆环按5%粒度分段刷新） */
    if (snap.progress != s_uc_prev_progress || s_b_first_run)
    {
        v_ui_update_gauge(&snap, s_b_first_run);
        s_uc_prev_progress = snap.progress;
    }

    /* L1: 状态文字（仅状态变化才全量重绘） */
    if (snap.p_status_str != s_pc_prev_status || s_b_first_run)
    {
        v_ui_update_status_text(&snap);
        s_pc_prev_status = snap.p_status_str;
    }

    /* L1: 状态动画点（仅等待/擦除状态，每1s刷新3字符区域） */
    if (b_need_anim && anim_idx > 0)
    {
        v_ui_update_status_dots(&snap, anim_idx);
    }

    /* L1: 倒计时（秒数变化才刷新数字区域） */
    if (snap.countdown_sec != s_us_prev_countdown || s_b_first_run)
    {
        v_ui_update_countdown(&snap);
        s_us_prev_countdown = snap.countdown_sec;
    }

    s_b_first_run = false;
}

#endif  /* update_mode_ui.c */
