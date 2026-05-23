/*****************************************************************************************************************
*                                                                                                                *
*                                         显示队列任务-工作中                                                   *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "MD_Display/icon_bitmaps.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif

#if(boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_hm_task.h"
#include "MD_HeatManage/md_hm_iface.h"
#endif

#include <stdio.h>
#include <string.h>

#define     dispTASK_WORK_CYCLE_TIME            100 // 任务周期时间，单位：毫秒
#define     DISP_HEAT_TARGET_TEMP_C             25  // 目标加热温度，单位：摄氏度

static bool b_disp_render_work_page(void);
static void v_disp_work_draw_full_top_bar(const char *title, const char *tag);
static void v_disp_work_draw_hint_line(void);
static const char *pc_disp_work_fan_mode(void);
static const char *pc_disp_work_light_mode(u8 mode);
static const char *pc_disp_work_rgb_mode(u8 mode);
static const char *pc_disp_work_dev_state(u8 state);
static const char *pc_disp_work_light_white_state(const DispUiSnapshot_T *tp_ui);
static const char *pc_disp_work_light_rgb_state(const DispUiSnapshot_T *tp_ui);
static void v_disp_work_format_curr_ma(char *dst, u16 ma);
static void v_disp_work_format_light_pwm(char *dst, u16 pwm);
static void v_disp_work_draw_field_row(u8 index, u8 y, const char *label, const char *value, bool edit_mark);
static void v_disp_work_home_module_power(char *dst, const DispUiSnapshot_T *tp_ui, DispHomeModule_E module);
static const unsigned char *pc_disp_work_home_module_icon(DispHomeModule_E module);
static void v_disp_work_draw_home_module_block(const DispUiSnapshot_T *tp_ui, u8 x, u8 y, DispHomeModule_E module, bool selected);
static void v_disp_work_draw_home_page(const DispUiSnapshot_T *tp_ui);
static void v_disp_work_draw_light_page(const DispUiSnapshot_T *tp_ui);
static void v_disp_work_draw_heat_page(const DispUiSnapshot_T *tp_ui);
static void v_disp_work_draw_wpump_page(const DispUiSnapshot_T *tp_ui);
static void v_disp_work_draw_o2pump_page(const DispUiSnapshot_T *tp_ui);
static void v_disp_work_draw_setting_page(void);
static void v_disp_work_draw_adc_page(const DispUiSnapshot_T *tp_ui);

/***********************************************************************************************************************
-----函数功能    工作显示任务
-----说明(备注)  刷新工作页面、处理长文本滚动并周期刷新实时数据
-----传入参数    tp_task:任务对象指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_disp_queue_task_work(Task_T *tp_task)
{
    static u8 s_refresh_div = 0; // 页面刷新计数器
    static u8 s_scroll_div = 0;  // 文本滚动计数器

    // 检查任务队列是否有新任务
    if(lwrb_get_full(&tp_task->tQueueBuff))
        cQueue_GotoStep(tp_task, STEP_END);

    // 检查是否息屏
    if(tDisp.bLight == false && tp_task->ucStep != 0) 
    {
        tp_task->ucStep = 0;
        #if(boardUSE_OS)
        vTaskDelay(dispTASK_WORK_CYCLE_TIME); // 延迟任务周期时间
        #endif  //boardUSE_OS
        return;
    }
    
    switch (tp_task->ucStep)
    {
        case 0:
        {
            // 初始化工作页面
            if(tDisp.eDevState != DS_WORK)
                bDisp_SetDevState(DS_WORK);

            bDisp_Switch(ST_ON, true); // 打开显示
            tDispPageCtx.usDirtyMask = DDM_FULL; // 标记页面需要完全刷新
            g_bDispPageDirty = true;
            b_disp_render_work_page(); // 渲染工作页面
            cQueue_GotoStep(tp_task, STEP_NEXT); // 进入下一步
        }
        break;

        case 1:
        {
            // 处理文本滚动逻辑
            if(g_bDispPageDirty == false)
            {
                s_scroll_div++;
                if(s_scroll_div >= 1U)
                {
                    s_scroll_div = 0U;
                    if(tDispPageCtx.tTextScroll.ucMaxOffset > 0U)
                    {
                        if(tDispPageCtx.tTextScroll.ucHoldCnt < 7U)
                            tDispPageCtx.tTextScroll.ucHoldCnt++;
                        else
                        {
                            tDispPageCtx.tTextScroll.ucHoldCnt = 0U;
                            if(tDispPageCtx.tTextScroll.ucOffset < tDispPageCtx.tTextScroll.ucMaxOffset)
                                tDispPageCtx.tTextScroll.ucOffset++;
                            else
                                tDispPageCtx.tTextScroll.ucOffset = 0U;

                            tDispPageCtx.usDirtyMask |= DDM_CONTENT; // 标记内容需要刷新
                            g_bDispPageDirty = true;
                        }
                    }
                }
            }

            // 处理页面刷新逻辑
            if(g_bDispPageDirty == false)
            {
                s_refresh_div++;
                if(s_refresh_div >= 3U)
                {
                    s_refresh_div = 0U;
                    switch(tDispPageCtx.ePageId)
                    {
                        case DPI_HOME:
                        case DPI_LIGHT:
                        case DPI_HEAT:
                        case DPI_WPUMP:
                        case DPI_O2PUMP:
                        case DPI_SETTING:
                        case DPI_ADC:
                            if(tDispPageCtx.bEditing == false)
                            {
                                tDispPageCtx.usDirtyMask |= DDM_CONTENT; // 标记内容需要刷新
                                g_bDispPageDirty = true;
                            }
                            break;

                        default:
                            break;
                    }
                }
            }

            // 如果页面需要刷新，则重新渲染
            // if(g_bDispPageDirty)
            b_disp_render_work_page();
            cQueue_GotoStep(tp_task, STEP_END); // 结束当前步骤
        }
        break;
        
        default:
            cQueue_GotoStep(tp_task, STEP_END); // 默认结束步骤
            break;
    }
    
    #if(boardUSE_OS)
    vTaskDelay(dispTASK_WORK_CYCLE_TIME); // 延迟任务周期时间
    #endif  //boardUSE_OS
}

/***********************************************************************************************************************
-----函数功能    渲染工作页面
-----说明(备注)  根据当前页面 ID 渲染对应的页面内容
-----传入参数    none
-----输出参数    none
-----返回值      true: 渲染成功
************************************************************************************************************************/
static bool b_disp_render_work_page(void)
{
    const DispUiSnapshot_T *tp_ui;

    vDisp_UpdateUiSnapshot(); // 更新 UI 快照
    tp_ui = ptDisp_GetUiSnapshot(); // 获取 UI 快照

    switch(tDispPageCtx.ePageId)
    {
        case DPI_HOME:
            v_disp_work_draw_home_page(tp_ui); // 渲染主页
            break;

        case DPI_LIGHT:
            v_disp_work_draw_light_page(tp_ui); // 渲染灯光页面
            break;

        case DPI_HEAT:
            v_disp_work_draw_heat_page(tp_ui); // 渲染加热页面
            break;

        case DPI_WPUMP:
            v_disp_work_draw_wpump_page(tp_ui); // 渲染水泵页面
            break;

        case DPI_O2PUMP:
            v_disp_work_draw_o2pump_page(tp_ui); // 渲染氧气泵页面
            break;

        case DPI_SETTING:
            v_disp_work_draw_setting_page(); // 渲染设置页面
            break;

        case DPI_ADC:
            v_disp_work_draw_adc_page(tp_ui); // 渲染 ADC 页面
            break;

        default:
            v_disp_work_draw_home_page(tp_ui); // 默认渲染主页
            break;
    }

    vDisp_Refresh(); // 刷新显示
    tDispPageCtx.usDirtyMask = DDM_NONE; // 清除刷新标记
    g_bDispPageDirty = false; // 页面不再脏
    return true;
}

