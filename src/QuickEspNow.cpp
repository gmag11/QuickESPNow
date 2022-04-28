#include "QuickEspNow.h"
#include "QuickDebug.h"

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
    if (channel < MIN_WIFI_CHANNEL || channel > MAX_WIFI_CHANNEL || channel == 255) {
        DEBUG_ERROR ("Invalid wifi channel");
        return false;
    }

    // use current channel
    if (channel == 255) {
        uint8_t ch;
        wifi_second_chan_t ch2;
        esp_wifi_get_channel (&ch, &ch2);
        DEBUG_INFO ("Current channel: %d", ch);
        channel = ch;
    } else {
        setChannel(channel);
    }

    DEBUG_INFO ("Starting ESP-NOW in in channel %u interface %s", channel, wifi_if == WIFI_IF_STA ? "STA" : "AP");
    this->channel = channel;
    initComms ();
    addPeer (ESPNOW_BROADCAST_ADDRESS);
    return true;
}

void QuickEspNow::stop () {
    DEBUG_INFO ("-------------> ESP-NOW STOP");
    vTaskDelete (espnowLoopTask);
    esp_now_unregister_recv_cb ();
    esp_now_unregister_send_cb ();
    esp_now_deinit ();

}

bool QuickEspNow::setChannel (uint8_t channel) {
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
    if (memcmp (message->dstAddress, ESPNOW_BROADCAST_ADDRESS, ESP_NOW_ETH_ALEN)) {
        error = esp_now_del_peer (message->dstAddress);
        DEBUG_DBG ("Peer deleted. Result %s", esp_err_to_name (error));
    } else {
        DEBUG_DBG ("Broadcast message. No peer deleted");
    }
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

bool QuickEspNow::addPeer (const uint8_t* peer_addr) {
    esp_now_peer_info_t peer;
    esp_err_t error = ESP_OK;

    if (esp_now_is_peer_exist (peer_addr)) {
        
        return true;
    }
    
    memcpy (peer.peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
    uint8_t ch;
    wifi_second_chan_t secondCh;
    esp_wifi_get_channel (&ch, &secondCh);
    peer.channel = ch;
    peer.ifidx = wifi_if;
    peer.encrypt = false;
    error = esp_now_add_peer (&peer);
    DEBUG_DBG ("Peer " MACSTR " added on channel %u. Result 0x%X %s", MAC2STR (peer_addr), ch, error, esp_err_to_name (error));
    return error == ESP_OK;
}

void QuickEspNow::initComms () {
    if (esp_now_init ()) {
        DEBUG_ERROR ("Failed to init ESP-NOW");
        ESP.restart ();
        delay (1);
    }

    esp_now_register_recv_cb (reinterpret_cast<esp_now_recv_cb_t>(rx_cb));
    esp_now_register_send_cb (reinterpret_cast<esp_now_send_cb_t>(tx_cb));

    xTaskCreateUniversal (runHandle, "espnow_loop", 2048, NULL, 1, &espnowLoopTask, CONFIG_ARDUINO_RUNNING_CORE);

}

void QuickEspNow::runHandle (void* param) {
    for (;;) {
        quickEspNow.handle ();
        vTaskDelay (1 / portTICK_PERIOD_MS);
    }

}

void ICACHE_FLASH_ATTR QuickEspNow::rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len) {
    //espnow_frame_format_t* espnow_data = (espnow_frame_format_t*)(data - sizeof (espnow_frame_format_t));
    wifi_promiscuous_pkt_t* promiscuous_pkt = (wifi_promiscuous_pkt_t*)(data - sizeof (wifi_pkt_rx_ctrl_t) - sizeof (espnow_frame_format_t));
    wifi_pkt_rx_ctrl_t* rx_ctrl = &promiscuous_pkt->rx_ctrl;

    if (quickEspNow.dataRcvd) {
        // quickEspNow.dataRcvd (mac_addr, data, len, rx_ctrl->rssi - 98); // rssi should be in dBm but it has added almost 100 dB. Do not know why
        quickEspNow.dataRcvd (mac_addr, data, len, rx_ctrl->rssi); // rssi should be in dBm but it has added almost 100 dB. Do not know why
    }
}

void ICACHE_FLASH_ATTR QuickEspNow::tx_cb (uint8_t* mac_addr, uint8_t status) {
    quickEspNow.readyToSend = true;
    DEBUG_DBG ("-------------- Ready to send: true");
    if (quickEspNow.sentResult) {
        quickEspNow.sentResult (mac_addr, status);
    }
}

uint8_t PeerListClass::get_peer_number () {
    return peer_list.peer_number;
}

bool PeerListClass::peer_exists (uint8_t* mac) {
    for (uint8_t i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (memcmp (peer_list.peer[i].mac, mac, ESP_NOW_ETH_ALEN) == 0) {
            if (peer_list.peer[i].active) {
                return true;
            }
        }
    }
    return false;
}

peer_t* PeerListClass::get_peer (uint8_t* mac) {
    for (uint8_t i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (memcmp (peer_list.peer[i].mac, mac, ESP_NOW_ETH_ALEN) == 0) {
            if (peer_list.peer[i].active) {
                return &(peer_list.peer[i]);
            }
        }
    }
    return NULL;
}

bool PeerListClass::update_peer_use (uint8_t* mac) {
    peer_t* peer = get_peer (mac);
    if (peer) {
        peer->last_msg = millis ();
        return true;
    }
    return false;
}

bool PeerListClass::add_peer (uint8_t* mac) {
    if (peer_exists (mac)) {
        return false;
    }
    for (uint8_t i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (!peer_list.peer[i].active) {
            memcpy (peer_list.peer[i].mac, mac, ESP_NOW_ETH_ALEN);
            peer_list.peer[i].active = true;
            peer_list.peer[i].last_msg = millis ();
            peer_list.peer_number++;
            return true;
        }
    }
}

bool PeerListClass::delete_peer (uint8_t* mac) {
    peer_t* peer = get_peer (mac);
    if (peer) {
        peer->active = false;
        peer_list.peer_number--;
        return true;
    }
    return false;
}

// Delete peer with older message
bool PeerListClass::delete_peer () {
    uint32_t oldest_msg = 0;
    int oldest_index = -1;
    for (int i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (peer_list.peer[i].active) {
            if (peer_list.peer[i].last_msg < oldest_msg || oldest_msg == 0) {
                oldest_msg = peer_list.peer[i].last_msg;
                oldest_index = i;
            }
        }
    }
    if (oldest_index != -1) {
        peer_list.peer[oldest_index].active = false;
        peer_list.peer_number--;
        return true;
    }
    return false;
}