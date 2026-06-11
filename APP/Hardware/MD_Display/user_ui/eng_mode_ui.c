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

#if(boardENG_MODE_EN && boardDISPLAY_EN)

#include <string.h>
#include <stdio.h>
#include "lvgl.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_iface.h"
#include "MD_Display/eez_ui/fonts.h"
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

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_task.h"
#endif

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#endif

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_task.h"
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


//****************************************************类型定义************************************************//

/* 工程模式UI状态 */
typedef struct
{
    EngModePage_E ePage;            /* 当前页面 */
    EngModePage_E ePrevPage;        /* 上一页面(用于确认对话框返回) */
    uint8_t ucMainMenuSel;          /* 主菜单选中项 0-2 */
    uint8_t ucPvTab;                /* 参数查看当前Tab 0-6 */
    uint8_t ucPsTab;                /* 参数设置当前Tab 0-6 */
    uint8_t ucPsItem;               /* 参数设置当前选中参数 */
    uint8_t ucSsSel;                /* 系统设置选中项 0-2 */
    uint8_t ucConfirmSel;           /* 确认对话框选中 0=取消 1=确认 */
    uint8_t ucTickCnt;              /* 刷新计数器 */
    bool bExitReq;                  /* 退出请求 */
    bool bNeedRefresh;              /* 需要数据刷新 */
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
static void v_page_show(EngModePage_E e_page);
static void v_pv_update_data(void);
static void v_ps_update_data(void);
static void v_ps_update_selection(void);
static void v_ss_update_selection(void);
static void v_cfm_update_selection(void);


//****************************************************辅助函数**************************************************//

/***********************************************************************************************************************
 * 函数功能    : 创建面板容器
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
 * 函数功能    : 创建标签
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

static void v_pv_hide_rows(uint8_t uc_from)
{
    uint8_t i;
    for(i = uc_from; i < ENG_MAX_VIEW_ROWS; i++)
    {
        lv_label_set_text(S_tObjs.p_pv_lbl_l[i], "");
        lv_label_set_text(S_tObjs.p_pv_lbl_r[i], "");
    }
}

static void v_pv_update_data(void)
{
    char buf_l[20], buf_r[24];
    uint32_t ul_accent = S_aulPvTabClr[S_tState.ucPvTab];

    switch(S_tState.ucPvTab)
    {
#if(boardBMS_EN)
        case 0: /* BMS */
        {
            snprintf(buf_l, sizeof(buf_l), "V_pack");
            snprintf(buf_r, sizeof(buf_r), "%.2fV", tBmsRx.tDevInfo[0].usVolt * 0.01f);
            v_pv_set_row(0, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "I_pack");
            snprintf(buf_r, sizeof(buf_r), "%.2fA", tBmsRx.tDevInfo[0].sCurr * 0.01f);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "SOC");
            snprintf(buf_r, sizeof(buf_r), "%u%%", tBmsRx.tDevInfo[0].usSOC);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Cycle");
            snprintf(buf_r, sizeof(buf_r), "%u", tBmsRx.tDevInfo[0].usCycleCnt);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "T_max");
            snprintf(buf_r, sizeof(buf_r), "%d C", tBmsRx.tDevInfo[0].sMaxTemp);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "T_min");
            snprintf(buf_r, sizeof(buf_r), "%d C", tBmsRx.tDevInfo[0].sMinTemp);
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Online");
            {
                const char *p_st = "NULL";
                if(tBms.eWorkState == BWS_DISCHG) p_st = "DISG";
                else if(tBms.eWorkState == BWS_CHG) p_st = "CHG";
                snprintf(buf_r, sizeof(buf_r), "%u St:%s", tBmsRx.tDevNum.ucOnlineNum, p_st);
            }
            v_pv_set_row(6, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Cap");
            snprintf(buf_r, sizeof(buf_r), "%.1fAH B:%dC",
                tBmsRx.tDevInfo[0].usCalcCapAH * 0.1f, tBmsRx.tDevInfo[0].sBoardTempMax);
            v_pv_set_row(7, buf_l, buf_r, ul_accent);

            v_pv_hide_rows(8);
        }break;
#endif  /* boardBMS_EN */

