#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN_BOOTING = 1,
    SCREEN_ID_MAIN_WORK = 2,
    SCREEN_ID_MAIN_CLOSING = 3,
    _SCREEN_ID_LAST = 3
};

typedef struct _objects_t {
    lv_obj_t *main_booting;
    lv_obj_t *main_work;
    lv_obj_t *main_closing;
    lv_obj_t *uc_booting_bar;
    lv_obj_t *b_dev_pv_state;
    lv_obj_t *b_dev_ac_in_state;
    lv_obj_t *b_dev_ac_out_state;
    lv_obj_t *b_dev_usb_state;
    lv_obj_t *b_dev_dc_state;
    lv_obj_t *b_err_icon_ol;
    lv_obj_t *b_err_icon_ot;
    lv_obj_t *uca_err_code;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
} objects_t;

extern objects_t objects;

void create_screen_main_booting();
void tick_screen_main_booting();

void create_screen_main_work();
void tick_screen_main_work();

void create_screen_main_closing();
void tick_screen_main_closing();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/