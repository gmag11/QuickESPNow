#include <Arduino.h>
#include <WiFi.h>
#include <QuickEspNow.h>
#include <esp_wifi.h>

static const String msg = "Hello esp-now!";

static uint8_t receiver[] = { 0x12, 0x34, 0x56, 0x78, 0x90, 0x12 };

#define DEST_ADDR receiver
//#define DEST_ADDR ESPNOW_BROADCAST_ADDRESS 

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    Serial.print ("Received: ");
    Serial.printf ("%.*s\n", len, data);
    Serial.printf ("RSSI: %d dBm\n", rssi);
    Serial.printf ("From: " MACSTR "\n", MAC2STR(address));
    Serial.printf ("%s\n", broadcast ? "Broadcast" : "Unicast");

}

void setup () {
    Serial.begin (115200);
    WiFi.mode (WIFI_MODE_STA);
    WiFi.begin ("ssid", "pass");
    while (WiFi.status () != WL_CONNECTED) {
        delay (500);
        Serial.print (".");
    }
    Serial.printf ("Connected to %s in channel %d\n", WiFi.SSID ().c_str (), WiFi.channel ());
    Serial.printf ("IP address: %s\n", WiFi.localIP ().toString ().c_str ());
    Serial.printf ("MAC address: %s\n", WiFi.macAddress ().c_str ());
    quickEspNow.begin (); // Use no parameters to start ESP-NOW on same channel as WiFi, in STA mode
    quickEspNow.onDataRcvd (dataReceived);
}

void loop() { 
    static time_t lastSend = 60000;
    static uint counter = 0;

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