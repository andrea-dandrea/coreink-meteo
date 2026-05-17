#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>

// Struttura record dati (30 bytes)
#pragma pack(push, 1)
struct LogRecord {
    uint32_t timestamp;     // Unix time
    int16_t  temperature;   // °C × 10
    uint16_t humidity;      // % × 10
    uint16_t pressure;      // (hPa - 900) × 10
    uint16_t battery;       // mV
    int32_t  latitude;      // gradi × 1e6
    int32_t  longitude;     // gradi × 1e6
    int16_t  altitude;      // metri
    uint16_t speed;         // km/h × 10
    uint16_t course;        // gradi × 10
    uint8_t  quality;       // sat count o HDOP×10
    uint8_t  flags;         // bit: [0]=gps_fix, [1]=wifi, [2]=charging, [3]=tx_ok
};
#pragma pack(pop)

// Header file di log
#pragma pack(push, 1)
struct LogHeader {
    char     magic[4];      // "CMRK"
    uint8_t  version;       // Versione formato
    uint16_t fieldsMask;    // Bitmask campi presenti
    uint16_t intervalSec;   // Intervallo campionamento
    uint8_t  qualType;      // 0=satelliti, 1=HDOP
    uint8_t  reserved[6];   // Padding per allineamento (16 bytes totale header)
};
#pragma pack(pop)

bool logger_init();
void logger_set_enabled(bool enabled);
bool logger_is_enabled();
void logger_set_interval(uint16_t seconds);
uint16_t logger_get_interval();
void logger_write_record(const LogRecord& record);
uint32_t logger_get_count();
uint32_t logger_get_capacity();
float logger_get_usage_percent();
void logger_reset();
String logger_get_csv_header();
String logger_record_to_csv(const LogRecord& record);
void logger_serve_csv(void* server);  // Serve CSV via WebServer

#endif
