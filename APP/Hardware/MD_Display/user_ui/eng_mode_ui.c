/*******************************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS800\APP\Hardware\MD_Display\user_ui
 * File    : eng_mode_ui.c
 * Date    : 2026-06-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : 工程模式LVGL UI实现 - 主菜单/参数查看/记忆参数设置/系统设置
 * -------------------------------------------------------
 * todo    :
 * 1. 按键事件集成 (需要在key_task中调用vEngMode_KeyXxx)
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "MD_Display/user_ui/eng_mode_ui.h"
#include <stdbool.h>

#if(boardENG_MODE_EN && boardDISPLAY_EN)

#include <string.h>
#include <stdio.h>
#include "lvgl.h"
#include "Print/print_api.h"
#include "Middlewares/LVGL/src/stdlib/lv_mem.h"
#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_iface.h"
#include "MD_Display/md_display_eng_mode.h"
#include "MD_Display/eez_ui/fonts.h"
#include "MD_Display/eez_ui/screens.h"
#include "MD_Display/eez_ui/ui.h"
#endif
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_eng.h"
#include "app_info.h"

#if(boardADC_EN)
#include "Adc/adc_task.h"
#endif

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#endif

#if(boardDC_EN)
#include "Dc/dc_task.h"
#endif

#if(boardHEAT_MANAGE_EN)
#include "MD_HeatManage/md_hm_task.h"
#endif

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_task.h"
#endif

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_task.h"
#endif

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif


//****************************************************Macros*******************************************************************//

/* 颜色定义 */
#define ENG_CLR_BG              0x0F131A
#define ENG_CLR_CARD            0x1A202C
#define ENG_CLR_BORDER          0x2D3748
#define ENG_CLR_BMS             0x00E676
#define ENG_CLR_MPPT            0x00E5FF
#define ENG_CLR_DCAC            0xFFB300
#define ENG_CLR_SYS             0x718096
#define ENG_CLR_USB             0x9C27B0
#define ENG_CLR_DC              0xFF5722
#define ENG_CLR_ADC             0xE91E63
#define ENG_CLR_TEXT            0xFFFFFF
#define ENG_CLR_TEXT_SEC        0x718096
#define ENG_CLR_SELECTED        0x00E5FF
#define ENG_CLR_MENU_BG         0x1E293B
#define ENG_CLR_SEL_BG          0x0D2847

/* 布局常量 */
#define ENG_SCREEN_W            320
#define ENG_SCREEN_H            240
#define ENG_TITLE_H             24
#define ENG_TAB_BAR_H           18
#define ENG_CONTENT_Y           28
#define ENG_MENU_ITEM_H         48
#define ENG_MENU_ITEM_GAP       8
#define ENG_MAX_VIEW_ROWS       8
#define ENG_MAX_SET_ITEMS       13
#define ENG_NUM_VIEW_TABS       7
#define ENG_NUM_SET_TABS        7

/* 刷新周期: 50 * 10ms = 500ms */
#define ENG_DATA_REFRESH_CNT    50

/* 字体选择 */
#define ENG_FONT_TITLE          (&ui_font_barlow_condensed_regular_26)
#define ENG_FONT_NORMAL         LV_FONT_DEFAULT
#if LV_FONT_MONTSERRAT_12
#define ENG_FONT_SMALL          (&lv_font_montserrat_12)
#else
#define ENG_FONT_SMALL          LV_FONT_DEFAULT
#endif

/* Param Set页: 不可编辑项标记(只读/开关类型) */
#define PS_ITEM_READONLY        0xFF


//****************************************************类型定义************************************************//

/* 工程模式UI动作枚举(按键任务设置, 显示任务执行, 避免跨任务调用LVGL) */
typedef enum
{
    ENG_UI_ACTION_NONE = 0,     /* 无待执行动作 */
    ENG_UI_ACTION_MENU_SEL,     /* 主菜单选中项变化 */
    ENG_UI_ACTION_PV_TAB,       /* 参数查看Tab切换 */
    ENG_UI_ACTION_PS_TAB,       /* 参数设置Tab切换 */
    ENG_UI_ACTION_PS_ITEM,      /* 参数设置选中项变化 */
    ENG_UI_ACTION_SS_SEL,       /* 系统设置选中项变化 */
    ENG_UI_ACTION_CFM_SEL,      /* 确认对话框选中变化 */
} EngUiAction_E;

/* 工程模式UI状态 */
typedef struct
{
    EngModePage_E ePage;            /* 当前页面 */
    EngModePage_E ePrevPage;        /* 上一页面(用于确认对话框返回) */
    EngModePage_E ePendingPage;     /* 待切换页面(按键任务设置, 显示任务执行) */
    EngUiAction_E eUiAction;        /* 待执行的UI动作(按键任务设置, 显示任务执行) */
    uint8_t ucMainMenuSel;          /* 主菜单选中项 0-2 */
    uint8_t ucPvTab;                /* 参数查看当前Tab 0-6 */
    uint8_t ucPsTab;                /* 参数设置当前Tab 0-6 */
    uint8_t ucPsItem;               /* 参数设置当前选中参数 */
    uint8_t ucSsSel;                /* 系统设置选中项 0-2 */
    uint8_t ucConfirmSel;           /* 确认对话框选中 0=取消 1=确认 */
    uint8_t ucTickCnt;              /* 刷新计数器 */
    bool bExitReq;                  /* 退出请求 */
    bool bNeedRefresh;              /* 需要数据刷新 */
    bool bPsEditing;                /* 参数设置编辑模式(true=调值, false=选项目) */
} EngModeState_T;


//****************************************************LVGL对象存储*********************************************//

typedef struct
{
    lv_obj_t *p_base;               /* 基础容器 */

    /* 标题栏 */
    lv_obj_t *p_title_bar;          /* 标题栏容器 */
    lv_obj_t *p_title_label;        /* 标题文本 */

    /* 主菜单页面 */
    lv_obj_t *p_menu_page;
    lv_obj_t *p_menu_items[3];
    lv_obj_t *p_menu_labels[3];
    lv_obj_t *p_menu_sub_labels[3];

    /* 参数查看页面 */
    lv_obj_t *p_pv_page;
    lv_obj_t *p_pv_tab_title;
    lv_obj_t *p_pv_idx_label;
    lv_obj_t *p_pv_rows[ENG_MAX_VIEW_ROWS];
    lv_obj_t *p_pv_lbl_l[ENG_MAX_VIEW_ROWS];
    lv_obj_t *p_pv_lbl_r[ENG_MAX_VIEW_ROWS];
    lv_obj_t *p_pv_tab_bar;
    lv_obj_t *p_pv_tab_lbl[ENG_NUM_VIEW_TABS];

    /* 参数设置页面 */
    lv_obj_t *p_ps_page;
    lv_obj_t *p_ps_tab_title;
    lv_obj_t *p_ps_idx_label;
    lv_obj_t *p_ps_list;
    lv_obj_t *p_ps_items[ENG_MAX_SET_ITEMS];
    lv_obj_t *p_ps_lbl_n[ENG_MAX_SET_ITEMS];
    lv_obj_t *p_ps_lbl_v[ENG_MAX_SET_ITEMS];
    lv_obj_t *p_ps_tab_bar;
    lv_obj_t *p_ps_tab_lbl[ENG_NUM_SET_TABS];

    /* 系统设置页面 */
    lv_obj_t *p_ss_page;
    lv_obj_t *p_ss_items[3];
    lv_obj_t *p_ss_lbl_title[3];
    lv_obj_t *p_ss_lbl_desc[3];

    /* 确认对话框 */
    lv_obj_t *p_cfm_page;
    lv_obj_t *p_cfm_box;
    lv_obj_t *p_cfm_text;
    lv_obj_t *p_cfm_btns[2];
    lv_obj_t *p_cfm_lbls[2];
} EngModeObjs_T;


//****************************************************静态变量**************************************************//
static EngModeState_T S_tState;
static EngModeObjs_T  S_tObjs;

/* 参数查看Tab名称 */
static const char *S_apcPvTabNames[] = { "BMS", "MPPT", "DCAC", "USB", "DC", "ADC", "SYS" };
static const uint32_t S_aulPvTabClr[] = {
    ENG_CLR_BMS, ENG_CLR_MPPT, ENG_CLR_DCAC, ENG_CLR_USB, ENG_CLR_DC, ENG_CLR_ADC, ENG_CLR_SYS
};

/* 参数设置Tab名称 */
static const char *S_apcPsTabNames[] = { "SYS", "LCD", "BAT", "MPPT", "DCAC", "USB", "DC" };
static const uint32_t S_aulPsTabClr[] = {
    ENG_CLR_SYS, ENG_CLR_SELECTED, ENG_CLR_BMS, ENG_CLR_MPPT, ENG_CLR_DCAC, ENG_CLR_USB, ENG_CLR_DC
};

/* 参数设置Tab -> EMS步骤映射 */
static const uint8_t S_aucPsTabToEms[] = { EMS_SYS, EMS_LCD, EMS_BAT, EMS_MPPT, EMS_DCAC, EMS_USB, EMS_DC };

/* 主菜单项颜色 */
static const uint32_t S_aulMenuClr[] = { ENG_CLR_BMS, ENG_CLR_DCAC, ENG_CLR_SELECTED };
/* 主菜单标题 */
static const char *S_apcMenuTitle[] = { "PARAM VIEW", "PARAM SET", "SYS SET" };
/* 主菜单副标题 */
static const char *S_apcMenuSub[] = { "Realtime telemetry", "Memory param config", "Save/Reset/Upgrade" };
/* 系统设置标题 */
static const char *S_apcSsTitle[] = { "SAVE & EXIT", "RESET DEFAULTS", "FIRMWARE UPDATE" };
/* 系统设置描述 */
static const char *S_apcSsDesc[] = {
    "Save params & exit eng mode",
    "Restore factory defaults",
    "Jump to bootloader upgrade"
};
/* 确认对话框文本 */
static const char *S_apcCfmText[] = {
    "Confirm save & exit?",
    "Confirm reset defaults?",
    "Confirm firmware update?"
};


