#include "event.h"
#include <Arduino.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// --- ข้อมูล Wi-Fi และ LINE API ---
const char* WIFI_SSID     = "Free";
const char* WIFI_PASSWORD = "123456900";
const char* LINE_TOKEN    = "eYgGJtjVWqL9OSL26SxeNn04+uE7Q02bRvkOKZ8HtL8Smb7W+ExQKRa7+24AMufPNqOAu1Uh6rqhIQvj5HnVjcmQzNEK1PIdCMLcLxj5sR84Ow8yqHqMFvQv/KxegH430fbe5I/LHiSHrJT6ajr+agdB04t89/1O/w1cDnyilFU=";
const char* LINE_USER_ID  = "U4959f7913765cd32012e9c1f3b313f0f";

// --- กำหนด Pinout อุปกรณ์ ---
#define DHTPIN         16
#define DHTTYPE        DHT22
#define PIN_TRIG       17
#define PIN_ECHO       35
#define PIN_SERVO      25
#define PIN_RFID_SS    5
#define PIN_RFID_RST   22
#define PIN_SMOKE_AO   34
#define PIN_RELAY_FAN  26
#define PIN_BUZZER     14

// ประกาศ Object
static DHT dht(DHTPIN, DHTTYPE);
static MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);
static Servo doorServo;

// กำหนด UID ของบัตรที่อนุญาต (นำบัตรจริงมาสแกนดู UID บน Serial Monitor แล้วมาแก้ตรงนี้)
static const byte VALID_CARD[4] = {0xDE, 0xAD, 0xBE, 0xEF};

// State Machine สำหรับระบบประตู (ประกาศเพียงจุดเดียว)
enum DoorState { 
    DOOR_LOCKED,        // 1. ประตูปิด ล็อค รอแตะบัตร
    CARD_AUTHENTICATED, // 2. บัตรผ่านแล้ว กำลังรอคนเดินเข้ามาใกล้
    DOOR_OPEN,          // 3. ประตูเปิดแล้ว คนกำลังเดินผ่าน
    WAITING_LEAVE       // 4. รอคนเดินพ้นระยะเพื่อปิดประตู
};

static DoorState currentDoorState = DOOR_LOCKED;
static unsigned long personClearTimer = 0;
static unsigned long authTimeoutTimer = 0;
static bool smokeAlertSent = false;

void init_wifi_network() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nWiFi Connection Failed (Running Offline)");
    }
}

