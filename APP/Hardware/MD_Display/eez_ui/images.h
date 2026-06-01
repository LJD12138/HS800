#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_icon_w;
extern const lv_img_dsc_t img_icon_out;
extern const lv_img_dsc_t img_icon_in;
extern const lv_img_dsc_t img_icon_tx60;
extern const lv_img_dsc_t img_icon__;
extern const lv_img_dsc_t img_icon_ac;
extern const lv_img_dsc_t img_icon_dc;
extern const lv_img_dsc_t img_icon_ot;
extern const lv_img_dsc_t img_icon_bat_ot;
extern const lv_img_dsc_t img_icon_time;
extern const lv_img_dsc_t img_icon_usb;
extern const lv_img_dsc_t img_1;
extern const lv_img_dsc_t img_2;
extern const lv_img_dsc_t img_3;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[14];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/