//****************************************************函数声明**************************************************//
static void v_page_create_menu(void);
static void v_page_create_pv(void);
static void v_page_create_ps(void);
static void v_page_create_ss(void);
static void v_page_create_cfm(void);
static void v_page_delete_menu(void);
static void v_page_delete_pv(void);
static void v_page_delete_ps(void);
static void v_page_delete_ss(void);
static void v_page_delete_cfm(void);
static void v_page_show(EngModePage_E e_page);
static void v_pv_update_data(void);
static void v_ps_update_data(void);
static void v_ps_update_selection(void);
static void v_ss_update_selection(void);
static void v_cfm_update_selection(void);


//****************************************************辅助函数**************************************************//

/***********************************************************************************************************************
 -----函数功能    创建面板容器
 -----说明(备注)  DispTask上下文: 在p_parent上创建lv_obj, 设置位置/尺寸/背景色+透明覆盖+无边框+圆角6+无内边距;
				  用于菜单项/参数项/系统设置项等容器
 -----传入参数    p_parent: 父对象
				  x, y: 相对父对象的坐标
				  w, h: 面板宽高
				  ul_bg: 背景色(24bit RGB)
 -----输出参数    none
 -----返回值      创建的lv_obj_t对象指针
 ************************************************************************************************************************/
static lv_obj_t *p_create_panel(lv_obj_t *p_parent, lv_coord_t x, lv_coord_t y,
                                 lv_coord_t w, lv_coord_t h, uint32_t ul_bg)
{
    lv_obj_t *p_obj = lv_obj_create(p_parent);
    lv_obj_set_pos(p_obj, x, y);
    lv_obj_set_size(p_obj, w, h);
    lv_obj_remove_flag(p_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(p_obj, lv_color_hex(ul_bg), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(p_obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(p_obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(p_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return p_obj;
}

/***********************************************************************************************************************
 -----函数功能    创建标签
 -----说明(备注)  DispTask上下文: 在p_parent上创建lv_label, 设置位置+内容自适应+字体+颜色+初始空文本;
				  用于菜单标题/数据/参数等所有文本展示
 -----传入参数    p_parent: 父对象
				  x, y: 相对父对象的坐标
				  p_font: 字体指针
				  ul_color: 文本颜色(24bit RGB)
 -----输出参数    none
 -----返回值      创建的lv_obj_t对象指针
 ************************************************************************************************************************/
static lv_obj_t *p_create_label(lv_obj_t *p_parent, lv_coord_t x, lv_coord_t y,
                                 const lv_font_t *p_font, uint32_t ul_color)
{
    lv_obj_t *p_obj = lv_label_create(p_parent);
    lv_obj_set_pos(p_obj, x, y);
    lv_obj_set_size(p_obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(p_obj, p_font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(p_obj, lv_color_hex(ul_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(p_obj, "");
    return p_obj;
}


//****************************************************主菜单页面************************************************//

/***********************************************************************************************************************
 -----函数功能    创建主菜单页面
 -----说明(备注)  DispTask上下文: 在S_tObjs.p_base上创建3个菜单项面板及标题/副标题;
				  S_tObjs.p_menu_page首次为NULL时调用; 创建后由v_menu_update_sel绘制选中态
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_page_create_menu(void)
{
    uint8_t i;
    lv_obj_t *p_page = lv_obj_create(S_tObjs.p_base);
    lv_obj_set_pos(p_page, 0, 0);
    lv_obj_set_size(p_page, ENG_SCREEN_W, ENG_SCREEN_H);
    lv_obj_remove_flag(p_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(p_page, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p_page, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(p_page, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    S_tObjs.p_menu_page = p_page;

    for(i = 0; i < 3; i++)
    {
        lv_coord_t y = ENG_CONTENT_Y + i * (ENG_MENU_ITEM_H + ENG_MENU_ITEM_GAP);

        /* 菜单项面板 */
        lv_obj_t *p_item = p_create_panel(p_page, 8, y, ENG_SCREEN_W - 16, ENG_MENU_ITEM_H, ENG_CLR_MENU_BG);
        S_tObjs.p_menu_items[i] = p_item;

        /* 标题 */
        S_tObjs.p_menu_labels[i] = p_create_label(p_item, 12, 4, ENG_FONT_NORMAL, S_aulMenuClr[i]);
        lv_label_set_text(S_tObjs.p_menu_labels[i], S_apcMenuTitle[i]);

        /* 副标题 */
        S_tObjs.p_menu_sub_labels[i] = p_create_label(p_item, 12, 24, ENG_FONT_SMALL, ENG_CLR_TEXT_SEC);
        lv_label_set_text(S_tObjs.p_menu_sub_labels[i], S_apcMenuSub[i]);
    }
}

/***********************************************************************************************************************
 -----函数功能    更新主菜单选中项样式
 -----说明(备注)  DispTask上下文: 按S_tState.ucMainMenuSel高亮对应项(蓝边+深底);
				  由vEngMode_UiTick通过ENG_UI_ACTION_MENU_SEL动作触发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_menu_update_sel(void)
{
    uint8_t i;
    for(i = 0; i < 3; i++)
    {
        if(i == S_tState.ucMainMenuSel)
        {
            lv_obj_set_style_bg_color(S_tObjs.p_menu_items[i], lv_color_hex(ENG_CLR_SEL_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(S_tObjs.p_menu_items[i], 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(S_tObjs.p_menu_items[i], lv_color_hex(ENG_CLR_SELECTED), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(S_tObjs.p_menu_items[i], LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_obj_set_style_bg_color(S_tObjs.p_menu_items[i], lv_color_hex(ENG_CLR_MENU_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(S_tObjs.p_menu_items[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}


//****************************************************参数查看页面************************************************//

/***********************************************************************************************************************
 -----函数功能    创建参数查看(PV)页面
 -----说明(备注)  DispTask上下文: 在S_tObjs.p_base上创建Tab标题+8行数据+7Tab底部栏;
				  S_tObjs.p_pv_page首次为NULL时调用; 数据更新由v_pv_update_data负责
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_page_create_pv(void)
{
    uint8_t i;
    lv_obj_t *p_page = lv_obj_create(S_tObjs.p_base);
    lv_obj_set_pos(p_page, 0, 0);
    lv_obj_set_size(p_page, ENG_SCREEN_W, ENG_SCREEN_H);
    lv_obj_remove_flag(p_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(p_page, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p_page, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(p_page, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    S_tObjs.p_pv_page = p_page;

    /* Tab标题 (中央) */
    S_tObjs.p_pv_tab_title = p_create_label(p_page, 0, 4, ENG_FONT_TITLE, ENG_CLR_TEXT);
    lv_obj_set_style_align(S_tObjs.p_pv_tab_title, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(S_tObjs.p_pv_tab_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Tab索引 (右侧) */
    S_tObjs.p_pv_idx_label = p_create_label(p_page, ENG_SCREEN_W - 40, 4, ENG_FONT_SMALL, ENG_CLR_TEXT_SEC);

    /* 数据行 */
    for(i = 0; i < ENG_MAX_VIEW_ROWS; i++)
    {
        lv_coord_t y = ENG_CONTENT_Y + 4 + i * 24;
        S_tObjs.p_pv_lbl_l[i] = p_create_label(p_page, 8, y, ENG_FONT_NORMAL, ENG_CLR_TEXT_SEC);
        S_tObjs.p_pv_lbl_r[i] = p_create_label(p_page, 164, y, ENG_FONT_NORMAL, ENG_CLR_TEXT);
    }

    /* 底部Tab栏 */
    S_tObjs.p_pv_tab_bar = lv_obj_create(p_page);
    lv_obj_set_pos(S_tObjs.p_pv_tab_bar, 0, ENG_SCREEN_H - ENG_TAB_BAR_H);
    lv_obj_set_size(S_tObjs.p_pv_tab_bar, ENG_SCREEN_W, ENG_TAB_BAR_H);
    lv_obj_remove_flag(S_tObjs.p_pv_tab_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(S_tObjs.p_pv_tab_bar, lv_color_hex(ENG_CLR_CARD), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(S_tObjs.p_pv_tab_bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(S_tObjs.p_pv_tab_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(S_tObjs.p_pv_tab_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(S_tObjs.p_pv_tab_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    for(i = 0; i < ENG_NUM_VIEW_TABS; i++)
    {
        lv_coord_t x = (lv_coord_t)(i * (ENG_SCREEN_W / ENG_NUM_VIEW_TABS));
        lv_coord_t w = (lv_coord_t)(ENG_SCREEN_W / ENG_NUM_VIEW_TABS);
        S_tObjs.p_pv_tab_lbl[i] = p_create_label(S_tObjs.p_pv_tab_bar,
            x + 2, 2, ENG_FONT_SMALL, ENG_CLR_TEXT_SEC);
        lv_obj_set_width(S_tObjs.p_pv_tab_lbl[i], w - 4);
        lv_obj_set_style_text_align(S_tObjs.p_pv_tab_lbl[i], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(S_tObjs.p_pv_tab_lbl[i], S_apcPvTabNames[i]);
    }
}

/***********************************************************************************************************************
 -----函数功能    切换参数查看Tab
 -----说明(备注)  DispTask上下文: 更新S_tState.ucPvTab+Tab标题颜色+Tab栏高亮+索引文本+刷新数据;
				  越界uc_tab直接返回; 由vEngMode_UiTick通过ENG_UI_ACTION_PV_TAB动作触发
 -----传入参数    uc_tab: 目标Tab索引(0~ENG_NUM_VIEW_TABS-1)
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_pv_switch_tab(uint8_t uc_tab)
{
    if(uc_tab >= ENG_NUM_VIEW_TABS)
        return;

    S_tState.ucPvTab = uc_tab;

    /* 更新Tab标题颜色和文本 */
    lv_obj_set_style_text_color(S_tObjs.p_pv_tab_title,
        lv_color_hex(S_aulPvTabClr[uc_tab]), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(S_tObjs.p_pv_tab_title, S_apcPvTabNames[uc_tab]);

    /* 更新Tab索引 */
    char buf[8];
    snprintf(buf, sizeof(buf), "%u/%u", uc_tab + 1, ENG_NUM_VIEW_TABS);
    lv_label_set_text(S_tObjs.p_pv_idx_label, buf);

    /* 更新Tab栏高亮 */
    uint8_t i;
    for(i = 0; i < ENG_NUM_VIEW_TABS; i++)
    {
        if(i == uc_tab)
        {
            lv_obj_set_style_text_color(S_tObjs.p_pv_tab_lbl[i],
                lv_color_hex(S_aulPvTabClr[i]), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_obj_set_style_text_color(S_tObjs.p_pv_tab_lbl[i],
                lv_color_hex(ENG_CLR_TEXT_SEC), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    /* 更新数据 */
    S_tState.bNeedRefresh = true;
    v_pv_update_data();
}

/***********************************************************************************************************************
 -----函数功能    设置参数查看页单行数据
 -----说明(备注)  DispTask上下文: 设置左右标签文本+左侧颜色+清除隐藏标志;
				  越界uc_row直接返回
 -----传入参数    uc_row: 行号(0~ENG_MAX_VIEW_ROWS-1)
				  pc_left: 左侧名称字符串
				  pc_right: 右侧数值字符串
				  ul_accent: 左侧标签颜色(24bit RGB)
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_pv_set_row(uint8_t uc_row, const char *pc_left, const char *pc_right, uint32_t ul_accent)
{
    if(uc_row >= ENG_MAX_VIEW_ROWS)
        return;

    lv_obj_clear_flag(S_tObjs.p_pv_lbl_l[uc_row], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(S_tObjs.p_pv_lbl_r[uc_row], LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(S_tObjs.p_pv_lbl_l[uc_row], pc_left);
    lv_label_set_text(S_tObjs.p_pv_lbl_r[uc_row], pc_right);
    lv_obj_set_style_text_color(S_tObjs.p_pv_lbl_l[uc_row],
        lv_color_hex(ul_accent), LV_PART_MAIN | LV_STATE_DEFAULT);
}

/***********************************************************************************************************************
 -----函数功能    清空参数查看页指定行之后的内容
 -----说明(备注)  DispTask上下文: 将uc_from及之后所有行的左右标签文本置空;
				  用于不同Tab数据行数不同时清理多余行
 -----传入参数    uc_from: 起始行(0~ENG_MAX_VIEW_ROWS-1)
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_pv_hide_rows(uint8_t uc_from)
{
    uint8_t i;
    for(i = uc_from; i < ENG_MAX_VIEW_ROWS; i++)
    {
        lv_label_set_text(S_tObjs.p_pv_lbl_l[i], "");
        lv_label_set_text(S_tObjs.p_pv_lbl_r[i], "");
    }
}

/***********************************************************************************************************************
 -----函数功能    刷新参数查看页当前Tab数据
 -----说明(备注)  DispTask上下文: 按S_tState.ucPvTab读取BMS/MPPT/DCAC/USB/DC/ADC/SYS遥测并填入行;
				  由vEngMode_UiTick通过bNeedRefresh周期触发或切Tab时立即触发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_pv_update_data(void)
{
    char buf_l[20], buf_r[24];
    uint32_t ul_accent = S_aulPvTabClr[S_tState.ucPvTab];

    switch(S_tState.ucPvTab)
    {
        #if(boardBMS_EN)
        case 0: /* BMS */
        {
            snprintf(buf_l, sizeof(buf_l), "ErrCode");
            snprintf(buf_r, sizeof(buf_r), "0x%x", tBms.uErrCode.ullCode);
            v_pv_set_row(0, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Permit");
            snprintf(buf_r, sizeof(buf_r), "0x%X", tBms.uPerm.ucPerm);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Online");
            {
                const char *p_st = "NULL";
                if(tBms.eWorkState == BWS_DISCHG) p_st = "DISG";
                else if(tBms.eWorkState == BWS_CHG) p_st = "CHG";
                snprintf(buf_r, sizeof(buf_r), "%u St:%s", tBmsRx.tDevNum.ucOnlineNum, p_st);
            }
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "MaxTemp");
            snprintf(buf_r, sizeof(buf_r), "%d C", tBms.sMaxTemp);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "MinTemp");
            snprintf(buf_r, sizeof(buf_r), "%d C", tBms.sMinTemp);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "TotalCurr");
            snprintf(buf_r, sizeof(buf_r), "%.2fA", tBmsRx.sTotalCurr * 0.01f);
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "PermMaxChgPwr");
            snprintf(buf_r, sizeof(buf_r), "%dW", tBmsRx.usPermMaxChgPwr);
            v_pv_set_row(6, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "PermMaxDisChgPwr");
            snprintf(buf_r, sizeof(buf_r), "%dW", tBmsRx.usPermMaxDisChgPwr);
            v_pv_set_row(7, buf_l, buf_r, ul_accent);

            v_pv_hide_rows(8);
        }break;
        #endif  /* boardBMS_EN */

        #if(boardMPPT_EN)
        case 1: /* MPPT */
        {
            snprintf(buf_l, sizeof(buf_l), "ErrCode");
            snprintf(buf_r, sizeof(buf_r), "0x%X", tMppt.uErrCode.ulCode);
            v_pv_set_row(0, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "ChgPermit");
            snprintf(buf_r, sizeof(buf_r), "0x%d", tMppt.bChgPerm);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "InVolt");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tMpptRx.usInVolt * 0.1f);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "InCurr");
            snprintf(buf_r, sizeof(buf_r), "%.2fA", tMpptRx.usInCurr * 0.01f);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "InPwr");
            snprintf(buf_r, sizeof(buf_r), "%.1fW", tMpptRx.usInPwr * 0.1f);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "MaxTemp");
            snprintf(buf_r, sizeof(buf_r), "%dC", tMpptRx.sMaxTemp);
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "InType");
            snprintf(buf_r, sizeof(buf_r), "T:%u C:%u",
                (uint8_t)tMpptRx.uInType, (uint8_t)tMppt.bChgPerm);
            v_pv_set_row(6, buf_l, buf_r, ul_accent);
            
            v_pv_hide_rows(7);
        }break;
        #endif  /* boardMPPT_EN */

        #if(boardDCAC_EN)
        case 2: /* DCAC */
        {
            snprintf(buf_l, sizeof(buf_l), "V_ac_in");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tDcacRx.usInVolt * 0.1f);
            v_pv_set_row(0, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "I_ac_in");
            snprintf(buf_r, sizeof(buf_r), "%.1fA", tDcacRx.usInCurr * 0.1f);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "P_ac_in");
            snprintf(buf_r, sizeof(buf_r), "%uW %.1fHz", tDcacRx.usInPwr, tDcacRx.usInFreq * 0.1f);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "V_ac_out");
            snprintf(buf_r, sizeof(buf_r), "%.1fV %.1fA",
                tDcacRx.usOutVolt * 0.1f, tDcacRx.usOutCurr * 0.1f);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "P_ac_out");
            snprintf(buf_r, sizeof(buf_r), "%uW %.1fHz", tDcacRx.usOutPwr, tDcacRx.usOutFreq * 0.1f);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "ChgSt");
            {
                const char *p_c = (tDcac.eChgState == IOS_WORK) ? "CHG" : "OFF";
                const char *p_d = (tDcac.eDisChgState == IOS_WORK) ? "DISG" : "OFF";
                snprintf(buf_r, sizeof(buf_r), "C:%s D:%s", p_c, p_d);
            }
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Temp");
            snprintf(buf_r, sizeof(buf_r), "H:%dC L:%dC", tDcacRx.sMaxTemp, tDcacRx.sMinTemp);
            v_pv_set_row(6, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "State");
            snprintf(buf_r, sizeof(buf_r), "0x%04X E:0x%04X",
                tDcacRx.uState.usState, tDcacRx.uErrCode.usCode[0]);
            v_pv_set_row(7, buf_l, buf_r, ul_accent);

            v_pv_hide_rows(8);
        }break;
        #endif  /* boardDCAC_EN */

        #if(boardUSB_EN)
        case 3: /* USB */
        {
            snprintf(buf_l, sizeof(buf_l), "V_in");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tUsb.usInVolt * 0.1f);
            v_pv_set_row(0, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "I_in");
            snprintf(buf_r, sizeof(buf_r), "%.1fA", tUsb.usInCurr * 0.1f);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "P_out");
            snprintf(buf_r, sizeof(buf_r), "%uW", tUsb.usOutPwr);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "P_wc/pd");
            snprintf(buf_r, sizeof(buf_r), "%uW/%uW", tUsb.usWcPwr, tUsb.usPdPwr);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Temp");
            snprintf(buf_r, sizeof(buf_r), "%d C", tUsb.sMaxTemp);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "State");
            snprintf(buf_r, sizeof(buf_r), "St:%u E:0x%02X",
                (uint8_t)tUsb.eDevState, tUsb.uErrCode.ucErrCode);
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            v_pv_hide_rows(6);
        }break;
        #endif  /* boardUSB_EN */

        #if(boardDC_EN) 
        case 4: /* DC */
        {
            snprintf(buf_l, sizeof(buf_l), "V_in");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tDc.usInVolt * 0.1f);
            v_pv_set_row(0, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "I_in");
            snprintf(buf_r, sizeof(buf_r), "%.1fA", tDc.usInCurr * 0.1f);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "V_out");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tDc.usOutVolt * 0.1f);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "I_out");
            snprintf(buf_r, sizeof(buf_r), "%.1fA", tDc.usOutCurr * 0.1f);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "P_out");
            snprintf(buf_r, sizeof(buf_r), "%uW", tDc.usOutPwr);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Temp");
            snprintf(buf_r, sizeof(buf_r), "%d C", tDc.sMaxTemp);
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "State");
            snprintf(buf_r, sizeof(buf_r), "St:%u E:0x%02X",
                (uint8_t)tDc.eDevState, tDc.uErrCode.ucErrCode);
            v_pv_set_row(6, buf_l, buf_r, ul_accent);

            v_pv_hide_rows(7);
        }break;
        #endif  /* boardDC_EN */

        #if(boardADC_EN)
        case 5: /* ADC */
        {
            snprintf(buf_l, sizeof(buf_l), "V_sys");
            snprintf(buf_r, sizeof(buf_r), "%.2fV", tAdcSamp.usSysInVolt * 0.01f);
            v_pv_set_row(0, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "KeyPwr");
            snprintf(buf_r, sizeof(buf_r), "%u AD", tAdcSamp.usKeyPower);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "DC_Temp");
            snprintf(buf_r, sizeof(buf_r), "%d C", tAdcSamp.sDcOutTemp);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "DC_Curr");
            snprintf(buf_r, sizeof(buf_r), "%.2fA", (double)tAdcSamp.fDcOutCurr);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "DC_Vout");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tAdcSamp.usDcOutVolt * 0.1f);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "DC_Vin1");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tAdcSamp.usDcIn1Volt * 0.1f);
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "DC_Vin2");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tAdcSamp.usDcIn2Volt * 0.1f);
            v_pv_set_row(6, buf_l, buf_r, ul_accent);

            v_pv_hide_rows(7);
        }break;
        #endif  /* boardADC_EN */

        case 6: /* SYS */
        {
            snprintf(buf_l, sizeof(buf_l), "Version");
            snprintf(buf_r, sizeof(buf_r), "%s", tAppMemParam.tVerInfo.saVersion);
            v_pv_set_row(0, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "BuildDate");
            snprintf(buf_r, sizeof(buf_r), "%s", tAppMemParam.tVerInfo.saBuildDate);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "ErrCode");
            snprintf(buf_r, sizeof(buf_r), "0x%04X", tSysInfo.uErrCode.usCode);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "MaxTemp");
            snprintf(buf_r, sizeof(buf_r), "%d C", tSysInfo.sMaxTemp);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "MinTemp");
            snprintf(buf_r, sizeof(buf_r), "%d C", tSysInfo.sMinTemp);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "BoardTempMax");
            snprintf(buf_r, sizeof(buf_r), "%d C", tSysInfo.sBoardTempMax);
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            v_pv_hide_rows(6);
        }break;

        default:
            v_pv_hide_rows(0);
            break;
    }
}


