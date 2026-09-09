#ifndef EVENT_H
#define EVENT_H
#include <lvgl.h>
#include "ui.h"
#if defined(EEZ_FOR_LVGL)
#include <eez/flow/lvgl_api.h>
#endif
#if !defined(EEZ_FOR_LVGL)
#include "screens.h"
#endif
#ifdef __cplusplus
extern "C"
{
#endif

    void event_handler(lv_event_t*e);
    void smoke_check();
    ///เพิ่ม
    void sensor_init();
    void update_dht_sensor(lv_timer_t *timer);
    ///  
    void update_smoke_status(lv_timer_t *timer);

#ifdef __cplusplus
}
#endif
#endif