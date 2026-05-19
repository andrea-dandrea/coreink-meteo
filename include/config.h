#ifndef CONFIG_H
#define CONFIG_H

// === Versione firmware ===
#ifndef FW_VERSION
#define FW_VERSION "1.2.7"
#endif

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
// Timezone POSIX: gestisce automaticamente ora legale/solare
// Italia: CET-1CEST,M3.5.0,M10.5.0/3 (UTC+1 inverno, UTC+2 estate)
// UK: GMT0BST,M3.5.0/1,M10.5.0  |  UTC puro: UTC0
struct StationProfile {
    const char* callsign;
    int ssid;
    const char* passcode;
    const char* label;            // Etichetta mostrata sul display
    char symbolTable;             // Tabella simbolo: '/' o '\\'
    char symbolCode;              // Codice simbolo (es. '_' per WX)
    const char* timezone;         // Timezone POSIX per ora locale display
};

#define NUM_PROFILES 3

const StationProfile profiles[NUM_PROFILES] = {
    { "NOCALL",  13, "-1", "Meteo casa", '/', '_', "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "NOCALL",  9,  "-1", "Mobile",     '/', '_', "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "NOCALL",  13, "-1", "Remota",    '\\', '_', "CET-1CEST,M3.5.0,M10.5.0/3" },
};

// === Posizione della stazione (locatore Maidenhead) ===
#define STATION_LOCATOR "JN61fw"  // Locatore Maidenhead (4 o 6 caratteri)

// === Modalità PORT A (GPIO 32/33) ===
// Il CoreInk ha un solo PORT esterno. Va configurato in base al modulo collegato:
//   0 = ENV III (I2C): SDA=32, SCL=33 → sensori SHT30 + QMP6988
//   1 = GPS (UART):    RX=33, TX=32   → modulo AT6558
// Selezionabile anche al boot tramite menu, salvato in NVS.
#define PORT_MODE_ENV  0
#define PORT_MODE_GPS  1
#define PORT_MODE_DEFAULT PORT_MODE_ENV  // Default al primo avvio

// === GPS ===
// Attivo solo se PORT_MODE = GPS (selezionato al boot o da NVS)
#define GPS_RX_PIN 33             // Pin RX ESP32 ← TX GPS (Grove bianco)
#define GPS_TX_PIN 32             // Pin TX ESP32 → RX GPS (Grove giallo)
#define GPS_BAUD 9600             // Baud rate del modulo GPS

// === SmartBeaconing (attivo solo con GPS abilitato) ===
// Algoritmo che adatta l'intervallo di beacon alla velocità e ai cambi di rotta.
#define SMARTBEACON_ENABLED 1     // 1 = abilitato, 0 = intervallo fisso
#define SB_FAST_SPEED 100         // Velocità alta (km/h) -> intervallo minimo
#define SB_FAST_RATE 120          // Intervallo minimo (secondi) ad alta velocità
#define SB_SLOW_SPEED 5           // Velocità bassa (km/h) -> intervallo massimo
#define SB_SLOW_RATE 1800         // Intervallo massimo (secondi) a bassa velocità
#define SB_TURN_MIN_ANGLE 30      // Angolo minimo (gradi) per trigger svolta
#define SB_TURN_SLOPE 240         // Slope: angolo/velocità per trigger
#define SB_TURN_TIME 15           // Tempo minimo (sec) tra beacon per svolta
#define SB_MIN_DIST_M 100         // Distanza minima (m) tra beacon (anti-jitter GPS)

// === OTA (Over-The-Air update) ===
// Abilita aggiornamento firmware via WiFi (ArduinoOTA + Web upload).
// Una volta flashato con OTA attivo, non serve più la porta USB.
#define OTA_ENABLED 1             // 1 = OTA abilitato, 0 = disabilitato
#define OTA_HOSTNAME "coreink-meteo"
#define OTA_PASSWORD ""           // Lasciare vuoto per nessuna password
#define OTA_WEB_PORT 8080         // Porta per la pagina web di upload firmware

// === BLE OTA (firmware via Bluetooth) ===
// Usa un servizio GATT per ricevere il firmware in chunk da telefono (nRF Connect)
// o app custom. Funziona anche senza WiFi.
#ifndef BLE_OTA_ENABLED
#define BLE_OTA_ENABLED 1
#endif         // 1 = BLE OTA abilitato, 0 = disabilitato

// === Bluetooth (BLE) ===
// WiFi e BLE funzionano in parallelo sull'ESP32 (time-division multiplexing).
#ifndef BLE_ENABLED
#define BLE_ENABLED 0
#endif             // 1 = BLE abilitato, 0 = disabilitato
#define BLE_DEVICE_NAME "CoreInkMeteo"

