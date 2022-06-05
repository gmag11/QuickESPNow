#include "QuickEspNow.h"
#include "QuickDebug.h"

#ifdef ESP32

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
        wifi_second_chan_t ch2;
        esp_wifi_get_channel (&ch, &ch2);
        DEBUG_INFO ("Current channel: %d", ch);
        channel = ch;
    } else {
        setChannel (channel);
    }
    DEBUG_INFO ("Starting ESP-NOW in in channel %u interface %s", channel, wifi_if == WIFI_IF_STA ? "STA" : "AP");

    this->channel = channel;
    initComms ();
    // addPeer (ESPNOW_BROADCAST_ADDRESS); // Not needed ?
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
        DEBUG_WARN ("Length error. %d", payload_len);
        return -1;
    }

    if (uxQueueMessagesWaiting (out_queue) >= ESPNOW_QUEUE_SIZE) {
        comms_queue_item_t tempBuffer;
        xQueueReceive (out_queue, &tempBuffer, 0);
        txDataDropped += tempBuffer.payload_len;
        DEBUG_DBG ("Message dropped");
    }
    memcpy (message.dstAddress, dstAddress, ESP_NOW_ETH_ALEN);
    message.payload_len = payload_len;
    memcpy (message.payload, payload, payload_len);

    if (xQueueSend (out_queue, &message, pdMS_TO_TICKS (10))) {
        txDataSent += message.payload_len;
        DEBUG_DBG ("--------- %d Comms messages queued. Len: %d", uxQueueMessagesWaiting (out_queue), payload_len);
        DEBUG_VERBOSE ("--------- Ready to send is %s", readyToSend ? "true" : "false");
        return 0;
    } else {
        DEBUG_WARN ("Error queuing Comms message to " MACSTR, MAC2STR (dstAddress));
        return -1;
    }
}

void QuickEspNow::onDataRcvd (comms_hal_rcvd_data dataRcvd) {
    this->dataRcvd = dataRcvd;
}

void QuickEspNow::calculateDataTP () {
    time_t measTime = (millis () - lastDataTPMeas);
    lastDataTPMeas = millis ();

    if (txDataSent > 0) {
        txDataTP = txDataSent * 1000 / measTime;
        //DEBUG_WARN("Meas time: %d, Data sent: %d, Data TP: %f", measTime, txDataSent, txDataTP);
        txDroppedDataRatio = (float)txDataDropped / (float)txDataSent;
        //DEBUG_WARN("Data dropped: %d, Drop ratio: %f", txDataDropped, txDroppedDataRatio);
        txDataSent = 0;
    } else {
        txDataTP = 0;
        txDroppedDataRatio = 0;
    }
    if (rxDataReceived > 0) {
        rxDataTP = rxDataReceived * 1000 / measTime;
        //DEBUG_WARN("Meas time: %d, Data received: %d, Data TP: %f", measTime, rxDataReceived, rxDataTP);
        rxDataReceived = 0;
    } else {
        rxDataTP = 0;
    }
    txDataDropped = 0;
}

void QuickEspNow::onDataSent (comms_hal_sent_data sentResult) {
    this->sentResult = sentResult;
}

int32_t QuickEspNow::sendEspNowMessage (comms_queue_item_t* message) {
    int32_t error;

    if (!message) {
        DEBUG_WARN ("Message is null");
        return -1;
    }
    if (!(message->payload_len) || (message->payload_len > ESP_NOW_MAX_DATA_LEN)) {
        DEBUG_WARN ("Message length error");
        return -1;
    }

    DEBUG_VERBOSE ("ESP-NOW message to " MACSTR, MAC2STR (message->dstAddress));


    addPeer (message->dstAddress);
    DEBUG_DBG ("Peer added " MACSTR, MAC2STR (message->dstAddress));
    readyToSend = false;
    DEBUG_VERBOSE ("-------------- Ready to send: false");

    error = esp_now_send (message->dstAddress, message->payload, message->payload_len);
    DEBUG_DBG ("esp now send result = %s", esp_err_to_name (error));
    if (error != ESP_OK) {
        DEBUG_WARN ("Error sending message: %s", esp_err_to_name (error));
    }
    // if (error == ESP_OK) {
    //     txDataSent += message->payload_len;
    // }
    if (error == ESP_ERR_ESPNOW_NO_MEM) {
        delay (2);
    }

    return error;
}

