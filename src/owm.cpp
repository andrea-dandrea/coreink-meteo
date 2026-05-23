// ============================================================================
// OpenWeatherMap client — CoreInk-Meteo v1.3
// ============================================================================
// Fetch dati meteo da OWM Current Weather API (free tier, 1000 call/giorno).
// Parsing JSON manuale (senza ArduinoJson) per risparmiare flash.
// ============================================================================

#include "owm.h"
#include "config.h"
#include "nvs_config.h"
#include <WiFi.h>
#include <HTTPClient.h>

// --- Stato modulo ---
static OwmData _data;
static OwmForecastSlot _forecast[OWM_FORECAST_SLOTS];
static bool _forecastValid = false;
static unsigned long _lastFetchMs = 0;
static unsigned long _lastSuccessMs = 0;
static unsigned long _intervalMs = OWM_DEFAULT_INTERVAL * 60000UL;
static char _apiKey[48] = "";
static float _lat = 0.0f;
static float _lon = 0.0f;
static bool _configured = false;

// --- Helper: estrai valore numerico da JSON (cerca "key":value) ---
static float jsonFloat(const String& json, const char* key) {
    String search = String("\"") + key + "\":";
    int idx = json.indexOf(search);
    if (idx < 0) return -999.0f;
    idx += search.length();
    // Skip whitespace
    while (idx < (int)json.length() && json[idx] == ' ') idx++;
    int end = idx;
    while (end < (int)json.length() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-')) end++;
    if (end == idx) return -999.0f;
    return json.substring(idx, end).toFloat();
}

// --- Helper: estrai valore intero da JSON ---
static int jsonInt(const String& json, const char* key) {
    float v = jsonFloat(json, key);
    return (v == -999.0f) ? -1 : (int)v;
}

// --- Helper: estrai stringa da JSON (cerca "key":"value") ---
static bool jsonString(const String& json, const char* key, char* out, int maxLen) {
    String search = String("\"") + key + "\":\"";
    int idx = json.indexOf(search);
    if (idx < 0) return false;
    idx += search.length();
    int end = json.indexOf('"', idx);
    if (end < 0) return false;
    int len = min(end - idx, maxLen - 1);
    json.substring(idx, idx + len).toCharArray(out, maxLen);
    return true;
}

// --- Parsing risposta JSON OWM ---
static bool parseResponse(const String& json) {
    // Verifica che sia una risposta valida (contiene "main")
    if (json.indexOf("\"main\"") < 0) return false;

    _data.temp = jsonFloat(json, "temp");
    _data.feels_like = jsonFloat(json, "feels_like");
    _data.humidity = jsonFloat(json, "humidity");
    _data.pressure = jsonFloat(json, "pressure");
    _data.wind_speed = jsonFloat(json, "speed");
    _data.wind_gust = jsonFloat(json, "gust");
    _data.wind_deg = jsonInt(json, "deg");
    _data.weather_id = jsonInt(json, "id");
    _data.uvi = -1.0f;  // Non disponibile in Current Weather API (solo One Call)
    _data.clouds = jsonFloat(json, "all");     // clouds.all
    _data.pop = 0.0f;                          // Non disponibile in current weather
    float r3h = jsonFloat(json, "3h");
    _data.rain_3h = (r3h == -999.0f) ? 0.0f : r3h;

    jsonString(json, "description", _data.description, sizeof(_data.description));
    jsonString(json, "icon", _data.icon, sizeof(_data.icon));

    // Validazione minima
    if (_data.temp < -80.0f || _data.temp > 70.0f) return false;
    if (_data.pressure < 800.0f || _data.pressure > 1100.0f) return false;

    _data.timestamp = (unsigned long)time(nullptr);
    _data.valid = true;
    return true;
}

// --- Parsing forecast JSON ---
// Il JSON contiene "list":[{...},{...},...] con fino a 16 blocchi da 3h.
// Estraiamo: +3h, +6h per oggi; mattina (09:00) e pomeriggio (15:00) per domani.
static void parseForecast(const String& json, int tzOffset) {
    memset(_forecast, 0, sizeof(_forecast));
    _forecastValid = false;

    time_t nowUtc = time(nullptr);
    time_t nowLocal = nowUtc + tzOffset;
    int todayDay = (nowLocal / 86400);   // Giorno corrente (epoch days)
    int tomorrowDay = todayDay + 1;

    int todayCount = 0;   // Slot di oggi trovati
    int tomMornIdx = -1, tomAftIdx = -1;
    int tomMornDist = 999, tomAftDist = 999;

    // Scorre i blocchi nella lista JSON
    int pos = json.indexOf("\"list\"");
    if (pos < 0) return;
    pos = json.indexOf('[', pos);
    if (pos < 0) return;

    // Itera sui blocchi {...}
    int searchFrom = pos;
    for (int i = 0; i < 16; i++) {
        int blockStart = json.indexOf('{', searchFrom + 1);
        if (blockStart < 0) break;
        int blockEnd = blockStart;
        int braces = 1;
        for (int j = blockStart + 1; j < (int)json.length() && braces > 0; j++) {
            if (json[j] == '{') braces++;
            else if (json[j] == '}') braces--;
            if (braces == 0) blockEnd = j;
        }
        searchFrom = blockEnd;  // Avanza oltre il blocco corrente
        String block = json.substring(blockStart, blockEnd + 1);

        long dt = (long)jsonFloat(block, "dt");
        if (dt < 1000000) continue;
        time_t dtLocal = dt + tzOffset;
        int blockDay = (dtLocal / 86400);
        int blockHour = (int)((dtLocal % 86400) / 3600);

        float tmin = jsonFloat(block, "temp_min");
        float tmax = jsonFloat(block, "temp_max");
        char desc[24] = "";
        jsonString(block, "description", desc, sizeof(desc));
        int dayOfMonth = -1;
        {
            struct tm tmInfo;
            gmtime_r(&dtLocal, &tmInfo);
            dayOfMonth = tmInfo.tm_mday;
        }

        // Oggi: primi 3 slot futuri (+3h, +6h, +9h)
        if (blockDay == todayDay && dt > nowUtc && todayCount < 3) {
            OwmForecastSlot& s = _forecast[todayCount];
            s.temp_min = tmin;
            s.temp_max = tmax;
            strncpy(s.description, desc, sizeof(s.description) - 1);
            s.hour = blockHour;
            s.day = dayOfMonth;
            s.valid = true;
            todayCount++;
        }

        // Domani mattina: slot più vicino alle 9:00
        if (blockDay == tomorrowDay) {
            int distM = abs(blockHour - 9);
            if (distM < tomMornDist) {
                tomMornDist = distM;
                tomMornIdx = i;
                OwmForecastSlot& s = _forecast[3];
                s.temp_min = tmin;
                s.temp_max = tmax;
                strncpy(s.description, desc, sizeof(s.description) - 1);
                s.hour = blockHour;
                s.day = dayOfMonth;
                s.valid = true;
            }
            // Domani pomeriggio: slot più vicino alle 15:00
            int distA = abs(blockHour - 15);
            if (distA < tomAftDist) {
                tomAftDist = distA;
                tomAftIdx = i;
                OwmForecastSlot& s = _forecast[4];
                s.temp_min = tmin;
                s.temp_max = tmax;
                strncpy(s.description, desc, sizeof(s.description) - 1);
                s.hour = blockHour;
                s.day = dayOfMonth;
                s.valid = true;
            }
        }
    }

    _forecastValid = (_forecast[0].valid || _forecast[3].valid);
    Serial.printf("[OWM] Forecast: oggi=%d slot, dom=%s/%s\n",
        todayCount, _forecast[3].valid ? "matt" : "-", _forecast[4].valid ? "pom" : "-");
}

// ============================================================================
// API pubblica
// ============================================================================

void owm_init() {
    memset(&_data, 0, sizeof(_data));
    _data.uvi = -1.0f;

    // Carica API key da NVS
    String key = nvs_load_string(NVS_KEY_OWM_KEY, "");
    if (key.length() > 0 && key.length() < sizeof(_apiKey)) {
        strncpy(_apiKey, key.c_str(), sizeof(_apiKey) - 1);
        _configured = true;
    }

    // Intervallo da NVS (minuti → ms)
    int intv = nvs_load_int(NVS_KEY_OWM_INTERVAL, OWM_DEFAULT_INTERVAL);
    if (intv < 5) intv = 5;       // Minimo 5 min (protezione rate limit)
    if (intv > 180) intv = 180;   // Massimo 3h
    _intervalMs = (unsigned long)intv * 60000UL;
    _lastFetchMs = 0;  // Forza fetch immediata al prossimo owm_update()

    Serial.printf("[OWM] Init: configured=%d, interval=%d min\n", _configured, intv);
}

bool owm_update(unsigned long now_ms) {
    if (!_configured) return false;
    if (WiFi.status() != WL_CONNECTED) return false;
    // Prima fetch immediata (_lastFetchMs == 0), poi rispetta intervallo
    if (_lastFetchMs != 0 && (now_ms - _lastFetchMs < _intervalMs)) return false;

    _lastFetchMs = now_ms;
    return owm_fetch_now();
}

bool owm_fetch_now() {
    if (!_configured) return false;
    if (WiFi.status() != WL_CONNECTED) return false;

    extern float stationLat, stationLon;

    // --- Current Weather ---
    char url[256];
    snprintf(url, sizeof(url),
        "http://api.openweathermap.org/data/2.5/weather?lat=%.4f&lon=%.4f&appid=%s&units=metric&lang=it",
        stationLat, stationLon, _apiKey);

    Serial.printf("[OWM] Fetch: %.4f,%.4f\n", stationLat, stationLon);

    HTTPClient http;
    http.setTimeout(10000);
    http.begin(url);
    int httpCode = http.GET();

    bool ok = false;
    int tzOffset = 0;
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        ok = parseResponse(payload);
        if (ok) {
            _lastSuccessMs = millis();
            tzOffset = jsonInt(payload, "timezone");
            Serial.printf("[OWM] OK: %.1fC, %s, vento %.1fm/s\n",
                _data.temp, _data.description, _data.wind_speed);
        } else {
            Serial.println("[OWM] Errore parsing JSON");
        }
    } else {
        Serial.printf("[OWM] HTTP error: %d\n", httpCode);
    }
    http.end();

    // --- Forecast 5-day/3h ---
    if (ok) {
        snprintf(url, sizeof(url),
            "http://api.openweathermap.org/data/2.5/forecast?lat=%.4f&lon=%.4f&appid=%s&units=metric&lang=it&cnt=16",
            stationLat, stationLon, _apiKey);

        http.begin(url);
        httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String fc = http.getString();
            parseForecast(fc, tzOffset);
        } else {
            Serial.printf("[OWM] Forecast HTTP error: %d\n", httpCode);
        }
        http.end();
    }

    return ok;
}

const OwmData& owm_get_data() {
    return _data;
}

unsigned long owm_last_success() {
    return _lastSuccessMs;
}

bool owm_is_configured() {
    return _configured;
}

const OwmForecastSlot* owm_get_forecast() {
    return _forecast;
}

bool owm_forecast_valid() {
    return _forecastValid;
}
