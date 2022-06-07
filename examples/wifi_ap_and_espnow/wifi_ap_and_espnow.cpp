#include <Arduino.h>
#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#define WIFI_MODE_STA WIFI_STA 
#define WIFI_MODE_AP WIFI_AP
#else
#error "Unsupported platform"
#endif //ESP32
#include <QuickEspNow.h>

static const String msg = "Hello esp-now from TTGO";

static uint8_t receiver[] = { 0x12, 0x34, 0x56, 0x78, 0x90, 0x12 };

#define DEST_ADDR receiver
//#define DEST_ADDR ESPNOW_BROADCAST_ADDRESS 

static const uint8_t CHANNEL = 1;

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    Serial.print ("Received: ");
    Serial.printf ("%.*s\n", len, data);
    Serial.printf ("RSSI: %d dBm\n", rssi);
    Serial.printf ("From: " MACSTR "\n", MAC2STR (address));
    Serial.printf ("%s\n", broadcast ? "Broadcast" : "Unicast");
}

void setup () {
    Serial.begin (115200);
    if (!WiFi.mode (WIFI_MODE_AP)) {
        Serial.println ("WiFi mode not supported");
    }
    if (!WiFi.softAP ("espnowAP", "1234567890", CHANNEL)) {
        Serial.println ("WiFi access point not started");
    }

    uint8_t protocol_bitmap;
    //esp_wifi_set_protocol (WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    // esp_err_t error = esp_wifi_get_protocol (WIFI_IF_AP, &protocol_bitmap);
    // if (error != ESP_OK) {
    //     Serial.printf ("esp_wifi_get_protocol failed: %d\n", error);
    // }
    // else {
    //     Serial.printf ("Protocol bitmap: %d\n", protocol_bitmap);
    //     if (protocol_bitmap & WIFI_PROTOCOL_11B) {
    //         Serial.println ("Protocol: 802.11b");
    //     }
    //     if (protocol_bitmap & WIFI_PROTOCOL_11G) {
    //         Serial.println ("Protocol: 802.11g");
    //     }
    //     if (protocol_bitmap & WIFI_PROTOCOL_11N) {
    //         Serial.println ("Protocol: 802.11n");
    //     }
    //     if (protocol_bitmap & WIFI_PROTOCOL_LR) {
    //         Serial.println ("Protocol: Low Rate");
    //     }
    // }

    Serial.printf ("Started AP %s in channel %d\n", WiFi.softAPSSID ().c_str (), WiFi.channel ());
    Serial.printf ("IP address: %s\n", WiFi.softAPIP ().toString ().c_str ());
    Serial.printf ("MAC address: %s\n", WiFi.softAPmacAddress ().c_str ());
    quickEspNow.begin (CURRENT_WIFI_CHANNEL, WIFI_IF_AP); // Use no parameters to start ESP-NOW on same channel as WiFi, in STA mode
    quickEspNow.onDataRcvd (dataReceived);
    //Serial.printf("%s\n", esp_get_idf_version());
}

void loop () {
    static time_t lastSend = 60000;
    static unsigned int counter = 0;

    if (millis () - lastSend >= 1000) {
        lastSend = millis ();
        String message = String (msg) + " " + String (counter++);
        if (!quickEspNow.send (DEST_ADDR, (uint8_t*)message.c_str (), message.length ())) {
            //Serial.printf (">>>>>>>>>> %ld: Message sent\n", micros());
        } else {
            Serial.printf (">>>>>>>>>> %ld: Message not sent\n", micros ());
        }

    }

}