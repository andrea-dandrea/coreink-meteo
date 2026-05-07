#include "nvs_config.h"
#include <Preferences.h>

static Preferences prefs;

void nvs_config_init() {
    prefs.begin(NVS_NAMESPACE, false);
    Serial.println("[NVS] Configurazione inizializzata");
}

void nvs_save_string(const char* key, const char* value) {
    prefs.putString(key, value);
    Serial.printf("[NVS] Salvato %s = %s\n", key, value);
}

String nvs_load_string(const char* key, const char* defaultValue) {
    if (prefs.isKey(key)) {
        return prefs.getString(key, defaultValue);
    }
    return String(defaultValue);
}

void nvs_save_int(const char* key, int value) {
    prefs.putInt(key, value);
    Serial.printf("[NVS] Salvato %s = %d\n", key, value);
}

int nvs_load_int(const char* key, int defaultValue) {
    if (prefs.isKey(key)) {
        return prefs.getInt(key, defaultValue);
    }
    return defaultValue;
}

void nvs_config_clear() {
    prefs.clear();
    Serial.println("[NVS] Configurazione cancellata (reset ai default)");
}