//****************************************************参数设置页面************************************************//

/* 各Tab参数名称 */
static const char *S_apcPsSysNames[] = { "Version", "FanCtrl", "AutoOff", "MaxTemp", "MinTemp", "MinOpenV", "Buzzer" };
static const char *S_apcPsLcdNames[] = { "HighLight", "LowLight", "AutoOff" };
static const char *S_apcPsBatNames[] = { "ChgMaxT", "DisChgMaxT", "ChgMinT", "DisChgMinT", "MaxVolt", "MinVolt" };
static const char *S_apcPsMpptNames[] = { "AutoOff", "MaxTemp", "MaxInV", "MinInV", "InPwrRat" };
static const char *S_apcPsDcacNames[] = { "AutoOff", "MinOpenV", "VoltRat", "MaxInV", "MinInV",
    "InPwrRat", "MinInPwr", "MaxInCurr", "OutPwrRat", "OverLoad", "ParaInPwr", "AcOutFreq", "MaxTemp" };
static const char *S_apcPsUsbNames[] = { "AutoOff", "MaxInV", "MinInV", "MinOpenV", "MaxTemp" };
static const char *S_apcPsDcNames[] = { "AutoOff", "MaxOutV", "MinOutV", "OverLoad", "MinOpenV", "MaxTemp" };

