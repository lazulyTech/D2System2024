#ifdef ARDUINO_M5STACK_CORE
#include <M5Unified.h>
#include <M5GFX.h>
#else
#include <Arduino.h>
#include <Esp.h>
#include <ServoEasing.hpp>
#endif
#include <EspNow.h>
#include "addr.h"

#define BUTTON_COOLTIME 1000
#define SERVO_FEED  D0
#define SERVO_SHIP  D1
#define BUTTON_FEED D2
#define LED_FEED    D3
#define BUTTON_SHIP D4
#define LED_SHIP    D5
#define SHIP_UP     0
#define SHIP_DOWN 180

EspNow* espNow;
// uint8_t addrIndex[DEVICE_NUM-1] = {0};
bool isHost = false;
enum Status {
    OFFLINE = 0,
    ON_SETTING = 1,
    ON_READY = 2,
    ON_MATCH = 3,
    ON_RESULT = 4,
    TEST = 5,
};

String strStatus(Status st) {
    switch (st) {
        case 0:
            return "OFFLINE";
        break;
        case 1:
            return "ON_SETTING";
        break;
        case 2:
            return "ON_READY";
        break;
        case 3:
            return "ON_MATCH";
        break;
        case 4:
            return "ON_RESULT";
        break;
        case 5:
            return "TEST";
        break;
    }
    return "OFFLINE";
}

Status state = OFFLINE;
Status state_prev = OFFLINE;

typedef struct __attribute__((__packed__)) {
    uint8_t state; // cast from enum Status
    uint8_t step_feeder;
    uint8_t isShip;
} send_struct;

send_struct sendStruct = {0, 0, 0};
send_struct recvStruct = {0, 0, 0};
send_struct field_r = {0, 0, 0};
send_struct field_b = {0, 0, 0};

void OnDataSent(const uint8_t* macAddr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t* macAddr, const uint8_t* data, int len);

unsigned long time_prev = 0;
volatile bool wasHostRecv = false;

unsigned long time_prev_r = 0;
unsigned long time_prev_b = 0;

#ifndef ARDUINO_M5STACK_CORE

/*--- ボタン割り込み ---*/
// ボタン時刻記録
unsigned long time_feed = 0;
unsigned long time_ship = 0;
//フィーダー
volatile bool wasFeedPushed = false;
void IRAM_ATTR feedPushed() {
    if (!wasFeedPushed && millis()-time_feed >= BUTTON_COOLTIME && state != ON_READY && state != ON_RESULT) {
        wasFeedPushed = true;
        // Serial.println("feed!");
        time_feed = millis();
    }
}
int step_feeder = 0;

//出荷ボタン
volatile bool wasShipPushed = false;
volatile bool prev_shiped = false;
void IRAM_ATTR shipPushed() {
    if (!wasShipPushed && millis()-time_ship >= BUTTON_COOLTIME && state != ON_READY && state != ON_RESULT) {
        wasShipPushed = true;
        // Serial.println("ship!");
        time_ship = millis();
    }
}

/*--- ボタン割り込み ---*/

/*--- サーボ ---*/

ServoEasing servo_feed;
ServoEasing servo_ship;

/*--- サーボ ---*/
#else

bool offline = false;

// 画面
// M5Canvas canvas(&display);
M5GFX display;

#endif

void D2Init() {

    getWiFiMacAddr(myAddr);

    espNow = new EspNow();
    espNow->set_SentCallback(NULL);
    espNow->set_RecvCallback(OnDataRecv);
    espNow->Init(MEMBER, sizeof(send_struct));

    // Adding peers
    if (cmpAddr(myAddr, hostAddr)) isHost = true;
    else isHost = false;

    if (isHost) {
        //ホスト機
        for (int i = 0; i < DEVICE_NUM; i++) {
            if (!espNow->addPeer(subAddr[i])) {
                Serial.println("Failed to add peer");
                return;
            }
        }
    } else {
        // フィールド側
        if (!espNow->addPeer(hostAddr)) {
                Serial.println("Failed to add peer");
                return;
        }
        #ifndef ARDUINO_M5STACK_CORE
        pinMode(BUTTON_FEED, INPUT_PULLUP);
        pinMode(BUTTON_SHIP, INPUT_PULLUP);
        attachInterrupt(BUTTON_FEED, feedPushed, FALLING);
        attachInterrupt(BUTTON_SHIP, shipPushed, FALLING);

        pinMode(LED_FEED, OUTPUT);
        pinMode(LED_SHIP, OUTPUT);
        pinMode(SERVO_FEED, OUTPUT);
        pinMode(SERVO_SHIP, OUTPUT);
        // ledcSetup(1, 39000, 10);
        // ledcAttachPin(D5, 1);
        // ledcWrite(1, 1023);

        servo_feed.attach(SERVO_FEED, 180, 540, 2500);
        servo_ship.attach(SERVO_SHIP, SHIP_UP, 540, 2500);
        setSpeedForAllServos(360);
        #endif
    }
}

void OnDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
    char buf[40];
    sprintf(buf, "Send to %02x:%02x:%02x:%02x:%02x:%02x",
        macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    Serial.println(buf);
}

// void OnDataRecv(const uint8_t* macAddr, const uint8_t* data, int len){}
