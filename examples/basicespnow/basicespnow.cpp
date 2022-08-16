#include <Arduino.h>
#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#define WIFI_MODE_STA WIFI_STA 
#else
#error "Unsupported platform"
#endif //ESP32
#include <QuickEspNow.h>

static const String msg = "Hello ESP-NOW!";

static uint8_t receiver[] = { 0x12, 0x34, 0x56, 0x78, 0x90, 0x12 };

#define DEST_ADDR receiver
//#define DEST_ADDR ESPNOW_BROADCAST_ADDRESS 

static const char* TAG_MAIN = "MAIN";

const unsigned int SEND_MSG_MSEC = 2000;

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast, unsigned int channel) {
    DEBUG_INFO (TAG_MAIN, "Received: %.*s", len, data);
    DEBUG_INFO (TAG_MAIN, "RSSI: %d dBm", rssi);
    DEBUG_INFO (TAG_MAIN, "Channel: %d", channel);
    DEBUG_INFO (TAG_MAIN, "From: " MACSTR, MAC2STR (address));
    DEBUG_INFO (TAG_MAIN, "%s", broadcast ? "Broadcast" : "Unicast");
}

void setup () {
    Serial.begin (115200);
    WiFi.mode (WIFI_MODE_STA);
#if defined ESP32
    WiFi.disconnect (false, true);
#elif defined ESP8266
    WiFi.disconnect (false);
#endif //ESP32
    Serial.printf ("Connected to %s in channel %d\n", WiFi.SSID ().c_str (), WiFi.channel ());
    Serial.printf ("IP address: %s\n", WiFi.localIP ().toString ().c_str ());
    Serial.printf ("MAC address: %s\n", WiFi.macAddress ().c_str ());
    quickEspNow.onDataRcvd (dataReceived);
#ifdef ESP32
    quickEspNow.setWiFiBandwidth (WIFI_IF_STA, WIFI_BW_HT20); // Only needed for ESP32 in case you need coexistence with ESP8266 in the same network
#endif //ESP32
    quickEspNow.begin (1); // If you use no connected WiFi channel should be specified
}

void loop () {
    static unsigned int counter = 0;

    String message = String (msg) + " " + String (counter++);
    if (!quickEspNow.send (DEST_ADDR, (uint8_t*)message.c_str (), message.length ())) {
        Serial.println (">>>>>>>>>> Message sent");
    } else {
        Serial.println (">>>>>>>>>> Message not sent");
    }

    delay (SEND_MSG_MSEC);

}

