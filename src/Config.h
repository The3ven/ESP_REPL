#ifndef CONFIG_H
#define CONFIG_H

#define MAX_LINES 200
#define MAX_VARS 50
#define MAX_STACK 8
#define MAX_FUNCS 15
#define MAX_TASKS 4
#define MAX_TOKENS 20
#define MAX_BUFFER_SIZE 512
#define MAX_SOURCE_SIZE 4096
#define MAX_VAR_NAME 16
#define MAX_ALIASES 10
#define MAX_ALIAS_NAME 8
#define MAX_ALIAS_CMD 32

/* ================= STORAGE ================= */
#define ENABLE_SD 0 // Set to 1 for SD Card, 0 for LittleFS
#define SD_CS_PIN 5

#if ENABLE_SD
#include <SD.h>
#include <SPI.h>
#define FILESYSTEM SD
#else
#include <LittleFS.h>
#define FILESYSTEM LittleFS
#endif

/* ================= TELNET ================= */
#define ENABLE_TELNET 1
#define TELNET_PORT 23

/* ================= LOGGING ================= */
#define LOG_ENABLE 1
#if LOG_ENABLE
#include <Arduino.h>
#define LOGI(msg)                                                              \
  {                                                                            \
    Serial.print("[INF] ");                                                    \
    Serial.println(msg);                                                       \
  }
#define LOGE(msg)                                                              \
  {                                                                            \
    Serial.print("[ERR] ");                                                    \
    Serial.println(msg);                                                       \
  }
#else
#define LOGI(msg)
#define LOGE(msg)
#endif

#endif