// === Batteria ===
#define BAT_ADC_PIN 35            // Pin ADC per lettura tensione batteria
#define BAT_USB_THRESHOLD_V 4.4f  // Vbat > soglia => USB collegata (backdrive SY7088)
#define BAT_LOW_THRESHOLD_V 3.5f  // Vbat < soglia => batteria bassa: LED_FAST + buzzer
#define BAT_CRITICAL_THRESHOLD_V 3.2f // Vbat < soglia => batteria critica: shutdown
// Il CoreInk usa un partitore 25.1/5.1 — la conversione è gestita nel codice
// con esp_adc_cal per maggiore precisione.

// === WiFiManager ===
// Tenere premuto il pulsante centrale durante il boot per forzare il portale captive.
#define WIFIMANAGER_AP_NAME "CoreInkMeteo-Setup"
#define WIFIMANAGER_TIMEOUT 600   // Timeout portale captive (secondi) — 10 minuti

// === Telemetria APRS ===
// PARM/UNIT/EQNS inviati UNA SOLA VOLTA al boot (non a intervallo).
// Canali: 1=Vbat(mV), 2=RSSI(dBm+100), 3=Uptime(min), 4=SatGPS, 5=libero
#define TELEMETRY_DEFINITION_INTERVAL_MS 7200000  // Intervallo backup (ogni 2 ore)

// === Temporizzazione ===
#define WEATHER_INTERVAL_MS 300000   // Meteo ogni 5 minuti (default)
#define TELEMETRY_INTERVAL_MS 600000 // Telemetria ogni 10 minuti
#define POSITION_INTERVAL_MS 0       // 0 = solo al boot se posizione fissa (no GPS)
#define WIFI_TIMEOUT_MS 8000         // Timeout connessione WiFi
#define WIFI_MENU_TIMEOUT_S 60       // Timeout schermata selezione WiFi al boot (S2)

// === Display ===
#define DISPLAY_UPDATE_MS 60000   // Aggiornare il display ogni minuto
#define NUM_PAGES 9               // Numero totale di pagine display

// === Batteria ===
#define BAT_ADC_SAMPLES 5         // Media mobile su N campioni

// === Buzzer ===
#define BUZZER_PIN 2              // GPIO buzzer passivo
#define BUZZER_DEFAULT_VOLUME 50  // Duty cycle PWM default (0-100)

// === APRS Status ===
#define APRS_STATUS_DEFAULT "CoreInk-Meteo v" FW_VERSION " by EA5JDG/IZ3ARR"
#define APRS_STATUS_INTERVAL_MS 3600000  // Status ogni 60 minuti (default)

// === Data Logger ===
#define LOG_RECORD_SIZE 30        // Bytes per record
#define LOG_DEFAULT_INTERVAL 600  // Secondi (10 min)
#define LOG_MAGIC "CMRK"          // Magic header file

// === OpenWeatherMap ===
#define OWM_DEFAULT_INTERVAL 30   // Minuti tra aggiornamenti

// === NVS Keys aggiuntive v1.3 ===
#define NVS_KEY_PORT_MODE "port_mode"
#define NVS_KEY_SYMBOL "aprs_sym"
#define NVS_KEY_WIFI_ENABLED "wifi_en"
#define NVS_KEY_WIFI_ON_BOOT "wifi_boot"
#define NVS_KEY_BT_ENABLED "bt_en"
#define NVS_KEY_BT_ON_BOOT "bt_boot"
#define NVS_KEY_BUZZER_ENABLED "buzz_en"
#define NVS_KEY_BUZZER_VOLUME "buzz_vol"
#define NVS_KEY_BOOT_MELODY "boot_mel"
#define NVS_KEY_LED_ENABLED "led_en"
#define NVS_KEY_LOG_ENABLED "log_en"
#define NVS_KEY_LOG_INTERVAL "log_intv"
#define NVS_KEY_LOG_QUAL_TYPE "log_qual"
#define NVS_KEY_BAT_SAMPLES "bat_samp"
#define NVS_KEY_APRS_STATUS "aprs_stat"
#define NVS_KEY_STATUS_INTERVAL "stat_intv"
#define NVS_KEY_DISPLAY_REFRESH "disp_ref"
#define NVS_KEY_WEATHER_INTERVAL "wx_intv"
#define NVS_KEY_OWM_KEY "owm_key"
#define NVS_KEY_OWM_INTERVAL "owm_intv"
#define NVS_KEY_TIDE_KEY "tide_key"
#define NVS_KEY_TIDE_PORT "tide_port"

#endif