/* 各Tab参数数量 */
static const uint8_t S_aucPsItemCount[] = { 7, 3, 6, 5, 13, 5, 6 };

/***********************************************************************************************************************
 -----函数功能    创建记忆参数设置(PS)页面
 -----说明(备注)  DispTask上下文: 在S_tObjs.p_base上创建Tab标题+可滚动参数列表+7Tab底部栏;
				  S_tObjs.p_ps_page首次为NULL时调用; 数据由v_ps_update_data填充
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
static void v_page_create_ps(void)
{
    uint8_t i;
    lv_obj_t *p_page = lv_obj_create(S_tObjs.p_base);
    lv_obj_set_pos(p_page, 0, 0);
    lv_obj_set_size(p_page, ENG_SCREEN_W, ENG_SCREEN_H);
    lv_obj_remove_flag(p_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(p_page, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p_page, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(p_page, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    S_tObjs.p_ps_page = p_page;

    /* Tab标题 */
    S_tObjs.p_ps_tab_title = p_create_label(p_page, 0, 4, ENG_FONT_TITLE, ENG_CLR_TEXT);
    lv_obj_set_style_align(S_tObjs.p_ps_tab_title, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(S_tObjs.p_ps_tab_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Tab索引 */
    S_tObjs.p_ps_idx_label = p_create_label(p_page, ENG_SCREEN_W - 40, 4, ENG_FONT_SMALL, ENG_CLR_TEXT_SEC);

    /* 可滚动参数列表 */
    lv_obj_t *p_list = lv_obj_create(p_page);
    lv_obj_set_pos(p_list, 4, ENG_CONTENT_Y);
    lv_obj_set_size(p_list, ENG_SCREEN_W - 8, ENG_SCREEN_H - ENG_CONTENT_Y - ENG_TAB_BAR_H - 2);
    lv_obj_set_style_bg_opa(p_list, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(p_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scroll_dir(p_list, LV_DIR_VER);
    S_tObjs.p_ps_list = p_list;

    for(i = 0; i < ENG_MAX_SET_ITEMS; i++)
    {
        lv_coord_t y = (lv_coord_t)(i * 26);

        /* 参数项面板 */
        lv_obj_t *p_item = p_create_panel(p_list, 0, y, ENG_SCREEN_W - 12, 24, ENG_CLR_MENU_BG);
        lv_obj_set_style_radius(p_item, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        S_tObjs.p_ps_items[i] = p_item;

        /* 参数名 */
        S_tObjs.p_ps_lbl_n[i] = p_create_label(p_item, 6, 3, ENG_FONT_NORMAL, ENG_CLR_TEXT_SEC);
        lv_label_set_text(S_tObjs.p_ps_lbl_n[i], "");

        /* 参数值 */
        S_tObjs.p_ps_lbl_v[i] = p_create_label(p_item, 170, 3, ENG_FONT_NORMAL, ENG_CLR_TEXT);
        lv_label_set_text(S_tObjs.p_ps_lbl_v[i], "");
    }

    /* 底部Tab栏 */
    S_tObjs.p_ps_tab_bar = lv_obj_create(p_page);
    lv_obj_set_pos(S_tObjs.p_ps_tab_bar, 0, ENG_SCREEN_H - ENG_TAB_BAR_H);
    lv_obj_set_size(S_tObjs.p_ps_tab_bar, ENG_SCREEN_W, ENG_TAB_BAR_H);
    lv_obj_remove_flag(S_tObjs.p_ps_tab_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(S_tObjs.p_ps_tab_bar, lv_color_hex(ENG_CLR_CARD), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(S_tObjs.p_ps_tab_bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(S_tObjs.p_ps_tab_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(S_tObjs.p_ps_tab_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(S_tObjs.p_ps_tab_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    for(i = 0; i < ENG_NUM_SET_TABS; i++)
    {
        lv_coord_t x = (lv_coord_t)(i * (ENG_SCREEN_W / ENG_NUM_SET_TABS));
        lv_coord_t w = (lv_coord_t)(ENG_SCREEN_W / ENG_NUM_SET_TABS);
        S_tObjs.p_ps_tab_lbl[i] = p_create_label(S_tObjs.p_ps_tab_bar,
            x + 2, 2, ENG_FONT_SMALL, ENG_CLR_TEXT_SEC);
        lv_obj_set_width(S_tObjs.p_ps_tab_lbl[i], w - 4);
        lv_obj_set_style_text_align(S_tObjs.p_ps_tab_lbl[i], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(S_tObjs.p_ps_tab_lbl[i], S_apcPsTabNames[i]);
    }
}

static void v_ps_switch_tab(uint8_t uc_tab)
{
/***********************************************************************************************************************
 -----函数功能    切换记忆参数设置Tab
 -----说明(备注)  DispTask上下文: 更新S_tState.ucPsTab+选中项清零+Tab标题+索引+高亮+后端tEngMode同步+刷新数据;
				  越界uc_tab直接返回; 由vEngMode_UiTick通过ENG_UI_ACTION_PS_TAB动作触发
 -----传入参数    uc_tab: 目标Tab索引(0~ENG_NUM_SET_TABS-1)
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    if(uc_tab >= ENG_NUM_SET_TABS)
        return;

    S_tState.ucPsTab = uc_tab;
    S_tState.ucPsItem = 0;
    S_tState.bPsEditing = false;

    /* 更新Tab标题 */
    lv_obj_set_style_text_color(S_tObjs.p_ps_tab_title,
        lv_color_hex(S_aulPsTabClr[uc_tab]), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(S_tObjs.p_ps_tab_title, S_apcPsTabNames[uc_tab]);

    /* 更新Tab索引 */
    char buf[8];
    snprintf(buf, sizeof(buf), "%u/%u", uc_tab + 1, ENG_NUM_SET_TABS);
    lv_label_set_text(S_tObjs.p_ps_idx_label, buf);

    /* 更新Tab栏高亮 */
    uint8_t i;
    for(i = 0; i < ENG_NUM_SET_TABS; i++)
    {
        if(i == uc_tab)
            lv_obj_set_style_text_color(S_tObjs.p_ps_tab_lbl[i],
                lv_color_hex(S_aulPsTabClr[i]), LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(S_tObjs.p_ps_tab_lbl[i],
                lv_color_hex(ENG_CLR_TEXT_SEC), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    /* 同步到后端tEngMode */
    tpSysTask->ucStep = S_aucPsTabToEms[uc_tab];
    tEngMode.ucEngModeItem = 0;
    tEngMode.cEngModeState = 0;

    v_ps_update_data();
}

static void v_ps_get_value_str(uint8_t uc_tab, uint8_t uc_item, char *pc_buf, uint8_t uc_size)
{
/***********************************************************************************************************************
 -----函数功能    获取记忆参数项的当前显示字符串
 -----说明(备注)  DispTask上下文: 按uc_tab+uc_item读取对应模块(tSYS/tDISP/tBMS/tMPPT/tDCAC/tUSB/tDC)的值;
				  通过snprintf写入pc_buf; 无匹配项时写入"-"
 -----传入参数    uc_tab: 参数Tab索引(0~ENG_NUM_SET_TABS-1)
				  uc_item: Tab内参数索引
				  pc_buf: 输出字符串缓冲区
				  uc_size: 缓冲区大小
 -----输出参数    pc_buf: 填入格式化后的参数字符串
 -----返回值      none
 ************************************************************************************************************************/
    switch(uc_tab)
    {
        case 0: /* SYS */
            switch(uc_item)
            {
                case 0: snprintf(pc_buf, uc_size, "%s", tAppMemParam.tVerInfo.saVersion + 10); break;
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
                case 2: snprintf(pc_buf, uc_size, "%u min", tAppMemParam.tSYS.usAutoOffTime); break;
                case 3: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tSYS.sMaxTemp); break;
                case 4: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tSYS.sMinTemp); break;
                case 5: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tSYS.usMinOpenVolt); break;
                case 6: snprintf(pc_buf, uc_size, "%s", tAppMemParam.tSYS.bBuzSwitchOff ? "OFF" : "ON"); break;
                default: snprintf(pc_buf, uc_size, "-"); break;
            }
            break;

        case 1: /* LCD */
            switch(uc_item)
            {
                case 0: snprintf(pc_buf, uc_size, "0x%02X", tAppMemParam.tDISP.ucHighLightValue); break;
                case 1: snprintf(pc_buf, uc_size, "0x%02X", tAppMemParam.tDISP.ucLowLightValue); break;
                case 2: snprintf(pc_buf, uc_size, "%u min", tAppMemParam.tDISP.usAutoOffTime); break;
                default: snprintf(pc_buf, uc_size, "-"); break;
            }
            break;

#if(boardBMS_EN)
        case 2: /* BAT */
            switch(uc_item)
            {
                case 0: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tBMS.cChgMaxTemp); break;
                case 1: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tBMS.cDisChgMaxTemp); break;
                case 2: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tBMS.cChgMinTemp); break;
                case 3: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tBMS.cDisChgMinTemp); break;
                case 4: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tBMS.usMaxVolt); break;
                case 5: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tBMS.usMinVolt); break;
                default: snprintf(pc_buf, uc_size, "-"); break;
            }
            break;
#endif

#if(boardMPPT_EN)
        case 3: /* MPPT */
            switch(uc_item)
            {
                case 0: snprintf(pc_buf, uc_size, "%u min", tAppMemParam.tMPPT.usAutoOffTime); break;
                case 1: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tMPPT.cAllowMaxTemp); break;
                case 2: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tMPPT.usMaxInVolt); break;
                case 3: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tMPPT.usMinInVolt); break;
                case 4: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tMPPT.usInPwrRating); break;
                default: snprintf(pc_buf, uc_size, "-"); break;
            }
            break;
#endif

#if(boardDCAC_EN)
        case 4: /* DCAC */
            switch(uc_item)
            {
                case 0:  snprintf(pc_buf, uc_size, "%u min", tAppMemParam.tDCAC.usAutoOffTime); break;
                case 1:  snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDCAC.usMinOpenVolt); break;
                case 2:  snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDCAC.usVoltRating); break;
                case 3:  snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDCAC.usMaxInVolt); break;
                case 4:  snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDCAC.usMinInVolt); break;
                case 5:  snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDCAC.usInPwrRating); break;
                case 6:  snprintf(pc_buf, uc_size, "%u W", tAppMemParam.tDCAC.usMinInPwr); break;
                case 7:  snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDCAC.usMaxInCurr); break;
                case 8:  snprintf(pc_buf, uc_size, "%u W", tAppMemParam.tDCAC.usOutPwrRating); break;
                case 9:  snprintf(pc_buf, uc_size, "%u W", tAppMemParam.tDCAC.usOverLoadPwr); break;
                case 10: snprintf(pc_buf, uc_size, "%u W", tAppMemParam.tDCAC.usParaInPwr); break;
                case 11: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDCAC.usAcOutFreq); break;
                case 12: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tDCAC.sMaxTemp); break;
                default: snprintf(pc_buf, uc_size, "-"); break;
            }
            break;
