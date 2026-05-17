#include "data_logger.h"
#include "config.h"
#include "nvs_config.h"
#include <LittleFS.h>
#if OTA_ENABLED
#include <WebServer.h>
#endif

#define LOG_FILE "/datalog.bin"
#define LOG_HEADER_SIZE sizeof(LogHeader)
#define LOG_RECORD_SIZE_ACTUAL sizeof(LogRecord)

static bool _initialized = false;
static bool _enabled = false;
static uint16_t _interval = LOG_DEFAULT_INTERVAL;
static uint32_t _maxRecords = 0;

bool logger_init() {
    if (!LittleFS.begin(true)) {
        Serial.println("[LOG] LittleFS mount fallito!");
        return false;
    }

    _enabled = nvs_load_int(NVS_KEY_LOG_ENABLED, 0) != 0;
    _interval = (uint16_t)nvs_load_int(NVS_KEY_LOG_INTERVAL, LOG_DEFAULT_INTERVAL);

    // Calcolare capacità massima
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    // Lasciare 4KB di margine per filesystem
    _maxRecords = (totalBytes - 4096 - LOG_HEADER_SIZE) / LOG_RECORD_SIZE_ACTUAL;

    // Creare file se non esiste
    if (!LittleFS.exists(LOG_FILE)) {
        File f = LittleFS.open(LOG_FILE, "w");
        if (f) {
            LogHeader hdr;
            memcpy(hdr.magic, LOG_MAGIC, 4);
            hdr.version = 1;
            hdr.fieldsMask = 0xFFFF;
            hdr.intervalSec = _interval;
            hdr.qualType = 0;
            memset(hdr.reserved, 0, sizeof(hdr.reserved));
            f.write((uint8_t*)&hdr, sizeof(hdr));
            f.close();
            Serial.println("[LOG] File creato");
        }
    }

    _initialized = true;
    Serial.printf("[LOG] Init OK, capacita: %u record, intervallo: %us\n",
                  _maxRecords, _interval);
    return true;
}

void logger_set_enabled(bool enabled) {
    _enabled = enabled;
    nvs_save_int(NVS_KEY_LOG_ENABLED, enabled ? 1 : 0);
}

bool logger_is_enabled() { return _enabled; }

void logger_set_interval(uint16_t seconds) {
    _interval = seconds;
    nvs_save_int(NVS_KEY_LOG_INTERVAL, seconds);
}

uint16_t logger_get_interval() { return _interval; }

void logger_write_record(const LogRecord& record) {
    if (!_initialized || !_enabled) return;

    File f = LittleFS.open(LOG_FILE, "a");
    if (!f) return;

    // Se superata la capacità massima, ring buffer: troncare dal fondo
    size_t fileSize = f.size();
    uint32_t currentRecords = (fileSize - LOG_HEADER_SIZE) / LOG_RECORD_SIZE_ACTUAL;

    if (currentRecords >= _maxRecords) {
        f.close();
        // Rimuovere il primo record (shift)
        File src = LittleFS.open(LOG_FILE, "r");
        File dst = LittleFS.open("/datalog.tmp", "w");
        if (src && dst) {
            // Copiare header
            uint8_t buf[LOG_HEADER_SIZE];
            src.read(buf, LOG_HEADER_SIZE);
            dst.write(buf, LOG_HEADER_SIZE);
            // Saltare primo record
            src.seek(LOG_HEADER_SIZE + LOG_RECORD_SIZE_ACTUAL);
            // Copiare il resto
            uint8_t rbuf[LOG_RECORD_SIZE_ACTUAL];
            while (src.read(rbuf, LOG_RECORD_SIZE_ACTUAL) == LOG_RECORD_SIZE_ACTUAL) {
                dst.write(rbuf, LOG_RECORD_SIZE_ACTUAL);
            }
            src.close();
            dst.close();
            LittleFS.remove(LOG_FILE);
            LittleFS.rename("/datalog.tmp", LOG_FILE);
        }
        f = LittleFS.open(LOG_FILE, "a");
        if (!f) return;
    }

    f.write((const uint8_t*)&record, LOG_RECORD_SIZE_ACTUAL);
    f.close();
}

uint32_t logger_get_count() {
    if (!_initialized) return 0;
    File f = LittleFS.open(LOG_FILE, "r");
    if (!f) return 0;
    size_t sz = f.size();
    f.close();
    if (sz <= LOG_HEADER_SIZE) return 0;
    return (sz - LOG_HEADER_SIZE) / LOG_RECORD_SIZE_ACTUAL;
}

uint32_t logger_get_capacity() { return _maxRecords; }

float logger_get_usage_percent() {
    if (_maxRecords == 0) return 0;
    return (float)logger_get_count() / _maxRecords * 100.0f;
}

void logger_reset() {
    if (!_initialized) return;
    LittleFS.remove(LOG_FILE);
    // Ricreare con header
    File f = LittleFS.open(LOG_FILE, "w");
    if (f) {
        LogHeader hdr;
        memcpy(hdr.magic, LOG_MAGIC, 4);
        hdr.version = 1;
        hdr.fieldsMask = 0xFFFF;
        hdr.intervalSec = _interval;
        hdr.qualType = 0;
        memset(hdr.reserved, 0, sizeof(hdr.reserved));
        f.write((uint8_t*)&hdr, sizeof(hdr));
        f.close();
    }
    Serial.println("[LOG] Reset completato");
}

String logger_get_csv_header() {
    return "timestamp,temp_c,humidity,pressure_hpa,battery_v,lat,lon,alt_m,speed_kmh,course,quality,flags";
}

String logger_record_to_csv(const LogRecord& record) {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "%u,%.1f,%.1f,%.1f,%.3f,%.6f,%.6f,%d,%.1f,%.1f,%u,%u",
             record.timestamp,
             record.temperature / 10.0f,
             record.humidity / 10.0f,
             (record.pressure / 10.0f) + 900.0f,
             record.battery / 1000.0f,
             record.latitude / 1e6f,
             record.longitude / 1e6f,
             record.altitude,
             record.speed / 10.0f,
             record.course / 10.0f,
             record.quality,
             record.flags);
    return String(buf);
}

void logger_serve_csv(void* serverPtr) {
    if (!_initialized) return;

    WebServer* server = (WebServer*)serverPtr;
    File f = LittleFS.open(LOG_FILE, "r");
    if (!f || f.size() <= LOG_HEADER_SIZE) {
        server->send(200, "text/csv", logger_get_csv_header() + "\n");
        if (f) f.close();
        return;
    }

    // Saltare header
    f.seek(LOG_HEADER_SIZE);

    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send(200, "text/csv", "");
    server->sendContent(logger_get_csv_header() + "\n");

    LogRecord rec;
    while (f.read((uint8_t*)&rec, sizeof(rec)) == sizeof(rec)) {
        server->sendContent(logger_record_to_csv(rec) + "\n");
    }
    f.close();
}