void send_line_message(const char* message) {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, "https://api.line.me/v2/bot/message/push");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + LINE_TOKEN);

    String payload = "{\"to\":\"" + String(LINE_USER_ID) + "\",\"messages\":[{\"type\":\"text\",\"text\":\"" + String(message) + "\"}]}";

    int httpCode = http.POST(payload);
    if (httpCode > 0) {
        Serial.printf("[LINE] Message sent, Code: %d\n", httpCode);
    } else {
        Serial.printf("[LINE] Failed, Error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

void smartroom_hw_init() {
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    pinMode(PIN_RELAY_FAN, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_RELAY_FAN, LOW);
    digitalWrite(PIN_BUZZER, LOW);

    dht.begin();
    rfid.PCD_Init();
    doorServo.attach(PIN_SERVO);
    doorServo.detach(); // ป้องกันเซอร์โว 360 หมุนค้างตอนเปิดเครื่อง
}

long get_distance_cm() {
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    long duration = pulseIn(PIN_ECHO, HIGH, 25000);
    if (duration == 0) return 999;
    return duration * 0.034 / 2;
}

void task_environment_update(lv_timer_t *timer) {
    LV_UNUSED(timer);

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    char str_buf[24];

    if (!isnan(t) && objects.temp_val != NULL) {
        snprintf(str_buf, sizeof(str_buf), "Temp: %.1f °C", t);
        lv_label_set_text(objects.temp_val, str_buf);
    }
    if (!isnan(h) && objects.humi_val != NULL) {
        snprintf(str_buf, sizeof(str_buf), "Humi: %.1f %%", h);
        lv_label_set_text(objects.humi_val, str_buf);
    }

    int smoke_raw = analogRead(PIN_SMOKE_AO);
    if (objects.smoke_val != NULL) {
        if (smoke_raw > 480) {
            lv_label_set_text(objects.smoke_val, "Smoke: DANGER!");
            digitalWrite(PIN_BUZZER, HIGH);
            digitalWrite(PIN_RELAY_FAN, HIGH);

            if (!smokeAlertSent) {
                send_line_message("⚠️ แจ้งเตือนด่วน: พบควันไฟหนาแน่นผิดปกติในห้อง Smart Room!");
                smokeAlertSent = true;
            }
        } else {
            lv_label_set_text(objects.smoke_val, "Smoke: NORMAL");
            digitalWrite(PIN_BUZZER, LOW);
            smokeAlertSent = false;
        }
    }
}
/*
void task_door_security(lv_timer_t *timer) {
    LV_UNUSED(timer);
    long dist = get_distance_cm();

    if (objects.dist_val != NULL) {
        char dist_str[24];
        snprintf(dist_str, sizeof(dist_str), "Dist: %ld cm", dist);
        lv_label_set_text(objects.dist_val, dist_str);
    }

    // 1. รอการสแกนบัตร RFID
    if (currentDoorState == DOOR_LOCKED) {
        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
            // แสดง UID ที่อ่านได้จริงบน Serial Monitor เพื่อนำไปใช้ตั้งค่า
            Serial.print("Card UID: ");
            for (byte i = 0; i < rfid.uid.size; i++) {
                Serial.printf("0x%02X ", rfid.uid.uidByte[i]);
            }
            Serial.println();

            bool access_granted = true;
            for (byte i = 0; i < 4; i++) {
                if (rfid.uid.uidByte[i] != VALID_CARD[i]) {
                    access_granted = false;
                    break;
                }
            }

            if (access_granted) {
                currentDoorState = CARD_AUTHENTICATED;
                authTimeoutTimer = millis();

                if (objects.door_status) lv_label_set_text(objects.door_status, "DOOR: [ READY ]");
                if (objects.rfid_msg) lv_label_set_text(objects.rfid_msg, "Please Walk In...");
                send_line_message("🔑 บัตรผ่าน: กำลังรอผู้ใช้เดินเข้าประตู");
            } else {
                if (objects.rfid_msg) lv_label_set_text(objects.rfid_msg, "Access Denied!");
                digitalWrite(PIN_BUZZER, HIGH);
                delay(100);
                digitalWrite(PIN_BUZZER, LOW);
                send_line_message("⛔ เตือน: มีการแตะบัตรไม่ถูกต้อง");
            }

            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
        }
    }
    // 2. สิทธิ์ผ่านแล้ว รอคนเดินเข้ามาใกล้ (< 30 cm)
    else if (currentDoorState == CARD_AUTHENTICATED) {
        if (millis() - authTimeoutTimer > 10000) {
            currentDoorState = DOOR_LOCKED;
            if (objects.door_status) lv_label_set_text(objects.door_status, "DOOR: [ CLOSED ]");
            if (objects.rfid_msg) lv_label_set_text(objects.rfid_msg, "Scan Timeout!");
        }
        else if (dist > 0 && dist < 30) {
            doorServo.attach(PIN_SERVO);
            doorServo.write(180);
            delay(300);
            doorServo.detach();

            currentDoorState = DOOR_OPEN;
            if (objects.door_status) lv_label_set_text(objects.door_status, "DOOR: [ OPEN ]");
            if (objects.rfid_msg) lv_label_set_text(objects.rfid_msg, "Welcome!");
            send_line_message("🚪 ประตูเปิดเรียบร้อย");
        }
    }
    // 3. ประตูเปิดแล้ว รอตรวจจับคนก้าวข้ามประตู
    else if (currentDoorState == DOOR_OPEN) {
        if (dist > 0 && dist < 10) {
            currentDoorState = WAITING_LEAVE;
            personClearTimer = millis();
        }
    }
    // 4. คนเดินพ้นระยะเกิน 2 วินาที สั่งปิดประตู
    else if (currentDoorState == WAITING_LEAVE) {
        if (dist >= 25 && (millis() - personClearTimer > 3000)) {
            doorServo.attach(PIN_SERVO);
            doorServo.write(0);
            delay(300);
            doorServo.detach();

            currentDoorState = DOOR_LOCKED;
            if (objects.door_status) lv_label_set_text(objects.door_status, "DOOR: [ CLOSED ]");
            if (objects.rfid_msg) lv_label_set_text(objects.rfid_msg, "Scan your card...");
        }
    }
}*/
void task_door_security(lv_timer_t *timer) {
    LV_UNUSED(timer);
    long dist = get_distance_cm();

    if (objects.dist_val != NULL) {
        char dist_str[24];
        snprintf(dist_str, sizeof(dist_str), "Dist: %ld cm", dist);
        lv_label_set_text(objects.dist_val, dist_str);
    }

    // 1. ตรวจสอบเงื่อนไขเปิดประตู (เอามือเข้าใกล้ < 10 cm)
    if (currentDoorState == DOOR_LOCKED) {
        if (dist > 0 && dist < 10) { 
            doorServo.attach(PIN_SERVO);
            doorServo.write(180); // หมุนเปิด
            delay(300);           // ปรับเวลาหมุนเปิด (ลองลด/เพิ่มได้ตามความกว้างประตู)
            doorServo.detach();   // ตัดสัญญาณทันที มอเตอร์จะหยุดหมุนสนิท

            currentDoorState = DOOR_OPEN;
            if (objects.door_status) lv_label_set_text(objects.door_status, "DOOR: [ OPEN ]");
            if (objects.rfid_msg) lv_label_set_text(objects.rfid_msg, "Door Opened (Bypass)");
            send_line_message("🚪 เปิดประตูด้วยเซนเซอร์ตรวจจับ");
        }
    }
    // 2. มีคนเดินผ่านประตูเข้ามา (< 25 cm)
    else if (currentDoorState == DOOR_OPEN) {
        if (dist > 0 && dist < 25) {
            currentDoorState = WAITING_LEAVE;
            personClearTimer = millis();
        }
    }
    // 3. เมื่อคนพ้นระยะ (> 25 cm) เกิน 2 วินาที สั่งปิดประตู
    else if (currentDoorState == WAITING_LEAVE) {
        if (dist >= 25 && (millis() - personClearTimer > 3000)) {
            doorServo.attach(PIN_SERVO);
            doorServo.write(0);   // หมุนปิดย้อนกลับ
            delay(300);           // หน่วงเวลาให้หมุนกลับตำแหน่งเดิม
            doorServo.detach();   // ตัดสัญญาณหยุดทันที

            currentDoorState = DOOR_LOCKED;
            if (objects.door_status) lv_label_set_text(objects.door_status, "DOOR: [ CLOSED ]");
            if (objects.rfid_msg) lv_label_set_text(objects.rfid_msg, "Scan your card...");
        }
    }
}

void event_fan_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        if (obj == objects.fan_on) {
            digitalWrite(PIN_RELAY_FAN, HIGH);
            if (objects.fan_state) lv_label_set_text(objects.fan_state, "Fan: [ ON ]");
        }
        else if (obj == objects.fan_off) {
            digitalWrite(PIN_RELAY_FAN, LOW);
            if (objects.fan_state) lv_label_set_text(objects.fan_state, "Fan: [ OFF ]");
        }
    }
}