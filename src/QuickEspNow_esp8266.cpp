#include "QuickEspNow.h"
#include "QuickDebug.h"
#ifdef ESP8266	

#ifdef ESP8266

typedef struct {
    signed rssi : 8;
    unsigned rate : 4;
    unsigned is_group : 1;
    unsigned : 1;
    unsigned sig_mode : 2;
    unsigned legacy_length : 12;
    unsigned damatch0 : 1;
    unsigned damatch1 : 1;
    unsigned bssidmatch0 : 1;
    unsigned bssidmatch1 : 1;
    unsigned MCS : 7;
    unsigned CWB : 1;
    unsigned HT_length : 16;
    unsigned Smoothing : 1;
    unsigned Not_Sounding : 1;
    unsigned : 1;
    unsigned Aggregation : 1;
    unsigned STBC : 2;
    unsigned FEC_CODING : 1;
    unsigned SGI : 1;
    unsigned rxend_state : 8;
    unsigned ampdu_cnt : 8;
    unsigned channel : 4;
    unsigned : 12;
} wifi_pkt_rx_ctrl_t;

typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t payload[0]; /* ieee80211 packet buff */
} wifi_promiscuous_pkt_t;
#endif

QuickEspNow quickEspNow;

bool QuickEspNow::begin (uint8_t channel, uint32_t wifi_interface) {

    DEBUG_INFO ("Channel: %d, Interface: %d", channel, wifi_interface);
    // Set the wifi interface
    switch (wifi_interface) {
    case WIFI_IF_STA:
        wifi_if = WIFI_IF_STA;
        break;
    case WIFI_IF_AP:
        wifi_if = WIFI_IF_AP;
        break;
    default:
        DEBUG_ERROR ("Unknown wifi interface");
        return false;
        break;
    }

    // check channel
    if (channel != CURRENT_WIFI_CHANNEL && (channel < MIN_WIFI_CHANNEL || channel > MAX_WIFI_CHANNEL)) {
        DEBUG_ERROR ("Invalid wifi channel %d", channel);
        return false;
    }

    // use current channel
    if (channel == CURRENT_WIFI_CHANNEL) {
        uint8_t ch;
#if defined ESP32
        wifi_second_chan_t ch2;
        esp_wifi_get_channel (&ch, &ch2);
#else
        ch = WiFi.channel ();
#endif //ESP32
        DEBUG_INFO ("Current channel: %d", ch);
        channel = ch;
    } else {
        setChannel(channel);
    }
#ifdef ESP32
    DEBUG_INFO ("Starting ESP-NOW in in channel %u interface %s", channel, wifi_if == WIFI_IF_STA ? "STA" : "AP");
#else
    DEBUG_INFO ("Starting ESP-NOW in in channel %u", channel);
#endif //ESP32
    
    this->channel = channel;
    initComms ();
    // addPeer (ESPNOW_BROADCAST_ADDRESS); // Not needed ?
    return true;
}

void QuickEspNow::stop () {
    DEBUG_INFO ("-------------> ESP-NOW STOP");
#ifdef ESP32
    vTaskDelete (espnowLoopTask);
#else
    os_timer_disarm (&espnowLoopTask);
#endif //ESP32
    esp_now_unregister_recv_cb ();
    esp_now_unregister_send_cb ();
    esp_now_deinit ();

}

bool QuickEspNow::setChannel (uint8_t channel) {
#ifdef ESP32
    esp_err_t err_ok;
    if ((err_ok = esp_wifi_set_promiscuous (true))) {
        DEBUG_ERROR ("Error setting promiscuous mode: %s", esp_err_to_name (err_ok));
        return false;
    }
    if ((err_ok = esp_wifi_set_channel (channel, WIFI_SECOND_CHAN_NONE))) {
        DEBUG_ERROR ("Error setting wifi channel: %s", esp_err_to_name (err_ok));
        return false;
    }
    if ((err_ok = esp_wifi_set_promiscuous (false))) {
        DEBUG_ERROR ("Error setting promiscuous mode off: %s", esp_err_to_name (err_ok));
        return false;
    }
#else
    if (!wifi_set_channel (channel)) {
        DEBUG_ERROR ("Error setting wifi channel: %u",channel);
        return false;
    }
#endif
    return true;
}

