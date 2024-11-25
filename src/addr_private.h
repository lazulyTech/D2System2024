#pragma once
#include <Arduino.h>
#include "esp_mac.h"

#define DEVICE_NUM 2
// #define PEER_NUM 2

uint8_t myAddr[6];

uint8_t subAddr[DEVICE_NUM][6] = {
    // {0x34, 0x85, 0x18, 0x26, 0x21, 0x48}, // main XIAO
    // {0x08, 0x3a, 0xf2, 0x68, 0x33, 0x08}, // M5Stack
    {0x64, 0xe8, 0x33, 0x86, 0x07, 0x98}, // 
    {0x64, 0xe8, 0x33, 0x87, 0x13, 0xe0}
};

uint8_t hostAddr[6] = {0x08, 0x3a, 0xf2, 0x68, 0x33, 0x08}; // M5Stack

void getWiFiMacAddr(uint8_t* _addr) {
    uint8_t addr[6];
    esp_read_mac(addr, ESP_MAC_WIFI_STA);
    char buf[64];
    sprintf(buf, "{0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x}", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    Serial.println(buf);
    if (_addr != NULL) {
        memcpy(_addr, addr, 6);
    }
}

bool cmpAddr(uint8_t* a, uint8_t* b) {
    bool isComp = true;
    for (int i = 0; i < 6; i++) {
        if (a[i] != b[i]) {
            isComp = false;
            break;
        }
    }
    return isComp;
}

// void peerIndexList(uint8_t* addrIndex) {
//     uint8_t my[6];
//     getWiFiMacAddr(my);
//     uint8_t list[DEVICE_NUM-2];
//     int num = 0;
//     for (int i = 1; i < DEVICE_NUM; i++) {
//         bool isMine = 1;
//         for(int j = 0; j < 6; j++) {
//             if (my[j] != subAddr[i][j]) {
//                 isMine = 0;
//                 break;
//             }
//         }
//         if (!isMine) {
//             list[num] = i;
//             num++;
//         }
//     }
//     memcpy(addrIndex, list, DEVICE_NUM-2);
// }