void QuickEspNow::handle () {
    if (readyToSend) {
    //DEBUG_WARN ("Process queue: Elements: %d", out_queue.size ());
        comms_queue_item_t message;
        while (xQueueReceive (out_queue, &message, pdMS_TO_TICKS (1000))) {
            DEBUG_DBG ("Comms message got from queue. %d left", uxQueueMessagesWaiting (out_queue));
            while (!readyToSend) {
                delay (1);
            }
            if (!sendEspNowMessage (&message)) {
                DEBUG_DBG ("Message to " MACSTR " sent. Len: %u", MAC2STR (message.dstAddress), message.payload_len);
            } else {
                DEBUG_WARN ("Error sending message to " MACSTR ". Len: %u", MAC2STR (message.dstAddress), message.payload_len);
            }
        //message.payload_len = 0;
            DEBUG_DBG ("Comms message pop. Queue size %d", uxQueueMessagesWaiting (out_queue));
        }

    } else {
        DEBUG_DBG ("Not ready to send");
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

    if (peer_list.peer_exists (peer_addr)) {
        DEBUG_VERBOSE ("Peer already exists");
        return true;
    }

    if (peer_list.get_peer_number () >= ESP_NOW_MAX_TOTAL_PEER_NUM) {
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

void QuickEspNow::tp_timer_cb (void* param) {
    quickEspNow.calculateDataTP ();
    DEBUG_WARN ("TxData TP: %.1f kbps, Drop Ratio: %.2f %%, RxDataTP: %.1f kbps",
                quickEspNow.txDataTP * 8 / 1000,
                quickEspNow.txDroppedDataRatio * 100,
                quickEspNow.rxDataTP * 8 / 1000);
}

void QuickEspNow::initComms () {
    if (esp_now_init ()) {
        DEBUG_ERROR ("Failed to init ESP-NOW");
        ESP.restart ();
        delay (1);
    }

    esp_now_register_recv_cb (reinterpret_cast<esp_now_recv_cb_t>(rx_cb));
    esp_now_register_send_cb (reinterpret_cast<esp_now_send_cb_t>(tx_cb));

    espnow_send_mutex = xSemaphoreCreateMutex ();
    out_queue = xQueueCreate (ESPNOW_QUEUE_SIZE, sizeof (comms_queue_item_t));
    xTaskCreateUniversal (runHandle, "espnow_loop", 8 * 1024, NULL, 1, &espnowLoopTask, CONFIG_ARDUINO_RUNNING_CORE);
    dataTPTimer = xTimerCreate ("espnow_tp_timer", pdMS_TO_TICKS (MEAS_TP_EVERY_MS), pdTRUE, NULL, tp_timer_cb);
    xTimerStart (dataTPTimer, 0);
}

void QuickEspNow::runHandle (void* param) {
    for (;;) {
        quickEspNow.handle ();
        //vTaskDelay (1 / portTICK_PERIOD_MS);
    }

}

void QuickEspNow::rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len) {
    //espnow_frame_format_t* espnow_data = (espnow_frame_format_t*)(data - sizeof (espnow_frame_format_t));
    wifi_promiscuous_pkt_t* promiscuous_pkt = (wifi_promiscuous_pkt_t*)(data - sizeof (wifi_pkt_rx_ctrl_t) - sizeof (espnow_frame_format_t));
    wifi_pkt_rx_ctrl_t* rx_ctrl = &promiscuous_pkt->rx_ctrl;

    quickEspNow.rxDataReceived += len;
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
        DEBUG_VERBOSE ("Peer " MACSTR " deleted. Last message %d ms ago. Total peers = %d", MAC2STR (mac), millis () - peer_list.peer[oldest_index].last_msg, peer_list.peer_number);
    }
    return mac;
}

#ifdef UNIT_TEST
void PeerListClass::dump_peer_list () {
    Serial.printf ("Number of peers %d\n", peer_list.peer_number);
    for (int i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++) {
        if (peer_list.peer[i].active) {
            Serial.printf ("Peer " MACSTR " is %d ms old\n", MAC2STR (peer_list.peer[i].mac), millis () - peer_list.peer[i].last_msg);
        }
    }
}
#endif // UNIT_TEST
#endif // ESP32