int32_t QuickEspNow::send (uint8_t* dstAddress, uint8_t* payload, size_t payload_len) {
    comms_queue_item_t message;

    if (!dstAddress || !payload || !payload_len) {
        DEBUG_WARN ("Parameters error");
        return -1;
    }

    if (payload_len > ESP_NOW_MAX_DATA_LEN) {
        DEBUG_WARN ("Length error");
        return -1;
    }

    if (out_queue.size () >= ESPNOW_QUEUE_SIZE) {
        out_queue.pop ();
    }

    memcpy (message.dstAddress, dstAddress, ESP_NOW_ETH_ALEN);
    message.payload_len = payload_len;
    memcpy (message.payload, payload, payload_len);

    if (out_queue.push (&message)) {
        DEBUG_DBG ("--------- %d Comms messages queued. Len: %d", out_queue.size (), payload_len);
        DEBUG_DBG ("--------- Ready to send is %s", readyToSend ? "true" : "false");
        return 0;
    } else {
        DEBUG_WARN ("Error queuing Comms message to " MACSTR, MAC2STR (dstAddress));
        return -1;
    }
}

void QuickEspNow::onDataRcvd (comms_hal_rcvd_data dataRcvd) {
    this->dataRcvd = dataRcvd;
}

void QuickEspNow::onDataSent (comms_hal_sent_data sentResult) {
    this->sentResult = sentResult;
}

int32_t QuickEspNow::sendEspNowMessage (comms_queue_item_t* message) {
    int32_t error;

    if (!message) {
        return -1;
    }
    if (!(message->payload_len) || (message->payload_len > ESP_NOW_MAX_DATA_LEN)) {
        return -1;
    }

    DEBUG_DBG ("ESP-NOW message to ", MACSTR, MAC2STR (message->dstAddress));


#ifdef ESP32
    addPeer (message->dstAddress);
    DEBUG_DBG ("Peer added");
#endif
    readyToSend = false;
    DEBUG_DBG ("-------------- Ready to send: false");

    error = esp_now_send (message->dstAddress, message->payload, message->payload_len);

#ifdef ESP32
    DEBUG_DBG ("esp now send result = %s", esp_err_to_name (error));
    // if (memcmp (message->dstAddress, ESPNOW_BROADCAST_ADDRESS, ESP_NOW_ETH_ALEN)) {
    //     error = esp_now_del_peer (message->dstAddress);
    //     DEBUG_DBG ("Peer deleted. Result %s", esp_err_to_name (error));
    // } else {
    //     DEBUG_DBG ("Broadcast message. No peer deleted");
    // }
#endif
    return error;
}

void QuickEspNow::handle () {
    if (readyToSend) {
    //DEBUG_WARN ("Process queue: Elements: %d", out_queue.size ());
        if (!out_queue.empty ()) {
            comms_queue_item_t* message;
            message = out_queue.front ();
            if (message) {
                DEBUG_DBG ("Comms message got from queue");
                if (!sendEspNowMessage (message)) {
                    DEBUG_DBG ("Message to " MACSTR " sent. Len: %u", MAC2STR (message->dstAddress), message->payload_len);
                } else {
                    DEBUG_WARN ("Error sending message to " MACSTR ". Len: %u", MAC2STR (message->dstAddress), message->payload_len);
                }
                message->payload_len = 0;
                out_queue.pop ();
                DEBUG_DBG ("Comms message pop. Queue size %d", out_queue.size ());
            }
        }
    }

}

#ifdef ESP32
void QuickEspNow::enableTransmit (bool enable) {
    DEBUG_DBG ("Send esp-now task %s", enable ? "enabled" : "disabled");
    if (enable) {
        if (espnowLoopTask) {
            vTaskResume (espnowLoopTask);
        }
    } else {
        if (espnowLoopTask) {
            vTaskSuspend (espnowLoopTask);
        }
    }
}
#else
void QuickEspNow::enableTransmit (bool enable) {
    DEBUG_DBG ("Send esp-now task %s", enable ? "enabled" : "disabled");
    if (enable) {
        os_timer_arm (&espnowLoopTask, 20, true);
    } else {
        os_timer_disarm (&espnowLoopTask);
    }
}
#endif

