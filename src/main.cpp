#include <Arduino.h>
#include <WiFi.h>
#include <QuickEspNow.h>
#include <esp_wifi.h>

static const String msg = "Hello esp-now from TTGO";
static uint8_t addr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; 
static uint8_t addr_t_display[] = { 0x08, 0x3a, 0xf2, 0x69, 0xc4, 0x04 };
static uint8_t addr_ttgo[] = { 0xb4, 0xe6, 0x2d, 0x9d, 0xab, 0x15 };

bool sent = false;
bool received = false;

void dataSent (uint8_t* address, uint8_t status) {
    sent = true;
    Serial.printf("Message sent to " MACSTR ", status: %d\n", MAC2STR(address), status);
}

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi) {
    Serial.print ("Received: ");
    Serial.printf ("%.*s\n", len, data);
    Serial.printf ("RSSI: %d dBm\n", rssi);
    Serial.printf ("From: " MACSTR "\n", MAC2STR(address));
}

void setup () {
    Serial.begin (115200);
    WiFi.mode (WIFI_MODE_STA);
    //WiFi.begin ("ssid", "pass");
    // while (WiFi.status () != WL_CONNECTED) {
    //     delay (500);
    //     Serial.print (".");
    // }
    WiFi.disconnect (false, true);
    Serial.printf ("Connected to %s in channel %d\n", WiFi.SSID ().c_str (), WiFi.channel ());
    Serial.printf ("IP address: %s\n", WiFi.localIP ().toString ().c_str ());
    Serial.printf ("MAC address: %s\n", WiFi.macAddress ().c_str ());
    quickEspNow.begin (1);
    quickEspNow.onDataSent (dataSent);
    quickEspNow.onDataRcvd (dataReceived);
    //Serial.printf("%s\n", esp_get_idf_version());
}

void loop() { 
    static time_t lastSend = 60000;
    static uint counter = 0;

    if (millis () - lastSend >= 15000) {
        lastSend = millis ();
        String message = String (msg) + " " + String (counter++);
        if (!quickEspNow.send (addr_t_display, (uint8_t*)message.c_str (), message.length ())) {
            //Serial.printf (">>>>>>>>>> %ld: Message sent\n", micros());
        } else {
            Serial.printf (">>>>>>>>>> %ld: Message not sent\n", micros ());
        }

    }

    if (sent){
        sent = false;
        //Serial.printf (">>>>>>>>>> %ld: Message send confirmation\n", micros ());
    }
    
    

}