/***********************************************************************************************************************
-----函数功能    绘制页面顶部的全宽标题栏（黑底白字）
-----说明(备注)  清除顶部区域并绘制标题与右侧短标签，使用 u8g2 反色绘制实现背景/文字反转
-----传入参数    title: 标题字符串，NULL 表示不绘制标题
-----传入参数    tag: 右侧短标签，NULL 表示不绘制标签
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_full_top_bar(const char *title, const char *tag)
{
    u8 title_len = (u8)((title != NULL) ? strlen(title) : 0U);
    u8 tag_len = (u8)((tag != NULL) ? strlen(tag) : 0U);

    vDisp_ClearRegion(0, 0, OLED_WIDTH_PIXELS, 9);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawBox(&u8g2, 0, 0, OLED_WIDTH_PIXELS, 9);
    u8g2_SetDrawColor(&u8g2, 0);
    if(title_len > 0U)
        u8g2_DrawStr(&u8g2, 2, 7, title);
    if(tag_len > 0U)
        u8g2_DrawStr(&u8g2, (u8)(OLED_WIDTH_PIXELS - (tag_len * 6U) - 2U), 7, tag);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawLine(&u8g2, 0, 9, OLED_WIDTH_PIXELS - 1U, 9);
}

/***********************************************************************************************************************
-----函数功能    绘制底部提示行
-----说明(备注)  当 tDispPageCtx.usHintCnt>0 且提示文本非空时在底部显示提示内容
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_hint_line(void)
{
    if(tDispPageCtx.usHintCnt > 0U && tDispPageCtx.acHint[0] != '\0')
    {
        u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
        u8g2_DrawStr(&u8g2, 52, 61, tDispPageCtx.acHint);
    }
}

/***********************************************************************************************************************
-----函数功能    获取风扇/散热模式文本
-----说明(备注)  根据风扇当前 PWM 值返回简短文本描述:
                - "OFF": PWM 为 0
                - "AUTO": PWM 未达到最大
                - "FULL": PWM 达到最大值
-----传入参数    none
-----输出参数    none
-----返回值      const char*: 模式字符串
************************************************************************************************************************/
static const char *pc_disp_work_fan_mode(void)
{
    const char *pc_mode = "OFF";

#if(boardHEAT_MANAGE_EN)
    u16 us_fan_pwm = (u16)usHM_GetDevPwm(HM_OBJ_FAN);
    if(us_fan_pwm == 0)
        pc_mode = "OFF";
    else
        pc_mode = (us_fan_pwm >= hmPWM_MAX_VALUE) ? "FULL" : "AUTO";
#endif

    return pc_mode;
}

