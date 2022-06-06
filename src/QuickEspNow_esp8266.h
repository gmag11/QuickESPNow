#ifndef _QUICK_ESPNOW_ESP8266_h
#define _QUICK_ESPNOW_ESP8266_h
#ifdef ESP8266

#include "Arduino.h"
#include "Comms_hal.h"

#include "espnow.h"
#include "ESP8266WiFi.h"
#include "RingBuffer.h"

//#define MEAS_TPUT

static const uint8_t ESPNOW_BROADCAST_ADDRESS[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const uint8_t MIN_WIFI_CHANNEL = 0;
static const uint8_t MAX_WIFI_CHANNEL = 14;
static const uint8_t CURRENT_WIFI_CHANNEL = 255;
static const size_t ESPNOW_MAX_MESSAGE_LENGTH = 255; ///< @brief Maximum message length
static const uint8_t ESPNOW_ADDR_LEN = 6; ///< @brief Address length
static const uint8_t ESPNOW_QUEUE_SIZE = 3; ///< @brief Queue size
static const time_t MEAS_TP_EVERY_MS = 15000; ///< @brief Measurement time period

#define ESP_NOW_ETH_ALEN 6
#define ESP_NOW_MAX_DATA_LEN 255
#define WIFI_IF_STA STATION_IF
#define WIFI_IF_AP SOFTAP_IF

typedef struct {
    uint16_t frame_head;
    uint16_t duration;
    uint8_t destination_address[6];
    uint8_t source_address[6];
    uint8_t broadcast_address[6];
    uint16_t sequence_control;

    uint8_t category_code;
    uint8_t organization_identifier[3]; // 0x18fe34
    uint8_t random_values[4];
    struct {
        uint8_t element_id;                 // 0xdd
        uint8_t lenght;                     //
        uint8_t organization_identifier[3]; // 0x18fe34
        uint8_t type;                       // 4
        uint8_t version;
        uint8_t body[0];
    } vendor_specific_content;
} __attribute__ ((packed)) espnow_frame_format_t;

typedef struct {
    uint8_t dstAddress[ESPNOW_ADDR_LEN]; /**< Message topic*/
    uint8_t payload[ESPNOW_MAX_MESSAGE_LENGTH]; /**< Message payload*/
    size_t payload_len; /**< Payload length*/
} comms_queue_item_t;


class QuickEspNow : public Comms_halClass {
public:
    QuickEspNow () :
        out_queue (ESPNOW_QUEUE_SIZE) {}
    bool begin (uint8_t channel = 255, uint32_t interface = 0);
    void stop ();
    int32_t send (const uint8_t* dstAddress, uint8_t* payload, size_t payload_len);
    void onDataRcvd (comms_hal_rcvd_data dataRcvd);
    void onDataSent (comms_hal_sent_data sentResult);
    uint8_t getAddressLength () { return ESPNOW_ADDR_LEN; }
    uint8_t getMaxMessageLength () { return ESPNOW_MAX_MESSAGE_LENGTH; }
    void handle ();
    void enableTransmit (bool enable);
    bool setChannel (uint8_t channel);

protected:
    uint8_t wifi_if;
    ETSTimer espnowLoopTask;
#ifdef MEAS_TPUT
    ETSTimer dataTPTimer;
    unsigned long txDataSent = 0;
    unsigned long rxDataReceived = 0;
    unsigned long txDataDropped = 0;
    time_t lastDataTPMeas = 0;
    float txDataTP = 0;
    float rxDataTP = 0;
    float txDroppedDataRatio = 0;

    static void tp_timer_cb (void* param);
    void calculateDataTP ();
#endif // MEAS_TPUT

    bool readyToSend = true;
    RingBuffer<comms_queue_item_t> out_queue;
    uint8_t channel;

    void initComms ();
    static void runHandle (void* param);
    int32_t sendEspNowMessage (comms_queue_item_t* message);

    static void ICACHE_FLASH_ATTR rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len);
    static void ICACHE_FLASH_ATTR tx_cb (uint8_t* mac_addr, uint8_t status);
};

extern QuickEspNow quickEspNow;

#endif // ESP8266
#endif // _QUICK_ESPNOW_ESP8266_h