#ifdef ESP32
bool QuickEspNow::addPeer (const uint8_t* peer_addr) {
    esp_now_peer_info_t peer;
    esp_err_t error = ESP_OK;

    if (peer_list.peer_exists (peer_addr)) {
        DEBUG_VERBOSE ("Peer already exists");
        return true;
    }

    if (peer_list.get_peer_number() >= ESP_NOW_MAX_TOTAL_PEER_NUM) {
        DEBUG_VERBOSE ("Peer list full. Deleting older");
        if (uint8_t* deleted_mac = peer_list.delete_peer ()) {
            esp_now_del_peer (deleted_mac);
        } else {
            DEBUG_ERROR ("Error deleting peer");
            return false;
        }
    }

    memcpy (peer.peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
    uint8_t ch;
    wifi_second_chan_t secondCh;
    esp_wifi_get_channel (&ch, &secondCh);
    peer.channel = ch;
    peer.ifidx = wifi_if;
    peer.encrypt = false;
    error = esp_now_add_peer (&peer);
    if (!error) {
        DEBUG_DBG ("Peer added");
        peer_list.add_peer (peer_addr);
    } else {
        DEBUG_ERROR ("Error adding peer: %s", esp_err_to_name (error));
        return false;
    }
    DEBUG_DBG ("Peer " MACSTR " added on channel %u. Result 0x%X %s", MAC2STR (peer_addr), ch, error, esp_err_to_name (error));
    return error == ESP_OK;
}
#endif

void QuickEspNow::initComms () {
    if (esp_now_init ()) {
        DEBUG_ERROR ("Failed to init ESP-NOW");
        ESP.restart ();
        delay (1);
    }

    esp_now_register_recv_cb (reinterpret_cast<esp_now_recv_cb_t>(rx_cb));
    esp_now_register_send_cb (reinterpret_cast<esp_now_send_cb_t>(tx_cb));

#ifdef ESP32
    xTaskCreateUniversal (runHandle, "espnow_loop", 2048, NULL, 1, &espnowLoopTask, CONFIG_ARDUINO_RUNNING_CORE);
#else
    os_timer_setfn (&espnowLoopTask, runHandle, NULL);
    os_timer_arm (&espnowLoopTask, 20, true);
#endif // ESP32
    

}

#ifdef ESP32
void QuickEspNow::runHandle (void* param) {
    for (;;) {
        quickEspNow.handle ();
        vTaskDelay (1 / portTICK_PERIOD_MS);
    }

}
#else
void QuickEspNow::runHandle (void* param) {
    quickEspNow.handle ();
}
#endif // ESP32

void QuickEspNow::rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len) {
    //espnow_frame_format_t* espnow_data = (espnow_frame_format_t*)(data - sizeof (espnow_frame_format_t));
    wifi_promiscuous_pkt_t* promiscuous_pkt = (wifi_promiscuous_pkt_t*)(data - sizeof (wifi_pkt_rx_ctrl_t) - sizeof (espnow_frame_format_t));
    wifi_pkt_rx_ctrl_t* rx_ctrl = &promiscuous_pkt->rx_ctrl;

    if (quickEspNow.dataRcvd) {
        // quickEspNow.dataRcvd (mac_addr, data, len, rx_ctrl->rssi - 98); // rssi should be in dBm but it has added almost 100 dB. Do not know why
        quickEspNow.dataRcvd (mac_addr, data, len, rx_ctrl->rssi); // rssi should be in dBm but it has added almost 100 dB. Do not know why
    }
}
void QuickEspNow::tx_cb (uint8_t* mac_addr, uint8_t status) {
    quickEspNow.readyToSend = true;
    DEBUG_DBG ("-------------- Ready to send: true");
    if (quickEspNow.sentResult) {
        quickEspNow.sentResult (mac_addr, status);
    }
}

#ifdef ESP32
uint8_t PeerListClass::get_peer_number () {
    return peer_list.peer_number;
}

