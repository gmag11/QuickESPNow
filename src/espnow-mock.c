#include "espnow-mock.h"

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

typedef struct macaddress_t { uint8_t addr[6]; } macaddress_t;
const macaddress_t mac1 = {
    .addr = { 1,1,1,1,1,1 }
};
const macaddress_t mac2 = {
    .addr = { 2,2,2,2,2,2 }
};
const macaddress_t mac3 = {
    .addr = { 3,3,3,3,3,3 }
};
const char data[] = "Hello world!";

wifi_pkt_rx_ctrl_t rx_crtl = {
    .rssi = -50,
    .sig_len = sizeof (espnow_frame_format_t) + sizeof (data)
};

espnow_frame_format_t espnow_frame;

struct espnow_packet_t {
    wifi_promiscuous_pkt_t packet_header;
    espnow_frame_format_t espnow_header;
    uint8_t payload[250];
} __attribute__ ((packed)) espnow_packet;

#ifdef ESP32
#define ESPNOW_MOCK_TAG "ESPNOW MOCK"

#ifdef __cplusplus
extern "C" {
#endif

#include <RTOS.h>
#include <freertos/task.h>
#include <string.h>

    esp_now_recv_cb_t recv_cb;
    esp_now_send_cb_t send_cb;
    xTaskHandle recv_sim_task;

    void recv_sim_task_fun (void* param) {
        macaddress_t mac[3] = { mac1, mac2, mac3 };
        static int index = 0;

        for (;;) {
            if (recv_cb) {
                recv_cb (mac[index].addr, (uint8_t*)&(espnow_packet.payload), sizeof (data) - 1);
            }
            index++;
            if (index >= 3) {
                index = 0;
            }
            vTaskDelay (pdMS_TO_TICKS (5000));
        }
    }

    esp_err_t esp_now_init_ (void) {
        espnow_packet.packet_header.rx_ctrl = rx_crtl;
        espnow_packet.espnow_header = espnow_frame;
        memcpy (&(espnow_packet.payload), data, sizeof (data) - 1);
        xTaskCreate (recv_sim_task_fun, "SendSimTask", 4096, NULL, 1, recv_sim_task);
        return ESP_OK;
    }
    
    esp_err_t esp_now_deinit_ (void) {
        vTaskDelete (recv_sim_task);
        return ESP_OK;
    }

    esp_err_t esp_now_register_recv_cb_ (esp_now_recv_cb_t cb) {
        recv_cb = cb;
        ESP_LOGI (ESPNOW_MOCK_TAG, "esp_now_register_recv_cb_");
        printf ("esp_now_register_recv_cb_");
        return ESP_OK;
    }

    esp_err_t esp_now_unregister_recv_cb_ (void) {
        recv_cb = NULL;
        return ESP_OK;
    }

    esp_err_t esp_now_register_send_cb_ (esp_now_send_cb_t cb) {
        send_cb = cb;
        ESP_LOGI (ESPNOW_MOCK_TAG, "esp_now_register_send_cb_");
        return ESP_OK;
    }

    esp_err_t esp_now_unregister_send_cb_ (void) {
        send_cb = NULL;
        return ESP_OK;
    }

    esp_err_t esp_now_send_ (const uint8_t* peer_addr, const uint8_t* data, size_t len) {
        ESP_LOGI (ESPNOW_MOCK_TAG, "esp_now_send: peer_addr: " MACSTR " data: %.*s", MAC2STR (peer_addr), len, data);
        //delay (10);
        vTaskDelay (pdMS_TO_TICKS (10));
        if (send_cb != NULL) {
            send_cb (peer_addr, ESP_NOW_SEND_SUCCESS);
        }
        return ESP_OK;
    }

    esp_err_t esp_now_add_peer_ (const esp_now_peer_info_t* peer) {
        return ESP_OK;
    }
    esp_err_t esp_now_del_peer_ (const uint8_t* peer_addr) {
        return ESP_OK;
    }
    esp_err_t esp_now_mod_peer_ (const esp_now_peer_info_t* peer) {
        return ESP_OK;
    }

    esp_err_t esp_now_get_peer_ (const uint8_t* peer_addr, esp_now_peer_info_t* peer) {
        // static esp_now_peer_info_t my_peer;

        // my_peer.channel = 1;
        // my_peer.encrypt = false;
        // my_peer.ifidx = WIFI_IF_STA;
        // my_peer.lmk = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };

        static esp_now_peer_info_t my_peer = {
            .channel = 1,
            .encrypt = false,
            .ifidx = WIFI_IF_STA,
            .lmk = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 },
            .peer_addr = { 1,1,1,1,1,1 },
            .priv = NULL
        };

        memcpy (&(my_peer.peer_addr), peer_addr, 6);

        peer = &my_peer;

        return ESP_OK;
    }

#ifdef __cplusplus
}
#endif

#endif // ESP32


#ifdef ESP8266

    #include <user_interface.h>

#ifdef __cplusplus
    extern "C" {
#endif

    esp_now_recv_cb_t recv_cb;
    esp_now_send_cb_t send_cb;

    int esp_now_init_ (void) {
        return 0;
    }

    int esp_now_deinit_ (void) {
        return 0;
    }

    int esp_now_register_recv_cb_ (esp_now_recv_cb_t cb) {
        recv_cb = cb;
        return 0;
    }

    int esp_now_unregister_recv_cb_ (void) {
        recv_cb = NULL;
        return 0;
    }

    int esp_now_register_send_cb_ (esp_now_send_cb_t cb) {
        send_cb = cb;
        return 0;
    }

    int esp_now_unregister_send_cb_ (void) {
        send_cb = NULL;
        return 0;
    }

    int esp_now_send_ (u8* da, u8* data, int len) {
        os_printf ("esp_now_send: peer_addr: " MACSTR " data: %.*s", MAC2STR (da), len, data);
        //delay (10);
        os_delay_us (10000);
        if (send_cb != NULL) {
            send_cb (da, 0);
        }
        return 0;
    }

    int esp_now_set_self_role_ (u8 role) {
        return 0;
    }

#ifdef __cplusplus
}
#endif

#endif // ESP8266

