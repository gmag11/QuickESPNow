#include "QuickEspNow.h"
#include "QuickDebug.h"

#ifdef ESP8266

constexpr auto TAG = "QESPNOW";

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

QuickEspNow quickEspNow;

bool QuickEspNow::begin (uint8_t channel, uint32_t wifi_interface) {

    DEBUG_INFO (TAG, "Channel: %d, Interface: %d", channel, wifi_interface);
    // Set the wifi interface
    switch (wifi_interface) {
    case WIFI_IF_STA:
        wifi_if = WIFI_IF_STA;
        break;
    case WIFI_IF_AP:
        wifi_if = WIFI_IF_AP;
        break;
    default:
        DEBUG_ERROR (TAG, "Unknown wifi interface");
        return false;
        break;
    }

    // check channel
    if (channel != CURRENT_WIFI_CHANNEL && (channel < MIN_WIFI_CHANNEL || channel > MAX_WIFI_CHANNEL)) {
        DEBUG_ERROR (TAG, "Invalid wifi channel %d", channel);
        return false;
    }

    // use current channel
    if (channel == CURRENT_WIFI_CHANNEL) {
        uint8_t ch;
        ch = WiFi.channel ();
        DEBUG_INFO (TAG, "Current channel: %d", ch);
        channel = ch;
    } else {
        setChannel (channel);
    }
    DEBUG_INFO (TAG, "Starting ESP-NOW in in channel %u interface %s", channel, wifi_if == WIFI_IF_STA ? "STA" : "AP");

    this->channel = channel;
    initComms ();
    // addPeer (ESPNOW_BROADCAST_ADDRESS); // Not needed ?
    return true;
}

void QuickEspNow::stop () {
    DEBUG_INFO (TAG, "-------------> ESP-NOW STOP");
    os_timer_disarm (&espnowTxTask);
    os_timer_disarm (&espnowRxTask);
    esp_now_unregister_recv_cb ();
    esp_now_unregister_send_cb ();
    esp_now_deinit ();

}

bool QuickEspNow::setChannel (uint8_t channel) {
    if (!wifi_set_channel (channel)) {
        DEBUG_ERROR (TAG, "Error setting wifi channel: %u", channel);
        return false;
    }
    return true;
}

int32_t QuickEspNow::send (const uint8_t* dstAddress, const uint8_t* payload, size_t payload_len) {
    comms_tx_queue_item_t message;

    if (!dstAddress || !payload || !payload_len) {
        DEBUG_WARN (TAG, "Parameters error");
        return -1;
    }

    if (payload_len > ESP_NOW_MAX_DATA_LEN) {
        DEBUG_WARN (TAG, "Length error. %d", payload_len);
        return -1;
    }

    if (tx_queue.size () >= ESPNOW_QUEUE_SIZE) {
#ifdef MEAS_TPUT
        comms_tx_queue_item_t* tempBuffer;
        tempBuffer = tx_queue.front ();
        txDataDropped += tempBuffer->payload_len;
#endif // MEAS_TPUT
        tx_queue.pop ();
        DEBUG_DBG (TAG, "Message dropped");
    }

    memcpy (message.dstAddress, dstAddress, ESP_NOW_ETH_ALEN);
    message.payload_len = payload_len;
    memcpy (message.payload, payload, payload_len);

    if (tx_queue.push (&message)) {
#ifdef MEAS_TPUT
        txDataSent += message.payload_len;
#endif // MEAS_TPUT
        DEBUG_DBG (TAG, "--------- %d Comms messages queued. Len: %d", tx_queue.size (), payload_len);
        DEBUG_VERBOSE (TAG, "--------- Ready to send is %s", readyToSend ? "true" : "false");
        return 0;
    } else {
        DEBUG_WARN (TAG, "Error queuing Comms message to " MACSTR, MAC2STR (dstAddress));
        return -1;
    }
}

void QuickEspNow::onDataRcvd (comms_hal_rcvd_data dataRcvd) {
    this->dataRcvd = dataRcvd;
}

