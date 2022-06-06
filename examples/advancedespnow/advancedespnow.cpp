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

bool sent = true;
bool received = false;

const unsigned int SEND_MSG_MSEC = 2000;

//------------ SET ACCORDING YOUR BOARD -----------
static const int LED_PIN = 5;
static const int LED_ON = LOW;
//-------------------------------------------------

void dataSent (uint8_t* address, uint8_t status) {
    sent = true;
    //Serial.printf ("Message sent to " MACSTR ", status: %d\n", MAC2STR (address), status);
}

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi) {
    digitalWrite (LED_PIN, LED_ON);
    Serial.print ("Received: ");
    Serial.printf ("%.*s\n", len, data);
    Serial.printf ("RSSI: %d dBm\n", rssi);
    Serial.printf ("From: " MACSTR "\n", MAC2STR (address));
    digitalWrite (LED_PIN, !LED_ON);
}

void setup () {
    Serial.begin (115200);
    WiFi.mode (WIFI_MODE_STA);
    pinMode (LED_PIN, OUTPUT);
    digitalWrite (LED_PIN, !LED_ON);
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
}

void loop () {
    static time_t lastSend = 0;
    static unsigned int counter = 0;

    if (sent && ((millis () - lastSend) > SEND_MSG_MSEC)) {
        lastSend = millis ();
        String message = String (msg);// +" " + String (counter++);
        sent = false;
        if (!quickEspNow.send (DEST_ADDR, (uint8_t*)message.c_str (), message.length ())) {
            //Serial.printf (">>>>>>>>>> %ld: Message sent\n", micros());
        } else {
            Serial.printf (">>>>>>>>>> %ld: Message not sent\n", micros ());
            sent = true;
        }

    }

}
