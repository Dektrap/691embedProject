#include "event.h"
#include <Arduino.h>

static const lv_color_t COLOR_ON  = lv_color_hex(0x65FF00); // green = on
static const lv_color_t COLOR_OFF = lv_color_hex(0xFF0000); // red = off

#define PIN_LED1 25
#define PIN_LED2 27
#define PIN_LED3 33

#define MQ2_pin 34 // รอปรับค่า pin ตามที่ใช้งานจริง
#define buzzerpin 26 // รอปรับค่า pin ตามที่ใช้งานจริง

int smoke_value = 0;


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

static void apply_led_color(lv_obj_t *led, lv_color_t color)
{
    lv_led_set_color(led, color);
    lv_obj_set_style_bg_color(led, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(led, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(led, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(led, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(led, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_outline_color(led, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_shadow_color(led, lv_color_white(), LV_PART_MAIN);
}

static void set_light(lv_obj_t *status_led, int pin, bool on)
{
    digitalWrite(pin, on ? HIGH : LOW);
    apply_led_color(status_led, on ? COLOR_ON : COLOR_OFF);
    lv_led_on(status_led);
}

void init_status_leds()
{
    set_light(objects.st_led1, PIN_LED1, false);
    set_light(objects.st_led2, PIN_LED2, false);
    set_light(objects.st_led3, PIN_LED3, false);
}

// The theme paints the checked track with its primary color, so the checked
// state needs the same override as the default one to stay a single color.
void init_switch_colors()
{
    lv_obj_t *switches[] = {objects.bt_led1, objects.bt_led2, objects.bt_led3};
    lv_color_t track_color = lv_color_hex(0x8c8c8c);

    for (unsigned i = 0; i < sizeof(switches) / sizeof(switches[0]); i++) {
        lv_obj_set_style_bg_color(switches[i], track_color, LV_PART_MAIN);
        lv_obj_set_style_bg_color(switches[i], track_color, LV_PART_INDICATOR | LV_STATE_CHECKED);
    }
}

void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    lv_obj_t *obj = lv_event_get_target(e);
    bool on = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (obj == objects.bt_led1) {
        set_light(objects.st_led1, PIN_LED1, on);
        Serial.println(on ? "LED1 ON" : "LED1 OFF");
    } else if (obj == objects.bt_led2) {
        set_light(objects.st_led2, PIN_LED2, on);
        Serial.println(on ? "LED2 ON" : "LED2 OFF");
    } else if (obj == objects.bt_led3) {
        set_light(objects.st_led3, PIN_LED3, on);
        Serial.println(on ? "LED3 ON" : "LED3 OFF");
    }
}


