# QuickESPNow Library - AI Coding Agent Instructions

## Project Overview

QuickESPNow is an Arduino/PlatformIO library that simplifies ESP-NOW wireless communication for ESP32 and ESP8266 microcontrollers. It abstracts ESP-NOW complexities while adding features like unlimited peer management, RSSI information, and dual synchronous/asynchronous sending modes.

## Architecture & Key Components

### Platform Abstraction Pattern

- **Entry Point**: `src/QuickEspNow.h` - Platform-specific header selection via `#ifdef ESP32/#elif ESP8266`
- **Implementation Files**: Separate platform implementations in `QuickEspNow_esp32.cpp/.h` and `QuickEspNow_esp8266.cpp/.h`
- **HAL Interface**: `Comms_hal.h` defines abstract communication interface that platform implementations inherit from

### Core Classes & Responsibilities

- **`QuickEspNow`**: Main API class inheriting from `Comms_halClass`, handles message queuing, peer management, and FreeRTOS tasks
- **`PeerListClass`**: Manages unlimited peer connections (bypasses ESP-NOW's 20-device limit via automatic registration/deregistration)
- **`RingBuffer<T>`**: Template-based thread-safe circular buffer for message queuing
- **Global Instance**: `extern QuickEspNow quickEspNow` - Single global instance pattern

### Threading & Queue Architecture (ESP32)

- **TX Task**: `espnowTxTask_cb()` - Handles outbound message queue processing
- **RX Task**: `espnowRxTask_cb()` - Processes incoming messages
- **Queue Management**: FreeRTOS queues (`tx_queue`, `rx_queue`) with configurable `ESPNOW_QUEUE_SIZE = 3`
- **Synchronous Mode**: Uses `waitingForConfirmation` flag to block until send confirmation

## Development Workflows

### Building Examples

```bash
# Build for specific platform/example
pio run -e esp32_basic_espnow
pio run -e esp8266_advanced_espnow

# Upload and monitor
pio run -e esp32_basic_espnow -t upload -t monitor
```

### PlatformIO Environment Structure

- **Common Configs**: `[esp32_common]`, `[esp8266_common]` - Shared platform settings
- **Example Mapping**: Each environment uses `build_src_filter = -<*> +<examplefolder/>` to build specific examples
- **Debug Levels**: Configurable via `[debug]` section with `CORE_DEBUG_LEVEL`/`DEBUG_LEVEL` macros

### Testing

- **Unit Tests**: Located in `test/test_peer_list/` using Unity framework
- **Test Pattern**: Use `#define UNIT_TEST` to enable test-specific code paths
- **Build Command**: `pio test -e esp32_basic_espnow`

## Project-Specific Conventions

### Callback Function Patterns

```cpp
// Standard callback signatures - use std::function, not raw function pointers
typedef std::function<void(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast)> comms_hal_rcvd_data;
typedef std::function<void(uint8_t* address, uint8_t status)> comms_hal_sent_data;

// Usage pattern in examples
void dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    Serial.printf("%.*s\n", len, data); // Note: %.*s for length-limited string printing
}
quickEspNow.onDataRcvd(dataReceived);
```

### WiFi Setup Pattern

```cpp
WiFi.mode(WIFI_MODE_STA);
#if defined ESP32
    WiFi.disconnect(false, true);
#elif defined ESP8266
    WiFi.disconnect(false);
#endif
quickEspNow.begin(1); // Channel must be specified if not connected to WiFi
```

### Error Handling Convention

- **Send Return Codes**: `comms_send_error_t` enum with specific error conditions
- **Platform Differences**: ESP8266 uses `#define WIFI_MODE_STA WIFI_STA` compatibility macro
- **Debug Macros**: Conditional compilation based on debug levels (avoid QuickDebug dependency at level 0)

### Performance Considerations

- **Message Limits**: `ESPNOW_MAX_MESSAGE_LENGTH = 250` bytes max payload
- **Throughput Optimization**: Use `readyToSendData()` + sent flag pattern for maximum performance
- **Queue Sizing**: Default `ESPNOW_QUEUE_SIZE = 3` - increase for high-throughput scenarios

## Integration Points

### External Dependencies

- **QuickDebug**: Optional debugging library (version 0.7.0) - only linked when debug level > 0
- **Platform Libraries**: ESP-IDF/Arduino core functions for ESP-NOW, WiFi, and FreeRTOS
- **Unity**: Testing framework for unit tests

### WiFi Coexistence

- **Channel Management**: `setChannel()` and `setWiFiBandwidth()` for WiFi coexistence
- **ESP32 Specific**: Use `WIFI_BW_HT20` bandwidth when mixing with ESP8266 devices
- **STA Mode**: Library works alongside WiFi station connections without interference

### Memory Management

- **Static Buffers**: Fixed-size message buffers to avoid dynamic allocation
- **Peer Tracking**: Automatic cleanup of inactive peers using timestamp-based eviction
- **Thread Safety**: Critical sections (`portENTER_CRITICAL`) for shared data structures

## Common Patterns for AI Agents

1. **Adding New Examples**: Create `.cpp` file in `examples/newexample/`, add corresponding `[env:platform_newexample]` section in `platformio.ini`

2. **Platform-Specific Code**: Always wrap in `#if defined ESP32/#elif defined ESP8266` with appropriate includes

3. **Message Handling**: Use broadcast (`ESPNOW_BROADCAST_ADDRESS`) vs unicast patterns consistently across examples

4. **Performance Testing**: Reference `Throughput.xlsx` data when making performance-related changes

5. **Debug Integration**: Follow conditional QuickDebug inclusion pattern to maintain zero-dependency builds
