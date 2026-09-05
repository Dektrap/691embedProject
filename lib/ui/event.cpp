#include "event.h"
#include <Arduino.h>

#define MQ2_pin 34 // รอปรับค่า pin ตามที่ใช้งานจริง
int smoke_value = 0;

void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
}

void smoke_check()
{
    if (smoke_value >= 500) // รอปรับค่า threshold ตามที่ต้องการ
    {
        lv_obj_clear_flag(objects.smoke_popup, LV_OBJ_FLAG_HIDDEN);
    }
}

void update_smoke_status(lv_timer_t * timer)
{
    int smoke_value = analogRead(MQ2_pin);
    Serial.print("Smoke value: ");
    Serial.println(smoke_value);

}