#if(boardMPPT_EN)
        case 1: /* MPPT */
        {
            snprintf(buf_l, sizeof(buf_l), "V_pv_in");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tMpptRx.usInVolt * 0.1f);
            v_pv_set_row(0, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "I_pv_in");
            snprintf(buf_r, sizeof(buf_r), "%.2fA", tMpptRx.usInCurr * 0.01f);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "P_pv_in");
            snprintf(buf_r, sizeof(buf_r), "%.1fW", tMpptRx.usInPwr * 0.1f);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "P_out");
            snprintf(buf_r, sizeof(buf_r), "%.1fW", tMpptRx.usOutPwr * 0.1f);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "V_out");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tMpptRx.usOutVolt * 0.1f);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Temp");
            snprintf(buf_r, sizeof(buf_r), "%d C", tMpptRx.sMaxTemp);
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "InType");
            snprintf(buf_r, sizeof(buf_r), "T:%u", (uint8_t)tMpptRx.uInType);
            v_pv_set_row(6, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "ErrCode");
            snprintf(buf_r, sizeof(buf_r), "0x%04X", tMpptRx.uErrCode.usCode);
            v_pv_set_row(7, buf_l, buf_r, ul_accent);

            v_pv_hide_rows(8);
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
            snprintf(buf_r, sizeof(buf_r), "%uW", tDcacRx.usInPwr);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "F_ac_in");
            snprintf(buf_r, sizeof(buf_r), "%.1fHz", tDcacRx.usInFreq * 0.1f);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "V_ac_out");
            snprintf(buf_r, sizeof(buf_r), "%.1fV", tDcacRx.usOutVolt * 0.1f);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "P_ac_out");
            snprintf(buf_r, sizeof(buf_r), "%uW", tDcacRx.usOutPwr);
            v_pv_set_row(5, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "ChgSt");
            {
                const char *p_c = (tDcac.eChgState == IOS_WORK) ? "CHG" : "OFF";
                const char *p_d = (tDcac.eDisChgState == IOS_WORK) ? "DISG" : "OFF";
                snprintf(buf_r, sizeof(buf_r), "C:%s D:%s", p_c, p_d);
            }
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
#if(boardUSE_OS)
            {
                uint32_t ul_sec = (uint32_t)(xTaskGetTickCount() / 1000);
                uint32_t ul_days = ul_sec / 86400; ul_sec %= 86400;
                uint32_t ul_hours = ul_sec / 3600; ul_sec %= 3600;
                uint32_t ul_mins = ul_sec / 60;
                snprintf(buf_l, sizeof(buf_l), "Uptime");
                snprintf(buf_r, sizeof(buf_r), "%lud%02luh%02lum", ul_days, ul_hours, ul_mins);
                v_pv_set_row(0, buf_l, buf_r, ul_accent);
            }
#endif
            snprintf(buf_l, sizeof(buf_l), "ErrCode");
            snprintf(buf_r, sizeof(buf_r), "0x%08lX", (unsigned long)tSysInfo.uErrCode);
            v_pv_set_row(1, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "Version");
            snprintf(buf_r, sizeof(buf_r), "%s", tAppMemParam.tVerInfo.saVersion);
            v_pv_set_row(2, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "BuildDate");
            snprintf(buf_r, sizeof(buf_r), "%s", tAppMemParam.tVerInfo.saBuildDate);
            v_pv_set_row(3, buf_l, buf_r, ul_accent);

#if(boardADC_EN)
            snprintf(buf_l, sizeof(buf_l), "BoardTmp");
            snprintf(buf_r, sizeof(buf_r), "DC:%dC USB:%dC", tAdcSamp.sDcOutTemp, tAdcSamp.sUsbTemp);
            v_pv_set_row(4, buf_l, buf_r, ul_accent);

            snprintf(buf_l, sizeof(buf_l), "SysVolt");
#if(boardHEAT_MANAGE_EN)
            snprintf(buf_r, sizeof(buf_r), "%.2fV F:%u",
                tAdcSamp.usSysInVolt * 0.01f, (unsigned)eFan_GetWorkMode());
#else
            snprintf(buf_r, sizeof(buf_r), "%.2fV", tAdcSamp.usSysInVolt * 0.01f);
#endif
            v_pv_set_row(5, buf_l, buf_r, ul_accent);
