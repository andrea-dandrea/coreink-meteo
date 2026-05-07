#ifndef CONFIG_H
#define CONFIG_H

// === Reti WiFi memorizzate (si connette alla prima disponibile) ===
struct WiFiAP {
    const char* ssid;
    const char* password;
};

#define NUM_WIFI_APS 3

const WiFiAP wifiNetworks[NUM_WIFI_APS] = {
    { "RETE_1", "PASSWORD_1" },
    { "RETE_2", "PASSWORD_2" },
    { "RETE_3", "PASSWORD_3" },
};

// === Configurazione APRS-IS ===
#define APRS_SERVER "rotate.aprs2.net"
#define APRS_PORT 14580

// === Simboli APRS per aprs.fi ===
// Il simbolo APRS è composto da tabella (char) + codice (char).
// Tabella '/' = primaria, '\\' = alternativa
// Simboli meteo comuni:
//   '/' + '_' = Stazione meteo WX (icona blu standard)
//   '\\' + '_' = Stazione meteo WX (icona verde)
//   '/' + 'W' = Sito NWS
//   '\\' + 'W' = Stazione meteo con cielo (WX supersite)
//   '/' + '-' = Casa / QTH
//   '/' + 'y' = Antenna radio

// === Profili stazione (selezionabili al boot con i pulsanti) ===
// Ogni profilo ha: nominativo, SSID APRS, passcode, etichetta, simbolo APRS
struct StationProfile {
    const char* callsign;
    int ssid;
    const char* passcode;
    const char* label;            // Etichetta mostrata sul display
    char symbolTable;             // Tabella simbolo: '/' o '\\'
    char symbolCode;              // Codice simbolo (es. '_' per WX)
};

#define NUM_PROFILES 3

const StationProfile profiles[NUM_PROFILES] = {
    { "NOCALL",  13, "-1", "Meteo casa", '/', '_' },   // WX standard
    { "NOCALL",  9,  "-1", "Mobile",     '/', '_' },   // WX standard
    { "NOCALL",  13, "-1", "Remota",    '\\', '_' },   // WX verde
};

// === Posizione della stazione (locatore Maidenhead) ===
#define STATION_LOCATOR "JN61fw"  // Locatore Maidenhead (4 o 6 caratteri)

// === GPS (opzionale) ===
// Se abilitato e con fix valido, le coordinate GPS sovrascrivono il locatore.
#define GPS_ENABLED 0             // 1 = GPS abilitato, 0 = disabilitato
#define GPS_RX_PIN 26             // Pin RX della seriale GPS
#define GPS_TX_PIN 0              // Pin TX della seriale GPS
#define GPS_BAUD 9600             // Baud rate del modulo GPS

// === SmartBeaconing (attivo solo con GPS abilitato) ===
// Algoritmo che adatta l'intervallo di beacon alla velocità e ai cambi di rotta.
#define SMARTBEACON_ENABLED 1     // 1 = abilitato, 0 = intervallo fisso
#define SB_FAST_SPEED 100         // Velocità alta (km/h) -> intervallo minimo
#define SB_FAST_RATE 60           // Intervallo minimo (secondi) ad alta velocità
#define SB_SLOW_SPEED 5           // Velocità bassa (km/h) -> intervallo massimo
#define SB_SLOW_RATE 1800         // Intervallo massimo (secondi) a bassa velocità
#define SB_TURN_MIN_ANGLE 30      // Angolo minimo (gradi) per trigger svolta
#define SB_TURN_SLOPE 240         // Slope: angolo/velocità per trigger
#define SB_TURN_TIME 15           // Tempo minimo (sec) tra beacon per svolta

// === Bluetooth (BLE) ===
// WiFi e BLE funzionano in parallelo sull'ESP32 (time-division multiplexing).
#define BLE_ENABLED 0             // 1 = BLE abilitato, 0 = disabilitato
#define BLE_DEVICE_NAME "CoreInkMeteo"

// === Batteria ===
#define BAT_ADC_PIN 35            // Pin ADC per lettura tensione batteria
// Il CoreInk usa un partitore 25.1/5.1 — la conversione è gestita nel codice
// con esp_adc_cal per maggiore precisione.

// === WiFiManager ===
// Tenere premuto il pulsante centrale durante il boot per forzare il portale captive.
#define WIFIMANAGER_AP_NAME "CoreInkMeteo-Setup"
#define WIFIMANAGER_TIMEOUT 180   // Timeout portale captive (secondi)

// === Telemetria APRS ===
// Intervallo di invio pacchetti PARM/UNIT/EQNS (definizione canali)
#define TELEMETRY_DEFINITION_INTERVAL_MS 7200000  // Ogni 2 ore
// Canali: 1=Vbat(mV), 2=RSSI(dBm+100), 3=Uptime(min), 4=SatGPS, 5=libero

// === Temporizzazione ===
#define SEND_INTERVAL_MS 600000   // Intervallo di invio: 10 minuti (usato senza GPS/SmartBeacon)
#define WIFI_TIMEOUT_MS 15000     // Timeout connessione WiFi

// === Display ===
#define DISPLAY_UPDATE_MS 60000   // Aggiornare il display ogni minuto

#endif
