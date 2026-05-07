#ifndef NVS_CONFIG_H
#define NVS_CONFIG_H

#include <Arduino.h>

/**
 * Gestione configurazione persistente in NVS (Non-Volatile Storage).
 * I valori salvati in NVS sovrascrivono i default definiti in config.h.
 */

// Chiavi NVS
#define NVS_NAMESPACE "meteo_cfg"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASS "wifi_pass"
#define NVS_KEY_CALLSIGN "callsign"
#define NVS_KEY_PASSCODE "passcode"
#define NVS_KEY_SSID_APRS "ssid_aprs"
#define NVS_KEY_LOCATOR "locator"
#define NVS_KEY_SYMBOL_TABLE "sym_table"
#define NVS_KEY_SYMBOL_CODE "sym_code"
#define NVS_KEY_PROFILE "profile"

/**
 * Inizializza il namespace NVS. Chiamare una volta nel setup.
 */
void nvs_config_init();

/**
 * Salva una stringa in NVS.
 */
void nvs_save_string(const char* key, const char* value);

/**
 * Legge una stringa da NVS. Restituisce il default se non trovata.
 */
String nvs_load_string(const char* key, const char* defaultValue);

/**
 * Salva un intero in NVS.
 */
void nvs_save_int(const char* key, int value);

/**
 * Legge un intero da NVS. Restituisce il default se non trovato.
 */
int nvs_load_int(const char* key, int defaultValue);

/**
 * Cancella tutte le impostazioni salvate (reset ai default).
 */
void nvs_config_clear();

#endif
