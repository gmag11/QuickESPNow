/**
  * @brief Auxiliary functions for debugging over Serial
  *
  * Format used on debug functions is the same as `printf()`. Check source code for usage examples
  * Debug calls will be enabled or disabled automatically before compiling according defined `DEBUG_LEVEL`.
  *
  * If `DEBUG_ESP_PORT` is not defined library will give no debug output at all
  *
  * @file EnigmaIOTdebug.h
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  */

#ifndef _DEBUG_h
#define _DEBUG_h

#include <Arduino.h>
#ifdef ESP32
#include <esp_log.h>
#endif

#define DEBUG_ESP_PORT Serial

#define NO_DEBUG	0 ///< @brief Debug level that will give no debug output
#define ERROR	1 ///< @brief Debug level that will give error messages
#define WARN	2 ///< @brief Debug level that will give error and warning messages
#define INFO	3 ///< @brief Debug level that will give error, warning and info messages
#define DBG	    4 ///< @brief Debug level that will give error, warning,info AND dbg messages
#define VERBOSE	5 ///< @brief Debug level that will give all defined messages

#ifdef ESP8266
const char* extractFileName (const char* path);
#define DEBUG_LINE_PREFIX(TAG) DEBUG_ESP_PORT.printf_P (PSTR("[%6lu][H:%5lu][%s:%d] %s(): ["),millis(),(unsigned long)ESP.getFreeHeap(),extractFileName(__FILE__),__LINE__,__FUNCTION__); DEBUG_ESP_PORT.printf_P(TAG); DEBUG_ESP_PORT.printf_P("] ");
#endif

#ifdef DEBUG_ESP_PORT

#ifdef ESP8266
#if DEBUG_LEVEL >= VERBOSE
#define DEBUG_VERBOSE(TAG,text,...) DEBUG_ESP_PORT.print("V ");DEBUG_LINE_PREFIX(TAG);DEBUG_ESP_PORT.printf_P(PSTR(text),##__VA_ARGS__);DEBUG_ESP_PORT.println()
#else
#define DEBUG_VERBOSE(...)
#endif

#if DEBUG_LEVEL >= DBG
#define DEBUG_DBG(TAG,text,...) DEBUG_ESP_PORT.print("D ");DEBUG_LINE_PREFIX(TAG); DEBUG_ESP_PORT.printf_P(PSTR(text),##__VA_ARGS__);DEBUG_ESP_PORT.println()
#else
#define DEBUG_DBG(...)
#endif

#if DEBUG_LEVEL >= INFO
#define DEBUG_INFO(TAG,text,...) DEBUG_ESP_PORT.print("I ");DEBUG_LINE_PREFIX(TAG);DEBUG_ESP_PORT.printf_P(PSTR(text),##__VA_ARGS__);DEBUG_ESP_PORT.println()
#else
#define DEBUG_INFO(...)
#endif

#if DEBUG_LEVEL >= WARN
#define DEBUG_WARN(TAG,text,...) DEBUG_ESP_PORT.print("W ");DEBUG_LINE_PREFIX(TAG);DEBUG_ESP_PORT.printf_P(PSTR(text),##__VA_ARGS__);DEBUG_ESP_PORT.println()
#else
#define DEBUG_WARN(...)
#endif

#if DEBUG_LEVEL >= ERROR
#define DEBUG_ERROR(TAG,text,...) DEBUG_ESP_PORT.print("E ");DEBUG_LINE_PREFIX(TAG);DEBUG_ESP_PORT.printf_P(PSTR(text),##__VA_ARGS__);DEBUG_ESP_PORT.println()
#else
#define DEBUG_ERROR(...)
#endif
#elif defined ESP32
#define DEBUG_VERBOSE(TAG,format,...) ESP_LOGV (TAG,"Heap: %6d. " format, ESP.getFreeHeap(), ##__VA_ARGS__)
#define DEBUG_DBG(TAG,format,...) ESP_LOGD (TAG,"Heap: %6d " format, ESP.getFreeHeap(), ##__VA_ARGS__)
#define DEBUG_INFO(TAG,format,...) ESP_LOGI (TAG,"Heap: %6d " format, ESP.getFreeHeap(), ##__VA_ARGS__)
#define DEBUG_WARN(TAG,format,...) ESP_LOGW (TAG,"Heap: %6d " format, ESP.getFreeHeap(), ##__VA_ARGS__)
#define DEBUG_ERROR(TAG,format,...) ESP_LOGE (TAG,"Heap: %6d " format, ESP.getFreeHeap(), ##__VA_ARGS__)
#endif
#else
#define DEBUG_VERBOSE(...)
#define DEBUG_DBG(...)
#define DEBUG_INFO(...)
#define DEBUG_WARN(...)
#define DEBUG_ERROR(...)
#endif

#endif

