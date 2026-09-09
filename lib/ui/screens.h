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
    SCREEN_ID_WELCOME_PAGE = 2,
    SCREEN_ID_INFO_PAGE = 3,
    _SCREEN_ID_LAST = 3
};

typedef struct _objects_t {
    lv_obj_t *home_page;
    lv_obj_t *welcome_page;
    lv_obj_t *info_page;
    lv_obj_t *led1_bt;
    lv_obj_t *led2_bt;
    lv_obj_t *led3_bt;
    lv_obj_t *led1_st;
    lv_obj_t *led2_st;
    lv_obj_t *led3_st;
    lv_obj_t *temp_val;
    lv_obj_t *smoke_popup;
    lv_obj_t *info_bt;
    lv_obj_t *humi_val;
    lv_obj_t *home_bt;
    lv_obj_t *name_txet;
    lv_obj_t *ids_txet;
} objects_t;

extern objects_t objects;

void create_screen_home_page();
void tick_screen_home_page();

void create_screen_welcome_page();
void tick_screen_welcome_page();

void create_screen_info_page();
void tick_screen_info_page();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/