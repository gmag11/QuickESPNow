#include <Arduino.h>
#include <WiFi.h>
#include <QuickEspNow.h>
#include <esp_wifi.h>

static const String msg = "Hello esp-now from TTGO";
static uint8_t addr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; 
static uint8_t addr_t_display[] = { 0x08, 0x3a, 0xf2, 0x69, 0xc4, 0x04 };
static uint8_t addr_ttgo[] = { 0xb4, 0xe6, 0x2d, 0x9d, 0xab, 0x15 };
static uint8_t addr_ttgo_ap[] = { 0xb4, 0xe6, 0x2d, 0x9d, 0xab, 0x16 };
static uint8_t addr_m5[] = { 0x4c, 0x75, 0x25, 0xac, 0xc8, 0x50 };

static const uint8_t CHANNEL = 1;

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
    if (!WiFi.mode (WIFI_MODE_AP)) {
        Serial.println ("WiFi mode not supported");
    }
    if (!WiFi.softAP ("espnowAP", "1234567890", CHANNEL)) {
        Serial.println ("WiFi access point not started");
    }
    
    // WiFi.begin ("ssid", "pass");
    // while (WiFi.status () != WL_CONNECTED) {
    //     delay (500);
    //     Serial.print (".");
    // }
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
    quickEspNow.onDataSent (dataSent);
    quickEspNow.onDataRcvd (dataReceived);
    //Serial.printf("%s\n", esp_get_idf_version());
}

void loop() { 
    static time_t lastSend = 60000;
    static uint counter = 0;

    if (millis () - lastSend >= 1000) {
        lastSend = millis ();
        String message = String (msg) + " " + String (counter++);
        if (!quickEspNow.send (addr_m5, (uint8_t*)message.c_str (), message.length ())) {
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