#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_main_booting() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_booting = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 130, 87);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_66, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "ON");
        }
        {
            // uc_booting_bar
            lv_obj_t *obj = lv_bar_create(parent_obj);
            objects.uc_booting_bar = obj;
            lv_obj_set_pos(obj, 45, 182);
            lv_obj_set_size(obj, 230, 10);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
    }
    
    tick_screen_main_booting();
}

void tick_screen_main_booting() {
}

void create_screen_main_work() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_work = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, -3, -16);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_66, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 116, 66);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_50, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 134, 192);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, -113, 66);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_50, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 34, 213);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_w);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 260, 213);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_w);
        }
        {
            // b_dev_pv_state
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.b_dev_pv_state = obj;
            lv_obj_set_pos(obj, 20, 102);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_tx60);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // b_dev_ac_in_state
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.b_dev_ac_in_state = obj;
            lv_obj_set_pos(obj, 53, 101);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_ac);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // b_dev_ac_out_state
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.b_dev_ac_out_state = obj;
            lv_obj_set_pos(obj, 285, 76);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_ac);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // b_dev_usb_state
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.b_dev_usb_state = obj;
            lv_obj_set_pos(obj, 254, 105);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_usb);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // b_dev_dc_state
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.b_dev_dc_state = obj;
            lv_obj_set_pos(obj, 285, 101);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_dc);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 195, 93);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon__);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 110, 195);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_time);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 11, 133);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_in);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 238, 133);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_out);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 1, 29);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        }
        {
            // b_err_icon_ol
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.b_err_icon_ol = obj;
            lv_obj_set_pos(obj, 238, 51);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_ol);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // b_err_icon_ot
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.b_err_icon_ot = obj;
            lv_obj_set_pos(obj, 280, 18);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon_ot);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // uca_err_code
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.uca_err_code = obj;
            lv_obj_set_pos(obj, 143, 2);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xf40202), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
    }
    
    tick_screen_main_work();
}

void tick_screen_main_work() {
    {
        const char *new_val = get_var_uca_bat_soc_value();
        const char *cur_val = lv_label_get_text(objects.obj0);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj0;
            lv_label_set_text(objects.obj0, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_uca_out_pwr_value();
        const char *cur_val = lv_label_get_text(objects.obj1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj1;
            lv_label_set_text(objects.obj1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_uca_remaining_usage_time();
        const char *cur_val = lv_label_get_text(objects.obj2);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj2;
            lv_label_set_text(objects.obj2, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_uca_in_pwr_value();
        const char *cur_val = lv_label_get_text(objects.obj3);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj3;
            lv_label_set_text(objects.obj3, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_uca_err_code_value();
        const char *cur_val = lv_label_get_text(objects.uca_err_code);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.uca_err_code;
            lv_label_set_text(objects.uca_err_code, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_main_closing() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_closing = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 120, 87);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_66, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "OFF");
        }
    }
    
    tick_screen_main_closing();
}

void tick_screen_main_closing() {
}

void create_screen_main_update() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_update = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, 0, 20);
            lv_obj_set_size(obj, 300, 30);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_DOTS);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "FIRMWARE UPDATE");
        }
        {
            // uc_update_spinner
            lv_obj_t *obj = lv_spinner_create(parent_obj);
            objects.uc_update_spinner = obj;
            lv_obj_set_pos(obj, 113, 55);
            lv_obj_set_size(obj, 99, 87);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x2d3748), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x00e5ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }
        {
            // uc_update_waiting_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.uc_update_waiting_label = obj;
            lv_obj_set_pos(obj, 0, 150);
            lv_obj_set_size(obj, 300, 26);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // uc_update_bar
            lv_obj_t *obj = lv_bar_create(parent_obj);
            objects.uc_update_bar = obj;
            lv_obj_set_pos(obj, 31, 186);
            lv_obj_set_size(obj, 260, 14);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 7, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x1e88e5), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 7, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, -4, 73);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_50, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj6 = obj;
            lv_obj_set_pos(obj, 0, 210);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_MID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 183, 96);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_icon__);
        }
    }
    
    tick_screen_main_update();
}

void tick_screen_main_update() {
    {
        const char *new_val = get_var_uca_update_msg();
        const char *cur_val = lv_label_get_text(objects.uc_update_waiting_label);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.uc_update_waiting_label;
            lv_label_set_text(objects.uc_update_waiting_label, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_uca_update_progress();
        const char *cur_val = lv_label_get_text(objects.obj5);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj5;
            lv_label_set_text(objects.obj5, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_uca_update_countdown();
        const char *cur_val = lv_label_get_text(objects.obj6);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj6;
            lv_label_set_text(objects.obj6, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_main_eng() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_eng = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 120, 87);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_barlow_condensed_regular_66, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "OFF");
        }
    }
    
    tick_screen_main_eng();
}

void tick_screen_main_eng() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main_booting,
    tick_screen_main_work,
    tick_screen_main_closing,
    tick_screen_main_update,
    tick_screen_main_eng,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 5) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "BarlowCondensed-Regular-66", &ui_font_barlow_condensed_regular_66 },
    { "BarlowCondensed-Regular-50", &ui_font_barlow_condensed_regular_50 },
    { "BarlowCondensed-Regular-26", &ui_font_barlow_condensed_regular_26 },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main_booting();
    create_screen_main_work();
    create_screen_main_closing();
    create_screen_main_update();
    create_screen_main_eng();
}