#ifdef MEAS_TPUT
void QuickEspNow::calculateDataTP () {
    time_t measTime = (millis () - lastDataTPMeas);
    lastDataTPMeas = millis ();

    if (txDataSent > 0) {
        txDataTP = txDataSent * 1000 / measTime;
        //DEBUG_WARN(TAG, "Meas time: %d, Data sent: %d, Data TP: %f", measTime, txDataSent, txDataTP);
        txDroppedDataRatio = (float)txDataDropped / (float)txDataSent;
        //DEBUG_WARN(TAG, "Data dropped: %d, Drop ratio: %f", txDataDropped, txDroppedDataRatio);
        txDataSent = 0;
    } else {
        txDataTP = 0;
        txDroppedDataRatio = 0;
    }
    if (rxDataReceived > 0) {
        rxDataTP = rxDataReceived * 1000 / measTime;
        //DEBUG_WARN(TAG, "Meas time: %d, Data received: %d, Data TP: %f", measTime, rxDataReceived, rxDataTP);
        rxDataReceived = 0;
    } else {
        rxDataTP = 0;
    }
    txDataDropped = 0;
}

void QuickEspNow::tp_timer_cb (void* param) {
    quickEspNow.calculateDataTP ();
    DEBUG_WARN (TAG, "TxData TP: %.3f kbps, Drop Ratio: %.2f %%, RxDataTP: %.3f kbps",
                quickEspNow.txDataTP * 8 / 1000,
                quickEspNow.txDroppedDataRatio * 100,
                quickEspNow.rxDataTP * 8 / 1000);
}
#endif // MEAS_TPUT

void QuickEspNow::onDataSent (comms_hal_sent_data sentResult) {
    this->sentResult = sentResult;
}

int32_t QuickEspNow::sendEspNowMessage (comms_tx_queue_item_t* message) {
    int32_t error;

    if (!message) {
        DEBUG_WARN (TAG, "Message is null");
        return -1;
    }
    if (!(message->payload_len) || (message->payload_len > ESP_NOW_MAX_DATA_LEN)) {
        DEBUG_WARN (TAG, "Message length error");
        return -1;
    }

    DEBUG_VERBOSE (TAG, "ESP-NOW message to " MACSTR, MAC2STR (message->dstAddress));

    readyToSend = false;
    DEBUG_VERBOSE (TAG, "-------------- Ready to send: false");

    error = esp_now_send (message->dstAddress, message->payload, message->payload_len);
    DEBUG_DBG (TAG, "esp now send result = %d", error);

    return error;
}

void QuickEspNow::espnowTxHandle () {
    if (readyToSend) {
        //DEBUG_WARN ("Process queue: Elements: %d", tx_queue.size ());
        comms_tx_queue_item_t* message;
        while (!tx_queue.empty ()) {
            message = tx_queue.front ();
            DEBUG_DBG (TAG, "Comms message got from queue. %d left", tx_queue.size ());
            while (!readyToSend) {
                delay (1);
            }
            if (!sendEspNowMessage (message)) {
                DEBUG_DBG (TAG, "Message to " MACSTR " sent. Len: %u", MAC2STR (message->dstAddress), message->payload_len);
            } else {
                DEBUG_WARN (TAG, "Error sending message to " MACSTR ". Len: %u", MAC2STR (message->dstAddress), message->payload_len);
            }
            message->payload_len = 0;
            tx_queue.pop ();
            DEBUG_DBG (TAG, "Comms message pop. Queue size %d", tx_queue.size ());
        }

    } else {
        DEBUG_DBG (TAG, "Not ready to send");
    }
}

void QuickEspNow::enableTransmit (bool enable) {
    DEBUG_DBG (TAG, "Send esp-now task %s", enable ? "enabled" : "disabled");
    if (enable) {
        os_timer_arm (&espnowTxTask, TASK_PERIOD, true);
        os_timer_arm (&espnowRxTask, TASK_PERIOD, true);
    } else {
        os_timer_disarm (&espnowTxTask);
        os_timer_disarm (&espnowRxTask);
    }
}

