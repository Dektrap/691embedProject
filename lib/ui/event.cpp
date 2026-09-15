#include "event.h"

// --- ข้อมูล Wi-Fi และ LINE API ---
const char* WIFI_SSID     = "Free";
const char* WIFI_PASSWORD = "123456900";
const char* LINE_TOKEN    = "eYgGJtjVWqL9OSL26SxeNn04+uE7Q02bRvkOKZ8HtL8Smb7W+ExQKRa7+24AMufPNqOAu1Uh6rqhIQvj5HnVjcmQzNEK1PIdCMLcLxj5sR84Ow8yqHqMFvQv/KxegH430fbe5I/LHiSHrJT6ajr+agdB04t89/1O/w1cDnyilFU=";
const char* LINE_USER_ID  = "U4959f7913765cd32012e9c1f3b313f0f";

// ประกาศ Object
static DHT dht(DHTPIN, DHTTYPE);
static MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);
static Servo doorServo;

std::vector<Student> studentList; //เก็บรายชื่อจาก csv

void smartroom_hw_init() {
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    pinMode(PIN_RELAY_FAN, INPUT_PULLUP);
    pinMode(PIN_BUZZER, OUTPUT);
    // digitalWrite(PIN_RELAY_FAN, HIGH);
    digitalWrite(PIN_BUZZER, LOW);
    pinMode(PIN_SMOKE_AO, INPUT);
    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
    pinMode(PIN_LED3, OUTPUT);
    digitalWrite(PIN_LED1, LOW);
    digitalWrite(PIN_LED2, LOW);
    digitalWrite(PIN_LED3, LOW);


    dht.begin();
    SPI.begin();
    rfid.PCD_Init();
    doorServo.attach(PIN_SERVO);
    doorServo.detach(); // ป้องกันเซอร์โว 360 หมุนค้างตอนเปิดเครื่อง
}

void init_wifi_network() 
{
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

void load_csv() {
    File file = LittleFS.open("/student.csv", "r");
    if (!file) {
        Serial.println("Failed to open students.csv");
        return; // ยกเลิกการทำงานของฟังก์ชันนี้ทันที
    }
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim(); // ลบ whitespace และ newline
        if (line.length() == 0) continue;
        int firstComma = line.indexOf(',');
        int secondComma = line.indexOf(',', firstComma + 1);
        if (firstComma >0 && secondComma > 0){
            Student s_data;
            s_data.rfid_uid = line.substring(0, firstComma);
            s_data.student_id = line.substring(firstComma + 1, secondComma);
            s_data.name = line.substring(secondComma + 1);
            s_data.is_checked_in = false; // ยังไม่ได้เช็คชื่อ 
            studentList.push_back(s_data);
        }
    }
    file.close();
}

Student* scanRFID() {
    if (!rfid.PICC_IsNewCardPresent() /**มีบัตรมาแตะหรือไม่**/|| !rfid.PICC_ReadCardSerial()/**มี uid ในบัตรไหม**/) 
    {
        return nullptr; // ไม่มีบัตรใหม่
    }
    String rfid_uid = ""; //อ่านค่า uid
    for (byte i = 0; i < rfid.uid.size; i++) {
        rfid_uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        rfid_uid += String(rfid.uid.uidByte[i], HEX);
    }
    rfid_uid.toUpperCase(); // แปลงเป็นตัวพิมพ์ใหญ่เพื่อความสอดคล้องกับ CSV
    rfid.PICC_HaltA(); // กันสแกนเบิ้ล
    for (int i = 0; i < studentList.size(); i++) {
        if (studentList[i].rfid_uid == rfid_uid) { // ถ้ารหัส UID ตรงกัน
            
            Serial.println("UID: " + rfid_uid);
            Serial.println("Name: " + studentList[i].name);
            Serial.println("Student ID: " + studentList[i].student_id);
            // คืนค่าตำแหน่งหน่วยความจำ (Pointer) ของนักศึกษาคนนี้กลับไปให้ main.cpp เอาไปใช้งานต่อ
            return &studentList[i]; 
        }
    }
    String error_msg = "Unknown UID: " + rfid_uid;
    Serial.println(error_msg);
    if (objects.info_text) lv_label_set_text(objects.info_text, error_msg.c_str());
    return nullptr; // ไม่พบนักศึกษาที่ตรงกับบัตร
}

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
    printf("Smoke : " + smoke_raw);
    if (objects.smoke_val != NULL) {
        if (smoke_raw > 480) {
            lv_label_set_text(objects.smoke_val, "Smoke: DANGER!");
            digitalWrite(PIN_BUZZER, HIGH);
            pinMode(PIN_RELAY_FAN, OUTPUT);
            //digitalWrite(PIN_RELAY_FAN, LOW);

            if (!smokeAlertSent) {
                send_line_message("⚠️ แจ้งเตือนด่วน: พบควันไฟหนาแน่นผิดปกติในห้อง Smart Room!");
                smokeAlertSent = true;
            }
        }   else {
            lv_label_set_text(objects.smoke_val, "Smoke: NORMAL");
            digitalWrite(PIN_BUZZER, LOW);
            smokeAlertSent = false;
        }
    }
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

