#include "event.h"
#include <Arduino.h>

#define MQ2_pin 34 // รอปรับค่า pin ตามที่ใช้งานจริง
#define buzzerpin 26 // รอปรับค่า pin ตามที่ใช้งานจริง
int smoke_value = 0;

void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
}

void smoke_check()
{
    Serial.println("Checking smoke value...");

    if (smoke_value >= 480) // รอปรับค่า threshold ตามที่ต้องการ
    {
        lv_obj_clear_flag(objects.smoke_popup, LV_OBJ_FLAG_HIDDEN);
        digitalWrite(buzzerpin,HIGH);
    }
    else if (smoke_value < 480) // รอปรับค่า threshold ตามที่ต้องการ
    {
        lv_obj_add_flag(objects.smoke_popup, LV_OBJ_FLAG_HIDDEN);
        digitalWrite(buzzerpin,LOW);

    }
}

void update_smoke_status(lv_timer_t * timer)
{
    smoke_value = analogRead(MQ2_pin);
    Serial.print("Smoke value: ");
    Serial.println(smoke_value);
    Serial.println("test smoke update");

    smoke_check();
}

void temp_check()
{
    Serial.println("Checking smoke value...");
    
}