#include "event.h"
#include <Arduino.h>
#include <DHT.h>
#define MQ2_pin 34 // รอปรับค่า pin ตามที่ใช้งานจริง
#define buzzerpin 26 // รอปรับค่า pin ตามที่ใช้งานจริง
#define DHTPIN 16       // ขา Data ของ DHT22 (เลือกใช้ GPIO4 หรือขาที่ว่าง)
#define DHTTYPE DHT22  // DHT11
int smoke_value = 0;

static DHT dht(DHTPIN, DHTTYPE);

void sensor_init() {
    dht.begin();
}

void update_dht_sensor(lv_timer_t *timer) {
    LV_UNUSED(timer);

    float temp = dht.readTemperature();
    float humi = dht.readHumidity();

    char temp_str[16];
    char humi_str[16];

    if (isnan(temp) || isnan(humi)) {
        snprintf(temp_str, sizeof(temp_str), "--.- C");
        snprintf(humi_str, sizeof(humi_str), "--.- %%");
    } else {
        snprintf(temp_str, sizeof(temp_str), "%.1f C", temp);
        snprintf(humi_str, sizeof(humi_str), "%.1f %%", humi);
    }

    // อัปเดตค่าลง Object Label ที่สร้างจาก EEZ Studio
    if (objects.temp_val != NULL) {
        lv_label_set_text(objects.temp_val, temp_str);
    }
    if (objects.humi_val != NULL) {
        lv_label_set_text(objects.humi_val, humi_str);
    }
}

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
    else if (smoke_value < 1 480) // รอปรับค่า threshold ตามที่ต้องการ
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