void task_door_security(lv_timer_t *timer) {
    LV_UNUSED(timer);
    long dist = get_distance_cm();
    Student* currentStudent = scanRFID();

    static unsigned long displayTimer = 0; //จับเวลา
    static bool isShowingData = false;

    if (objects.dist_val != NULL) {
        char dist_str[24];
        snprintf(dist_str, sizeof(dist_str), "Dist: %ld cm", dist);
        lv_label_set_text(objects.dist_val, dist_str);
    }

    // 1. ตรวจสอบเงื่อนไขเปิดประตู
    if (currentDoorState == DOOR_LOCKED) {
        
        // กรณีที่ 1: มีการแตะบัตร
        if (currentStudent != nullptr) { 
            doorServo.attach(PIN_SERVO);
            doorServo.write(180); // หมุนเปิด
            delay(300);           
            doorServo.detach();   

            currentDoorState = DOOR_OPEN;
            if (objects.door_status) lv_label_set_text(objects.door_status, "DOOR: [ OPEN ]");
            
            // โชว์รหัสนักศึกษา/ชื่อ ขึ้นจอ
            if (objects.name_txet) lv_label_set_text(objects.name_txet, currentStudent->name.c_str());
            if (objects.ids_txet) lv_label_set_text(objects.ids_txet, currentStudent->student_id.c_str());

            // เช็คชื่อและส่ง LINE
            if (currentStudent->is_checked_in == false) {
                String line_msg = "✅ " + currentStudent->student_id + " " + currentStudent->name + " เช็คชื่อเข้าเรียนสำเร็จ";
                send_line_message(line_msg.c_str());
                currentStudent->is_checked_in = true; 
                if (objects.info_text) lv_label_set_text(objects.info_text, "Success Check in !!");
            } else {
                if (objects.info_text) lv_label_set_text(objects.info_text, "Door open by id !!");
            }

            displayTimer = millis(); 
            isShowingData = true;
        }
        else if (dist < 5) {
            doorServo.attach(PIN_SERVO);
            doorServo.write(180); // หมุนเปิด
            delay(500);           
            doorServo.detach();   

            currentDoorState = DOOR_OPEN;
            if (objects.door_status) lv_label_set_text(objects.door_status, "DOOR: [ OPEN ]");
            if (objects.info_text) lv_label_set_text(objects.info_text, "Door open by sensor !");
            
            displayTimer = millis(); 
            isShowingData = true;
        }
    }
    
    // 2. มีคนเดินผ่านประตูเข้ามา (< 10 cm) ตอนประตูเปิดอยู่
    else if (currentDoorState == DOOR_OPEN) {
        if (dist < 10) {
            currentDoorState = WAITING_LEAVE;
            personClearTimer = millis();
        }
    }
    
    // 3. เมื่อคนพ้นระยะ (> 10 cm) เกิน 3 วินาที สั่งปิดประตู
    else if (currentDoorState == WAITING_LEAVE) {
        if (dist > 10 && (millis() - personClearTimer > 3000)) {
            doorServo.attach(PIN_SERVO);
            doorServo.write(0);   // หมุนปิดย้อนกลับ
            delay(300);           
            doorServo.detach();   

            currentDoorState = DOOR_LOCKED;
            if (objects.door_status) lv_label_set_text(objects.door_status, "DOOR: [ CLOSED ]");
            if (objects.info_text) lv_label_set_text(objects.info_text, "Scan your card...");
        }
    }

    // 4. เคลียร์ข้อความบนหน้าจอเมื่อครบ 3 วินาที
    if (isShowingData == true && (millis() - displayTimer > 3000)) {
        if (objects.info_text) {
            lv_label_set_text(objects.info_text, "Scan Student id ..."); // แก้คำผิดให้ด้วยจาก Studebt เป็น Student
        }
        if (objects.name_txet) lv_label_set_text(objects.name_txet, "");
        if (objects.ids_txet) lv_label_set_text(objects.ids_txet, "");
        
        isShowingData = false; 
    }
}

