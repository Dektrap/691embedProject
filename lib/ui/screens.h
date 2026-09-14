#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_HOME_PAGE = 1,
    SCREEN_ID_INFO_PAGE = 2,
    _SCREEN_ID_LAST = 2
};

typedef struct _objects_t {
    lv_obj_t *home_page;
    lv_obj_t *info_page;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *temp_val;
    lv_obj_t *info_bt;
    lv_obj_t *obj3;
    lv_obj_t *humi_val;
    lv_obj_t *fan_on;
    lv_obj_t *fan_state;
    lv_obj_t *fan_off;
    lv_obj_t *door_status;
    lv_obj_t *dist_val;
    lv_obj_t *smoke_val;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *bt_led1;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *bt_led2;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *bt_led3;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *home_bt;
    lv_obj_t *name_txet;
    lv_obj_t *ids_txet;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *info_text;
} objects_t;

extern objects_t objects;

void create_screen_home_page();
void tick_screen_home_page();

void create_screen_info_page();
void tick_screen_info_page();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/