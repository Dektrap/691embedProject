#include "event.h"
#include <Arduino.h>

#define MQ2_pin 34 ; // รอขาจริง


void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
}

void smoke_check(lv_event_t *e)
{

}