bool PeerListClass::peer_exists (const uint8_t* mac) {
    for (uint8_t i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (memcmp (peer_list.peer[i].mac, mac, ESP_NOW_ETH_ALEN) == 0) {
            if (peer_list.peer[i].active) {
                peer_list.peer[i].last_msg = millis ();
                DEBUG_VERBOSE ("Peer " MACSTR " found. Updated last_msg", MAC2STR (mac));
                return true;
            }
        }
    }
    return false;
}

peer_t* PeerListClass::get_peer (const uint8_t* mac) {
    for (uint8_t i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (memcmp (peer_list.peer[i].mac, mac, ESP_NOW_ETH_ALEN) == 0) {
            if (peer_list.peer[i].active) {
                DEBUG_VERBOSE ("Peer " MACSTR " found", MAC2STR (mac));
                return &(peer_list.peer[i]);
            }
        }
    }
    return NULL;
}

bool PeerListClass::update_peer_use (const uint8_t* mac) {
    peer_t* peer = get_peer (mac);
    if (peer) {
        peer->last_msg = millis ();
        return true;
    }
    return false;
}

bool PeerListClass::add_peer (const uint8_t* mac) {
    if (int i = peer_exists (mac)) {
        DEBUG_VERBOSE ("Peer " MACSTR " already exists", MAC2STR (mac));
        return false;
    }
    if (peer_list.peer_number >= ESP_NOW_MAX_TOTAL_PEER_NUM) {
        //DEBUG_VERBOSE ("Peer list full. Deleting older");
#ifndef UNIT_TEST
        DEBUG_ERROR ("Should never happen");
#endif
        return false;
        // delete_peer (); // Delete should happen in higher level
    }

    for (uint8_t i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (!peer_list.peer[i].active) {
            memcpy (peer_list.peer[i].mac, mac, ESP_NOW_ETH_ALEN);
            peer_list.peer[i].active = true;
            peer_list.peer[i].last_msg = millis ();
            peer_list.peer_number++;
            DEBUG_VERBOSE ("Peer " MACSTR " added. Total peers = %d", MAC2STR (mac), peer_list.peer_number);
            return true;
        }
    }

    return false;
}

bool PeerListClass::delete_peer (const uint8_t* mac) {
    peer_t* peer = get_peer (mac);
    if (peer) {
        peer->active = false;
        peer_list.peer_number--;
        DEBUG_VERBOSE ("Peer " MACSTR " deleted. Total peers = %d", MAC2STR (mac), peer_list.peer_number);
        return true;
    }
    return false;
}

// Delete peer with older message
uint8_t* PeerListClass::delete_peer () {
    uint32_t oldest_msg = 0;
    int oldest_index = -1;
    uint8_t* mac = NULL;
    for (int i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (peer_list.peer[i].active) {
            if (peer_list.peer[i].last_msg < oldest_msg || oldest_msg == 0) {
                oldest_msg = peer_list.peer[i].last_msg;
                oldest_index = i;
                DEBUG_VERBOSE ("Peer " MACSTR " is %d ms old. Deleting", MAC2STR (peer_list.peer[i].mac), oldest_msg);
            }
        }
    }
    if (oldest_index != -1) {
        peer_list.peer[oldest_index].active = false;
        peer_list.peer_number--;
        mac = peer_list.peer[oldest_index].mac;
        DEBUG_VERBOSE ("Peer " MACSTR " deleted. Last message %d ms ago. Total peers = %d", MAC2STR (mac), millis() - peer_list.peer[oldest_index].last_msg, peer_list.peer_number);
    }
    return mac;
}

#ifdef UNIT_TEST
void PeerListClass::dump_peer_list () {
    Serial.printf ("Number of peers %d\n", peer_list.peer_number);
    for (int i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (peer_list.peer[i].active) {
            Serial.printf ("Peer " MACSTR " is %d ms old\n", MAC2STR (peer_list.peer[i].mac), millis() - peer_list.peer[i].last_msg);
        }
    }
}
#endif // UNIT_TEST
#endif // ESP8266
#endif // ESP32