void QuickEspNow::initComms () {
    if (esp_now_init ()) {
        DEBUG_ERROR (TAG, "Failed to init ESP-NOW");
        ESP.restart ();
        delay (1);
    }

    if (wifi_if == WIFI_IF_STA) {
        esp_now_set_self_role (ESP_NOW_ROLE_SLAVE);
    } else {
        esp_now_set_self_role (ESP_NOW_ROLE_CONTROLLER);
    }

    esp_now_register_recv_cb (reinterpret_cast<esp_now_recv_cb_t>(rx_cb));
    esp_now_register_send_cb (reinterpret_cast<esp_now_send_cb_t>(tx_cb));

    os_timer_setfn (&espnowTxTask, espnowTxTask_cb, NULL);
    os_timer_arm (&espnowTxTask, TASK_PERIOD, true);

    os_timer_setfn (&espnowRxTask, espnowRxTask_cb, NULL);
    os_timer_arm (&espnowRxTask, TASK_PERIOD, true);


#ifdef MEAS_TPUT
    os_timer_setfn (&dataTPTimer, tp_timer_cb, NULL);
    os_timer_arm (&dataTPTimer, MEAS_TP_EVERY_MS, true);
#endif // MEAS_TPUT

}

void QuickEspNow::espnowTxTask_cb (void* param) {
    quickEspNow.espnowTxHandle ();
}

void QuickEspNow::rx_cb (uint8_t* mac_addr, uint8_t* data, uint8_t len) {
    espnow_frame_format_t* espnow_data = (espnow_frame_format_t*)(data - sizeof (espnow_frame_format_t));
    wifi_promiscuous_pkt_t* promiscuous_pkt = (wifi_promiscuous_pkt_t*)(data - sizeof (wifi_pkt_rx_ctrl_t) - sizeof (espnow_frame_format_t));
    wifi_pkt_rx_ctrl_t* rx_ctrl = &promiscuous_pkt->rx_ctrl;

    comms_rx_queue_item_t message;

    DEBUG_DBG (TAG, "Received message with RSSI %d from " MACSTR " Len: %u", rx_ctrl->rssi, MAC2STR (mac_addr), len);

    memcpy (message.srcAddress, mac_addr, ESP_NOW_ETH_ALEN);
    memcpy (message.payload, data, len);
    message.payload_len = len;
    message.rssi = rx_ctrl->rssi - 100;
    message.channel = rx_ctrl->channel;
    memcpy (message.dstAddress, espnow_data->destination_address, ESP_NOW_ETH_ALEN);
    
    if (quickEspNow.rx_queue.size () >= ESPNOW_QUEUE_SIZE) {
        quickEspNow.tx_queue.pop ();
        DEBUG_DBG (TAG, "Rx Message dropped");
    }
    
#ifdef MEAS_TPUT
    quickEspNow.rxDataReceived += len;
#endif // MEAS_TPUT

    if (quickEspNow.rx_queue.push (&message)) {
        DEBUG_DBG (TAG, "Message pushed to queue");
    } else {
        DEBUG_WARN (TAG, "Error queuing message");
    }
}

void QuickEspNow::espnowRxTask_cb (void* param) {
    quickEspNow.espnowRxHandle ();
}

void QuickEspNow::espnowRxHandle () {
    comms_rx_queue_item_t *rxMessage;

    if (!rx_queue.empty ()) {
        rxMessage = rx_queue.front ();
        DEBUG_DBG (TAG, "Comms message got from queue. %d left", rx_queue.size ());
        DEBUG_VERBOSE (TAG, "Received message from " MACSTR " Len: %u", MAC2STR (rxMessage->srcAddress), rxMessage->payload_len);
        DEBUG_VERBOSE (TAG, "Message: %.*s", rxMessage->payload_len, rxMessage->payload);


        if (quickEspNow.dataRcvd) {
            bool broadcast = ! memcmp (rxMessage->dstAddress, ESPNOW_BROADCAST_ADDRESS, ESP_NOW_ETH_ALEN);
        // quickEspNow.dataRcvd (mac_addr, data, len, rx_ctrl->rssi - 98); // rssi should be in dBm but it has added almost 100 dB. Do not know why
            quickEspNow.dataRcvd (rxMessage->srcAddress, rxMessage->payload, rxMessage->payload_len, rxMessage->rssi, broadcast, rxMessage->channel); // rssi should be in dBm but it has added almost 100 dB. Do not know why
        }

        rxMessage->payload_len = 0;
        rx_queue.pop ();
        DEBUG_DBG (TAG, "RX Comms message pop. Queue size %d", rx_queue.size ());
    }

}

void QuickEspNow::tx_cb (uint8_t* mac_addr, uint8_t status) {
    quickEspNow.readyToSend = true;
    DEBUG_DBG (TAG, "-------------- Ready to send: true");
    if (quickEspNow.sentResult) {
        quickEspNow.sentResult (mac_addr, status);
    }
}

#endif // ESP8266