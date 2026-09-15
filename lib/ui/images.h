#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_home__1_;
extern const lv_img_dsc_t img_engine_warning;
extern const lv_img_dsc_t img_lightbulb_on;
extern const lv_img_dsc_t img_lightbulb_slash;
extern const lv_img_dsc_t img_fan;
extern const lv_img_dsc_t img_fire_smoke;
extern const lv_img_dsc_t img_ruler_triangle;
extern const lv_img_dsc_t img_door_closed;
extern const lv_img_dsc_t img_humidity;
extern const lv_img_dsc_t img_thermometer_notification;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[10];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/