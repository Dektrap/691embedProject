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

    // เริ่มต้นฮาร์ดแวร์ภายนอกและระบบเครือข่าย
    void smartroom_hw_init();
    void init_wifi_network();

    // ฟังก์ชันส่งข้อความแจ้งเตือนผ่าน LINE Messaging API
    void send_line_message(const char* message);

    // Tasks ประจำที่รันวนลูปผ่าน lv_timer (Non-blocking)
    void task_environment_update(lv_timer_t *timer); // อุณหภูมิ, ความชื้น, ควัน
    void task_door_security(lv_timer_t *timer);       // RFID, Ultrasonic, Servo ประตู

    // Callback รองรับปุ่มกดพัดลมบนหน้าจอ TFT
    void event_fan_handler(lv_event_t *e);

#ifdef __cplusplus
}
#endif
#endif