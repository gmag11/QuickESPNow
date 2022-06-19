# Quick ESP-NOW

[![PlatformIO Registry](https://badges.registry.platformio.org/packages/gmag11/library/QuickEspNow.svg)](https://registry.platformio.org/libraries/gmag11/QuickEspNow)

## Introduction
This is a library for Arduino framework to be used with Espressif ESP8266 and ESP32 series microcontrollers.

ESP-NOW is a wireless communication protocol that allows two devices to send data to each other without the need for a wireless network. You can read more about it here: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html

## Benefits
Usage of ESP-NOW may not be straightforward. There are some restrictions that has to be considered, as limit of number of devices, WiFi channel selection, and other factors.

This library pretends to hide all that restrictions so that it can be used with just a few lines of code.

Besides, it removes some limitations:

- No more 20 devices limit. You can use ESP-NOW with **any number of devices**. Library takes control of peer registration and makes it transparent to you.
- Channel selection is not required for WiFi coexistence.
- No need to assign a role to each device. Just use it for peer to peer communication.
- **RSSI** information of every message.
- Receiver can distiguish between broadcast and unicast messages.
- Tested maximum througput, about 1500 kbps continuous with default parameters.
- Encryption is not supported. Usage of ESP-NOW encryption restrict system to 6 devices. You can implement data encryption in a higher layer.

## Usage

One just only needs to call begin and set a receivin callback function, if you need to receive data. Then you can use send function to send data to a specific device or use `ESPNOW_BROADCAST_ADDRESS` to send data to all listening devices in the same channel.

```C++
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    Serial.printf("Received message: %.*s\n", len, data);
}

void setup () {
    Serial.begin (115200);
    WiFi.mode (WIFI_MODE_STA);
    WiFi.disconnect (false);
    quickEspNow.onDataRcvd (dataReceived);
    quickEspNow.begin (1); // If you are not connected to WiFi, channel should be specified
}

void loop () {
    String message = "Hello, world!";
    quickEspNow.send (DEST_ADDR, (uint8_t*)message.c_str (), message.length ());
    delay (1000);
}
```