void event_fan_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        if (obj == objects.fan_on) {
            // digitalWrite(PIN_RELAY_FAN, LOW);
            pinMode(PIN_RELAY_FAN, OUTPUT);
            digitalWrite(PIN_RELAY_FAN, LOW);
            if (objects.fan_state) lv_label_set_text(objects.fan_state, "Fan: [ ON ]");
        }
        else if (obj == objects.fan_off) {
            // digitalWrite(PIN_RELAY_FAN, HIGH);
            pinMode(PIN_RELAY_FAN, INPUT_PULLUP);
            if (objects.fan_state) lv_label_set_text(objects.fan_state, "Fan: [ OFF ]");
        }
    }
}

void home_info_bt (lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (obj == objects.home_bt){
        lv_scr_load(objects.home_page);
    }

    if (obj == objects.info_bt){
        lv_scr_load(objects.info_page);
    }
}

void event_switch_led1_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    // สำหรับ Switch ใน LVGL จะใช้ Event: LV_EVENT_VALUE_CHANGED
    if (code == LV_EVENT_VALUE_CHANGED) {
        // เช็คว่าสวิตช์ถูกเปิด (ON) อยู่หรือไม่
        bool is_checked = lv_obj_has_state(obj, LV_STATE_CHECKED);

        if (is_checked) {
            // เมื่อสวิตช์เลื่อนไปที่ ON -> เปลี่ยนเป็นรูปไฟติด
            if (objects.img_led1_on)  lv_obj_clear_flag(objects.img_led1_on, LV_OBJ_FLAG_HIDDEN); // แสดง (Show)
            if (objects.img_led1_off) lv_obj_add_flag(objects.img_led1_off, LV_OBJ_FLAG_HIDDEN);   // ซ่อน (Hide)
            digitalWrite(PIN_LED1, HIGH);

        } else {
            if (objects.img_led1_on)  lv_obj_add_flag(objects.img_led1_on, LV_OBJ_FLAG_HIDDEN);   // ซ่อน (Hide)
            if (objects.img_led1_off) lv_obj_clear_flag(objects.img_led1_off, LV_OBJ_FLAG_HIDDEN); // แสดง (Show)
            digitalWrite(PIN_LED1, LOW);
            
        }
    }
}

void event_switch_led2_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    // สำหรับ Switch ใน LVGL จะใช้ Event: LV_EVENT_VALUE_CHANGED
    if (code == LV_EVENT_VALUE_CHANGED) {
        // เช็คว่าสวิตช์ถูกเปิด (ON) อยู่หรือไม่
        bool is_checked2 = lv_obj_has_state(obj, LV_STATE_CHECKED);

        if (is_checked2) {
            // เมื่อสวิตช์เลื่อนไปที่ ON -> เปลี่ยนเป็นรูปไฟติด
            if (objects.img_led2_on)  lv_obj_clear_flag(objects.img_led2_on, LV_OBJ_FLAG_HIDDEN); // แสดง (Show)
            if (objects.img_led2_off) lv_obj_add_flag(objects.img_led2_off, LV_OBJ_FLAG_HIDDEN);   // ซ่อน (Hide)
            digitalWrite(PIN_LED2, HIGH);

        } else {
            if (objects.img_led2_on)  lv_obj_add_flag(objects.img_led2_on, LV_OBJ_FLAG_HIDDEN);   // ซ่อน (Hide)
            if (objects.img_led2_off) lv_obj_clear_flag(objects.img_led2_off, LV_OBJ_FLAG_HIDDEN); // แสดง (Show)
            digitalWrite(PIN_LED2, LOW);
            
        }
    }
}

void event_switch_led3_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    // สำหรับ Switch ใน LVGL จะใช้ Event: LV_EVENT_VALUE_CHANGED
    if (code == LV_EVENT_VALUE_CHANGED) {
        // เช็คว่าสวิตช์ถูกเปิด (ON) อยู่หรือไม่
        bool is_checked3 = lv_obj_has_state(obj, LV_STATE_CHECKED);

        if (is_checked3) {
            // เมื่อสวิตช์เลื่อนไปที่ ON -> เปลี่ยนเป็นรูปไฟติด
            if (objects.img_led3_on)  lv_obj_clear_flag(objects.img_led3_on, LV_OBJ_FLAG_HIDDEN); // แสดง (Show)
            if (objects.img_led3_off) lv_obj_add_flag(objects.img_led3_off, LV_OBJ_FLAG_HIDDEN);   // ซ่อน (Hide)
            digitalWrite(PIN_LED3, HIGH);

        } else {
            if (objects.img_led3_on)  lv_obj_add_flag(objects.img_led3_on, LV_OBJ_FLAG_HIDDEN);   // ซ่อน (Hide)
            if (objects.img_led3_off) lv_obj_clear_flag(objects.img_led3_off, LV_OBJ_FLAG_HIDDEN); // แสดง (Show)
            digitalWrite(PIN_LED3, LOW);
            
        }
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