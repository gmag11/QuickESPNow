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

//#defide NOTIFY_DATA
#define SEND_DATA

// static const String msg = "Hello esp-now";
static const String msg = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";

static uint8_t addr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t addr_t_display[] = { 0x08, 0x3a, 0xf2, 0x69, 0xc4, 0x04 };
static uint8_t addr_ttgo[] = { 0xb4, 0xe6, 0x2d, 0x9d, 0xab, 0x15 };
static uint8_t addr_ttgo_ap[] = { 0xb4, 0xe6, 0x2d, 0x9d, 0xab, 0x16 };
static uint8_t addr_m5[] = { 0x4c, 0x75, 0x25, 0xac, 0xc8, 0x50 };

bool sent = true;
bool received = false;

const unsigned int SEND_MSG_MSEC = 1;

//-----------------------
static const int LED_PIN = 10;
static const int LED_ON = LOW;
//-----------------------

void dataSent (uint8_t* address, uint8_t status) {
    sent = true;
    //Serial.printf ("Message sent to " MACSTR ", status: %d\n", MAC2STR (address), status);
}

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi) {
    digitalWrite (LED_PIN, LED_ON);
#ifdef NOTIFY_DATA
    Serial.print ("Received: ");
    Serial.printf ("%.*s\n", len, data);
    Serial.printf ("RSSI: %d dBm\n", rssi);
    Serial.printf ("From: " MACSTR "\n", MAC2STR (address));
#endif //NOTIFY_DATA
    digitalWrite (LED_PIN, !LED_ON);
}

void setup () {
    Serial.begin (115200);
    WiFi.mode (WIFI_MODE_STA);
    //WiFi.begin ("ssid", "pass");
    // while (WiFi.status () != WL_CONNECTED) {
    //     delay (500);
    //     Serial.print (".");
    // }
    //-------------------------------
    pinMode (LED_PIN, OUTPUT);
    digitalWrite (LED_PIN, !LED_ON);
    //-------------------------------
#if defined ESP32
    WiFi.disconnect (false, true);
#elif defined ESP8266
    WiFi.disconnect (false);
#endif //ESP32
    Serial.printf ("Connected to %s in channel %d\n", WiFi.SSID ().c_str (), WiFi.channel ());
    Serial.printf ("IP address: %s\n", WiFi.localIP ().toString ().c_str ());
    Serial.printf ("MAC address: %s\n", WiFi.macAddress ().c_str ());
    quickEspNow.begin (1);
    quickEspNow.onDataSent (dataSent);
    quickEspNow.onDataRcvd (dataReceived);
    //Serial.printf("%s\n", esp_get_idf_version());
}

void loop () {
#ifdef SEND_DATA
    static time_t lastSend = 0;
    static unsigned int counter = 0;

    if (sent && ((millis () - lastSend) > SEND_MSG_MSEC)) {
        lastSend = millis ();
        String message = String (msg);// +" " + String (counter++);
        sent = false;
        if (!quickEspNow.send (/*ESPNOW_BROADCAST_ADDRESS*/addr_m5, (uint8_t*)message.c_str (), message.length ())) {
            //Serial.printf (">>>>>>>>>> %ld: Message sent\n", micros());
        } else {
            Serial.printf (">>>>>>>>>> %ld: Message not sent\n", micros ());
            sent = true;
        }

    } else {
        //Serial.printf (">>>>>>>>>> %ld: Sent: %d\n", micros (), sent);
    }
#endif //SEND_DATA

}
