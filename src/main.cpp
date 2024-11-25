// #define ARDUINO_M5STACK_CORE
#include "d2system.h"
// #include <M5Unified.h>

#define SEND_RATE 50 // ms
#define RATE_MERGIN 500
#define SHIP_RECHARGE 5000 //ms

void setup() {
    #ifdef ARDUINO_M5STACK_CORE
    auto cfg = M5.config();
    M5.begin(cfg);
    display.begin();
    #endif
    Serial.begin(115200);

    espNow->set_RecvCallback(OnDataRecv);
    D2Init();
}

unsigned long time_serialPrev = 0;

void loop() {
    if (isHost) { // ホスト機
        #ifdef ARDUINO_M5STACK_CORE
        M5.update();
        // シリアル受信
        if (Serial.available()) {
            // 受信
            char sData = Serial.read();
            // time_serialPrev = millis();
            switch (sData) {
                case '0':
                    state = OFFLINE;
                    offline = true;
                    break;
                case '1':
                    state = ON_SETTING;
                    break;
                case '2':
                    state = ON_READY;
                    break;
                case '3':
                    state = ON_MATCH;
                    break;
                case '4':
                    state = ON_RESULT;
                    break;
                case '5':
                    state = TEST;
                default:
                    break;
            }
            
        }// else if (millis() - time_serialPrev >= SERIAL_TOUT) {
            // オフライン処理
        // }

        if (state >= ON_SETTING && state <= ON_RESULT) {
            // オフライン処理
            if (millis()-time_prev_r >= RATE_MERGIN) {
                field_r.state = OFFLINE;
            }
            if (millis()-time_prev_b >= RATE_MERGIN) {
                field_b.state = OFFLINE;
            }

            // 送信
            if (millis() - time_prev >= SEND_RATE) {
                sendStruct.state = (uint8_t)state;
                espNow->send(0, (uint8_t*)&sendStruct);
                time_prev = millis();
            }
        }


        if (state == OFFLINE && offline) {
            offline = false;
            sendStruct.state = OFFLINE;
            espNow->send(0, (uint8_t*)&sendStruct);
            time_prev = millis();
        }

        // オフライン判定

        // 画面表示
        static int i = 0;
        display.startWrite();
        // display.fillScreen(TFT_BLACK);
        display.setTextSize(2);
        display.setTextColor(TFT_WHITE, TFT_BLACK);
        display.setCursor(0, 0);
        char buf[50];
        sprintf(buf, "STATE: %11s\n", strStatus(state));
        display.println(buf);
        display.setTextColor(TFT_RED);
        display.println("-- Red  --");
        display.setTextColor(TFT_WHITE, TFT_BLACK);
        sprintf(buf, "STATE  : %11s", strStatus((Status)field_r.state));
        display.println(buf);
        sprintf(buf, "Feeder : %d", field_r.step_feeder);
        display.println(buf);
        sprintf(buf, "Shipped: %d\n", field_r.isShip);
        display.println(buf);
        display.setTextColor(TFT_BLUE);
        display.println("-- Blue --");
        display.setTextColor(TFT_WHITE, TFT_BLACK);
        sprintf(buf, "STATE  : %11s", strStatus((Status)field_b.state));
        display.println(buf);
        sprintf(buf, "Feeder : %d", field_b.step_feeder);
        display.println(buf);
        sprintf(buf, "Shipped: %d", field_b.isShip);
        display.println(buf);
        display.endWrite();
        // delay(1000);

        #endif
        
    } else {

        // char buf2[50];
        // sprintf(buf2, "%11s, %d, %d", strStatus(state), wasFeedPushed, wasShipPushed);
        // sprintf(buf2, "%11s, %d, %d", strStatus(state), digitalRead(BUTTON_FEED), digitalRead(BUTTON_SHIP));
        // Serial.println(buf2);
        // delay(500);

        #ifndef ARDUINO_M5STACK_CORE
        // フィールド側
        // static int step_feeder = 0;

        // 準備時リセット
        if (state == ON_READY && state_prev != ON_READY) {
            step_feeder = 0;
            wasFeedPushed = false;
            wasShipPushed = false;
            servo_feed.setEaseTo(180);
            servo_ship.setEaseTo(SHIP_UP);
            synchronizeAllServosStartAndWaitForAllServosToStop();
        }

        // ボタン管理
        // フィーダー
        if (wasFeedPushed) {
            step_feeder++;
            if (step_feeder > 3) {
                if (state >= ON_READY && state <= ON_RESULT) {
                    step_feeder = 3;
                } else {
                    step_feeder = 0;
                }
            }
            wasFeedPushed = false;
            
            // フィーダーサーボの処理
            digitalWrite(LED_FEED, HIGH);
            servo_feed.easeTo(180 - step_feeder * 60);
            digitalWrite(LED_FEED, LOW);
        }

        // 出荷ボタン
        if (wasShipPushed) {
            digitalWrite(LED_SHIP, HIGH);
            servo_ship.easeTo(SHIP_DOWN);
            digitalWrite(LED_SHIP, LOW);
            if (state >= ON_READY && state <= ON_RESULT) {
                // wasShipPushedは切り替えない
            } else {
                if (millis() - time_ship >= SHIP_RECHARGE) {
                    wasShipPushed = false;
                    servo_ship.easeTo(SHIP_UP);
                }
            }
        }// else {
        //     servo_ship.easeTo(SHIP_UP);
        // }

        // ホスト側に回答
        if (wasHostRecv) {
            sendStruct.isShip = wasShipPushed;
            sendStruct.state = recvStruct.state;
            sendStruct.step_feeder = step_feeder;
            if (cmpAddr(myAddr, subAddr[1])) delay(5);
            espNow->send(hostAddr, (uint8_t*)&sendStruct);
            wasHostRecv = false;
        }

        char buf2[50];
        sprintf(buf2, "%d, %d", wasFeedPushed, step_feeder);
        // sprintf(buf2, "%11s, %d, %d", strStatus(state), digitalRead(BUTTON_FEED), digitalRead(BUTTON_SHIP));
        Serial.println(buf2);

        #endif
    }
}


void OnDataRecv(const uint8_t* macAddr, const uint8_t* data, int len) {
    if (!isHost && cmpAddr((uint8_t*)macAddr, hostAddr)) {
        // フィールド側
        memcpy(&recvStruct, data, len);
        state_prev = state;
        state = (Status)recvStruct.state;
        // sendStruct.state = recvStruct.state;
        wasHostRecv = true;
        time_prev = millis();
    }
    else if (isHost) {
        if (cmpAddr((uint8_t*)macAddr, subAddr[0])) {
            memcpy(&field_r, data, len);
            time_prev_r = millis();

        } else if (cmpAddr((uint8_t*)macAddr, subAddr[1])) {
            memcpy(&field_b, data, len);
            time_prev_b = millis();
        }
    }
}