/***********************************************************************************************************************
-----函数功能    获取白光模式文本
-----说明(备注)  将白光模式枚举值转换成简短显示文本
-----传入参数    mode: 白光模式值
-----输出参数    none
-----返回值      const char*: 模式字符串
************************************************************************************************************************/
static const char *pc_disp_work_light_mode(u8 mode)
{
    switch((LampWorkMode_E)mode)
    {
        case LWM_LOW: return "LOW";
        case LWM_HALF: return "HALF";
        case LWM_FULL: return "FULL";
        case LWM_AUTO: return "AUTO";
        case LWM_SOS: return "SOS";
        case LWM_TWINKLE: return "TWKL";
        default: break;
    }

    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    获取 RGB 模式文本
-----说明(备注)  将 RGB 模式枚举值转换成简短显示文本
-----传入参数    mode: RGB 模式值
-----输出参数    none
-----返回值      const char*: 模式字符串
************************************************************************************************************************/
static const char *pc_disp_work_rgb_mode(u8 mode)
{
    switch((RGBWorkMode_E)mode)
    {
        case RWM_LOW: return "LOW";
        case RWM_HALF: return "HALF";
        case RWM_FULL: return "FULL";
        case RWM_AUTO: return "AUTO";
        default: break;
    }

    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    获取设备状态文本
-----说明(备注)  将设备状态枚举值转换成简短显示文本
-----传入参数    state: 设备状态值
-----输出参数    none
-----返回值      const char*: 状态字符串
************************************************************************************************************************/
static const char *pc_disp_work_dev_state(u8 state)
{
    switch((DevState_E)state)
    {
        case DS_INIT: return "INIT";
        case DS_CLOSING: return "CLOSE";
        case DS_SHUT_DOWN: return "OFF";
        case DS_ERR: return "ERR";
        case DS_BOOTING: return "BOOT";
        case DS_WORK: return "WORK";
        case DS_UPDATA_MODE: return "UP";
        case DS_ENG_MODE: return "ENG";
        default: break;
    }

    return "LOST";
}

/***********************************************************************************************************************
-----函数功能    获取白光灯状态文本
-----说明(备注)  根据白光模式值返回短文本，用于概览和详情页摘要
-----传入参数    tp_ui: UI 快照指针
-----输出参数    none
-----返回值      const char*: 状态字符串
************************************************************************************************************************/
static const char *pc_disp_work_light_white_state(const DispUiSnapshot_T *tp_ui)
{
    if(tp_ui == NULL)
        return "OFF";

    switch((LampWorkMode_E)tp_ui->ucLightMode)
    {
        case LWM_LOW: return "LOW";
        case LWM_HALF: return "DIM";
        case LWM_FULL: return "FUL";
        case LWM_AUTO: return "AUT";
        case LWM_SOS: return "SOS";
        case LWM_TWINKLE: return "TWK";
        default: break;
    }

    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    获取 RGB 灯状态文本
-----说明(备注)  优先返回 RGB 模式，其次依据 RGB 通道输出判断开关状态
-----传入参数    tp_ui: UI 快照指针
-----输出参数    none
-----返回值      const char*: 状态字符串
************************************************************************************************************************/
static const char *pc_disp_work_light_rgb_state(const DispUiSnapshot_T *tp_ui)
{
    if(tp_ui == NULL)
        return "OFF";

    switch((RGBWorkMode_E)tp_ui->ucLightRgbMode)
    {
        case RWM_LOW: return "LOW";
        case RWM_HALF: return "DIM";
        case RWM_FULL: return "FUL";
        case RWM_AUTO: return "AUT";
        default: break;
    }

    if(tp_ui->usLightBlue > 0U || tp_ui->usLightGreen > 0U || tp_ui->usLightRed > 0U)
        return "ON";

    return "OFF";
}

/***********************************************************************************************************************
-----函数功能    格式化电流值为字符串
-----说明(备注)  将单位为 mA 的值格式化为带两位小数的安培表示，例如 1234 -> "1.23A"
-----传入参数    dst: 目标字符串缓冲区
-----传入参数    ma: 电流值（mA）
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_format_curr_ma(char *dst, u16 ma)
{
    sprintf(dst, "%u.%02uA", ma / 1000U, (ma % 1000U) / 10U);
}

/***********************************************************************************************************************
-----函数功能    格式化灯光 PWM 值
-----说明(备注)  同时显示占空比百分比与原始 PWM 值，便于对照调试
-----传入参数    dst: 目标字符串缓冲区
-----传入参数    pwm: PWM 原始值
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_format_light_pwm(char *dst, u16 pwm)
{
    sprintf(dst, "%u%%/%u", pwm / 10U, pwm);
}

/***********************************************************************************************************************
-----函数功能    绘制一行字段（标签-值）
-----说明(备注)  根据 index 判断是否为焦点行，焦点行采用反色背景；当 edit_mark 为 true 且为焦点时右侧显示 '*' 标记
-----传入参数    index: 字段索引
-----传入参数    y: 垂直坐标
-----传入参数    label: 左侧标签文本
-----传入参数    value: 右侧值文本
-----传入参数    edit_mark: 编辑标记开关
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_field_row(u8 index, u8 y, const char *label, const char *value, bool edit_mark)
{
    bool focus = (tDispPageCtx.ucFieldIndex == index);

    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    if(focus == true)
    {
        u8g2_DrawBox(&u8g2, 0, y - 7U, OLED_WIDTH_PIXELS, 8);
        u8g2_SetDrawColor(&u8g2, 0);
    }

    u8g2_DrawStr(&u8g2, 4, y, label);
    u8g2_DrawStr(&u8g2, 52, y, value);
    if(focus == true && edit_mark == true)
        u8g2_DrawStr(&u8g2, 122, y, "*");

    u8g2_SetDrawColor(&u8g2, 1);
}

/***********************************************************************************************************************
-----函数功能    获取首页模块功率文本
-----说明(备注)  根据模块类型将对应功率格式化为字符串写入 dst（如 "10W"）
-----传入参数    dst:  目标字符串缓冲区
-----传入参数    tp_ui: UI 快照指针
-----传入参数    module: 模块枚举
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_home_module_power(char *dst, const DispUiSnapshot_T *tp_ui, DispHomeModule_E module)
{
    switch(module)
    {
        case DHM_HEAT:   sprintf(dst, "%uW", tp_ui->usHeatPowerW); break;
        case DHM_WPUMP:  sprintf(dst, "%uW", tp_ui->usPumpPowerW); break;
        case DHM_O2PUMP: sprintf(dst, "%uW", tp_ui->usO2PowerW);   break;
        case DHM_LIGHT:
        default:         sprintf(dst, "%uW", tp_ui->usLightPowerW); break;
    }
}

/***********************************************************************************************************************
-----函数功能    获取首页模块图标
-----说明(备注)  返回模块对应的 16x16 位图指针，用于绘制图标
-----传入参数    module: 模块枚举
-----输出参数    none
-----返回值      const unsigned char*: 图标数据指针
************************************************************************************************************************/
static const unsigned char *pc_disp_work_home_module_icon(DispHomeModule_E module)
{
    switch(module)
    {
        case DHM_HEAT: return icon_heat_16x16;
        case DHM_WPUMP: return icon_wpump_16x16;
        case DHM_O2PUMP: return icon_o2pump_16x16;
        case DHM_LIGHT:
        default: break;
    }

    return icon_light_16x16;
}

/***********************************************************************************************************************
-----函数功能    绘制首页单个模块块
-----说明(备注)  包含图标、模块名以及一行简短状态信息，选中时绘制加粗边框
-----传入参数    tp_ui: UI 快照指针
-----传入参数    x: 左上角 X 坐标
-----传入参数    y: 左上角 Y 坐标
-----传入参数    module: 模块类型
-----传入参数    selected: 是否选中
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_home_module_block(const DispUiSnapshot_T *tp_ui, u8 x, u8 y, DispHomeModule_E module, bool selected)
{
    char line1[18];
    char label[8];

    switch(module)
    {
        case DHM_HEAT:
        {
            sprintf(line1, "F:%u H:%u", usHM_GetDevPwm(HM_OBJ_FAN), usHM_GetDevPwm(HM_OBJ_HEAT));
            break;
        }

        case DHM_WPUMP:
            sprintf(line1, "%s %u%%", tp_ui->pcPumpMode, tp_ui->usPumpSpeed / 10U);
            break;

        case DHM_O2PUMP:
            sprintf(line1, "%s %u%%", tp_ui->pcO2PumpMode, tp_ui->usO2PumpSpeed / 10U);
            break;

        case DHM_LIGHT:
        default:
            sprintf(line1, "W:%s R:%s", pc_disp_work_light_white_state(tp_ui), pc_disp_work_light_rgb_state(tp_ui));
            break;
    }

    if(selected == true)
    {
        u8g2_DrawFrame(&u8g2, x, y, 64, 20);
        u8g2_DrawFrame(&u8g2, x + 1U, y + 1U, 62, 18);
    }
    else
        u8g2_DrawFrame(&u8g2, x, y, 64, 20);

    u8g2_DrawXBMP(&u8g2, x + 2U, y + 2U, 16, 16, pc_disp_work_home_module_icon(module));
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    v_disp_work_home_module_power(label, tp_ui, module);
    u8g2_DrawStr(&u8g2, x + 20U, y + 8U, label);
    u8g2_DrawStr(&u8g2, x + 20U, y + 15U, line1);
}

/***********************************************************************************************************************
-----函数功能    绘制 HOME 页面（工作态第一页）
-----说明(备注)  显示 VIN/I/P 信息，并在页面中绘制四个模块块（LIGHT/HEAT/WPUMP/O2PUMP）
-----传入参数    tp_ui: UI 快照指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_home_page(const DispUiSnapshot_T *tp_ui)
{
    char line[32];

    vDisp_ClearRegion(0, 0, OLED_WIDTH_PIXELS, OLED_HEIGHT_PIXELS);
    v_disp_work_draw_full_top_bar("HOME 1/3", "WORK");
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    sprintf(line, "%u.%uV %u.%02uA %uW", tp_ui->usVinVolt / 10U,
            tp_ui->usVinVolt % 10U, tp_ui->usVinCurrMa / 1000U,
            (tp_ui->usVinCurrMa % 1000U) / 10U, tp_ui->usVinPowerW);
    u8g2_DrawStr(&u8g2, 1, 16, line);

    v_disp_work_draw_home_module_block(tp_ui, 0, 17, DHM_LIGHT, tDispPageCtx.eHomeModule == DHM_LIGHT);
    v_disp_work_draw_home_module_block(tp_ui, 64, 17, DHM_HEAT, tDispPageCtx.eHomeModule == DHM_HEAT);
    v_disp_work_draw_home_module_block(tp_ui, 0, 38, DHM_WPUMP, tDispPageCtx.eHomeModule == DHM_WPUMP);
    v_disp_work_draw_home_module_block(tp_ui, 64, 38, DHM_O2PUMP, tDispPageCtx.eHomeModule == DHM_O2PUMP);
    v_disp_work_draw_hint_line();
}

/***********************************************************************************************************************
-----函数功能    绘制 LIGHT 页面（灯光详细信息）
-----说明(备注)  参照 tLight 结构体显示白光模式、RGB 模式、设备状态与各通道 PWM
-----传入参数    tp_ui: UI 快照指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_light_page(const DispUiSnapshot_T *tp_ui)
{
    char line1[24];
    char value[24];
    u8 i;
    u8 visible_start;
    u8 draw_count;
    const char *labels[8] = {"W MODE", "RGB MD", "STATE", "POWER", "WARM", "BLUE", "GREEN", "RED"};

    visible_start = (tDispPageCtx.ucFieldIndex > 3U) ? (u8)(tDispPageCtx.ucFieldIndex - 3U) : 0U;
    if(visible_start > 4U)
        visible_start = 4U;

    sprintf(line1, "P%uW I%umA", tp_ui->usLightPowerW, tp_ui->usLightCurrMa);
    vDisp_ClearRegion(0, 0, OLED_WIDTH_PIXELS, OLED_HEIGHT_PIXELS);
    v_disp_work_draw_full_top_bar("LIGHT", "1/4");
    u8g2_DrawXBMP(&u8g2, 0, 10, 16, 16, icon_light_16x16);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawStr(&u8g2, 20, 22, line1);

    draw_count = (u8)(8U - visible_start);
    if(draw_count > 4U)
        draw_count = 4U;

    for(i = 0U; i < draw_count; i++)
    {
        u8 field_idx = (u8)(visible_start + i);
        u8 y = (u8)(32U + (i * 8U));

        switch(field_idx)
        {
            case 0U:
                sprintf(value, "%s", pc_disp_work_light_mode(tp_ui->ucLightMode));
                break;
            case 1U:
                sprintf(value, "%s", pc_disp_work_rgb_mode(tp_ui->ucLightRgbMode));
                break;
            case 2U:
                sprintf(value, "%s", pc_disp_work_dev_state(tp_ui->ucLightDevState));
                break;
            case 3U:
                sprintf(value, "%uW/%uW", tp_ui->usLightPowerW, tp_ui->usLightCtrlPower);
                break;
            case 4U:
                v_disp_work_format_light_pwm(value, tp_ui->usLightWarm);
                break;
            case 5U:
                v_disp_work_format_light_pwm(value, tp_ui->usLightBlue);
                break;
            case 6U:
                v_disp_work_format_light_pwm(value, tp_ui->usLightGreen);
                break;
            case 7U:
                v_disp_work_format_light_pwm(value, tp_ui->usLightRed);
                break;
            default:
                value[0] = '\0';
                break;
        }

        v_disp_work_draw_field_row(field_idx, y, labels[field_idx], value, false);
    }

    v_disp_work_draw_hint_line();
}

/***********************************************************************************************************************
-----函数功能    绘制 HEAT 页面（加热控制信息）
-----说明(备注)  显示当前水温、两个水温采样、加热模式、风扇状态及目标温度
-----传入参数    tp_ui: UI 快照指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_heat_page(const DispUiSnapshot_T *tp_ui)
{
    char value[24];
    u8 i;
    u8 visible_start;
    u8 draw_count;
    const char *labels[6] = {"WATER", "HEAT", "HT TMP", "FAN", "FN ST", "FN FL"};

    /* 计算字段可见窗口起始索引 (最多显示4行字段, 共6个字段需滚动) */
    visible_start = (tDispPageCtx.ucFieldIndex > 3U) ? (u8)(tDispPageCtx.ucFieldIndex - 3U) : 0U;
    if(visible_start > 2U)
        visible_start = 2U;

    vDisp_ClearRegion(0, 0, OLED_WIDTH_PIXELS, OLED_HEIGHT_PIXELS);
    v_disp_work_draw_full_top_bar("HM", "1/4");

    /* 摘要行: icon + VIN / IIN / PWR */
    u8g2_DrawXBMP(&u8g2, 0, 10, 16, 16, icon_heat_16x16);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    sprintf(value, "%u.%uV %u.%02uA %uW",
            tp_ui->usVinVolt / 10U, tp_ui->usVinVolt % 10U,
            tp_ui->usVinCurrMa / 1000U, (tp_ui->usVinCurrMa % 1000U) / 10U,
            tp_ui->usVinPowerW);
    u8g2_DrawStr(&u8g2, 20, 22, value);

    /* 绘制可见范围内的字段行 */
    draw_count = (u8)(6U - visible_start);
    if(draw_count > 4U)
        draw_count = 4U;

    for(i = 0U; i < draw_count; i++)
    {
        u8 field_idx = (u8)(visible_start + i);
        u8 y = (u8)(36U + (i * 8U));

        switch(field_idx)
        {
            case 0U:
                sprintf(value, "%d/%dC", tp_ui->sWaterTemp1, tp_ui->sWaterTemp2);
                break;
            case 1U:
                sprintf(value, "%s/%u", tp_ui->ucHeatEnable ? "ON" : "OFF", tp_ui->usHeatPwm);
                break;
            case 2U:
                sprintf(value, "%dC", tp_ui->sHeatTargetTemp);
                break;
            case 3U:
                sprintf(value, "%s/%u", tp_ui->ucFanEnable ? "ON" : "OFF", tp_ui->usFanPwm);
                break;
            case 4U:
                sprintf(value, "%dC", tp_ui->sFanTempStart);
                break;
            case 5U:
                sprintf(value, "%dC", tp_ui->sFanTempFull);
                break;
            default:
                value[0] = '\0';
                break;
        }

        v_disp_work_draw_field_row(field_idx, y, labels[field_idx], value, false);
    }

    v_disp_work_draw_hint_line();
}
/***********************************************************************************************************************
-----函数功能    绘制 WPUMP 页面（水泵信息）
-----说明(备注)  显示泵的模式、速度百分比/原始值、运行状态及电流
-----传入参数    tp_ui: UI 快照指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_wpump_page(const DispUiSnapshot_T *tp_ui)
{
    char line1[24];
    char value[20];

    sprintf(line1, "MODE %s", tp_ui->pcPumpMode);
    vDisp_ClearRegion(0, 0, OLED_WIDTH_PIXELS, OLED_HEIGHT_PIXELS);
    v_disp_work_draw_full_top_bar("WPUMP", "1/4");
    u8g2_DrawXBMP(&u8g2, 0, 10, 16, 16, icon_wpump_16x16);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawStr(&u8g2, 20, 16, line1);

    v_disp_work_draw_field_row(0U, 32, "MODE", tp_ui->pcPumpMode, false);
    sprintf(value, "%u%%/%u", tp_ui->usPumpSpeed / 10U, tp_ui->usPumpSpeed);
    v_disp_work_draw_field_row(1U, 40, "SPEED", value, false);
    v_disp_work_draw_field_row(2U, 48, "STATE", tp_ui->usPumpSpeed > 0U ? "RUN" : "STOP", false);
    v_disp_work_format_curr_ma(value, tp_ui->usPumpCurrMa);
    v_disp_work_draw_field_row(3U, 56, "CURR", value, false);
    v_disp_work_draw_hint_line();
}

/***********************************************************************************************************************
-----函数功能    绘制 O2PUMP 页面（氧气泵信息）
-----说明(备注)  显示氧气泵的模式、速度、运行状态及电流
-----传入参数    tp_ui: UI 快照指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_o2pump_page(const DispUiSnapshot_T *tp_ui)
{
    char line1[24];
    char value[20];

    sprintf(line1, "MODE %s", tp_ui->pcO2PumpMode);
    vDisp_ClearRegion(0, 0, OLED_WIDTH_PIXELS, OLED_HEIGHT_PIXELS);
    v_disp_work_draw_full_top_bar("O2PUMP", "1/4");
    u8g2_DrawXBMP(&u8g2, 0, 10, 16, 16, icon_o2pump_16x16);
    u8g2_SetFont(&u8g2, u8g2_font_5x8_tr);
    u8g2_DrawStr(&u8g2, 20, 16, line1);

    v_disp_work_draw_field_row(0U, 32, "MODE", tp_ui->pcO2PumpMode, false);
    sprintf(value, "%u%%/%u", tp_ui->usO2PumpSpeed / 10U, tp_ui->usO2PumpSpeed);
    v_disp_work_draw_field_row(1U, 40, "SPEED", value, false);
    v_disp_work_draw_field_row(2U, 48, "STATE", tp_ui->usO2PumpSpeed > 0U ? "RUN" : "STOP", false);
    v_disp_work_format_curr_ma(value, tp_ui->usO2CurrMa);
    v_disp_work_draw_field_row(3U, 56, "CURR", value, false);
    v_disp_work_draw_hint_line();
}

/***********************************************************************************************************************
-----函数功能    绘制设置页面
-----说明(备注)  从 tDispPageCtx.tSettingCache 读取设置项并绘制，支持编辑态显示与 DIRTY 状态标签
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_setting_page(void)
{
    char value[20];
    const char *tag = tDispPageCtx.tSettingCache.bDirty ? "DIRTY" : (tDispPageCtx.bEditing ? "EDIT" : "VIEW");

    vDisp_ClearRegion(0, 0, OLED_WIDTH_PIXELS, OLED_HEIGHT_PIXELS);
    v_disp_work_draw_full_top_bar("SET 2/3", tag);

    sprintf(value, "%us", tDispPageCtx.tSettingCache.usSleepTime);
    v_disp_work_draw_field_row(0U, 17, "SLEEP", value, tDispPageCtx.bEditing);
    sprintf(value, "%s", tDispPageCtx.tSettingCache.bBuzOff ? "OFF" : "ON");
    v_disp_work_draw_field_row(1U, 26, "BUZZER", value, tDispPageCtx.bEditing);
    sprintf(value, "%u", tDispPageCtx.tSettingCache.ucHighLightValue);
    v_disp_work_draw_field_row(2U, 35, "HIGH BL", value, tDispPageCtx.bEditing);
    sprintf(value, "%u", tDispPageCtx.tSettingCache.ucLowLightValue);
    v_disp_work_draw_field_row(3U, 44, "LOW BL", value, tDispPageCtx.bEditing);
    sprintf(value, "%s", tDispPageCtx.tSettingCache.bRestoreDefault ? "YES" : "NO");
    v_disp_work_draw_field_row(4U, 53, "RESET", value, tDispPageCtx.bEditing);
    v_disp_work_draw_hint_line();
}

/***********************************************************************************************************************
-----函数功能    绘制 ADC 页面（传感器/ADC 读数）
-----说明(备注)  根据 tDispPageCtx.ucAdcGroupIndex 选择显示温度组/电源组/灯光组的数据
-----传入参数    tp_ui: UI 快照指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_work_draw_adc_page(const DispUiSnapshot_T *tp_ui)
{
    char value[20];
    const char *tag = "TEMP";

    if(tDispPageCtx.ucAdcGroupIndex == 1U)
        tag = "PWR";
    else if(tDispPageCtx.ucAdcGroupIndex == 2U)
        tag = "LIGHT";

    vDisp_ClearRegion(0, 0, OLED_WIDTH_PIXELS, OLED_HEIGHT_PIXELS);
    v_disp_work_draw_full_top_bar("ADC 3/3", tag);

    switch(tDispPageCtx.ucAdcGroupIndex)
    {
        case 0U:
            sprintf(value, "%dC", tp_ui->sWaterTemp1);
            v_disp_work_draw_field_row(0U, 17, "WT1", value, false);
            sprintf(value, "%dC", tp_ui->sWaterTemp2);
            v_disp_work_draw_field_row(1U, 26, "WT2", value, false);
            sprintf(value, "%dC", tp_ui->sBoardTemp5V);
            v_disp_work_draw_field_row(2U, 35, "BOARD5", value, false);
            sprintf(value, "%dC", tp_ui->sBoardTemp12V);
            v_disp_work_draw_field_row(3U, 44, "BOARD12", value, false);
            sprintf(value, "%dC", tp_ui->sWaterTemp);
            v_disp_work_draw_field_row(4U, 53, "MAXT", value, false);
            break;

        case 1U:
            sprintf(value, "%u.%uV", tp_ui->us12VVolt / 10U, tp_ui->us12VVolt % 10U);
            v_disp_work_draw_field_row(0U, 17, "12V", value, false);
            sprintf(value, "%u.%uV", tp_ui->usVinVolt / 10U, tp_ui->usVinVolt % 10U);
            v_disp_work_draw_field_row(1U, 26, "VIN", value, false);
            v_disp_work_format_curr_ma(value, tp_ui->usVinCurrMa);
            v_disp_work_draw_field_row(2U, 35, "IIN", value, false);
            sprintf(value, "%uW", tp_ui->usVinPowerW);
            v_disp_work_draw_field_row(3U, 44, "PIN", value, false);
            sprintf(value, "%umA", tp_ui->usLightCurrMa);
            v_disp_work_draw_field_row(4U, 53, "LCURR", value, false);
            break;

        default:
            sprintf(value, "%u", tp_ui->usLightAdc);
            v_disp_work_draw_field_row(0U, 17, "LIGHT", value, false);
            sprintf(value, "%umA", tp_ui->usLightCurrMa);
            v_disp_work_draw_field_row(1U, 26, "LCURR", value, false);
            sprintf(value, "%u%%", tp_ui->usLightWarm / 10U);
            v_disp_work_draw_field_row(2U, 35, "WARM", value, false);
            sprintf(value, "%s", pc_disp_work_light_rgb_state(tp_ui));
            v_disp_work_draw_field_row(3U, 44, "RGB", value, false);
            sprintf(value, "%d/%dC", tp_ui->sBoardTemp5V, tp_ui->sBoardTemp12V);
            v_disp_work_draw_field_row(4U, 53, "BOARD", value, false);
            break;
    }

    v_disp_work_draw_hint_line();
}