#endif  /* boardADC_EN */

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
    if(uc_tab >= ENG_NUM_SET_TABS)
        return;

    S_tState.ucPsTab = uc_tab;
    S_tState.ucPsItem = 0;

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
    switch(uc_tab)
    {
        case 0: /* SYS */
            switch(uc_item)
            {
                case 0: snprintf(pc_buf, uc_size, "%s", tAppMemParam.tVerInfo.saVersion + 10); break;
                case 1: snprintf(pc_buf, uc_size, "%d", S_tState.ucPsItem == 1 ? tEngMode.cEngModeState : 0); break;
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
    uint8_t i;
    uint8_t uc_cnt = S_aucPsItemCount[S_tState.ucPsTab];

    for(i = 0; i < uc_cnt; i++)
    {
        if(i == S_tState.ucPsItem)
        {
            lv_obj_set_style_bg_color(S_tObjs.p_ps_items[i],
                lv_color_hex(ENG_CLR_SEL_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(S_tObjs.p_ps_items[i], 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(S_tObjs.p_ps_items[i],
                lv_color_hex(ENG_CLR_SELECTED), LV_PART_MAIN | LV_STATE_DEFAULT);
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
}


//****************************************************系统设置页面************************************************//

static void v_page_create_ss(void)
{
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


//****************************************************页面导航************************************************//

static void v_page_show(EngModePage_E e_page)
{
    /* 隐藏所有页面 */
    if(S_tObjs.p_menu_page) lv_obj_add_flag(S_tObjs.p_menu_page, LV_OBJ_FLAG_HIDDEN);
    if(S_tObjs.p_pv_page)   lv_obj_add_flag(S_tObjs.p_pv_page, LV_OBJ_FLAG_HIDDEN);
    if(S_tObjs.p_ps_page)   lv_obj_add_flag(S_tObjs.p_ps_page, LV_OBJ_FLAG_HIDDEN);
    if(S_tObjs.p_ss_page)   lv_obj_add_flag(S_tObjs.p_ss_page, LV_OBJ_FLAG_HIDDEN);
    if(S_tObjs.p_cfm_page)  lv_obj_add_flag(S_tObjs.p_cfm_page, LV_OBJ_FLAG_HIDDEN);

    S_tState.ePage = e_page;

    switch(e_page)
    {
        case ENG_PAGE_MAIN_MENU:
            if(S_tObjs.p_menu_page) lv_obj_clear_flag(S_tObjs.p_menu_page, LV_OBJ_FLAG_HIDDEN);
            v_menu_update_sel();
            /* 更新标题 */
            lv_label_set_text(S_tObjs.p_title_label, "ENG MODE");
            lv_obj_set_style_text_color(S_tObjs.p_title_label,
                lv_color_hex(ENG_CLR_TEXT), LV_PART_MAIN | LV_STATE_DEFAULT);
            break;

        case ENG_PAGE_PARAM_VIEW:
            if(S_tObjs.p_pv_page) lv_obj_clear_flag(S_tObjs.p_pv_page, LV_OBJ_FLAG_HIDDEN);
            v_pv_switch_tab(S_tState.ucPvTab);
            break;

        case ENG_PAGE_PARAM_SET:
            if(S_tObjs.p_ps_page) lv_obj_clear_flag(S_tObjs.p_ps_page, LV_OBJ_FLAG_HIDDEN);
            v_ps_switch_tab(S_tState.ucPsTab);
            break;

        case ENG_PAGE_SYS_SET:
            if(S_tObjs.p_ss_page) lv_obj_clear_flag(S_tObjs.p_ss_page, LV_OBJ_FLAG_HIDDEN);
            v_ss_update_selection();
            lv_label_set_text(S_tObjs.p_title_label, "SYS SET");
            lv_obj_set_style_text_color(S_tObjs.p_title_label,
                lv_color_hex(ENG_CLR_SELECTED), LV_PART_MAIN | LV_STATE_DEFAULT);
            break;

        case ENG_PAGE_CONFIRM:
            S_tState.ePrevPage = S_tState.ePage;
            if(S_tObjs.p_cfm_page) lv_obj_clear_flag(S_tObjs.p_cfm_page, LV_OBJ_FLAG_HIDDEN);
            if(S_tState.ucSsSel < 3)
                lv_label_set_text(S_tObjs.p_cfm_text, S_apcCfmText[S_tState.ucSsSel]);
            v_cfm_update_selection();
            break;

        default:
            break;
    }
}


//****************************************************按键处理**************************************************/

void vEngMode_KeyUp(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_MAIN_MENU:
            if(S_tState.ucMainMenuSel > 0)
                S_tState.ucMainMenuSel--;
            v_menu_update_sel();
            break;

        case ENG_PAGE_PARAM_SET:
        {
            /* 调整参数值 (+1) */
            tEngMode.ucEngModeItem = S_tState.ucPsItem;
            tEngMode.cEngModeState = 1;
            S_tState.bNeedRefresh = true;
        }break;

        case ENG_PAGE_SYS_SET:
            if(S_tState.ucSsSel > 0)
                S_tState.ucSsSel--;
            v_ss_update_selection();
            break;

        case ENG_PAGE_CONFIRM:
            S_tState.ucConfirmSel = 0;
            v_cfm_update_selection();
            break;

        default:
            break;
    }
}

void vEngMode_KeyDown(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_MAIN_MENU:
            if(S_tState.ucMainMenuSel < 2)
                S_tState.ucMainMenuSel++;
            v_menu_update_sel();
            break;

        case ENG_PAGE_PARAM_SET:
        {
            /* 调整参数值 (-1) */
            tEngMode.ucEngModeItem = S_tState.ucPsItem;
            tEngMode.cEngModeState = -1;
            S_tState.bNeedRefresh = true;
        }break;

        case ENG_PAGE_SYS_SET:
            if(S_tState.ucSsSel < 2)
                S_tState.ucSsSel++;
            v_ss_update_selection();
            break;

        case ENG_PAGE_CONFIRM:
            S_tState.ucConfirmSel = 1;
            v_cfm_update_selection();
            break;

        default:
            break;
    }
}

void vEngMode_KeyLeft(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_PARAM_VIEW:
            if(S_tState.ucPvTab > 0)
                v_pv_switch_tab(S_tState.ucPvTab - 1);
            else
                v_pv_switch_tab(ENG_NUM_VIEW_TABS - 1);
            break;

        case ENG_PAGE_PARAM_SET:
            if(S_tState.ucPsTab > 0)
                v_ps_switch_tab(S_tState.ucPsTab - 1);
            else
                v_ps_switch_tab(ENG_NUM_SET_TABS - 1);
            break;

        default:
            break;
    }
}

void vEngMode_KeyRight(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_PARAM_VIEW:
            if(S_tState.ucPvTab < ENG_NUM_VIEW_TABS - 1)
                v_pv_switch_tab(S_tState.ucPvTab + 1);
            else
                v_pv_switch_tab(0);
            break;

        case ENG_PAGE_PARAM_SET:
            if(S_tState.ucPsTab < ENG_NUM_SET_TABS - 1)
                v_ps_switch_tab(S_tState.ucPsTab + 1);
            else
                v_ps_switch_tab(0);
            break;

        default:
            break;
    }
}

void vEngMode_KeyEnter(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_MAIN_MENU:
            switch(S_tState.ucMainMenuSel)
            {
                case 0: v_page_show(ENG_PAGE_PARAM_VIEW); break;
                case 1: v_page_show(ENG_PAGE_PARAM_SET); break;
                case 2: v_page_show(ENG_PAGE_SYS_SET); break;
                default: break;
            }
            break;

        case ENG_PAGE_PARAM_SET:
        {
            /* 切换到下一个参数 */
            uint8_t uc_cnt = S_aucPsItemCount[S_tState.ucPsTab];
            if(S_tState.ucPsItem < uc_cnt - 1)
                S_tState.ucPsItem++;
            else
                S_tState.ucPsItem = 0;

            tEngMode.ucEngModeItem = S_tState.ucPsItem;
            tEngMode.cEngModeState = 0;
            v_ps_update_selection();
            v_ps_update_data();
        }break;

        case ENG_PAGE_SYS_SET:
            S_tState.ucConfirmSel = 1;  /* 默认选中确认 */
            v_page_show(ENG_PAGE_CONFIRM);
            break;

        case ENG_PAGE_CONFIRM:
            if(S_tState.ucConfirmSel == 1)  /* 确认 */
            {
                switch(S_tState.ucSsSel)
                {
                    case 0: /* SAVE & EXIT */
                        tpSysTask->ucStep = EMS_SET;
                        tEngMode.ucEngModeItem = 0;
                        tEngMode.cEngModeState = 1;
                        break;

                    case 1: /* RESET DEFAULTS */
                        tpSysTask->ucStep = EMS_SET;
                        tEngMode.ucEngModeItem = 1;
                        tEngMode.cEngModeState = 1;
                        break;

                    case 2: /* FIRMWARE UPDATE */
                        vApp_JumpToBoot(mainUPDATE_FLAG);
                        break;

                    default:
                        break;
                }
            }
            else  /* 取消 */
            {
                v_page_show(ENG_PAGE_SYS_SET);
            }
            break;

        default:
            break;
    }
}

void vEngMode_KeyBack(void)
{
    vEng_RefreshEngModeTime();

    switch(S_tState.ePage)
    {
        case ENG_PAGE_PARAM_VIEW:
        case ENG_PAGE_PARAM_SET:
        case ENG_PAGE_SYS_SET:
            v_page_show(ENG_PAGE_MAIN_MENU);
            break;

        case ENG_PAGE_CONFIRM:
            v_page_show(ENG_PAGE_SYS_SET);
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

void vEngMode_UiCreate(void)
{
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

    /* 创建基础容器 */
    lv_obj_t *p_base = lv_obj_create(lv_screen_active());
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

    /* 创建各页面 */
    memset(&S_tObjs.p_menu_page, 0,
        (uint8_t *)&S_tObjs.p_cfm_lbls[1] + sizeof(lv_obj_t *) - (uint8_t *)&S_tObjs.p_menu_page);

    v_page_create_menu();
    v_page_create_pv();
    v_page_create_ps();
    v_page_create_ss();
    v_page_create_cfm();

    /* 显示主菜单 */
    v_page_show(ENG_PAGE_MAIN_MENU);
}

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

bool bEngMode_IsExitReq(void)
{
    return S_tState.bExitReq;
}

void vEngMode_UiTick(void)
{
    if(S_tObjs.p_base == NULL)
        return;

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