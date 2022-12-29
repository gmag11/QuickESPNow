#pragma once

#ifndef __ESP_NOW_H__
#define __ESP_NOW_H__

#ifdef ESP32

#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi_types.h"
#include "esp_log.h"
//#include <QuickDebug.h>

#ifdef __cplusplus

extern "C" {

#endif // __cplusplus

#define ESP_ERR_ESPNOW_BASE         (ESP_ERR_WIFI_BASE + 100) /*!< ESPNOW error number base. */
#define ESP_ERR_ESPNOW_NOT_INIT     (ESP_ERR_ESPNOW_BASE + 1) /*!< ESPNOW is not initialized. */
#define ESP_ERR_ESPNOW_ARG          (ESP_ERR_ESPNOW_BASE + 2) /*!< Invalid argument */
#define ESP_ERR_ESPNOW_NO_MEM       (ESP_ERR_ESPNOW_BASE + 3) /*!< Out of memory */
#define ESP_ERR_ESPNOW_FULL         (ESP_ERR_ESPNOW_BASE + 4) /*!< ESPNOW peer list is full */
#define ESP_ERR_ESPNOW_NOT_FOUND    (ESP_ERR_ESPNOW_BASE + 5) /*!< ESPNOW peer is not found */
#define ESP_ERR_ESPNOW_INTERNAL     (ESP_ERR_ESPNOW_BASE + 6) /*!< Internal error */
#define ESP_ERR_ESPNOW_EXIST        (ESP_ERR_ESPNOW_BASE + 7) /*!< ESPNOW peer has existed */
#define ESP_ERR_ESPNOW_IF           (ESP_ERR_ESPNOW_BASE + 8) /*!< Interface error */

#define ESP_NOW_ETH_ALEN             6         /*!< Length of ESPNOW peer MAC address */
#define ESP_NOW_KEY_LEN              16        /*!< Length of ESPNOW peer local master key */

#define ESP_NOW_MAX_TOTAL_PEER_NUM   20        /*!< Maximum number of ESPNOW total peers */
#define ESP_NOW_MAX_ENCRYPT_PEER_NUM 6         /*!< Maximum number of ESPNOW encrypted peers */

#define ESP_NOW_MAX_DATA_LEN         250       /*!< Maximum length of ESPNOW data which is sent very time */

typedef enum {
    ESP_NOW_SEND_SUCCESS = 0,       /**< Send ESPNOW data successfully */
    ESP_NOW_SEND_FAIL,              /**< Send ESPNOW data fail */
} esp_now_send_status_t;

// typedef enum {
//     ESP_NOW_SEND_SUCCESS = 0,       /**< Send ESPNOW data successfully */
//     ESP_NOW_SEND_FAIL,              /**< Send ESPNOW data fail */
// } esp_now_send_status_t;

typedef struct esp_now_peer_info {
    uint8_t peer_addr[ESP_NOW_ETH_ALEN];    /**< ESPNOW peer MAC address that is also the MAC address of station or softap */
    uint8_t lmk[ESP_NOW_KEY_LEN];           /**< ESPNOW peer local master key that is used to encrypt data */
    uint8_t channel;                        /**< Wi-Fi channel that peer uses to send/receive ESPNOW data. If the value is 0,
                                                 use the current channel which station or softap is on. Otherwise, it must be
                                                 set as the channel that station or softap is on. */
    wifi_interface_t ifidx;                 /**< Wi-Fi interface that peer uses to send/receive ESPNOW data */
    bool encrypt;                           /**< ESPNOW data that this peer sends/receives is encrypted or not */
    void* priv;                             /**< ESPNOW peer private data */
} esp_now_peer_info_t;

typedef struct esp_now_peer_num {
    int total_num;                           /**< Total number of ESPNOW peers, maximum value is ESP_NOW_MAX_TOTAL_PEER_NUM */
    int encrypt_num;                         /**< Number of encrypted ESPNOW peers, maximum value is ESP_NOW_MAX_ENCRYPT_PEER_NUM */
} esp_now_peer_num_t;


typedef void (*esp_now_recv_cb_t)(const uint8_t* mac_addr, const uint8_t* data, int data_len);
typedef void (*esp_now_send_cb_t)(const uint8_t* mac_addr, esp_now_send_status_t status);

#define esp_now_init esp_now_init_
#define esp_now_deinit esp_now_deinit_
#define esp_now_register_recv_cb esp_now_register_recv_cb_
#define esp_now_unregister_recv_cb esp_now_unregister_recv_cb_
#define esp_now_register_send_cb esp_now_register_send_cb_
#define esp_now_unregister_send_cb esp_now_unregister_send_cb_
#define esp_now_send esp_now_send_
#define esp_now_add_peer esp_now_add_peer_
#define esp_now_del_peer esp_now_del_peer_
#define esp_now_mod_peer esp_now_mod_peer_
#define esp_now_get_peer esp_now_get_peer_

esp_err_t esp_now_init_ (void);
esp_err_t esp_now_deinit_ (void);
// esp_err_t esp_now_get_version (uint32_t* version);
esp_err_t esp_now_register_recv_cb_ (esp_now_recv_cb_t cb);
esp_err_t esp_now_unregister_recv_cb_ (void);
esp_err_t esp_now_register_send_cb_ (esp_now_send_cb_t cb);
esp_err_t esp_now_unregister_send_cb_ (void);
esp_err_t esp_now_send_ (const uint8_t* peer_addr, const uint8_t* data, size_t len);
esp_err_t esp_now_add_peer_ (const esp_now_peer_info_t* peer);
esp_err_t esp_now_del_peer_ (const uint8_t* peer_addr);
esp_err_t esp_now_mod_peer_ (const esp_now_peer_info_t* peer);
// esp_err_t esp_wifi_config_espnow_rate (wifi_interface_t ifx, wifi_phy_rate_t rate);
esp_err_t esp_now_get_peer_ (const uint8_t* peer_addr, esp_now_peer_info_t* peer);
// esp_err_t esp_now_fetch_peer (bool from_head, esp_now_peer_info_t* peer);
// bool esp_now_is_peer_exist (const uint8_t* peer_addr);
// esp_err_t esp_now_get_peer_num (esp_now_peer_num_t* num);
// esp_err_t esp_now_set_pmk (const uint8_t* pmk);
// esp_err_t esp_now_set_wake_window (uint16_t window);

#ifdef __cplusplus

}