#endif

#if(boardUSB_EN)
        case 5: /* USB */
            switch(uc_item)
            {
                case 0: snprintf(pc_buf, uc_size, "%u min", tAppMemParam.tUSB.usAutoOffTime); break;
                case 1: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tUSB.usMaxInVolt); break;
                case 2: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tUSB.usMinInVolt); break;
                case 3: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tUSB.usMinOpenVolt); break;
                case 4: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tUSB.sMaxTemp); break;
                default: snprintf(pc_buf, uc_size, "-"); break;
            }
            break;
#endif

#if(boardDC_EN)
        case 6: /* DC */
            switch(uc_item)
            {
                case 0: snprintf(pc_buf, uc_size, "%u min", tAppMemParam.tDC.usAutoOffTime); break;
                case 1: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDC.usMaxOutVolt); break;
                case 2: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDC.usMinOutVolt); break;
                case 3: snprintf(pc_buf, uc_size, "%u W", tAppMemParam.tDC.usOverLoadPwr); break;
                case 4: snprintf(pc_buf, uc_size, "%u", tAppMemParam.tDC.usMinOpenVolt); break;
                case 5: snprintf(pc_buf, uc_size, "%d C", tAppMemParam.tDC.sMaxTemp); break;
                default: snprintf(pc_buf, uc_size, "-"); break;
            }
            break;
#endif

        default:
            snprintf(pc_buf, uc_size, "-");
            break;
    }
}

static void v_ps_update_data(void)
{
/***********************************************************************************************************************
 -----函数功能    刷新记忆参数设置页当前Tab数据
 -----说明(备注)  DispTask上下文: 按S_tState.ucPsTab遍历参数项, 调用v_ps_get_value_str填值, 隐藏多余项;
				  由vEngMode_UiTick通过bNeedRefresh周期触发或切Tab时立即触发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    uint8_t i;
    uint8_t uc_cnt = S_aucPsItemCount[S_tState.ucPsTab];
    const char **ppc_names = NULL;
    char buf[32];

    switch(S_tState.ucPsTab)
    {
        case 0: ppc_names = S_apcPsSysNames; break;
        case 1: ppc_names = S_apcPsLcdNames; break;
        case 2: ppc_names = S_apcPsBatNames; break;
        case 3: ppc_names = S_apcPsMpptNames; break;
        case 4: ppc_names = S_apcPsDcacNames; break;
        case 5: ppc_names = S_apcPsUsbNames; break;
        case 6: ppc_names = S_apcPsDcNames; break;
        default: break;
    }

    for(i = 0; i < ENG_MAX_SET_ITEMS; i++)
    {
        if(i < uc_cnt && ppc_names != NULL)
        {
            lv_obj_clear_flag(S_tObjs.p_ps_items[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(S_tObjs.p_ps_lbl_n[i], ppc_names[i]);
            v_ps_get_value_str(S_tState.ucPsTab, i, buf, sizeof(buf));
            lv_label_set_text(S_tObjs.p_ps_lbl_v[i], buf);
        }
        else
        {
            lv_obj_add_flag(S_tObjs.p_ps_items[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    v_ps_update_selection();
}

static void v_ps_update_selection(void)
{
/***********************************************************************************************************************
 -----函数功能    更新记忆参数设置页选中项样式
 -----说明(备注)  DispTask上下文: 按S_tState.ucPsItem高亮对应行(蓝边+深底+蓝字);
				  并将选中项自动滚动到可视区域; 由vEngMode_UiTick通过ENG_UI_ACTION_PS_ITEM触发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    uint8_t i;
    uint8_t uc_cnt = S_aucPsItemCount[S_tState.ucPsTab];

    for(i = 0; i < uc_cnt; i++)
    {
        if(i == S_tState.ucPsItem)
        {
            lv_obj_set_style_bg_color(S_tObjs.p_ps_items[i],
                lv_color_hex(ENG_CLR_SEL_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
            if(S_tState.bPsEditing)
            {
                /* 编辑模式: 加粗橙色左边框, 区别于浏览模式的青色细边框 */
                lv_obj_set_style_border_width(S_tObjs.p_ps_items[i], 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(S_tObjs.p_ps_items[i],
                    lv_color_hex(ENG_CLR_DCAC), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_side(S_tObjs.p_ps_items[i],
                    LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            else
            {
                /* 浏览模式: 青色细边框 */
                lv_obj_set_style_border_width(S_tObjs.p_ps_items[i], 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(S_tObjs.p_ps_items[i],
                    lv_color_hex(ENG_CLR_SELECTED), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_side(S_tObjs.p_ps_items[i],
                    LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            lv_obj_set_style_text_color(S_tObjs.p_ps_lbl_n[i],
                lv_color_hex(ENG_CLR_SELECTED), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_obj_set_style_bg_color(S_tObjs.p_ps_items[i],
                lv_color_hex(ENG_CLR_MENU_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(S_tObjs.p_ps_items[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(S_tObjs.p_ps_lbl_n[i],
                lv_color_hex(ENG_CLR_TEXT_SEC), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    /* 选中项自动滚动到可视区域, 避免长列表中选中项被遮挡 */
    if(S_tState.ucPsItem < uc_cnt && S_tObjs.p_ps_items[S_tState.ucPsItem] != NULL)
    {
        lv_obj_scroll_to_view(S_tObjs.p_ps_items[S_tState.ucPsItem], LV_ANIM_ON);
    }
}


//****************************************************系统设置页面************************************************//

static void v_page_create_ss(void)
{
/***********************************************************************************************************************
 -----函数功能    创建系统设置(SS)页面
 -----说明(备注)  DispTask上下文: 在S_tObjs.p_base上创建3个设置项面板(SAVE&EXIT/RESET/UPDATE);
				  S_tObjs.p_ss_page首次为NULL时调用; 选中态由v_ss_update_selection绘制
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    uint8_t i;
    lv_obj_t *p_page = lv_obj_create(S_tObjs.p_base);
    lv_obj_set_pos(p_page, 0, 0);
    lv_obj_set_size(p_page, ENG_SCREEN_W, ENG_SCREEN_H);
    lv_obj_remove_flag(p_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(p_page, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p_page, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(p_page, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    S_tObjs.p_ss_page = p_page;

    for(i = 0; i < 3; i++)
    {
        lv_coord_t y = (lv_coord_t)(ENG_CONTENT_Y + 8 + i * 64);

        lv_obj_t *p_item = p_create_panel(p_page, 8, y, ENG_SCREEN_W - 16, 56, ENG_CLR_MENU_BG);
        S_tObjs.p_ss_items[i] = p_item;

        S_tObjs.p_ss_lbl_title[i] = p_create_label(p_item, 12, 6, ENG_FONT_NORMAL, ENG_CLR_SELECTED);
        lv_label_set_text(S_tObjs.p_ss_lbl_title[i], S_apcSsTitle[i]);

        S_tObjs.p_ss_lbl_desc[i] = p_create_label(p_item, 12, 28, ENG_FONT_SMALL, ENG_CLR_TEXT_SEC);
        lv_label_set_text(S_tObjs.p_ss_lbl_desc[i], S_apcSsDesc[i]);
    }
}

static void v_ss_update_selection(void)
{
/***********************************************************************************************************************
 -----函数功能    更新系统设置页选中项样式
 -----说明(备注)  DispTask上下文: 按S_tState.ucSsSel高亮对应项(蓝边+深底);
				  由vEngMode_UiTick通过ENG_UI_ACTION_SS_SEL动作触发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    uint8_t i;
    for(i = 0; i < 3; i++)
    {
        if(i == S_tState.ucSsSel)
        {
            lv_obj_set_style_bg_color(S_tObjs.p_ss_items[i],
                lv_color_hex(ENG_CLR_SEL_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(S_tObjs.p_ss_items[i], 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(S_tObjs.p_ss_items[i],
                lv_color_hex(ENG_CLR_SELECTED), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(S_tObjs.p_ss_items[i], LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_obj_set_style_bg_color(S_tObjs.p_ss_items[i],
                lv_color_hex(ENG_CLR_MENU_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(S_tObjs.p_ss_items[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}


//****************************************************确认对话框************************************************//

static void v_page_create_cfm(void)
{
/***********************************************************************************************************************
 -----函数功能    创建确认对话框页面
 -----说明(备注)  DispTask上下文: 在S_tObjs.p_base上创建半透明遮罩+对话框面板+确认文本+Cancel/OK按钮;
				  S_tObjs.p_cfm_page首次为NULL时调用; 选中态由v_cfm_update_selection绘制
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    /* 半透明遮罩 */
    lv_obj_t *p_overlay = lv_obj_create(S_tObjs.p_base);
    lv_obj_set_pos(p_overlay, 0, 0);
    lv_obj_set_size(p_overlay, ENG_SCREEN_W, ENG_SCREEN_H);
    lv_obj_remove_flag(p_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(p_overlay, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(p_overlay, LV_OPA_60, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(p_overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    S_tObjs.p_cfm_page = p_overlay;

    /* 对话框面板 */
    lv_obj_t *p_box = p_create_panel(p_overlay, 30, 70, 260, 100, ENG_CLR_CARD);
    lv_obj_set_style_border_width(p_box, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(p_box, lv_color_hex(ENG_CLR_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    S_tObjs.p_cfm_box = p_box;

    /* 确认文本 */
    S_tObjs.p_cfm_text = p_create_label(p_box, 20, 12, ENG_FONT_NORMAL, ENG_CLR_TEXT);
    lv_label_set_text(S_tObjs.p_cfm_text, "Confirm?");

    /* 按钮 */
    S_tObjs.p_cfm_btns[0] = p_create_panel(p_box, 15, 60, 100, 30, ENG_CLR_MENU_BG);
    S_tObjs.p_cfm_lbls[0] = p_create_label(S_tObjs.p_cfm_btns[0], 20, 5, ENG_FONT_NORMAL, ENG_CLR_TEXT_SEC);
    lv_label_set_text(S_tObjs.p_cfm_lbls[0], "Cancel");

    S_tObjs.p_cfm_btns[1] = p_create_panel(p_box, 140, 60, 100, 30, ENG_CLR_MENU_BG);
    S_tObjs.p_cfm_lbls[1] = p_create_label(S_tObjs.p_cfm_btns[1], 25, 5, ENG_FONT_NORMAL, ENG_CLR_SELECTED);
    lv_label_set_text(S_tObjs.p_cfm_lbls[1], "OK");
}

static void v_cfm_update_selection(void)
{
/***********************************************************************************************************************
 -----函数功能    更新确认对话框选中按钮样式
 -----说明(备注)  DispTask上下文: 按S_tState.ucConfirmSel高亮对应按钮(蓝边+深底);
				  由vEngMode_UiTick通过ENG_UI_ACTION_CFM_SEL动作触发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    uint8_t i;
    for(i = 0; i < 2; i++)
    {
        if(i == S_tState.ucConfirmSel)
        {
            lv_obj_set_style_bg_color(S_tObjs.p_cfm_btns[i],
                lv_color_hex(ENG_CLR_SEL_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(S_tObjs.p_cfm_btns[i], 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(S_tObjs.p_cfm_btns[i],
                lv_color_hex(ENG_CLR_SELECTED), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_obj_set_style_bg_color(S_tObjs.p_cfm_btns[i],
                lv_color_hex(ENG_CLR_MENU_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(S_tObjs.p_cfm_btns[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}


//****************************************************页面销毁(懒加载用)****************************************//

static void v_page_delete_menu(void)
{
/***********************************************************************************************************************
 -----函数功能    删除主菜单页面LVGL对象
 -----说明(备注)  DispTask上下文: 删除p_menu_page并置NULL; 用于v_page_show切页时释放旧页
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    if(S_tObjs.p_menu_page)
    {
        lv_obj_delete(S_tObjs.p_menu_page);
        S_tObjs.p_menu_page = NULL;
    }
}

static void v_page_delete_pv(void)
{
/***********************************************************************************************************************
 -----函数功能    删除参数查看页LVGL对象
 -----说明(备注)  DispTask上下文: 删除p_pv_page并置NULL; 用于v_page_show切页时释放旧页
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    if(S_tObjs.p_pv_page)
    {
        lv_obj_delete(S_tObjs.p_pv_page);
        S_tObjs.p_pv_page = NULL;
    }
}

static void v_page_delete_ps(void)
{
/***********************************************************************************************************************
 -----函数功能    删除记忆参数设置页LVGL对象
 -----说明(备注)  DispTask上下文: 删除p_ps_page并置NULL; 用于v_page_show切页时释放旧页
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    if(S_tObjs.p_ps_page)
    {
        lv_obj_delete(S_tObjs.p_ps_page);
        S_tObjs.p_ps_page = NULL;
    }
}

static void v_page_delete_ss(void)
{
/***********************************************************************************************************************
 -----函数功能    删除系统设置页LVGL对象
 -----说明(备注)  DispTask上下文: 删除p_ss_page并置NULL; 用于v_page_show切页时释放旧页
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    if(S_tObjs.p_ss_page)
    {
        lv_obj_delete(S_tObjs.p_ss_page);
        S_tObjs.p_ss_page = NULL;
    }
}

static void v_page_delete_cfm(void)
{
/***********************************************************************************************************************
 -----函数功能    删除确认对话框LVGL对象
 -----说明(备注)  DispTask上下文: 删除p_cfm_page(半透明遮罩)并置NULL; 用于v_page_show切页时释放旧页
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    if(S_tObjs.p_cfm_page)
    {
        lv_obj_delete(S_tObjs.p_cfm_page);
        S_tObjs.p_cfm_page = NULL;
    }
}


//****************************************************页面导航************************************************//

static void v_page_show(EngModePage_E e_page)
{
/***********************************************************************************************************************
 -----函数功能    页面导航(销毁旧页+创建/显示新页)
 -----说明(备注)  DispTask上下文: 销毁上一页LVGL对象+更新S_tState.ePage+按需创建新页+绘制标题/选中态;
				  切到ENG_PAGE_CONFIRM时记录来源页ePrevPage; 同页调用安全;
				  由vEngMode_UiTick通过ePendingPage触发
 -----传入参数    e_page: 目标页面
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
    EngModePage_E e_prev = S_tState.ePage;

    /* 切换页面时销毁旧页面, 释放内存 */
    if(e_prev != e_page)
    {
        switch(e_prev)
        {
            case ENG_PAGE_MAIN_MENU:  v_page_delete_menu(); break;
            case ENG_PAGE_PARAM_VIEW: v_page_delete_pv();   break;
            case ENG_PAGE_PARAM_SET:  v_page_delete_ps();   break;
            case ENG_PAGE_SYS_SET:    v_page_delete_ss();   break;
            case ENG_PAGE_CONFIRM:    v_page_delete_cfm();  break;
            default: break;
        }
    }

    /* 确认对话框需要记录来源页面 */
    if(e_page == ENG_PAGE_CONFIRM)
        S_tState.ePrevPage = e_prev;

    S_tState.ePage = e_page;

    /* 按需创建并显示目标页面 */
    switch(e_page)
    {
        case ENG_PAGE_MAIN_MENU:
            if(S_tObjs.p_menu_page == NULL)
                v_page_create_menu();
            v_menu_update_sel();
            lv_label_set_text(S_tObjs.p_title_label, "ENG MODE");
            lv_obj_set_style_text_color(S_tObjs.p_title_label,
                lv_color_hex(ENG_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
            break;

        case ENG_PAGE_PARAM_VIEW:
            if(S_tObjs.p_pv_page == NULL)
                v_page_create_pv();
            lv_label_set_text(S_tObjs.p_title_label, "PARAM VIEW");
            lv_obj_set_style_text_color(S_tObjs.p_title_label,
                lv_color_hex(ENG_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
            v_pv_switch_tab(S_tState.ucPvTab);
            break;

        case ENG_PAGE_PARAM_SET:
            if(S_tObjs.p_ps_page == NULL)
                v_page_create_ps();
            lv_label_set_text(S_tObjs.p_title_label, "PARAM SET");
            lv_obj_set_style_text_color(S_tObjs.p_title_label,
                lv_color_hex(ENG_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
            v_ps_switch_tab(S_tState.ucPsTab);
            break;

        case ENG_PAGE_SYS_SET:
            if(S_tObjs.p_ss_page == NULL)
                v_page_create_ss();
            v_ss_update_selection();
            lv_label_set_text(S_tObjs.p_title_label, "SYS SET");
            lv_obj_set_style_text_color(S_tObjs.p_title_label,
                lv_color_hex(ENG_CLR_SELECTED), LV_PART_MAIN | LV_STATE_DEFAULT);
            break;

        case ENG_PAGE_CONFIRM:
            if(S_tObjs.p_cfm_page == NULL)
                v_page_create_cfm();
            if(S_tState.ucSsSel < 3)
                lv_label_set_text(S_tObjs.p_cfm_text, S_apcCfmText[S_tState.ucSsSel]);
            v_cfm_update_selection();
            break;

        default:
            break;
    }
}


//****************************************************按键处理**************************************************/

/***********************************************************************************************************************
 -----函数功能    工程模式下按Up键处理
 -----说明(备注)  按键任务上下文: 仅更新状态+设置eUiAction标记, 不直接调LVGL API;
				  UI刷新由DispTask在vEngMode_UiTick中执行, 避免跨任务并发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vEngMode_KeyUp(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_MAIN_MENU:
            if(S_tState.ucMainMenuSel > 0)
                S_tState.ucMainMenuSel--;
            S_tState.eUiAction = ENG_UI_ACTION_MENU_SEL;
            break;

        case ENG_PAGE_PARAM_SET:
        {
            if(S_tState.bPsEditing)
            {
                /* 编辑模式: 增加参数值 */
                vEng_AdjustParam(S_tState.ucPsTab, S_tState.ucPsItem, true);
                S_tState.bNeedRefresh = true;
            }
            else
            {
                /* 浏览模式: 上移选中项 */
                if(S_tState.ucPsItem > 0)
                    S_tState.ucPsItem--;
                tEngMode.ucEngModeItem = S_tState.ucPsItem;
                S_tState.eUiAction = ENG_UI_ACTION_PS_ITEM;
            }
        }break;

        case ENG_PAGE_SYS_SET:
            if(S_tState.ucSsSel > 0)
                S_tState.ucSsSel--;
            S_tState.eUiAction = ENG_UI_ACTION_SS_SEL;
            break;

        case ENG_PAGE_CONFIRM:
            S_tState.ucConfirmSel = 0;
            S_tState.eUiAction = ENG_UI_ACTION_CFM_SEL;
            break;

        default:
            break;
    }
}

/***********************************************************************************************************************
 -----函数功能    工程模式下按Down键处理
 -----说明(备注)  按键任务上下文: 仅更新状态+设置eUiAction标记, 不直接调LVGL API;
				  UI刷新由DispTask在vEngMode_UiTick中执行, 避免跨任务并发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vEngMode_KeyDown(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_MAIN_MENU:
            if(S_tState.ucMainMenuSel < 2)
                S_tState.ucMainMenuSel++;
            S_tState.eUiAction = ENG_UI_ACTION_MENU_SEL;
            break;

        case ENG_PAGE_PARAM_SET:
        {
            if(S_tState.bPsEditing)
            {
                /* 编辑模式: 减少参数值 */
                vEng_AdjustParam(S_tState.ucPsTab, S_tState.ucPsItem, false);
                S_tState.bNeedRefresh = true;
            }
            else
            {
                /* 浏览模式: 下移选中项 */
                uint8_t uc_cnt = S_aucPsItemCount[S_tState.ucPsTab];
                if(S_tState.ucPsItem < uc_cnt - 1)
                    S_tState.ucPsItem++;
                tEngMode.ucEngModeItem = S_tState.ucPsItem;
                S_tState.eUiAction = ENG_UI_ACTION_PS_ITEM;
            }
        }break;

        case ENG_PAGE_SYS_SET:
            if(S_tState.ucSsSel < 2)
                S_tState.ucSsSel++;
            S_tState.eUiAction = ENG_UI_ACTION_SS_SEL;
            break;

        case ENG_PAGE_CONFIRM:
            S_tState.ucConfirmSel = 1;
            S_tState.eUiAction = ENG_UI_ACTION_CFM_SEL;
            break;

        default:
            break;
    }
}

/***********************************************************************************************************************
 -----函数功能    工程模式下按Left键处理
 -----说明(备注)  按键任务上下文: 仅更新Tab状态+同步后端tEngMode+设置eUiAction标记;
				  UI渲染由DispTask在vEngMode_UiTick中执行, 避免跨任务并发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vEngMode_KeyLeft(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_PARAM_VIEW:
            if(S_tState.ucPvTab > 0)
                S_tState.ucPvTab--;
            else
                S_tState.ucPvTab = ENG_NUM_VIEW_TABS - 1;
            S_tState.eUiAction = ENG_UI_ACTION_PV_TAB;
            break;

        case ENG_PAGE_PARAM_SET:
            if(S_tState.ucPsTab > 0)
                S_tState.ucPsTab--;
            else
                S_tState.ucPsTab = ENG_NUM_SET_TABS - 1;
            S_tState.ucPsItem = 0;
            /* 同步到后端tEngMode (UI渲染由DispTask执行) */
            tpSysTask->ucStep = S_aucPsTabToEms[S_tState.ucPsTab];
            tEngMode.ucEngModeItem = 0;
            tEngMode.cEngModeState = 0;
            S_tState.eUiAction = ENG_UI_ACTION_PS_TAB;
            break;

        default:
            break;
    }
}

/***********************************************************************************************************************
 -----函数功能    工程模式下按Right键处理
 -----说明(备注)  按键任务上下文: 仅更新Tab状态+同步后端tEngMode+设置eUiAction标记;
				  UI渲染由DispTask在vEngMode_UiTick中执行, 避免跨任务并发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vEngMode_KeyRight(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_PARAM_VIEW:
            if(S_tState.ucPvTab < ENG_NUM_VIEW_TABS - 1)
                S_tState.ucPvTab++;
            else
                S_tState.ucPvTab = 0;
            S_tState.eUiAction = ENG_UI_ACTION_PV_TAB;
            break;

        case ENG_PAGE_PARAM_SET:
            if(S_tState.ucPsTab < ENG_NUM_SET_TABS - 1)
                S_tState.ucPsTab++;
            else
                S_tState.ucPsTab = 0;
            S_tState.ucPsItem = 0;
            /* 同步到后端tEngMode (UI渲染由DispTask执行) */
            tpSysTask->ucStep = S_aucPsTabToEms[S_tState.ucPsTab];
            tEngMode.ucEngModeItem = 0;
            tEngMode.cEngModeState = 0;
            S_tState.eUiAction = ENG_UI_ACTION_PS_TAB;
            break;

        default:
            break;
    }
}

/***********************************************************************************************************************
 -----函数功能    工程模式下按Enter键处理
 -----说明(备注)  按键任务上下文: 主菜单确认后仅设置ePendingPage(显示任务切页);
				  PARAM_SET选中项切换+后端tEngMode同步, UI刷新由DispTask通过eUiAction执行
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vEngMode_KeyEnter(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_MAIN_MENU:
            switch(S_tState.ucMainMenuSel)
            {
                case 0: S_tState.ePendingPage = ENG_PAGE_PARAM_VIEW; break;
                case 1: S_tState.ePendingPage = ENG_PAGE_PARAM_SET; break;
                case 2: S_tState.ePendingPage = ENG_PAGE_SYS_SET; break;
                default: break;
            }
            break;

        case ENG_PAGE_PARAM_SET:
        {
            /* 进入编辑模式 */
            S_tState.bPsEditing = true;
            tEngMode.ucEngModeItem = S_tState.ucPsItem;
            tEngMode.cEngModeState = 0;
            S_tState.eUiAction = ENG_UI_ACTION_PS_ITEM;
        }break;

        case ENG_PAGE_SYS_SET:
            /* 保存退出(SAVE&EXIT)默认选中确认, 重置/升级等危险操作默认选中取消, 防止误触发 */
            S_tState.ucConfirmSel = (S_tState.ucSsSel == 0) ? 1 : 0;
            S_tState.ePendingPage = ENG_PAGE_CONFIRM;
            break;

        case ENG_PAGE_CONFIRM:
            if(S_tState.ucConfirmSel == 1)  /* 确认 */
            {
                switch(S_tState.ucSsSel)
                {
                    case 0: /* SAVE & EXIT */
                    {
                        /* 保存所有参数到 Flash */
                        cApp_UpdateMemParam("tAppMemParam");
                        /* 退出工程模式 -> 关机 */
                        cSys_Switch(SO_KEY, ST_OFF, true);
                        S_tState.bExitReq = true;
                    }break;

                    case 1: /* RESET DEFAULTS */
                    {
                        /* 重置所有参数为默认值 */
                        cApp_MemParamInit("tAppMemParam");
                        /* 保存到 Flash */
                        cApp_UpdateMemParam("tAppMemParam");
                        /* 退出工程模式 -> 关机) */
                        cSys_Switch(SO_KEY, ST_OFF, true);
                        S_tState.bExitReq = true;
                    }break;

                    case 2: /* FIRMWARE UPDATE */
                        vApp_JumpToBoot(mainUPDATE_FLAG);
                        break;

                    default:
                        break;
                }
            }
            else  /* 取消 */
            {
                S_tState.ePendingPage = ENG_PAGE_SYS_SET;
            }
            break;

        default:
            break;
    }
}

/***********************************************************************************************************************
 -----函数功能    工程模式下按Back键处理
 -----说明(备注)  按键任务上下文: 仅设置ePendingPage(显示任务切页)或bExitReq;
				  UI销毁/页面切换由DispTask执行, 避免跨任务并发
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vEngMode_KeyBack(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_PARAM_VIEW:
        case ENG_PAGE_SYS_SET:
            S_tState.ePendingPage = ENG_PAGE_MAIN_MENU;
            break;

        case ENG_PAGE_PARAM_SET:
            if(S_tState.bPsEditing)
            {
                /* 编辑模式: 退出编辑, 恢复浏览 */
                S_tState.bPsEditing = false;
                S_tState.eUiAction = ENG_UI_ACTION_PS_ITEM;
            }
            else
            {
                /* 浏览模式: 返回主菜单 */
                S_tState.ePendingPage = ENG_PAGE_MAIN_MENU;
            }
            break;

        case ENG_PAGE_CONFIRM:
            S_tState.ePendingPage = ENG_PAGE_SYS_SET;
            break;

        case ENG_PAGE_MAIN_MENU:
            /* 退出工程模式 */
            S_tState.bExitReq = true;
            break;

        default:
            break;
    }
}


//****************************************************公共API**************************************************//

/***********************************************************************************************************************
 -----函数功能    创建工程模式UI(主菜单/参数查看/记忆参数设置/系统设置)
 -----说明(备注)  必须在DispTask上下文中调用: 切换EEZ屏幕+创建LVGL基础容器+显示主菜单;
				  若旧UI仍存在则先删除; 内存充足时按需懒加载子页面
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vEngMode_UiCreate(void)
{
    {
        lv_mem_monitor_t t_mon;
        lv_mem_monitor(&t_mon);
        sMyPrint("EngUiCreate: entering, free_size = %d\r\n", (int)t_mon.free_size);
    }

    /* 清除旧UI */
    if(S_tObjs.p_base != NULL)
        vEngMode_UiDelete();

    /* 初始化状态 */
    memset(&S_tState, 0, sizeof(S_tState));
    S_tState.ePage = ENG_PAGE_MAIN_MENU;
    S_tState.ucMainMenuSel = 0;
    S_tState.ucPvTab = 0;
    S_tState.ucPsTab = 0;
    S_tState.ucPsItem = 0;
    S_tState.ucSsSel = 0;
    S_tState.ucConfirmSel = 0;
    S_tState.bExitReq = false;
    S_tState.ePendingPage = ENG_PAGE_MAX;
    S_tState.eUiAction = ENG_UI_ACTION_NONE;

    /* 切换到EEZ Studio预定义的工程模式专用屏幕, 避免与工作屏幕的对象树重叠 */
    lv_screen_load(objects.main_eng);

    /* 创建基础容器 */
    lv_obj_t *p_base = lv_obj_create(objects.main_eng);
    lv_obj_set_pos(p_base, 0, 0);
    lv_obj_set_size(p_base, ENG_SCREEN_W, ENG_SCREEN_H);
    lv_obj_remove_flag(p_base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(p_base, lv_color_hex(ENG_CLR_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(p_base, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p_base, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(p_base, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(p_base, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_move_foreground(p_base);
    S_tObjs.p_base = p_base;

    /* 创建标题栏 */
    lv_obj_t *p_title_bar = lv_obj_create(p_base);
    lv_obj_set_pos(p_title_bar, 0, 0);
    lv_obj_set_size(p_title_bar, ENG_SCREEN_W, ENG_TITLE_H);
    lv_obj_remove_flag(p_title_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(p_title_bar, lv_color_hex(ENG_CLR_CARD), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(p_title_bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(p_title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(p_title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(p_title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    S_tObjs.p_title_bar = p_title_bar;

    S_tObjs.p_title_label = p_create_label(p_title_bar, 8, 2, ENG_FONT_TITLE, ENG_CLR_TEXT);
    lv_label_set_text(S_tObjs.p_title_label, "ENG MODE");

    /* 显示主菜单(页面按需创建, 避免一次性创建所有页面导致内存不足) */
    v_page_show(ENG_PAGE_MAIN_MENU);

    {
        lv_mem_monitor_t t_mon;
        lv_mem_monitor(&t_mon);
        sMyPrint("EngUiCreate: finished, free_size = %d\r\n", (int)t_mon.free_size);
    }
}

/***********************************************************************************************************************
 -----函数功能    删除工程模式UI(释放LVGL对象和状态)
 -----说明(备注)  必须在DispTask上下文中调用: 删除基础容器并清零S_tObjs/S_tState;
				  在超时退出或bExitReq时由显示队列任务调用
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vEngMode_UiDelete(void)
{
    if(S_tObjs.p_base != NULL)
    {
        lv_obj_delete(S_tObjs.p_base);
        S_tObjs.p_base = NULL;
    }
    memset(&S_tObjs, 0, sizeof(S_tObjs));
    memset(&S_tState, 0, sizeof(S_tState));
}

/***********************************************************************************************************************
 -----函数功能    查询工程模式是否请求退出
 -----说明(备注)  按键任务(Back/确认)置位bExitReq后, 显示任务通过此函数查询;
				  查询到true后由显示任务负责UI删除+系统状态切换
 -----传入参数    none
 -----输出参数    none
 -----返回值      true:有退出请求   false:无退出请求
 ************************************************************************************************************************/
bool bEngMode_IsExitReq(void)
{
    return S_tState.bExitReq;
}

/***********************************************************************************************************************
 -----函数功能    工程模式UI周期任务(由显示队列任务周期调用)
 -----说明(备注)  必须在DispTask上下文中执行: 1)处理按键设置的ePendingPage切页 2)处理eUiAction渲染请求
				  3)周期性刷新参数查看/参数设置页数据; 调用前确保ui_init已执行
 -----传入参数    none
 -----输出参数    none
 -----返回值      none
 ************************************************************************************************************************/
void vEngMode_UiTick(void)
{
    if(S_tObjs.p_base == NULL)
        return;

    /* 执行待切换页面(在显示任务上下文中创建/销毁LVGL对象, 避免与按键任务并发) */
    if(S_tState.ePendingPage < ENG_PAGE_MAX)
    {
        v_page_show(S_tState.ePendingPage);
        S_tState.ePendingPage = ENG_PAGE_MAX;
        /* 页面切换后, 旧页面的LVGL对象已被销毁, 清除遗留的UI动作请求以防止访问已释放对象 */
        S_tState.eUiAction = ENG_UI_ACTION_NONE;
    }

    /* 执行按键产生的UI更新请求(在显示任务上下文中操作LVGL对象, 避免跨任务并发) */
    if(S_tState.eUiAction != ENG_UI_ACTION_NONE)
    {
        EngUiAction_E e_action = S_tState.eUiAction;
        S_tState.eUiAction = ENG_UI_ACTION_NONE;

        switch(e_action)
        {
            case ENG_UI_ACTION_MENU_SEL:
                v_menu_update_sel();
                break;

            case ENG_UI_ACTION_PV_TAB:
                v_pv_switch_tab(S_tState.ucPvTab);
                break;

            case ENG_UI_ACTION_PS_TAB:
                v_ps_switch_tab(S_tState.ucPsTab);
                break;

            case ENG_UI_ACTION_PS_ITEM:
                v_ps_update_selection();
                v_ps_update_data();
                break;

            case ENG_UI_ACTION_SS_SEL:
                v_ss_update_selection();
                break;

            case ENG_UI_ACTION_CFM_SEL:
                v_cfm_update_selection();
                break;

            default:
                break;
        }
    }

    /* 周期性数据刷新 */
    S_tState.ucTickCnt++;
    if(S_tState.ucTickCnt >= ENG_DATA_REFRESH_CNT)
    {
        S_tState.ucTickCnt = 0;
        S_tState.bNeedRefresh = true;
    }

    if(S_tState.bNeedRefresh)
    {
        S_tState.bNeedRefresh = false;

        switch(S_tState.ePage)
        {
            case ENG_PAGE_PARAM_VIEW:
                v_pv_update_data();
                break;

            case ENG_PAGE_PARAM_SET:
                v_ps_update_data();
                break;

            default:
                break;
        }
    }
}


#endif  /* boardENG_MODE_EN && boardDISPLAY_EN */