#endif // __cplusplus


#endif // ESP32

#ifdef ESP8266

#include <c_types.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*esp_now_recv_cb_t)(u8* mac_addr, u8* data, u8 len);
typedef void (*esp_now_send_cb_t)(u8* mac_addr, u8 status);


enum esp_now_role {
    ESP_NOW_ROLE_IDLE = 0,
    ESP_NOW_ROLE_CONTROLLER,
    ESP_NOW_ROLE_SLAVE,
    ESP_NOW_ROLE_COMBO,
    ESP_NOW_ROLE_MAX,
};

#define esp_now_init esp_now_init_
#define esp_now_deinit esp_now_deinit_
#define esp_now_register_recv_cb esp_now_register_recv_cb_
#define esp_now_unregister_recv_cb esp_now_unregister_recv_cb_
#define esp_now_register_send_cb esp_now_register_send_cb_
#define esp_now_unregister_send_cb esp_now_unregister_send_cb_
#define esp_now_send esp_now_send_
#define esp_now_set_self_role esp_now_set_self_role_
//#define esp_now_get_self_role esp_now_get_self_role_
// #define esp_now_add_peer esp_now_add_peer_
// #define esp_now_del_peer esp_now_del_peer_
// #define esp_now_mod_peer esp_now_mod_peer_
// #define esp_now_get_peer esp_now_get_peer_

int esp_now_init_ (void);
int esp_now_deinit_ (void);

int esp_now_register_send_cb_ (esp_now_send_cb_t cb);
int esp_now_unregister_send_cb_ (void);

int esp_now_register_recv_cb_ (esp_now_recv_cb_t cb);
int esp_now_unregister_recv_cb_ (void);

int esp_now_send_ (u8* da, u8* data, int len);

// int esp_now_add_peer_ (u8* mac_addr, u8 role, u8 channel, u8* key, u8 key_len);
// int esp_now_del_peer_ (u8* mac_addr);

int esp_now_set_self_role_ (u8 role);
//int esp_now_get_self_role_ (void);

// int esp_now_set_peer_role (u8* mac_addr, u8 role);
// int esp_now_get_peer_role (u8* mac_addr);

// int esp_now_set_peer_channel (u8* mac_addr, u8 channel);
// int esp_now_get_peer_channel (u8* mac_addr);

// int esp_now_set_peer_key (u8* mac_addr, u8* key, u8 key_len);
// int esp_now_get_peer_key (u8* mac_addr, u8* key, u8* key_len);

// u8* esp_now_fetch_peer (bool restart);

// int esp_now_is_peer_exist (u8* mac_addr);

// int esp_now_get_cnt_info (u8* all_cnt, u8* encrypt_cnt);

// int esp_now_set_kok (u8* key, u8 len);

#ifdef __cplusplus
}
#endif

#endif // ESP8266

#endif