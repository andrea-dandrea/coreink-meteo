// ============================================================================
// CoreInk-Meteo v1.3 — Stazione meteo APRS completa
// ============================================================================
// Hardware: M5Stack CoreInk (ESP32-PICO-D4) + ENV III (SHT30+QMP6988) / GPS AT6558
// Autori: EA5JDG / IZ3ARR
// ============================================================================

// === Selezione hardware in base alla board ===
#if defined(BOARD_STICKCPLUS2)
  #include <M5StickCPlus2.h>
#elif defined(BOARD_COREINK)
  #if defined(USE_M5UNIFIED)
    #include "m5unified_compat.h"
  #else
    #include <M5CoreInk.h>
  #endif
#else
  #error "Definire BOARD_COREINK o BOARD_STICKCPLUS2 nei build_flags"
#endif

#include <Wire.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>
#include <TinyGPSPlus.h>
#include <esp_task_wdt.h>

#if defined(BOARD_COREINK)
  #include <esp_adc_cal.h>
  #include <soc/rtc_cntl_reg.h>
#endif

#include "config.h"
#include "M5UnitENV.h"
#include "aprs.h"
#include "aprs_is.h"
#include "aprs_telemetry.h"
#include "nvs_config.h"
#include "smartbeacon.h"
#include "buzzer.h"
#include "led_status.h"
#ifndef DATA_LOGGER_ENABLED
#define DATA_LOGGER_ENABLED 1
#endif
#ifndef GPS_EXTRA_ENABLED
#define GPS_EXTRA_ENABLED 1
#endif
#if DATA_LOGGER_ENABLED
#include "data_logger.h"
#endif
#if GPS_EXTRA_ENABLED
#include "gps_extra.h"
#endif
#include "astro.h"
#include "owm.h"

#if OTA_ENABLED
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Update.h>
#endif

#if BLE_OTA_ENABLED
#include "ble_ota.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#endif

#if BLE_ENABLED && !BLE_OTA_ENABLED
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#endif

// === Hardware ===
#if defined(BOARD_COREINK)
  #define LED_PIN 10
#elif defined(BOARD_STICKCPLUS2)
  #define LED_PIN -1
#endif

// === Display ===
#if defined(BOARD_COREINK)
  Ink_Sprite mainSprite(&M5.M5Ink);
#endif

// === OTA Web Server ===
#if OTA_ENABLED
WebServer otaServer(OTA_WEB_PORT);
#endif



// === Sensori ENV III (SHT30 + QMP6988) ===
SHT3X sht3x;
QMP6988 qmp;

// === Profilo attivo ===
int activeProfile = 0;

// === Modalita PORT A (runtime, cambiabile con tasto EXT) ===
int portMode = PORT_MODE_DEFAULT;

// === GPS ===
TinyGPSPlus gps;
#if GPS_EXTRA_ENABLED
GpsExtra gpsExtra;
#endif
HardwareSerial gpsSerial(1);
bool gpsFixValid = false;
float gpsLat = 0.0;
float gpsLon = 0.0;
float lastGpsLat = 0.0f;   // Ultima posizione GPS valida (persiste dopo commuta a ENV)
float lastGpsLon = 0.0f;
bool  lastGpsPosValid = false;
float gpsAlt = 0.0;
float gpsSpeed = 0.0;
float gpsCourse = 0.0;
int gpsSatellites = 0;

#if SMARTBEACON_ENABLED
SmartBeacon smartBeacon(SB_FAST_SPEED, SB_FAST_RATE,
                        SB_SLOW_SPEED, SB_SLOW_RATE,
                        SB_TURN_MIN_ANGLE, SB_TURN_SLOPE, SB_TURN_TIME);
#endif

// === Variabili globali ===
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;
float batVoltage = 0.0;
float stationLat = 0.0f;   // Coordinate stazione (da locator o GPS) — usate da OWM
float stationLon = 0.0f;
float batSamples[BAT_ADC_SAMPLES];
int batSampleIdx = 0;
bool batSamplesReady = false;

unsigned long lastWeatherTime = 0;
unsigned long lastPositionTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastTelemetryTime = 0;
unsigned long lastTelemetryDefTime = 0;
unsigned long lastStatusTime = 0;
unsigned long rtDisplayUpdateMs = DISPLAY_UPDATE_MS;  // Configurabile via WiFiManager
unsigned long rtWeatherIntervalMs = WEATHER_INTERVAL_MS;  // Configurabile
unsigned long rtStatusIntervalMs = APRS_STATUS_INTERVAL_MS;  // Configurabile
int einkRefreshCounter = 0;  // Contatore per full refresh anti-ghosting
unsigned long lastLogTime = 0;
unsigned long uptimeStart = 0;
int telemetrySeq = 0;

bool lastTxOk = false;
char lastTxTime[6] = "--:--";

// === Telemetria: definizioni inviate solo al boot (non a intervallo) ===
bool telemetryDefSent = false;

// === Charge detection ===
enum ChargeState { CHG_UNKNOWN, CHG_CHARGING, CHG_DISCHARGING, CHG_IDLE };
ChargeState chargeState = CHG_UNKNOWN;

// === WiFi FSM ===
enum WifiState {
    WIFI_ST_OFF,
    WIFI_ST_CONNECTING,
    WIFI_ST_CONNECTED,
    WIFI_ST_DISCONNECTED,
    WIFI_ST_WAITING,
    WIFI_ST_AP_CONFIG
};
WifiState wifiState   = WIFI_ST_OFF;
WifiState wifiStatePrev = WIFI_ST_OFF;
unsigned long wifiRetryTime    = 0;
int           wifiRetryBackoff = 15000;
int           wifiApIdx        = 0;   // Indice AP corrente nel ciclo round-robin (0-2)

// === SmartBeacon: posizione dell'ultimo beacon (filtro delta) ===
float sbLastTxLat  = 0.0f;
float sbLastTxLon  = 0.0f;
bool  sbLastTxValid = false;

// === Display multi-pagina ===
int currentPage = 0;
bool coordShowMaidenhead = false;

// === Parametri runtime (da NVS, fallback a profilo/config.h) ===
char rtCallsign[11] = "";
char rtPasscode[7] = "";
char rtLocator[11] = "";
char rtSymbolTable = '/';
char rtSymbolCode = '_';
char rtAprsStatus[64] = "";
int rtSsidAprs = 13;

// === WiFi/BT stato runtime ===
bool wifiEnabled = true;
bool btEnabled = false;

// === Prototipi ===
void selectProfile();
void connectWiFi();
void startWiFiManager();
void readSensors();
float readBatteryRaw();
float readBattery();
void updateDisplay();
void drawProfileMenu();
void drawPage(int page);
void sendWeatherPacket();
void sendPositionPacket();
void sendTelemetry();
void sendTelemetryDefinitions();
void sendStatusPacket();
void syncTime();
void readGps();
void switchPortMode(int newMode);
void drawPortModeNotice();
void writeLogRecord();
void showPage6Menu();
String latlon_to_maidenhead(float lat, float lon);
bool isOnUsb();
ChargeState detectChargeState();
void wifi_update();
void showWifiMenu();
float haversineM(float lat1, float lon1, float lat2, float lon2);
#if OTA_ENABLED
void setupOTA();
#endif

// ============================================================================
// SETUP
// ============================================================================
void setup() {
#if defined(BOARD_COREINK)
    // CRITICO: mantenere alimentazione attiva PRIMA di tutto
    pinMode(12, OUTPUT);
    digitalWrite(12, HIGH);

    // Disabilitare brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    M5.begin();
    Serial.begin(115200);
#if defined(USE_M5UNIFIED)
    m5unified_setup_eink();  // E-ink fast/partial refresh mode
#endif
#elif defined(BOARD_STICKCPLUS2)
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    Serial.begin(115200);
#endif

    Serial.println("\n========================================");
    Serial.println("[CoreInk-Meteo] Avvio v" FW_VERSION);
    Serial.println("========================================");

    // Watchdog 30 secondi
    esp_task_wdt_init(30, true);
    esp_task_wdt_add(NULL);

    // NVS
    nvs_config_init();

    // Profilo
    activeProfile = nvs_load_int(NVS_KEY_PROFILE, 0);
    if (activeProfile >= NUM_PROFILES) activeProfile = 0;

    // Buzzer e LED
    buzzer_init();
    led_init();
    led_set_state(LED_SLOW);

    // Porta: ENV III o GPS
    portMode = -1;
    int savedPortMode = nvs_load_int(NVS_KEY_PORT_MODE, PORT_MODE_DEFAULT);
    switchPortMode(savedPortMode);

    // Display sprite
#if defined(BOARD_COREINK)
    mainSprite.creatSprite(0, 0, 200, 200);
#elif defined(BOARD_STICKCPLUS2)
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextSize(1);
#endif

    // ── BOOT SPLASH ──────────────────────────────────────────────────────────────
    // WiFi parte NON-BLOCCANTE prima del pushSprite: lo stack ESP32 connette
    // in background durante i ~6.5s del boot splash (1.5s e-ink + 5s delay).
    // WiFi.begin() senza argomenti usa le ultime credenziali connesse con successo
    // (ESP32 flash interno) → non sovrascrive last-known-AP, evita 7.5s sprecati.
    if (nvs_load_int(NVS_KEY_WIFI_ENABLED, 1) != 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin();  // usa last-known-AP dall'ESP32 flash (preserva credenziali corrette)
    }
#if defined(BOARD_COREINK)
    {
        char bb[40];
        mainSprite.clear();
        mainSprite.drawString((200 - 13*8)/2, 36,  "CoreInk-Meteo",    &fonts::AsciiFont8x16);
        snprintf(bb, sizeof(bb), "v%s", FW_VERSION);
        mainSprite.drawString((200 - (int)strlen(bb)*8)/2, 62,  bb,                 &fonts::AsciiFont8x16);
        mainSprite.drawString((200 - 17*8)/2, 100, "Avvio in corso...", &fonts::AsciiFont8x16);
        snprintf(bb, sizeof(bb), "Porta: %s",
                 portMode == PORT_MODE_ENV ? "ENV III" : "GPS");
        mainSprite.drawString((200 - (int)strlen(bb)*8)/2, 130, bb,                 &fonts::AsciiFont8x16);
        mainSprite.pushSprite();  // ~1.5s bloccante: splash sull'e-ink (WiFi in background)
        delay(5000);              // 5s visibile: WiFi continua a connettersi in background
    }
#endif
    // Prima lettura ENV
    if (portMode == PORT_MODE_ENV) readSensors();
    // ─────────────────────────────────────────────────────────────────────────────

    // Selezione profilo al boot
    selectProfile();

    // Caricare TUTTI i parametri da NVS
    {
        String s;
        char pkey[16];

        // Callsign per-profilo: "call_N" — fallback legacy "callsign"
        snprintf(pkey, sizeof(pkey), "call_%d", activeProfile);
        s = nvs_load_string(pkey, "");
        if (s.length() == 0) s = nvs_load_string(NVS_KEY_CALLSIGN, profiles[activeProfile].callsign);
        strncpy(rtCallsign, s.c_str(), sizeof(rtCallsign) - 1);
        rtCallsign[sizeof(rtCallsign) - 1] = '\0';

        // Passcode per-profilo: "pass_N" — fallback legacy "passcode"
        snprintf(pkey, sizeof(pkey), "pass_%d", activeProfile);
        s = nvs_load_string(pkey, "");
        if (s.length() == 0) s = nvs_load_string(NVS_KEY_PASSCODE, profiles[activeProfile].passcode);
        strncpy(rtPasscode, s.c_str(), sizeof(rtPasscode) - 1);
        rtPasscode[sizeof(rtPasscode) - 1] = '\0';

        // SSID APRS per-profilo: "ssid_N" — fallback legacy "ssid_aprs"
        snprintf(pkey, sizeof(pkey), "ssid_%d", activeProfile);
        int ssidLegacy = nvs_load_int(NVS_KEY_SSID_APRS, profiles[activeProfile].ssid);
        rtSsidAprs = nvs_load_int(pkey, ssidLegacy);

        s = nvs_load_string(NVS_KEY_LOCATOR, STATION_LOCATOR);
        strncpy(rtLocator, s.c_str(), sizeof(rtLocator) - 1);
        rtLocator[sizeof(rtLocator) - 1] = '\0';

        // Simbolo APRS da NVS (2 char: tabella + codice)
        s = nvs_load_string(NVS_KEY_SYMBOL, "");
        if (s.length() >= 2) {
            rtSymbolTable = s.charAt(0);
            rtSymbolCode = s.charAt(1);
        } else {
            rtSymbolTable = profiles[activeProfile].symbolTable;
            rtSymbolCode = profiles[activeProfile].symbolCode;
        }

        // APRS Status — se il valore NVS e' il formato default (con versione), aggiorna sempre alla versione corrente
        s = nvs_load_string(NVS_KEY_APRS_STATUS, APRS_STATUS_DEFAULT);
        if (s.startsWith("CoreInk-Meteo v")) {
            s = APRS_STATUS_DEFAULT;
            nvs_save_string(NVS_KEY_APRS_STATUS, s.c_str());
        }
        strncpy(rtAprsStatus, s.c_str(), sizeof(rtAprsStatus) - 1);
        rtAprsStatus[sizeof(rtAprsStatus) - 1] = '\0';

        // Display refresh
        rtDisplayUpdateMs = nvs_load_int(NVS_KEY_DISPLAY_REFRESH, DISPLAY_UPDATE_MS / 1000) * 1000UL;

        // Intervalli APRS
        rtWeatherIntervalMs = nvs_load_int(NVS_KEY_WEATHER_INTERVAL, WEATHER_INTERVAL_MS / 60000) * 60000UL;
        rtStatusIntervalMs = nvs_load_int(NVS_KEY_STATUS_INTERVAL, APRS_STATUS_INTERVAL_MS / 60000) * 60000UL;
    }

    Serial.printf("[NVS] Call=%s Pass=%s Loc=%s Sym=%c%c\n",
                  rtCallsign, rtPasscode, rtLocator, rtSymbolTable, rtSymbolCode);

    // WiFi
    wifiEnabled = nvs_load_int(NVS_KEY_WIFI_ENABLED, 1) != 0;
    if (wifiEnabled) {
        // WiFi.mode(WIFI_STA) e WiFi.begin() già chiamati nel boot splash (non-bloccanti)
        // DOWN premuto al boot = reset credenziali WiFi
        M5.update();
        if (M5.BtnDOWN.isPressed()) {
            Serial.println("[WiFi] Reset credenziali (DOWN premuto)");
            WiFiManager wmReset;
            wmReset.resetSettings();
            WiFi.disconnect(true);
            buzzer_play_event(BUZZ_CONFIRM);
        }

        // Attende ancora un po' se lo stack WiFi è quasi connesso
        if (WiFi.status() != WL_CONNECTED) {
            unsigned long _wt = millis() + 1500;
            while (WiFi.status() != WL_CONNECTED && millis() < _wt) delay(100);
        }
        // connectWiFi() solo se non già connesso in background
        if (WiFi.status() != WL_CONNECTED) connectWiFi();
        if (WiFi.status() != WL_CONNECTED) {
            showWifiMenu();  // S2: menu WiFi con timeout, senza AP automatico
        }
        if (WiFi.status() == WL_CONNECTED) {
            wifiState = WIFI_ST_CONNECTED;
            if (strcmp(rtCallsign, "NOCALL") != 0 && strcmp(rtPasscode, "-1") != 0) {
                led_set_state(LED_SOLID);
            } else {
                led_set_state(LED_SLOW);  // WiFi ok ma APRS non configurato
            }
            buzzer_play_event(BUZZ_WIFI_OK);
        }
    } else {
        WiFi.mode(WIFI_OFF);
        wifiState = WIFI_ST_OFF;
        Serial.println("[WiFi] Disabilitato (NVS)");
    }

    // NTP
    if (wifiEnabled && WiFi.status() == WL_CONNECTED) {
        syncTime();
    }

    // OTA WiFi
#if OTA_ENABLED
    if (wifiEnabled && WiFi.status() == WL_CONNECTED) {
        setupOTA();
    }
#endif

    // BLE OTA — INDIPENDENTE da BLE_ENABLED (fix bug v1.2)
#if BLE_OTA_ENABLED
    if (!BLEDevice::getInitialized()) {
        BLEDevice::init(BLE_DEVICE_NAME);
    }
    BLEServer* pBleServer = BLEDevice::createServer();
    ble_ota_begin(pBleServer);
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_OTA_SERVICE_UUID);
    pAdvertising->start();
    Serial.println("[BLE-OTA] Attivo (indipendente da BLE_ENABLED)");
#endif

#if BLE_ENABLED
    btEnabled = nvs_load_int(NVS_KEY_BT_ENABLED, 1) != 0;
    if (btEnabled && !BLEDevice::getInitialized()) {
        BLEDevice::init(BLE_DEVICE_NAME);
        Serial.printf("[BLE] Inizializzato: %s\n", BLE_DEVICE_NAME);
    }
#endif

    // Data logger
#if DATA_LOGGER_ENABLED
    logger_init();
#endif

    // OpenWeatherMap: init + coordinate stazione da locator
    maidenhead_to_latlon(rtLocator, stationLat, stationLon);
    owm_init();

    // Prima lettura
    if (portMode == PORT_MODE_ENV) readSensors();
    batVoltage = readBattery();
    updateDisplay();

    // Melodia di boot
    buzzer_play_event(BUZZ_BOOT);

    // Primo invio APRS (solo se callsign e passcode configurati)
    bool canTx = wifiEnabled && WiFi.status() == WL_CONNECTED
                 && strcmp(rtCallsign, "NOCALL") != 0
                 && strcmp(rtPasscode, "-1") != 0;

    if (canTx) {
        if (portMode == PORT_MODE_ENV) {
            readSensors();
            delay(200);
            readSensors();  // Doppia lettura per QMP6988 cold boot
        }
        sendWeatherPacket();
        sendPositionPacket();  // Posizione iniziale (una tantum se fissa)
        smartBeacon.reset();   // Evita doppio TX: SmartBeacon non rifara' beacon subito nel loop
        // Definizioni telemetria: inviate UNA SOLA VOLTA al boot
        sendTelemetryDefinitions();
        telemetryDefSent = true;
    } else {
        Serial.println("[APRS] TX saltato: configura callsign/passcode via WiFiManager (MID 3s)");
    }

    uptimeStart = millis();
    lastWeatherTime = millis();
    lastPositionTime = millis();
    lastDisplayTime = millis();
    lastTelemetryTime = millis();
    lastTelemetryDefTime = millis();
    lastStatusTime = millis();
    lastLogTime = millis();
    einkRefreshCounter = EINK_FULL_REFRESH_EVERY - 1;  // Primo refresh nel loop sarà full (anti-ghosting)

    Serial.println("[SETUP] Completato!\n");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
    unsigned long now = millis();

    esp_task_wdt_reset();

#if defined(BOARD_COREINK)
    digitalWrite(12, HIGH);
#endif

    led_update();

    // GPS
    if (portMode == PORT_MODE_GPS) {
        readGps();
#if SMARTBEACON_ENABLED
        if (gpsFixValid && smartBeacon.shouldBeacon(gpsSpeed, gpsCourse)) {
            sendPositionPacket();
            lastPositionTime = now;
        }
#endif
    }

    bool canTx = wifiEnabled && WiFi.status() == WL_CONNECTED
                 && strcmp(rtCallsign, "NOCALL") != 0
                 && strcmp(rtPasscode, "-1") != 0;

    // Meteo ogni rtWeatherIntervalMs (default 5 min)
    if (now - lastWeatherTime >= rtWeatherIntervalMs) {
        if (portMode == PORT_MODE_ENV) readSensors();
        if (canTx) sendWeatherPacket();
        lastWeatherTime = now;
    }

    // Posizione GPS: gestita esclusivamente da SmartBeacon (blocco sopra).
    // BUG-02: rimosso timer 5-min che causava duplicate position packet ogni ciclo meteo.

    // Telemetria ogni TELEMETRY_INTERVAL_MS (default 10 min)
    if (now - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
        batVoltage = readBattery();
        chargeState = detectChargeState();
        if (canTx) sendTelemetry();
        lastTelemetryTime = now;
    }

    // Definizioni telemetria: al boot (già inviate) + backup ogni 2 ore
    if (!telemetryDefSent || (now - lastTelemetryDefTime >= TELEMETRY_DEFINITION_INTERVAL_MS)) {
        if (canTx) {
            sendTelemetryDefinitions();
            telemetryDefSent = true;
        }
        lastTelemetryDefTime = now;
    }

    // Status APRS
    if (now - lastStatusTime >= rtStatusIntervalMs) {
        if (canTx) sendStatusPacket();
        lastStatusTime = now;
    }

    // Data logger
#if DATA_LOGGER_ENABLED
    if (logger_is_enabled() && (now - lastLogTime >= (unsigned long)logger_get_interval() * 1000UL)) {
        writeLogRecord();
        lastLogTime = now;
    }
#endif

    // OpenWeatherMap: aggiorna dati se scaduto intervallo
    owm_update(now);

    // Display
    if (now - lastDisplayTime >= rtDisplayUpdateMs) {
        if (portMode == PORT_MODE_ENV) readSensors();
        batVoltage = readBattery();
        coordShowMaidenhead = !coordShowMaidenhead;
#if defined(USE_M5UNIFIED)
        // Anti-ghosting: ogni N refresh fa un ciclo nero→bianco che elimina ghosting residuo
        einkRefreshCounter++;
        if (einkRefreshCounter >= EINK_FULL_REFRESH_EVERY) {
            eink_clear_ghosting();  // nero→bianco→wait (pulisce pixel)
            einkRefreshCounter = 0;
        }
#endif
        updateDisplay();
        lastDisplayTime = now;
    }

    // === Pulsanti ===
    M5.update();
#if defined(BOARD_COREINK)
    // MID: long (5s) → WiFiManager | short → menu contestuale della pagina corrente
    {
        static bool midLongFired = false;
        if (!midLongFired && M5.BtnMID.pressedFor(5000)) {
            midLongFired = true;
            startWiFiManager();
        }
        if (M5.BtnMID.wasReleased()) {
            if (!midLongFired) {
                // MID short: menu contestuale (per ora implementato solo P6)
                if (currentPage == 6) {
                    showPage6Menu();
                    M5.update();  // consuma eventi tasto pendenti dopo menu (evita reinvocazione)
                }
                // v1.3: ogni pagina avrà il proprio menu (ui_button_model.md)
            }
            midLongFired = false;
        }
    }
    if (M5.BtnUP.wasPressed()) {
        currentPage = (currentPage - 1 + NUM_PAGES) % NUM_PAGES;
        buzzer_play_event(BUZZ_PAGE);
        updateDisplay();
        lastDisplayTime = millis();  // BUG-11: evita refresh automatico subito dopo cambio pagina
    }
    if (M5.BtnDOWN.wasPressed()) {
        currentPage = (currentPage + 1) % NUM_PAGES;
        buzzer_play_event(BUZZ_PAGE);
        updateDisplay();
        lastDisplayTime = millis();  // BUG-11: evita refresh automatico subito dopo cambio pagina
    }
    // EXT: long (3s) → menu emergenza WiFi + full refresh | short → nessuna azione (ui_button_model.md)
    {
        static bool extLongFired = false;
        if (!extLongFired && M5.BtnEXT.pressedFor(3000)) {
            extLongFired = true;
            // Menu emergenza: UP/DOWN per scegliere, MID per confermare, EXT per uscire
            int menuSel = 0;
            const int MENU_ITEMS = 3;
            const char* menuLabels[] = {"WiFi emergenza", "Pulizia display", "Standby display"};
            bool menuDone = false;
            while (!menuDone) {
#if defined(BOARD_COREINK)
                M5.M5Ink.clear();
                mainSprite.clear();
                mainSprite.drawString(5, 0, "MENU EMERGENZA", &fonts::AsciiFont8x16);
                mainSprite.drawString(5, 20, "UP/DN:scegli MID:ok", &fonts::AsciiFont8x16);
                mainSprite.drawString(5, 36, "EXT:esci", &fonts::AsciiFont8x16);
                for (int i = 0; i < MENU_ITEMS; i++) {
                    char line[32];
                    snprintf(line, sizeof(line), "%s %s", (i == menuSel) ? ">" : " ", menuLabels[i]);
                    mainSprite.drawString(5, 60 + i * 20, line, &fonts::AsciiFont8x16);
                }
                mainSprite.pushSprite();
#endif
                // Attendi input
                while (true) {
                    M5.update();
                    if (M5.BtnUP.wasPressed()) { menuSel = (menuSel - 1 + MENU_ITEMS) % MENU_ITEMS; break; }
                    if (M5.BtnDOWN.wasPressed()) { menuSel = (menuSel + 1) % MENU_ITEMS; break; }
                    if (M5.BtnMID.wasPressed()) { menuDone = true; break; }
                    if (M5.BtnEXT.wasPressed()) { menuSel = -1; menuDone = true; break; }
                    delay(50);
                    esp_task_wdt_reset();
                }
            }
            M5.update();
            if (menuSel == 0) {
                // WiFi emergenza
                showWifiMenu();
            } else if (menuSel == 1) {
                // Pulizia display: 10 cicli nero(1s) → bianco(1s)
#if defined(USE_M5UNIFIED)
                eink_deep_clean(10);
#endif
            } else if (menuSel == 2) {
                // Standby display: nero → bianco → resta bianco fino a joystick
#if defined(USE_M5UNIFIED)
                M5.Display.fillScreen(TFT_BLACK);
                M5.Display.display();
                M5.Display.waitDisplay();
                delay(1000);
                M5.Display.fillScreen(TFT_WHITE);
                M5.Display.display();
                M5.Display.waitDisplay();
#endif
                // Attendi qualsiasi tasto per uscire dallo standby
                while (true) {
                    M5.update();
                    if (M5.BtnUP.wasPressed() || M5.BtnDOWN.wasPressed() ||
                        M5.BtnMID.wasPressed() || M5.BtnEXT.wasPressed()) break;
                    delay(100);
                    esp_task_wdt_reset();
                }
            }
            // Ritorna alla pagina normale
#if defined(USE_M5UNIFIED)
            eink_clear_ghosting();
#endif
            updateDisplay();
            lastDisplayTime = millis();
        }
        if (M5.BtnEXT.wasReleased()) {
            extLongFired = false;
        }
    }
#elif defined(BOARD_STICKCPLUS2)
    if (M5.BtnA.pressedFor(3000)) startWiFiManager();
    if (M5.BtnA.wasClicked()) {
        currentPage = (currentPage + 1) % NUM_PAGES;
        updateDisplay();
    }
    if (M5.BtnB.wasClicked()) {
        currentPage = (currentPage - 1 + NUM_PAGES) % NUM_PAGES;
        updateDisplay();
    }
#endif

    // OTA
#if OTA_ENABLED
    if (wifiEnabled) {
        ArduinoOTA.handle();
        otaServer.handleClient();
    }
#endif
#if BLE_OTA_ENABLED
    ble_ota_handle();
#endif

    // WiFi FSM
    wifi_update();

    // Batteria bassa / critica (non su USB)
    if (!isOnUsb() && batVoltage > 0.5f) {
        if (batVoltage < BAT_CRITICAL_THRESHOLD_V) {
            // Batteria critica: spegni il dispositivo (BM8563 de-asserta PS_EN)
            // TODO v1.3: mostrare schermata "Batteria critica" prima dello spegnimento
#if defined(USE_M5UNIFIED)
            M5.Power.powerOff();
#else
            M5.shutdown();
#endif
        } else if (batVoltage < BAT_WARN_THRESHOLD_V) {
            // Batteria !!!: 3.2-3.3 V — buzzer urgente ogni 30s
            led_set_state(LED_FAST);
            static unsigned long lastBatCrit = 0;
            if (now - lastBatCrit > 30000) {
                buzzer_play_event(BUZZ_BAT_CRITICAL);
                lastBatCrit = now;
            }
        } else if (batVoltage < BAT_LOW_THRESHOLD_V) {
            // Batteria LOW: 3.3-3.5 V — buzzer ogni 60s
            led_set_state(LED_FAST);
            static unsigned long lastBatWarn = 0;
            if (now - lastBatWarn > 60000) {
                buzzer_play_event(BUZZ_BAT_LOW);
                lastBatWarn = now;
            }
        }
    }

    // WiFi reconnect gestito da wifi_update() — rimosso loop primitivo

    delay(50);
}

// ============================================================================
// BATTERIA: isOnUsb() e detectChargeState()
// ============================================================================
bool isOnUsb() {
    // Backdrive del body-diode del FET sincrono SY7088: quando USB è collegata,
    // la tensione sul partitore ADC supera la soglia della LiPo carica (4.2V).
    // Vedi docs/power_analysis.md per la derivazione della soglia 4.4V.
    return batVoltage > BAT_USB_THRESHOLD_V;
}

ChargeState detectChargeState() {
    if (isOnUsb()) return CHG_UNKNOWN;  // USB collegata: backdrive alza Vbat, slope non affidabile
    if (!batSamplesReady) return CHG_UNKNOWN;  // Non abbastanza campioni
    // Slope = campione più recente - campione più vecchio nel buffer circolare
    int oldestIdx = batSampleIdx;  // prossima scrittura = campione più vecchio
    int newestIdx = (batSampleIdx + BAT_ADC_SAMPLES - 1) % BAT_ADC_SAMPLES;
    float slope = batSamples[newestIdx] - batSamples[oldestIdx];
    if (slope >  0.02f) return CHG_CHARGING;
    if (slope < -0.02f) return CHG_DISCHARGING;
    return CHG_IDLE;
}

// ============================================================================
// WIFI FSM
// ============================================================================
void wifi_update() {
    if (!wifiEnabled) return;
    unsigned long now = millis();
    switch (wifiState) {
        case WIFI_ST_OFF:
            break;
        case WIFI_ST_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                wifiState = WIFI_ST_CONNECTED;
                wifiRetryBackoff = 15000;
                if (strcmp(rtCallsign, "NOCALL") != 0 && strcmp(rtPasscode, "-1") != 0)
                    led_set_state(LED_SOLID);
                else
                    led_set_state(LED_SLOW);
                buzzer_play_event(BUZZ_WIFI_OK);
                syncTime();  // Risincronizza NTP dopo riconnessione
                // B5/W3 fix: non forzare re-invio PARM/UNIT/EQNS su riconnessione.
                // Il timer 2h (TELEMETRY_DEFINITION_INTERVAL_MS) gestisce il refresh periodico.
            } else if (now - wifiRetryTime > WIFI_TIMEOUT_MS) {
                WiFi.disconnect();
                wifiState = (wifiStatePrev == WIFI_ST_CONNECTED ||
                             wifiStatePrev == WIFI_ST_DISCONNECTED)
                            ? WIFI_ST_DISCONNECTED : WIFI_ST_WAITING;
                wifiRetryTime = now;
                led_set_state(LED_SLOW);
            }
            break;
        case WIFI_ST_CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
                wifiState = WIFI_ST_DISCONNECTED;
                wifiStatePrev = WIFI_ST_CONNECTED;
                wifiRetryTime = now;
                wifiRetryBackoff = 2000;  // Primo retry rapido; aumenta dopo ogni ciclo completo
                wifiApIdx = 0;            // Riparti da AP1 al prossimo tentativo
                led_set_state(LED_SLOW);
            }
            break;
        case WIFI_ST_DISCONNECTED:
            if (now - wifiRetryTime >= (unsigned long)wifiRetryBackoff) {
                // Ciclo round-robin tra i 3 AP NVS: AP1 → AP2 → AP3 → AP1 ...
                // Ogni ciclo completo aumenta il backoff inter-ciclo (max 60s).
                static const char* apSsidKeys[] = { NVS_KEY_WIFI_SSID, NVS_KEY_WIFI2_SSID, NVS_KEY_WIFI3_SSID };
                static const char* apPassKeys[] = { NVS_KEY_WIFI_PASS, NVS_KEY_WIFI2_PASS, NVS_KEY_WIFI3_PASS };
                String apSsid = "", apPass = "";
                // Trova il prossimo AP configurato (salta le voci SSID vuote)
                for (int t = 0; t < 3; t++) {
                    int idx = wifiApIdx % 3;
                    apSsid = nvs_load_string(apSsidKeys[idx], "");
                    apPass = nvs_load_string(apPassKeys[idx], "");
                    wifiApIdx++;
                    if (apSsid.length() > 0) break;
                }
                if (apSsid.length() > 0) {
                    Serial.printf("[WiFi FSM] Retry AP%d: %s\n", ((wifiApIdx - 1) % 3) + 1, apSsid.c_str());
                    WiFi.begin(apSsid.c_str(), apPass.c_str());
                } else {
                    WiFi.begin();  // Nessun AP in NVS: usa credenziali salvate dall'SDK
                }
                wifiStatePrev = WIFI_ST_DISCONNECTED;
                wifiState = WIFI_ST_CONNECTING;
                wifiRetryTime = now;
                // Backoff breve tra AP dello stesso ciclo; aumenta esponenzialmente dopo ogni ciclo completo
                if ((wifiApIdx % 3) == 0) {
                    if (wifiRetryBackoff < 60000) wifiRetryBackoff = min(wifiRetryBackoff * 2, 60000);
                } else {
                    wifiRetryBackoff = 2000;  // 2s tra un AP e il successivo
                }
            }
            break;
        case WIFI_ST_WAITING:
            // Nessun retry automatico: l'utente deve agire dal menu
            break;
        case WIFI_ST_AP_CONFIG:
            // Gestito da startWiFiManager() (blocking)
            break;
    }
}

// ============================================================================
// S2 WIFI MENU (boot screen)
// ============================================================================
void showWifiMenu() {
    // Mostra schermata per WIFI_MENU_TIMEOUT_S secondi.
    // MID = prova connessione | EXT = salta
    unsigned long deadline = millis() + (unsigned long)WIFI_MENU_TIMEOUT_S * 1000UL;
    int remaining = WIFI_MENU_TIMEOUT_S;
    bool userChose = false;
    bool skip = false;

    while (millis() < deadline && !userChose) {
        remaining = (int)((deadline - millis()) / 1000);
#if defined(BOARD_COREINK)
        M5.M5Ink.clear();
        mainSprite.clear();
        char buf[48];
        mainSprite.drawString(5, 5,  "[S2] WiFi MENU",      &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 30, "MID = connetti",       &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 50, "EXT = salta",          &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 80, "AUTO-connect dopo:",   &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "%ds", remaining);
        mainSprite.drawString(5, 100, buf,                   &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 130, WiFi.SSID().length() ? WiFi.SSID().c_str() : "(nessuna rete salvata)",
                              &fonts::AsciiFont8x16);
        mainSprite.pushSprite();
#endif
        unsigned long frameEnd = millis() + 1000;
        while (millis() < frameEnd) {
            M5.update();
            if (M5.BtnMID.wasPressed()) { userChose = true; skip = false; break; }
            if (M5.BtnEXT.wasPressed()) { userChose = true; skip = true;  break; }
            delay(50);
        }
    }

    if (!skip) {
        // Tenta connessione con credenziali salvate
        connectWiFi();
        if (WiFi.status() == WL_CONNECTED) {
            wifiState = WIFI_ST_CONNECTED;
        } else {
            wifiState = WIFI_ST_WAITING;
        }
    } else {
        wifiState = WIFI_ST_WAITING;
        Serial.println("[S2] WiFi saltato dall'utente");
    }
}

// ============================================================================
// Haversine: distanza in metri tra due coordinate GPS
// ============================================================================
float haversineM(float lat1, float lon1, float lat2, float lon2) {
    const float R = 6371000.0f;  // Raggio Terra in metri
    float dLat = radians(lat2 - lat1);
    float dLon = radians(lon2 - lon1);
    float a = sinf(dLat/2)*sinf(dLat/2)
            + cosf(radians(lat1))*cosf(radians(lat2))
            * sinf(dLon/2)*sinf(dLon/2);
    return R * 2.0f * atan2f(sqrtf(a), sqrtf(1.0f-a));
}

// ============================================================================
// WIFI: connessione alle reti in config.h
// ============================================================================
void connectWiFi() {
    Serial.println("[WiFi] Connessione...");
    WiFi.begin();
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS / 2) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connesso: %s IP: %s\n",
                      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        return;
    }

    // Prova le 3 reti WiFi salvate in NVS (AP1=wifi_ssid legacy, AP2/AP3=wifi2/3_ssid)
    static const char* ssid_keys[] = { NVS_KEY_WIFI_SSID, NVS_KEY_WIFI2_SSID, NVS_KEY_WIFI3_SSID };
    static const char* pass_keys[] = { NVS_KEY_WIFI_PASS, NVS_KEY_WIFI2_PASS, NVS_KEY_WIFI3_PASS };
    Serial.println("\n[WiFi] Provo 3 reti NVS...");
    WiFi.disconnect();
    for (int i = 0; i < 3; i++) {
        String ss = nvs_load_string(ssid_keys[i], "");
        if (ss.length() == 0) continue;
        String pp = nvs_load_string(pass_keys[i], "");
        WiFi.begin(ss.c_str(), pp.c_str());
        start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS / 3) {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("\n[WiFi] Connesso AP%d: %s IP: %s\n", i + 1,
                          WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
            return;
        }
        WiFi.disconnect();
    }
    Serial.println("\n[WiFi] FALLITO!");
}

// ============================================================================
// WIFIMANAGER — portale captive con TUTTI i parametri configurabili
// ============================================================================
void startWiFiManager() {
    Serial.println("[WiFiManager] Portale captive...");
    esp_task_wdt_delete(NULL);  // Disabilita watchdog durante configurazione

#if defined(BOARD_COREINK)
    M5.M5Ink.clear();
    mainSprite.clear();
    mainSprite.drawString(5, 10, "WiFi Setup", &fonts::AsciiFont8x16);
    mainSprite.drawString(5, 35, "Connettiti a:", &fonts::AsciiFont8x16);
    mainSprite.drawString(5, 55, WIFIMANAGER_AP_NAME, &fonts::AsciiFont8x16);
    mainSprite.drawString(5, 80, "poi vai a:", &fonts::AsciiFont8x16);
    mainSprite.drawString(5, 100, "192.168.4.1", &fonts::AsciiFont8x16);
    mainSprite.drawString(5, 130, "Configura APRS:", &fonts::AsciiFont8x16);
    mainSprite.drawString(5, 150, "call/pass/loc/sym", &fonts::AsciiFont8x16);
    mainSprite.drawString(5, 175, "EXT: annulla", &fonts::AsciiFont8x16);
    mainSprite.pushSprite();
#elif defined(BOARD_STICKCPLUS2)
    auto &lcd = StickCP2.Display;
    lcd.fillScreen(BLACK);
    lcd.setTextColor(WHITE);
    lcd.setCursor(5, 10);
    lcd.println("WiFi Setup");
    lcd.println("AP: " WIFIMANAGER_AP_NAME);
    lcd.println("URL: 192.168.4.1");
#endif

    WiFiManager wm;
    wm.setConfigPortalTimeout(120);  // 2 minuti max
    wm.setConnectTimeout(10);        // Max 10s per tentativo di connessione
    wm.setBreakAfterConfig(true);    // Chiudi solo dopo salvataggio
    wm.setConfigPortalBlocking(false); // Non-blocking: EXT aborta il portale

    // ---- Parametri APRS ----
    WiFiManagerParameter hdr1("<hr><h3>Stazione APRS</h3>");
    wm.addParameter(&hdr1);

    WiFiManagerParameter p_call("callsign", "Nominativo (es. IZ3ARR)",
        nvs_load_string(NVS_KEY_CALLSIGN, profiles[activeProfile].callsign).c_str(), 10);
    wm.addParameter(&p_call);

    char ssidBuf[4];
    snprintf(ssidBuf, sizeof(ssidBuf), "%d", nvs_load_int(NVS_KEY_SSID_APRS, profiles[activeProfile].ssid));
    WiFiManagerParameter p_ssid("ssid_aprs", "SSID APRS (0-15, 13=meteo)", ssidBuf, 3);
    wm.addParameter(&p_ssid);

    WiFiManagerParameter p_pass("passcode", "Passcode APRS-IS (5 cifre, vedi aprs.fi)",
        nvs_load_string(NVS_KEY_PASSCODE, profiles[activeProfile].passcode).c_str(), 6);
    wm.addParameter(&p_pass);

    WiFiManagerParameter p_loc("locator", "Locatore Maidenhead max 8 char (es. JN61fw12)",
        nvs_load_string(NVS_KEY_LOCATOR, STATION_LOCATOR).c_str(), 10);
    wm.addParameter(&p_loc);

    char defSym[4];
    snprintf(defSym, sizeof(defSym), "%c%c", rtSymbolTable, rtSymbolCode);
    WiFiManagerParameter sym_help("<p><small>Simbolo APRS (2 char): /_ = WX &nbsp; /- = Casa &nbsp; /y = Antenna &nbsp; \\_ = WX verde</small></p>");
    wm.addParameter(&sym_help);
    WiFiManagerParameter p_sym("symbol", "Simbolo APRS (2 char)", defSym, 3);
    wm.addParameter(&p_sym);

    // ---- Status e Buzzer ----
    WiFiManagerParameter hdr2("<hr><h3>Status e Buzzer</h3>");
    wm.addParameter(&hdr2);

    WiFiManagerParameter p_status("status", "Messaggio Status APRS", rtAprsStatus, 60);
    wm.addParameter(&p_status);

    char volBuf[4];
    snprintf(volBuf, sizeof(volBuf), "%d", nvs_load_int(NVS_KEY_BUZZER_VOLUME, BUZZER_DEFAULT_VOLUME));
    WiFiManagerParameter p_vol("buzz_vol", "Volume buzzer (0-100)", volBuf, 4);
    wm.addParameter(&p_vol);

    char melBuf[2];
    snprintf(melBuf, sizeof(melBuf), "%d", nvs_load_int(NVS_KEY_BOOT_MELODY, 2));
    WiFiManagerParameter mel_help("<p><small>Melodia: 0=bip 1=Weather 2=Sailor 3=Sunrise 4=Radio 5=Jingle</small></p>");
    wm.addParameter(&mel_help);
    WiFiManagerParameter p_mel("melody", "Melodia boot (0-5)", melBuf, 2);
    wm.addParameter(&p_mel);

    // ---- Display ----
    WiFiManagerParameter hdr3("<hr><h3>Display</h3>");
    wm.addParameter(&hdr3);

    char refBuf[5];
    snprintf(refBuf, sizeof(refBuf), "%d", (int)(rtDisplayUpdateMs / 1000));
    WiFiManagerParameter p_ref("disp_ref", "Refresh display (secondi, 10-300)", refBuf, 4);
    wm.addParameter(&p_ref);

    // ---- Intervalli TX ----
    WiFiManagerParameter hdr4("<hr><h3>Intervalli TX</h3>");
    wm.addParameter(&hdr4);

    char wxBuf[4];
    snprintf(wxBuf, sizeof(wxBuf), "%d", (int)(rtWeatherIntervalMs / 60000));
    WiFiManagerParameter p_wx("wx_intv", "Meteo ogni N minuti (1-60)", wxBuf, 3);
    wm.addParameter(&p_wx);

    char stBuf[4];
    snprintf(stBuf, sizeof(stBuf), "%d", (int)(rtStatusIntervalMs / 60000));
    WiFiManagerParameter p_st("st_intv", "Status ogni N minuti (10-180)", stBuf, 4);
    wm.addParameter(&p_st);

    wm.startConfigPortal(WIFIMANAGER_AP_NAME);
    // Loop non-bloccante: EXT aborta, timeout via setConfigPortalTimeout
    // otaServer rimane attivo anche durante l'AP (porta 8080 != 80)
    while (true) {
        if (wm.process()) break;  // Configurazione salvata o timeout
        M5.update();
        if (M5.BtnEXT.wasPressed()) {
            wm.stopConfigPortal();
            break;
        }
#if OTA_ENABLED
        otaServer.handleClient();  // OTA raggiungibile su 192.168.4.1:8080
#endif
        delay(50);
    }
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    if (!wifiOk) {
        WiFi.mode(WIFI_STA);
        wifiState = WIFI_ST_DISCONNECTED;  // Attiva reconnect automatico via wifi_update()
        wifiRetryTime = millis();
    }

    // Salvare parametri SEMPRE (anche se WiFi non si connette)
    if (strlen(p_call.getValue()) > 0) {
        nvs_save_string(NVS_KEY_CALLSIGN, p_call.getValue());
        strncpy(rtCallsign, p_call.getValue(), sizeof(rtCallsign) - 1);
        rtCallsign[sizeof(rtCallsign) - 1] = '\0';
    }
    {
        int ssid = atoi(p_ssid.getValue());
        if (ssid >= 0 && ssid <= 15) {
            nvs_save_int(NVS_KEY_SSID_APRS, ssid);
            rtSsidAprs = ssid;
        }
    }
    if (strlen(p_pass.getValue()) > 0) {
        nvs_save_string(NVS_KEY_PASSCODE, p_pass.getValue());
        strncpy(rtPasscode, p_pass.getValue(), sizeof(rtPasscode) - 1);
        rtPasscode[sizeof(rtPasscode) - 1] = '\0';
    }
    if (strlen(p_loc.getValue()) > 0) {
        nvs_save_string(NVS_KEY_LOCATOR, p_loc.getValue());
        strncpy(rtLocator, p_loc.getValue(), sizeof(rtLocator) - 1);
        rtLocator[sizeof(rtLocator) - 1] = '\0';
    }
    if (strlen(p_sym.getValue()) >= 2) {
        nvs_save_string(NVS_KEY_SYMBOL, p_sym.getValue());
        rtSymbolTable = p_sym.getValue()[0];
        rtSymbolCode = p_sym.getValue()[1];
    }
    if (strlen(p_status.getValue()) > 0) {
        nvs_save_string(NVS_KEY_APRS_STATUS, p_status.getValue());
        strncpy(rtAprsStatus, p_status.getValue(), sizeof(rtAprsStatus) - 1);
        rtAprsStatus[sizeof(rtAprsStatus) - 1] = '\0';
    }

    int vol = atoi(p_vol.getValue());
    if (vol >= 0 && vol <= 100) buzzer_set_volume(vol);

    int mel = atoi(p_mel.getValue());
    if (mel >= 0 && mel <= 5) nvs_save_int(NVS_KEY_BOOT_MELODY, mel);

    int ref = atoi(p_ref.getValue());
    if (ref >= 10 && ref <= 300) {
        nvs_save_int(NVS_KEY_DISPLAY_REFRESH, ref);
        rtDisplayUpdateMs = ref * 1000UL;
    }

    int wx = atoi(p_wx.getValue());
    if (wx >= 1 && wx <= 60) {
        nvs_save_int(NVS_KEY_WEATHER_INTERVAL, wx);
        rtWeatherIntervalMs = wx * 60000UL;
    }

    int st = atoi(p_st.getValue());
    if (st >= 10 && st <= 180) {
        nvs_save_int(NVS_KEY_STATUS_INTERVAL, st);
        rtStatusIntervalMs = st * 60000UL;
    }

    Serial.printf("[WM] Call=%s Pass=%s Loc=%s Sym=%c%c SSID=%d\n",
                  rtCallsign, rtPasscode, rtLocator, rtSymbolTable, rtSymbolCode, rtSsidAprs);

    if (wifiOk) {
        syncTime();
    }

    esp_task_wdt_add(NULL);  // Riattiva watchdog dopo configurazione
}

// ============================================================================
// NTP
// ============================================================================
void syncTime() {
    configTzTime(profiles[activeProfile].timezone, "pool.ntp.org", "time.nist.gov");
    struct tm ti;
    int retries = 0;
    while (!getLocalTime(&ti) && retries < 10) { delay(1000); retries++; }
    Serial.println(retries < 10 ? "[NTP] Sincronizzato" : "[NTP] Fallito");
}

// ============================================================================
// SENSORI ENV III
// ============================================================================
void readSensors() {
    if (portMode != PORT_MODE_ENV) return;
    if (sht3x.update()) {
        float t = sht3x.cTemp;
        float h = sht3x.humidity;
        if (t >= -40.0f && t <= 85.0f)  temperature = t;
        else Serial.printf("[SHT3X] Temperatura fuori range: %.1f C — ignorata\n", t);
        if (h >= 0.0f && h <= 100.0f)   humidity = h;
        else Serial.printf("[SHT3X] Umidita' fuori range: %.1f %% — ignorata\n", h);
    }
    if (qmp.update()) {
        float p = qmp.pressure / 100.0f;
        if (p >= 500.0f && p <= 1100.0f) {
            pressure = p;
        } else {
            Serial.printf("[QMP] Pressione fuori range: %.1f hPa — reinit sensore\n", p);
            pressure = 0.0f;
            Wire.end();
            delay(10);
            Wire.begin(32, 33);
            qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, 32, 33, 400000U);
        }
    } else {
        pressure = 0.0f;
    }
    Serial.printf("[SENSORE] T=%.1fC H=%.1f%% P=%.1fhPa\n", temperature, humidity, pressure);
}

// ============================================================================
// BATTERIA — media mobile N campioni (fix rumore ADC)
// ============================================================================
float readBatteryRaw() {
#if defined(BOARD_COREINK)
    analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);
    esp_adc_cal_characteristics_t *adc_chars =
        (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 3600, adc_chars);
    uint16_t adcValue = analogRead(BAT_ADC_PIN);
    uint32_t voltageMv = esp_adc_cal_raw_to_voltage(adcValue, adc_chars);
    float voltage = float(voltageMv) * 25.1f / 5.1f / 1000.0f;
    free(adc_chars);
    return voltage;
#elif defined(BOARD_STICKCPLUS2)
    return StickCP2.Power.getBatteryVoltage() / 1000.0f;
#endif
}

float readBattery() {
    float raw = readBatteryRaw();
    batSamples[batSampleIdx] = raw;
    batSampleIdx = (batSampleIdx + 1) % BAT_ADC_SAMPLES;
    if (batSampleIdx == 0) batSamplesReady = true;
    int count = batSamplesReady ? BAT_ADC_SAMPLES : (batSampleIdx > 0 ? batSampleIdx : 1);
    float sum = 0;
    for (int i = 0; i < count; i++) sum += batSamples[i];
    float avg = sum / count;
    Serial.printf("[BAT] %.2fV (media %d)\n", avg, count);
    return avg;
}

// ============================================================================
// GPS
// ============================================================================
void readGps() {
    if (portMode != PORT_MODE_GPS) return;
    bool wasFix = gpsFixValid;
    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        gps.encode(c);
#if GPS_EXTRA_ENABLED
        gpsExtra.encode(c);
#endif
    }
    if (gps.location.isUpdated() && gps.location.isValid()) {
        gpsLat = gps.location.lat();
        gpsLon = gps.location.lng();
        gpsFixValid = true;
        lastGpsLat = gpsLat;  // Aggiorna posizione sticky
        lastGpsLon = gpsLon;
        lastGpsPosValid = true;
    }
    if (gps.altitude.isUpdated()) gpsAlt = gps.altitude.meters();
    if (gps.speed.isUpdated()) gpsSpeed = gps.speed.kmph();
    if (gps.course.isUpdated()) gpsCourse = gps.course.deg();
    if (gps.satellites.isUpdated()) gpsSatellites = gps.satellites.value();
    if (gpsFixValid && !wasFix) buzzer_play_event(BUZZ_GPS_FIX);
    if (!gpsFixValid && wasFix) buzzer_play_event(BUZZ_GPS_LOST);
}

// ============================================================================
// SWITCH PORTA ENV III <-> GPS (hot-swap runtime)
// ============================================================================
void switchPortMode(int newMode) {
    if (newMode == portMode) return;
    if (portMode == PORT_MODE_GPS) {
        gpsSerial.end();
    } else if (portMode == PORT_MODE_ENV) {
        Wire.end();
    }
    if (newMode == PORT_MODE_ENV) {
        // Reset valori: resteranno 0 finché non arriva la prima lettura valida
        temperature = 0.0f;
        humidity    = 0.0f;
        pressure    = 0.0f;
        Wire.begin(32, 33);
        sht3x.begin(&Wire, SHT3X_I2C_ADDR, 32, 33, 400000U);
        // QMP6988: retry init con delay crescente (fix cold boot)
        bool qmpOk = false;
        for (int attempt = 0; attempt < 3 && !qmpOk; attempt++) {
            delay(100 * (attempt + 1));
            qmpOk = qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, 32, 33, 400000U);
        }
        if (qmpOk) {
            delay(50);
            qmp.update();  // Warmup: scartare prima lettura
            qmp.update();  // Seconda lettura per stabilizzare
            // Verifica post-switch: se pressione fuori range, reinit aggiuntivo
            float p = qmp.pressure / 100.0f;
            if (p < 500.0f || p > 1100.0f) {
                Serial.printf("[QMP] Post-switch fuori range (%.1f hPa) — secondo reinit\n", p);
                Wire.end(); delay(20); Wire.begin(32, 33);
                qmpOk = qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, 32, 33, 400000U);
                if (qmpOk) { delay(100); qmp.update(); qmp.update(); }
            }
        }
        Serial.printf("[PORT] ENV III (I2C) attivo, QMP6988: %s\n", qmpOk ? "OK" : "FAIL");
    } else if (newMode == PORT_MODE_GPS) {
        // Reset valori ENV: evita che dati corrotti restino visibili in GPS mode
        temperature = 0.0f;
        humidity    = 0.0f;
        pressure    = 0.0f;
        gpsFixValid = false;
        gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        Serial.println("[PORT] GPS (UART) attivo");
    }
    portMode = newMode;
    if (newMode == PORT_MODE_ENV) {
        readSensors();  // Prima lettura obbligatoria dopo switch (dati non possono restare a 0)
    }
}

void drawPortModeNotice() {
#if defined(BOARD_COREINK)
    M5.M5Ink.clear();
    mainSprite.clear();
    mainSprite.drawString(10, 40, "Porta cambiata:", &fonts::AsciiFont8x16);
    mainSprite.drawString(10, 70,
        portMode == PORT_MODE_ENV ? ">>> ENV III (I2C)" : ">>> GPS (UART)",
        &fonts::AsciiFont8x16);
    mainSprite.drawString(10, 100,
        portMode == PORT_MODE_ENV ? "SHT30 + QMP6988" : "AT6558 9600baud",
        &fonts::AsciiFont8x16);
    mainSprite.drawString(10, 140, "Collega il modulo!", &fonts::AsciiFont8x16);
    mainSprite.pushSprite();
#elif defined(BOARD_STICKCPLUS2)
    auto &lcd = StickCP2.Display;
    lcd.fillScreen(BLACK);
    lcd.setTextColor(WHITE);
    lcd.setCursor(5, 40);
    lcd.println("Porta cambiata:");
    lcd.println(portMode == PORT_MODE_ENV ? ">>> ENV III" : ">>> GPS");
    lcd.println("\nCollega il modulo!");
#endif
}

// ============================================================================
// MENU CONTESTUALE P6 — Sensori ENV/GPS  (ui_button_model.md, livello 2)
// Attivato da MID short quando currentPage == 6.
// Opzioni: 1=Forza lettura  2=Commuta ENV/GPS
// Navigazione: UP/DOWN  |  MID = esegui  |  EXT = annulla  |  timeout 10s
// ============================================================================
void showPage6Menu() {
    static const char* items[] = { "1. Forza lettura", "2. Commuta ENV/GPS" };
    const int nItems = 2;
    int sel = 0;
    unsigned long deadline = millis() + 10000UL;
    bool done = false;

    auto redraw = [&]() {
#if defined(BOARD_COREINK)
        M5.M5Ink.clear();
        Ink_Sprite& sp = mainSprite;
        sp.clear();
        char hdr[24];
        snprintf(hdr, sizeof(hdr), "%s-%d", rtCallsign, rtSsidAprs);
        sp.drawString(2, 0, hdr, &fonts::AsciiFont8x16);
        sp.drawString(5, 20, "=== Menu Sensori ===", &fonts::AsciiFont8x16);
        for (int i = 0; i < nItems; i++) {
            char line[32];
            snprintf(line, sizeof(line), "%s %s", i == sel ? ">" : " ", items[i]);
            sp.drawString(5, 46 + i * 22, line, &fonts::AsciiFont8x16);
        }
        int rem = (int)((deadline - millis()) / 1000);
        char foot[40];
        snprintf(foot, sizeof(foot), "MID:ok  EXT:annulla  %ds", rem > 0 ? rem : 0);
        sp.drawString(2, 182, foot, &fonts::AsciiFont8x16);
        sp.pushSprite();
#endif
    };

    redraw();

    while (millis() < deadline && !done) {
        M5.update();
        if (M5.BtnUP.wasPressed() || M5.BtnDOWN.wasPressed()) {
            sel = (sel + (M5.BtnDOWN.wasPressed() ? 1 : -1) + nItems) % nItems;
            deadline = millis() + 10000UL;  // reset timeout su navigazione
            redraw();
        }
        if (M5.BtnMID.wasPressed()) {
            buzzer_play_event(BUZZ_CONFIRM);
            if (sel == 0) {
                readSensors(); delay(200); readSensors();  // doppia lettura warmup
            } else if (sel == 1) {
                int newMode = (portMode == PORT_MODE_ENV) ? PORT_MODE_GPS : PORT_MODE_ENV;
                switchPortMode(newMode);
                nvs_save_int(NVS_KEY_PORT_MODE, portMode);
                drawPortModeNotice();
                delay(2000);
                if (portMode == PORT_MODE_ENV) {
                    // switchPortMode() ha già chiamato readSensors()
                    lastWeatherTime = 0;  // Forza TX meteo immediato (include posizione sticky)
                }
                // In GPS mode: SmartBeacon invia automaticamente al primo fix valido
            }
            done = true;
        }
        if (M5.BtnEXT.wasPressed()) {
            buzzer_play_event(BUZZ_PAGE);
            done = true;
        }
        delay(50);
    }
    lastDisplayTime = millis();
    updateDisplay();
}

// ============================================================================
// Lat/Lon -> Maidenhead
// ============================================================================
String latlon_to_maidenhead(float lat, float lon) {
    lon += 180.0f; lat += 90.0f;
    char loc[11];
    loc[0] = 'A' + (int)(lon / 20.0f);
    loc[1] = 'A' + (int)(lat / 10.0f);
    float lon2 = fmod(lon, 20.0f);
    float lat2 = fmod(lat, 10.0f);
    loc[2] = '0' + (int)(lon2 / 2.0f);
    loc[3] = '0' + (int)(lat2);
    float lon3 = fmod(lon2, 2.0f);
    float lat3 = fmod(lat2, 1.0f);
    loc[4] = 'a' + (int)(lon3 / (2.0f / 24.0f));
    loc[5] = 'a' + (int)(lat3 / (1.0f / 24.0f));
    float lon4 = fmod(lon3, 2.0f / 24.0f);
    float lat4 = fmod(lat3, 1.0f / 24.0f);
    loc[6] = '0' + (int)(lon4 / (2.0f / 240.0f));
    loc[7] = '0' + (int)(lat4 / (1.0f / 240.0f));
    float lon5 = fmod(lon4, 2.0f / 240.0f);
    float lat5 = fmod(lat4, 1.0f / 240.0f);
    loc[8] = 'a' + (int)(lon5 / (2.0f / 5760.0f));
    loc[9] = 'a' + (int)(lat5 / (1.0f / 5760.0f));
    loc[10] = '\0';
    return String(loc);
}

// ============================================================================
// DISPLAY MULTI-PAGINA
// ============================================================================
void updateDisplay() {
    drawPage(currentPage);
}

void drawPage(int page) {
    const StationProfile& prof = profiles[activeProfile];
    char buf[48];

#if defined(BOARD_COREINK)
    M5.M5Ink.clear();
    mainSprite.clear();

    // Header: nominativo | pagina | versione (tutte le pag.) + ora (solo pag.1, riga 2)
    snprintf(buf, sizeof(buf), "%s-%d", rtCallsign, rtSsidAprs);
    mainSprite.drawString(2, 0, buf, &fonts::AsciiFont8x16);
    snprintf(buf, sizeof(buf), "%d/%d", page + 1, NUM_PAGES);
    mainSprite.drawString(90, 0, buf, &fonts::AsciiFont8x16);
    snprintf(buf, sizeof(buf), "v%s", FW_VERSION);
    int vx = 200 - (strlen(buf) * 8);
    mainSprite.drawString(vx, 0, buf, &fonts::AsciiFont8x16);
    if (page == 0) {  // ora in top-right riga 3 (altezza umidità), x diverso → no overlap
        struct tm tiHdr;
        char tbuf[8] = "--:--";
        if (getLocalTime(&tiHdr)) snprintf(tbuf, sizeof(tbuf), "%02d:%02d", tiHdr.tm_hour, tiHdr.tm_min);
        int tx = 200 - (strlen(tbuf) * 8);
        mainSprite.drawString(tx, 38, tbuf, &fonts::AsciiFont8x16);
    }

    switch (page) {

    case 0: { // === Principale: meteo + posizione ===
        snprintf(buf, sizeof(buf), "Temp: %.1f C", temperature);
        mainSprite.drawString(5, 20, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Umid: %.1f %%", humidity);
        mainSprite.drawString(5, 38, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Press: %.1f mbar", pressure);  // BUG-05
        mainSprite.drawString(5, 56, buf, &fonts::AsciiFont8x16);
        {   // BUG-07: warning visivo batteria
            const char* bw = batVoltage < BAT_WARN_THRESHOLD_V ? " !!!" : (batVoltage < BAT_LOW_THRESHOLD_V ? " LOW" : "");
            snprintf(buf, sizeof(buf), "Bat: %.2fV%s", batVoltage, bw);
        }
        mainSprite.drawString(5, 78, buf, &fonts::AsciiFont8x16);

        // Coordinate alternanti
        if (portMode == PORT_MODE_GPS && gpsFixValid) {
            if (coordShowMaidenhead) {
                String mh = latlon_to_maidenhead(gpsLat, gpsLon);
                snprintf(buf, sizeof(buf), "Loc: %s (GPS)", mh.c_str());
            } else {
                snprintf(buf, sizeof(buf), "%.5f, %.5f", gpsLat, gpsLon);
            }
        } else {
            float lat, lon;
            if (coordShowMaidenhead || !maidenhead_to_latlon(rtLocator, lat, lon)) {
                snprintf(buf, sizeof(buf), "Loc: %s", rtLocator);
            } else {
                snprintf(buf, sizeof(buf), "%.5f, %.5f", lat, lon);
            }
        }
        mainSprite.drawString(5, 100, buf, &fonts::AsciiFont8x16);

        if (portMode == PORT_MODE_GPS && gpsFixValid) {
            snprintf(buf, sizeof(buf), "Alt:%.0fm Spd:%.1fkm/h", gpsAlt, gpsSpeed);
            mainSprite.drawString(5, 118, buf, &fonts::AsciiFont8x16);
#if GPS_EXTRA_ENABLED
            snprintf(buf, sizeof(buf), "Sat:%d HDOP:%.1f", gpsSatellites, gpsExtra.getHdop());
#else
            snprintf(buf, sizeof(buf), "Sat:%d HDOP:%.1f", gpsSatellites, gps.hdop.value() / 100.0f);
#endif
            mainSprite.drawString(5, 136, buf, &fonts::AsciiFont8x16);
        }

        if (wifiEnabled && WiFi.status() == WL_CONNECTED) {
            snprintf(buf, sizeof(buf), "IP:%s", WiFi.localIP().toString().c_str());
        } else {
            snprintf(buf, sizeof(buf), "WiFi: %s", wifiEnabled ? "no conn" : "OFF");
        }
        mainSprite.drawString(5, 154, buf, &fonts::AsciiFont8x16);

        snprintf(buf, sizeof(buf), "TX:%s %s", lastTxTime, lastTxOk ? "OK" : "FAIL");
        mainSprite.drawString(5, 172, buf, &fonts::AsciiFont8x16);
        break;
    }

    case 1: { // === GPS Dettaglio ===
        mainSprite.drawString(5, 20, "=== GPS Dettaglio ===", &fonts::AsciiFont8x16);
        if (portMode != PORT_MODE_GPS) {
            mainSprite.drawString(5, 60, "GPS non attivo", &fonts::AsciiFont8x16);
            mainSprite.drawString(5, 80, "Vai a pag.7,", &fonts::AsciiFont8x16);
            mainSprite.drawString(5, 100, "premi MID per GPS", &fonts::AsciiFont8x16);  // BUG-08
            break;
        }
#if GPS_EXTRA_ENABLED
        snprintf(buf, sizeof(buf), "Fix: %s  Q:%d", gpsFixValid ? "SI" : "NO", gpsExtra.getFixQuality());
        mainSprite.drawString(5, 40, buf, &fonts::AsciiFont8x16);
        {
            int bdsSats = 0;
            const SatInfo* si = gpsExtra.getSatInfo();
            int sc = gpsExtra.getSatCount();
            for (int j = 0; j < sc; j++) if (si[j].constellation == SAT_BEIDOU) bdsSats++;
            snprintf(buf, sizeof(buf), "V:%d U:%d BDS:%d", gpsExtra.getSatsInView(), gpsExtra.getSatsTracked(), bdsSats);
        }
        mainSprite.drawString(5, 58, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "HDOP:%.1f PDOP:%.1f", gpsExtra.getHdop(), gpsExtra.getPdop());
        mainSprite.drawString(5, 76, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Alt: %.1f m", gpsExtra.getAltitude());
        mainSprite.drawString(5, 94, buf, &fonts::AsciiFont8x16);
#else
        snprintf(buf, sizeof(buf), "Fix: %s  Sat:%d", gpsFixValid ? "SI" : "NO", gpsSatellites);
        mainSprite.drawString(5, 40, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Alt:%.0fm  HDOP:%.1f", gpsAlt, gps.hdop.value() / 100.0f);
        mainSprite.drawString(5, 58, buf, &fonts::AsciiFont8x16);
#endif
        snprintf(buf, sizeof(buf), "Vel: %.1f km/h", gpsSpeed);
        mainSprite.drawString(5, 112, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Rotta: %.0f deg", gpsCourse);
        mainSprite.drawString(5, 130, buf, &fonts::AsciiFont8x16);
        if (gpsFixValid) {
            snprintf(buf, sizeof(buf), "%.6f, %.6f", gpsLat, gpsLon);
            mainSprite.drawString(5, 150, buf, &fonts::AsciiFont8x16);
            String mh = latlon_to_maidenhead(gpsLat, gpsLon);
            snprintf(buf, sizeof(buf), "Loc: %s", mh.c_str());
            mainSprite.drawString(5, 168, buf, &fonts::AsciiFont8x16);
        }
        break;
    }

    case 2: { // === SNR Satelliti (barre) ===
#if GPS_EXTRA_ENABLED
        mainSprite.drawString(5, 20, "=== Segnale SAT ===", &fonts::AsciiFont8x16);
        if (portMode != PORT_MODE_GPS) {
            mainSprite.drawString(5, 60, "GPS non attivo", &fonts::AsciiFont8x16);
            break;
        }
        const SatInfo* sats = gpsExtra.getSatInfo();
        int n = gpsExtra.getSatCount();
        int y = 38;
        for (int i = 0; i < n && i < 10; i++) {
            char pre = (sats[i].constellation == SAT_BEIDOU) ? 'B' :
                       (sats[i].constellation == SAT_GLONASS) ? 'G' : ' ';
            snprintf(buf, sizeof(buf), "%c%02d", pre, sats[i].prn);
            mainSprite.drawString(2, y, buf, &fonts::AsciiFont8x16);
            int barW = sats[i].snr * 2;
            if (barW > 120) barW = 120;
            if (barW > 0) mainSprite.fillRect(35, y + 2, barW, 10);
            snprintf(buf, sizeof(buf), "%d", sats[i].snr);
            mainSprite.drawString(160, y, buf, &fonts::AsciiFont8x16);
            y += 15;
        }
        if (n == 0) mainSprite.drawString(5, 60, "Nessun satellite", &fonts::AsciiFont8x16);
#else
        mainSprite.drawString(5, 20, "=== Stato ===", &fonts::AsciiFont8x16);
        {   // BUG-07: warning batteria
            const char* bw2 = batVoltage < BAT_WARN_THRESHOLD_V ? " !!!" : (batVoltage < BAT_LOW_THRESHOLD_V ? " LOW" : "");
            snprintf(buf, sizeof(buf), "Vbat: %.2fV%s", batVoltage, bw2);
        }
        mainSprite.drawString(5, 42, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "WiFi: %s", WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : "no conn");
        mainSprite.drawString(5, 62, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "IP: %s", WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "---");
        mainSprite.drawString(5, 80, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "RSSI: %d dBm", WiFi.RSSI());
        mainSprite.drawString(5, 98, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "TX: %s %s", lastTxTime, lastTxOk ? "OK" : "FAIL");  // BUG-09
        mainSprite.drawString(5, 116, buf, &fonts::AsciiFont8x16);
#endif
        break;
    }

    case 3: { // === Profili + parametri NVS ===
        mainSprite.drawString(5, 20, "=== Profili ===", &fonts::AsciiFont8x16);
        for (int i = 0; i < NUM_PROFILES; i++) {
            char ck[16], sk[16], lk[16];
            snprintf(ck, sizeof(ck), "call_%d", i);
            snprintf(sk, sizeof(sk), "ssid_%d", i);
            snprintf(lk, sizeof(lk), "lbl_%d",  i);
            String call = nvs_load_string(ck, "");
            if (call.length() == 0) call = nvs_load_string(NVS_KEY_CALLSIGN, profiles[i].callsign);
            int ssid = nvs_load_int(sk, nvs_load_int(NVS_KEY_SSID_APRS, profiles[i].ssid));
            String lbl = nvs_load_string(lk, profiles[i].label);
            snprintf(buf, sizeof(buf), "%s %s-%d %s",
                     i == activeProfile ? ">" : " ",
                     call.c_str(), ssid, lbl.c_str());
            mainSprite.drawString(5, 40 + i * 18, buf, &fonts::AsciiFont8x16);
        }
        mainSprite.drawString(5, 100, "--- NVS attivi ---", &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Loc: %s", rtLocator);
        mainSprite.drawString(5, 118, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Sym: %c%c", rtSymbolTable, rtSymbolCode);
        mainSprite.drawString(5, 136, buf, &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 154, "MID 5s: configura", &fonts::AsciiFont8x16);
        break;
    }

    case 4: { // === WiFi ===
        mainSprite.drawString(5, 20, "=== WiFi ===", &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Stato: %s", wifiEnabled ? "ON" : "OFF");
        mainSprite.drawString(5, 40, buf, &fonts::AsciiFont8x16);
        if (wifiEnabled && WiFi.status() == WL_CONNECTED) {
            snprintf(buf, sizeof(buf), "SSID: %s", WiFi.SSID().c_str());
            mainSprite.drawString(5, 58, buf, &fonts::AsciiFont8x16);
            snprintf(buf, sizeof(buf), "IP: %s", WiFi.localIP().toString().c_str());
            mainSprite.drawString(5, 76, buf, &fonts::AsciiFont8x16);
            snprintf(buf, sizeof(buf), "RSSI: %d dBm", WiFi.RSSI());
            mainSprite.drawString(5, 94, buf, &fonts::AsciiFont8x16);
            snprintf(buf, sizeof(buf), "mDNS: %s.local", OTA_HOSTNAME);
            mainSprite.drawString(5, 112, buf, &fonts::AsciiFont8x16);
#if OTA_ENABLED
            snprintf(buf, sizeof(buf), "OTA: :%d/update", OTA_WEB_PORT);
            mainSprite.drawString(5, 130, buf, &fonts::AsciiFont8x16);
#if DATA_LOGGER_ENABLED
            snprintf(buf, sizeof(buf), "CSV: :%d/data", OTA_WEB_PORT);
#else
            snprintf(buf, sizeof(buf), "CFG: :%d/config", OTA_WEB_PORT);
#endif
            mainSprite.drawString(5, 148, buf, &fonts::AsciiFont8x16);
#endif
        } else if (wifiEnabled) {
            mainSprite.drawString(5, 58, "Non connesso", &fonts::AsciiFont8x16);
        }
        mainSprite.drawString(5, 172, "MID 3s: WiFiManager", &fonts::AsciiFont8x16);
        break;
    }

    case 5: { // === Bluetooth ===
        mainSprite.drawString(5, 20, "=== Bluetooth ===", &fonts::AsciiFont8x16);
#if BLE_OTA_ENABLED
        mainSprite.drawString(5, 40, "BLE OTA: Attivo", &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Device: %s", BLE_DEVICE_NAME);
        mainSprite.drawString(5, 58, buf, &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 80, "Usa nRF Connect per", &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 98, "update firmware BLE", &fonts::AsciiFont8x16);
#else
        mainSprite.drawString(5, 40, "BLE OTA: Off", &fonts::AsciiFont8x16);
#endif
        break;
    }

    case 6: { // === Meteo avanzato ===
        mainSprite.drawString(5, 20, "=== Meteo ===", &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Temp: %.1f C", temperature);
        mainSprite.drawString(5, 40, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Umid: %.1f %%", humidity);
        mainSprite.drawString(5, 58, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Pres: %.1f hPa", pressure);
        mainSprite.drawString(5, 76, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Porta: %s", portMode == PORT_MODE_ENV ? "ENV III" : "GPS");
        mainSprite.drawString(5, 100, buf, &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 150, "MID: menu", &fonts::AsciiFont8x16);  // BUG-10
        mainSprite.drawString(5, 168, "EXT 3s: emergenza", &fonts::AsciiFont8x16);
        break;
    }

    case 7: { // === Astro: alba/tramonto + luna ===
        mainSprite.drawString(5, 20, "=== Astro ===", &fonts::AsciiFont8x16);
        struct tm ti;
        if (!getLocalTime(&ti)) {
            mainSprite.drawString(5, 60, "NTP non sincronizzato", &fonts::AsciiFont8x16);
            break;
        }
        time_t rawtime = time(nullptr);
        struct tm utc;
        gmtime_r(&rawtime, &utc);
        float tzOff = (float)(ti.tm_hour - utc.tm_hour);
        if (tzOff < -12) tzOff += 24;
        if (tzOff > 12) tzOff -= 24;

        float lat, lon;
        if (portMode == PORT_MODE_GPS && gpsFixValid) {
            lat = gpsLat; lon = gpsLon;
        } else {
            maidenhead_to_latlon(rtLocator, lat, lon);
        }

        SunTimes sun = astro_sun_times(ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, lat, lon, tzOff);
        int moonP = astro_moon_phase(ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday);

        snprintf(buf, sizeof(buf), "Alba:  %02d:%02d", sun.sunriseHour, sun.sunriseMin);
        mainSprite.drawString(5, 42, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Tram:  %02d:%02d", sun.sunsetHour, sun.sunsetMin);
        mainSprite.drawString(5, 60, buf, &fonts::AsciiFont8x16);
        int dh = sun.dayLengthMin / 60, dm = sun.dayLengthMin % 60;
        snprintf(buf, sizeof(buf), "Giorno: %dh %dm", dh, dm);
        mainSprite.drawString(5, 80, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Luna: %s %c", astro_moon_name(moonP), astro_moon_icon(moonP));
        mainSprite.drawString(5, 102, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Fase: %d/29", moonP);
        mainSprite.drawString(5, 120, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Data: %02d/%02d/%04d", ti.tm_mday, ti.tm_mon + 1, ti.tm_year + 1900);
        mainSprite.drawString(5, 145, buf, &fonts::AsciiFont8x16);
        break;
    }

    case 8: { // === OpenWeatherMap ===
        mainSprite.drawString(5, 20, "=== Meteo OWM ===", &fonts::AsciiFont8x16);
        if (!owm_is_configured()) {
            mainSprite.drawString(5, 60, "API key non config.", &fonts::AsciiFont8x16);
            mainSprite.drawString(5, 80, "Usa /config per", &fonts::AsciiFont8x16);
            mainSprite.drawString(5, 98, "inserire owm_key", &fonts::AsciiFont8x16);
        } else {
            const OwmData& owm = owm_get_data();
            if (!owm.valid) {
                mainSprite.drawString(5, 60, "In attesa dati...", &fonts::AsciiFont8x16);
            } else {
                snprintf(buf, sizeof(buf), "Temp: %.1f C (%.1f)", owm.temp, owm.feels_like);
                mainSprite.drawString(5, 38, buf, &fonts::AsciiFont8x16);
                snprintf(buf, sizeof(buf), "Umid: %.0f %%", owm.humidity);
                mainSprite.drawString(5, 54, buf, &fonts::AsciiFont8x16);
                snprintf(buf, sizeof(buf), "Press: %.0f hPa", owm.pressure);
                mainSprite.drawString(5, 70, buf, &fonts::AsciiFont8x16);
                snprintf(buf, sizeof(buf), "Cond: %s", owm.description);
                mainSprite.drawString(5, 86, buf, &fonts::AsciiFont8x16);
                snprintf(buf, sizeof(buf), "Vento: %.1f-%.1f m/s", owm.wind_speed, owm.wind_gust > 0 ? owm.wind_gust : owm.wind_speed);
                mainSprite.drawString(5, 102, buf, &fonts::AsciiFont8x16);
                snprintf(buf, sizeof(buf), "Nubi: %.0f %%", owm.clouds);
                mainSprite.drawString(5, 118, buf, &fonts::AsciiFont8x16);
                snprintf(buf, sizeof(buf), "Pioggia 3h: %.1f mm", owm.rain_3h);
                mainSprite.drawString(5, 134, buf, &fonts::AsciiFont8x16);
                int agoMin = (millis() - owm_last_success()) / 60000;
                snprintf(buf, sizeof(buf), "Agg: %d min fa", agoMin);
                mainSprite.drawString(5, 150, buf, &fonts::AsciiFont8x16);
            }
        }
        {
            int upMin = (millis() - uptimeStart) / 60000;
            snprintf(buf, sizeof(buf), "Up: %dh %dm", upMin / 60, upMin % 60);
            mainSprite.drawString(5, 168, buf, &fonts::AsciiFont8x16);
        }
        break;
    }

    case 9: { // === Previsioni OWM ===
        mainSprite.drawString(5, 18, "-- Previsioni --", &fonts::AsciiFont8x16);
        if (!owm_is_configured()) {
            mainSprite.drawString(5, 60, "API key non config.", &fonts::AsciiFont8x16);
        } else if (!owm_forecast_valid()) {
            mainSprite.drawString(5, 60, "In attesa dati...", &fonts::AsciiFont8x16);
        } else {
            const OwmForecastSlot* fc = owm_get_forecast();
            int y = 34;
            // Oggi +3h, +6h, +9h
            for (int i = 0; i < 3; i++) {
                if (fc[i].valid) {
                    snprintf(buf, sizeof(buf), "Oggi %02d:00", fc[i].hour);
                    mainSprite.drawString(5, y, buf, &fonts::AsciiFont8x16);
                    y += 16;
                    snprintf(buf, sizeof(buf), " %.0f-%.0fC %s", fc[i].temp_min, fc[i].temp_max, fc[i].description);
                    buf[25] = '\0';
                    mainSprite.drawString(5, y, buf, &fonts::AsciiFont8x16);
                    y += 16;
                }
            }
            // Domani matt e pom
            for (int i = 3; i < 5; i++) {
                if (fc[i].valid) {
                    snprintf(buf, sizeof(buf), "Dom. %s", i == 3 ? "mattina" : "pomeriggio");
                    mainSprite.drawString(5, y, buf, &fonts::AsciiFont8x16);
                    y += 16;
                    snprintf(buf, sizeof(buf), " %.0f-%.0fC %s", fc[i].temp_min, fc[i].temp_max, fc[i].description);
                    buf[25] = '\0';
                    mainSprite.drawString(5, y, buf, &fonts::AsciiFont8x16);
                    y += 16;
                }
            }
        }
        break;
    }

    } // switch

    mainSprite.pushSprite();

#elif defined(BOARD_STICKCPLUS2)
    auto &lcd = StickCP2.Display;
    lcd.fillScreen(BLACK);
    lcd.setTextColor(WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(5, 2);
    snprintf(buf, sizeof(buf), "%s-%d %s %d/%d",
             rtCallsign, rtSsidAprs,
             portMode == PORT_MODE_GPS ? "GPS" : "ENV",
             page + 1, NUM_PAGES);
    lcd.print(buf);

    switch (page) {
    case 0:
        lcd.setCursor(5, 20);
        snprintf(buf, sizeof(buf), "T:%.1fC H:%.1f%% P:%.0f", temperature, humidity, pressure);
        lcd.print(buf);
        lcd.setCursor(5, 38);
        snprintf(buf, sizeof(buf), "Bat:%.2fV", batVoltage);
        lcd.print(buf);
        lcd.setCursor(5, 56);
        if (portMode == PORT_MODE_GPS && gpsFixValid) {
            if (coordShowMaidenhead) {
                lcd.print(latlon_to_maidenhead(gpsLat, gpsLon));
            } else {
                snprintf(buf, sizeof(buf), "%.5f,%.5f", gpsLat, gpsLon);
                lcd.print(buf);
            }
        } else {
            float lat, lon;
            if (coordShowMaidenhead || !maidenhead_to_latlon(rtLocator, lat, lon)) {
                snprintf(buf, sizeof(buf), "Loc:%s", rtLocator);
            } else {
                snprintf(buf, sizeof(buf), "%.5f,%.5f", lat, lon);
            }
            lcd.print(buf);
        }
        lcd.setCursor(5, 74);
        if (wifiEnabled && WiFi.status() == WL_CONNECTED) {
            lcd.print(WiFi.localIP().toString());
        } else {
            lcd.print(wifiEnabled ? "No WiFi" : "WiFi OFF");
        }
        lcd.setCursor(5, 92);
        snprintf(buf, sizeof(buf), "TX:%s %s %c%c", lastTxTime, lastTxOk ? "OK" : "FAIL", rtSymbolTable, rtSymbolCode);
        lcd.print(buf);
        break;
    default:
        lcd.setCursor(5, 40);
        lcd.print("Pagina non impl.");
        break;
    }
#endif
}

// ============================================================================
// APRS: Pacchetto meteo
// ============================================================================
void sendWeatherPacket() {
    // Skip se ENV selezionato ma sensore scollegato o non inizializzato
    if (portMode == PORT_MODE_GPS) return;  // Dati ENV non validi in modalita GPS
    // B6 fix: skip se pressione=0 (QMP6988 scollegato/errore) — dato meteo APRS non valido
    if (portMode == PORT_MODE_ENV && pressure == 0.0f) {
        Serial.println("[APRS-WX] Skip: pressione=0, ENV scollegato o in errore");
        return;
    }
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) { lastTxOk = false; return; }
    }
    const StationProfile& prof = profiles[activeProfile];
    String packet;
    if (portMode == PORT_MODE_GPS && gpsFixValid) {
        packet = aprs_build_weather_packet(rtCallsign, rtSsidAprs,
            gpsLat, gpsLon, rtSymbolTable, rtSymbolCode,
            temperature, humidity, pressure);
    } else if (lastGpsPosValid) {
        // FIX: dopo commuta GPS->ENV usa ultima posizione GPS nota
        packet = aprs_build_weather_packet(rtCallsign, rtSsidAprs,
            lastGpsLat, lastGpsLon, rtSymbolTable, rtSymbolCode,
            temperature, humidity, pressure);
    } else {
        packet = aprs_build_weather_packet(rtCallsign, rtSsidAprs,
            rtLocator, rtSymbolTable, rtSymbolCode,
            temperature, humidity, pressure);
    }
    Serial.printf("[APRS-WX] %s\n", packet.c_str());
    AprsIs txClient(APRS_SERVER, APRS_PORT, rtCallsign, rtSsidAprs, rtPasscode);
    if (txClient.connect()) {
        if (txClient.sendPacket(packet)) {
            lastTxOk = true;
            led_flash_tx();
            buzzer_play_event(BUZZ_TX_OK);
        } else {
            lastTxOk = false;
            buzzer_play_event(BUZZ_TX_FAIL);
        }
        txClient.disconnect();
    } else {
        lastTxOk = false;
        buzzer_play_event(BUZZ_TX_FAIL);
    }
    struct tm ti;
    if (getLocalTime(&ti)) {
        snprintf(lastTxTime, sizeof(lastTxTime), "%02d:%02d", ti.tm_hour, ti.tm_min);
    }
}

// ============================================================================
// APRS: Posizione
// ============================================================================
void sendPositionPacket() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) { lastTxOk = false; return; }
    }
    const StationProfile& prof = profiles[activeProfile];
    char comment[24];
    snprintf(comment, sizeof(comment), "Vbat=%.2fV", batVoltage);
    String packet;
    if (portMode == PORT_MODE_GPS && gpsFixValid) {
        packet = aprs_build_position_packet(rtCallsign, rtSsidAprs,
            gpsLat, gpsLon, rtSymbolTable, rtSymbolCode,
            comment);
        // BUG-03: filtro distanza indipendente dalla velocità (anti-jitter GPS)
        if (sbLastTxValid) {
            float dist = haversineM(sbLastTxLat, sbLastTxLon, gpsLat, gpsLon);
            if (dist < SB_MIN_DIST_M) {
                Serial.printf("[APRS-POS] Skip: dist=%.0fm < %dm (jitter)\n", dist, SB_MIN_DIST_M);
                return;
            }
        }
    } else if (lastGpsPosValid) {
        // FIX: dopo commuta GPS->ENV usa ultima posizione GPS nota
        packet = aprs_build_position_packet(rtCallsign, rtSsidAprs,
            lastGpsLat, lastGpsLon, rtSymbolTable, rtSymbolCode, comment);
    } else {
        packet = aprs_build_position_packet(rtCallsign, rtSsidAprs,
            rtLocator, rtSymbolTable, rtSymbolCode, comment);
    }
    Serial.printf("[APRS-POS] %s\n", packet.c_str());
    AprsIs txClient(APRS_SERVER, APRS_PORT, rtCallsign, rtSsidAprs, rtPasscode);
    if (txClient.connect()) {
        if (txClient.sendPacket(packet)) {
            lastTxOk = true;
            led_flash_tx();
        } else {
            lastTxOk = false;
        }
        // Aggiorna ultima posizione TX per filtro delta SmartBeacon
        if (portMode == PORT_MODE_GPS && gpsFixValid) {
            sbLastTxLat = gpsLat;
            sbLastTxLon = gpsLon;
            sbLastTxValid = true;
        }
        txClient.disconnect();
    } else {
        lastTxOk = false;
    }
    struct tm ti;
    if (getLocalTime(&ti))
        snprintf(lastTxTime, sizeof(lastTxTime), "%02d:%02d", ti.tm_hour, ti.tm_min);
}

// ============================================================================
// APRS: Telemetria
// ============================================================================
void sendTelemetry() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) return;
    }
    const StationProfile& prof = profiles[activeProfile];
    int sats = (portMode == PORT_MODE_GPS) ? gpsSatellites : 0;
    uint8_t bits = 0;
    if (portMode == PORT_MODE_GPS && gpsFixValid)   bits |= 0x80;  // GPS
    if (WiFi.status() == WL_CONNECTED)              bits |= 0x40;  // WiFi
    chargeState = detectChargeState();
    if (chargeState == CHG_CHARGING)                bits |= 0x20;  // Chg
    if (lastTxOk)                                   bits |= 0x10;  // TX
    if (pressure == 0.0f && portMode == PORT_MODE_ENV) bits |= 0x08; // Err: sensore ENV in errore
    // LoRa (bit 2, 0x04): sempre 0 fino a v2.0
    // R2, R3 (bit 1, 0): riservati
    String packet = aprs_build_telemetry_data(rtCallsign, rtSsidAprs,
        telemetrySeq, (int)(batVoltage * 1000), WiFi.RSSI(),
        (int)((millis() - uptimeStart) / 60000), sats, bits);
    Serial.printf("[APRS-TLM] %s\n", packet.c_str());
    AprsIs txClient(APRS_SERVER, APRS_PORT, rtCallsign, rtSsidAprs, rtPasscode);
    if (txClient.connect()) { txClient.sendPacket(packet); txClient.disconnect(); }
    telemetrySeq++;
}

void sendTelemetryDefinitions() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) return;
    }
    const StationProfile& prof = profiles[activeProfile];
    AprsIs txClient(APRS_SERVER, APRS_PORT, rtCallsign, rtSsidAprs, rtPasscode);
    if (txClient.connect()) {
        txClient.sendPacket(aprs_build_telemetry_parm(rtCallsign, rtSsidAprs));
        delay(500);
        txClient.sendPacket(aprs_build_telemetry_unit(rtCallsign, rtSsidAprs));
        delay(500);
        txClient.sendPacket(aprs_build_telemetry_eqns(rtCallsign, rtSsidAprs));
        txClient.disconnect();
        Serial.println("[APRS-TLM] Definizioni inviate");
    }
}

// ============================================================================
// APRS: Status packet (v1.3)
// ============================================================================
void sendStatusPacket() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) return;
    }
    const StationProfile& prof = profiles[activeProfile];
    char header[64];
    snprintf(header, sizeof(header), "%s-%d>APRS,TCPIP*:", rtCallsign, rtSsidAprs);
    String packet = String(header) + ">" + String(rtAprsStatus);
    Serial.printf("[APRS-STS] %s\n", packet.c_str());
    AprsIs txClient(APRS_SERVER, APRS_PORT, rtCallsign, rtSsidAprs, rtPasscode);
    if (txClient.connect()) { txClient.sendPacket(packet); txClient.disconnect(); }
}

// ============================================================================
// Selezione profilo al boot
// ============================================================================
void selectProfile() {
    drawProfileMenu();
    unsigned long start = millis();
    while (millis() - start < 5000) {  // 5s: mostra profilo, utente può cambiarlo
        M5.update();
#if defined(BOARD_COREINK)
        if (M5.BtnUP.wasPressed() || M5.BtnEXT.wasPressed()) {
            activeProfile = (activeProfile - 1 + NUM_PROFILES) % NUM_PROFILES;
            start = millis(); drawProfileMenu(); delay(200);
        }
        if (M5.BtnDOWN.wasPressed()) {
            activeProfile = (activeProfile + 1) % NUM_PROFILES;
            start = millis(); drawProfileMenu(); delay(200);
        }
        if (M5.BtnMID.wasPressed() || M5.BtnPWR.wasPressed()) {
            buzzer_play_event(BUZZ_CONFIRM); break;
        }
#elif defined(BOARD_STICKCPLUS2)
        if (M5.BtnA.wasClicked()) {
            activeProfile = (activeProfile + 1) % NUM_PROFILES;
            start = millis(); drawProfileMenu();
        }
        if (M5.BtnB.wasClicked()) break;
#endif
        delay(50);
    }
    nvs_save_int(NVS_KEY_PROFILE, activeProfile);
    // Ricarica variabili runtime dal profilo selezionato
    {
        char pkey[16];
        String s;
        snprintf(pkey, sizeof(pkey), "call_%d", activeProfile);
        s = nvs_load_string(pkey, "");
        if (s.length() == 0) s = nvs_load_string(NVS_KEY_CALLSIGN, profiles[activeProfile].callsign);
        strncpy(rtCallsign, s.c_str(), sizeof(rtCallsign)-1); rtCallsign[sizeof(rtCallsign)-1] = '\0';

        snprintf(pkey, sizeof(pkey), "pass_%d", activeProfile);
        s = nvs_load_string(pkey, "");
        if (s.length() == 0) s = nvs_load_string(NVS_KEY_PASSCODE, profiles[activeProfile].passcode);
        strncpy(rtPasscode, s.c_str(), sizeof(rtPasscode)-1); rtPasscode[sizeof(rtPasscode)-1] = '\0';

        snprintf(pkey, sizeof(pkey), "ssid_%d", activeProfile);
        rtSsidAprs = nvs_load_int(pkey, nvs_load_int(NVS_KEY_SSID_APRS, profiles[activeProfile].ssid));
    }
    Serial.printf("[PROFILO] %s-%d\n", rtCallsign, rtSsidAprs);
}

void drawProfileMenu() {
    char buf[40];
#if defined(BOARD_COREINK)
    M5.M5Ink.clear();
    mainSprite.clear();
    mainSprite.drawString(5, 5, "Seleziona profilo:", &fonts::AsciiFont8x16);
    for (int i = 0; i < NUM_PROFILES; i++) {
        char call_key[16], ssid_key[16], lbl_key[16];
        snprintf(call_key, sizeof(call_key), "call_%d", i);
        snprintf(ssid_key, sizeof(ssid_key), "ssid_%d", i);
        snprintf(lbl_key,  sizeof(lbl_key),  "lbl_%d",  i);
        String call = nvs_load_string(call_key, "");
        if (call.length() == 0) call = nvs_load_string(NVS_KEY_CALLSIGN, profiles[i].callsign);
        int ssid = nvs_load_int(ssid_key, nvs_load_int(NVS_KEY_SSID_APRS, profiles[i].ssid));
        String lbl = nvs_load_string(lbl_key, profiles[i].label);
        snprintf(buf, sizeof(buf), "%s %s-%d %s",
                 i == activeProfile ? ">" : " ",
                 call.c_str(), ssid, lbl.c_str());
        mainSprite.drawString(5, 28 + i * 22, buf, &fonts::AsciiFont8x16);
    }
    mainSprite.drawString(5, 105, "SU/GIU: scegli", &fonts::AsciiFont8x16);
    mainSprite.drawString(5, 125, "MID: conferma", &fonts::AsciiFont8x16);
    snprintf(buf, sizeof(buf), "Porta: %s",
             nvs_load_int(NVS_KEY_PORT_MODE, PORT_MODE_DEFAULT) == PORT_MODE_ENV ? "ENV III" : "GPS");
    mainSprite.drawString(5, 155, buf, &fonts::AsciiFont8x16);
    mainSprite.drawString(5, 175, "EXT: cambia porta", &fonts::AsciiFont8x16);
    mainSprite.pushSprite();
#elif defined(BOARD_STICKCPLUS2)
    auto &lcd = StickCP2.Display;
    lcd.fillScreen(BLACK);
    lcd.setTextColor(WHITE);
    lcd.setCursor(5, 5);
    lcd.println("Seleziona profilo:");
    for (int i = 0; i < NUM_PROFILES; i++) {
        snprintf(buf, sizeof(buf), "%s %s-%d", i == activeProfile ? ">" : " ",
                 profiles[i].callsign, profiles[i].ssid);
        lcd.println(buf);
    }
    lcd.println("\nBtnA:cambia BtnB:ok");
#endif
}

// ============================================================================
// Data Logger
// ============================================================================
#if DATA_LOGGER_ENABLED
void writeLogRecord() {
    LogRecord rec;
    memset(&rec, 0, sizeof(rec));
    rec.timestamp = (uint32_t)time(nullptr);
    rec.temperature = (int16_t)(temperature * 10);
    rec.humidity = (uint16_t)(humidity * 10);
    rec.pressure = (uint16_t)((pressure - 900.0f) * 10);
    rec.battery = (uint16_t)(batVoltage * 1000);
    if (portMode == PORT_MODE_GPS && gpsFixValid) {
        rec.latitude = (int32_t)(gpsLat * 1e6);
        rec.longitude = (int32_t)(gpsLon * 1e6);
        rec.altitude = (int16_t)gpsAlt;
        rec.speed = (uint16_t)(gpsSpeed * 10);
        rec.course = (uint16_t)(gpsCourse * 10);
        rec.quality = (uint8_t)gpsSatellites;
        rec.flags |= 0x01;
    } else {
        float lat, lon;
        maidenhead_to_latlon(rtLocator, lat, lon);
        rec.latitude = (int32_t)(lat * 1e6);
        rec.longitude = (int32_t)(lon * 1e6);
    }
    if (wifiEnabled && WiFi.status() == WL_CONNECTED) rec.flags |= 0x02;
    if (lastTxOk) rec.flags |= 0x08;
    logger_write_record(rec);
    Serial.printf("[LOG] Record #%u\n", logger_get_count());
}
#endif // DATA_LOGGER_ENABLED

// ============================================================================
// OTA
// ============================================================================
#if OTA_ENABLED
static const char OTA_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head><title>CoreInk-Meteo OTA</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>body{font-family:sans-serif;max-width:400px;margin:40px auto;padding:20px;text-align:center}
h2{color:#333}input[type=file]{margin:20px 0}
button{background:#0078d4;color:#fff;border:none;padding:12px 24px;font-size:16px;cursor:pointer;border-radius:4px}
button:hover{background:#005a9e}a{display:block;margin-top:15px;color:#0078d4}</style></head>
<body><h2>CoreInk-Meteo v)rawhtml" FW_VERSION R"rawhtml(</h2>
<h3>Aggiornamento Firmware</h3>
<form method="POST" action="/update" enctype="multipart/form-data">
<input type="file" name="firmware" accept=".bin"><br>
<button type="submit">Carica Firmware</button></form>
<a href="/config">Configurazione APRS</a>
)rawhtml"
#if DATA_LOGGER_ENABLED
R"rawhtml(<a href="/data">Scarica dati CSV</a>)rawhtml"
#endif
R"rawhtml(
</body></html>
)rawhtml";

static const char CONFIG_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head><title>CoreInk-Meteo Config</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>body{font-family:sans-serif;max-width:540px;margin:16px auto;padding:16px}
h2{color:#333;margin-bottom:4px}
h3{color:#555;border-top:1px solid #ccc;padding-top:6px;margin-top:14px;margin-bottom:4px}
label{display:block;font-weight:bold;margin-top:6px;font-size:12px}
input{width:100%;padding:4px;margin:2px 0;box-sizing:border-box;font-size:13px}
.row{display:flex;gap:8px}.row input{flex:1}
button{background:#0078d4;color:#fff;border:none;padding:10px 20px;font-size:15px;cursor:pointer;border-radius:4px;margin-top:12px;width:100%}
a{display:block;margin-top:8px;color:#0078d4;font-size:13px}
.hint{font-size:11px;color:#777;font-weight:normal}</style></head>
<body><h2>CoreInk-Meteo v)rawhtml" FW_VERSION R"rawhtml( &mdash; Config</h2>
<form method="POST" action="/config">

<h3>Reti WiFi <span class="hint">(prova nell'ordine 1&#8594;2&#8594;3)</span></h3>
<label>AP 1 &#8212; SSID</label><input name="w1s" value="%W1S%" maxlength="32">
<label>AP 1 &#8212; Password</label><input name="w1p" value="%W1P%" maxlength="63">
<label>AP 2 &#8212; SSID</label><input name="w2s" value="%W2S%" maxlength="32">
<label>AP 2 &#8212; Password</label><input name="w2p" value="%W2P%" maxlength="63">
<label>AP 3 &#8212; SSID</label><input name="w3s" value="%W3S%" maxlength="32">
<label>AP 3 &#8212; Password</label><input name="w3p" value="%W3P%" maxlength="63">

<h3>Profilo attivo <span class="hint">(0=Meteo casa 1=Mobile 2=Remota)</span></h3>
<label>Profilo selezionato (0-2)</label><input name="active_prof" value="%ACT%" maxlength="1">

<h3>Profilo 0</h3>
<label>Label</label><input name="lbl0" value="%L0%" maxlength="16">
<div class="row">
<div><label>Callsign</label><input name="c0" value="%C0%" maxlength="10"></div>
<div><label>SSID APRS</label><input name="a0" value="%A0%" maxlength="3"></div>
<div><label>Passcode</label><input name="k0" value="%K0%" maxlength="6"></div>
</div>

<h3>Profilo 1</h3>
<label>Label</label><input name="lbl1" value="%L1%" maxlength="16">
<div class="row">
<div><label>Callsign</label><input name="c1" value="%C1%" maxlength="10"></div>
<div><label>SSID APRS</label><input name="a1" value="%A1%" maxlength="3"></div>
<div><label>Passcode</label><input name="k1" value="%K1%" maxlength="6"></div>
</div>

<h3>Profilo 2</h3>
<label>Label</label><input name="lbl2" value="%L2%" maxlength="16">
<div class="row">
<div><label>Callsign</label><input name="c2" value="%C2%" maxlength="10"></div>
<div><label>SSID APRS</label><input name="a2" value="%A2%" maxlength="3"></div>
<div><label>Passcode</label><input name="k2" value="%K2%" maxlength="6"></div>
</div>

<h3>Posizione e APRS</h3>
<label>Locatore Maidenhead <span class="hint">(es. IM99tl55)</span></label><input name="locator" value="%LOC%" maxlength="10">
<label>Simbolo APRS <span class="hint">(2 char: /_ =WX  /- =Casa  // =Auto)</span></label><input name="symbol" value="%SYM%" maxlength="3">
<label>Messaggio Status APRS</label><input name="status" value="%STA%" maxlength="60">

<h3>Display e suoni</h3>
<div class="row">
<div><label>Volume buzzer (0-100)</label><input name="buzz_vol" value="%VOL%" maxlength="4"></div>
<div><label>Melodia boot (0-5)</label><input name="melody" value="%MEL%" maxlength="2"></div>
<div><label>Refresh display (sec)</label><input name="disp_ref" value="%DREF%" maxlength="4"></div>
</div>

<h3>Intervalli TX</h3>
<div class="row">
<div><label>Meteo (minuti)</label><input name="wx_intv" value="%WX%" maxlength="3"></div>
<div><label>Status (minuti)</label><input name="st_intv" value="%ST%" maxlength="4"></div>
</div>

<h3>OpenWeatherMap <span class="hint">(gratis: openweathermap.org/appid)</span></h3>
<label>API Key <span class="hint">(32 char hex, lascia vuoto per disabilitare)</span></label><input name="owm_key" value="%OWM%" maxlength="48">

<button type="submit">&#128190; Salva configurazione</button>
</form>
<a href="/update">&#8593; Aggiorna firmware (OTA)</a>
)rawhtml"
#if DATA_LOGGER_ENABLED
R"rawhtml(<a href="/data">Dati CSV</a>)rawhtml"
#endif
R"rawhtml(
</body></html>
)rawhtml";

void setupOTA() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    if (strlen(OTA_PASSWORD) > 0) ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.onStart([]() { Serial.println("[OTA] Inizio..."); });
    ArduinoOTA.onEnd([]() { Serial.println("\n[OTA] OK!"); });
    ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
        Serial.printf("[OTA] %u%%\r", (p * 100) / t);
    });
    ArduinoOTA.onError([](ota_error_t e) { Serial.printf("[OTA] Err %u\n", e); });
    ArduinoOTA.begin();
    Serial.printf("[OTA] Pronto: %s (%s)\n", OTA_HOSTNAME, WiFi.localIP().toString().c_str());

    otaServer.on("/update", HTTP_GET, []() { otaServer.send(200, "text/html", OTA_HTML); });
    otaServer.on("/update", HTTP_POST, []() {
        otaServer.sendHeader("Connection", "close");
        otaServer.send(Update.hasError() ? 500 : 200, "text/plain",
                       Update.hasError() ? "ERRORE!" : "OK! Riavvio...");
        if (!Update.hasError()) { delay(1000); ESP.restart(); }
    }, []() {
        HTTPUpload& u = otaServer.upload();
        if (u.status == UPLOAD_FILE_START) {
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
        } else if (u.status == UPLOAD_FILE_WRITE) {
            if (Update.write(u.buf, u.currentSize) != u.currentSize) Update.printError(Serial);
        } else if (u.status == UPLOAD_FILE_END) {
            if (Update.end(true)) Serial.printf("[WebOTA] OK %u byte\n", u.totalSize);
            else Update.printError(Serial);
        }
    });
    otaServer.on("/config", HTTP_GET, []() {
        String html = String(CONFIG_HTML);
        // WiFi APs
        html.replace("%W1S%", nvs_load_string(NVS_KEY_WIFI_SSID,  ""));
        html.replace("%W1P%", nvs_load_string(NVS_KEY_WIFI_PASS,  ""));
        html.replace("%W2S%", nvs_load_string(NVS_KEY_WIFI2_SSID, ""));
        html.replace("%W2P%", nvs_load_string(NVS_KEY_WIFI2_PASS, ""));
        html.replace("%W3S%", nvs_load_string(NVS_KEY_WIFI3_SSID, ""));
        html.replace("%W3P%", nvs_load_string(NVS_KEY_WIFI3_PASS, ""));
        // Profilo attivo
        html.replace("%ACT%", String(activeProfile));
        // Dati per-profilo 0/1/2
        for (int i = 0; i < NUM_PROFILES; i++) {
            char ck[16], pk[16], sk[16], lk[16];
            snprintf(ck, sizeof(ck), "call_%d", i);
            snprintf(pk, sizeof(pk), "pass_%d", i);
            snprintf(sk, sizeof(sk), "ssid_%d", i);
            snprintf(lk, sizeof(lk), "lbl_%d",  i);
            String call = nvs_load_string(ck, "");
            if (call.length() == 0) call = (i == activeProfile) ? String(rtCallsign)
                                         : nvs_load_string(NVS_KEY_CALLSIGN, profiles[i].callsign);
            String pass = nvs_load_string(pk, "");
            if (pass.length() == 0 && i == activeProfile) pass = String(rtPasscode);
            int ssid = nvs_load_int(sk, (i == activeProfile) ? rtSsidAprs : profiles[i].ssid);
            String lbl = nvs_load_string(lk, profiles[i].label);
            String ci = String(i);
            html.replace("%C" + ci + "%", call);
            html.replace("%K" + ci + "%", pass);
            html.replace("%A" + ci + "%", String(ssid));
            html.replace("%L" + ci + "%", lbl);
        }
        // Globale
        char sym[3]; snprintf(sym, sizeof(sym), "%c%c", rtSymbolTable, rtSymbolCode);
        html.replace("%LOC%",  rtLocator);
        html.replace("%SYM%",  sym);
        html.replace("%STA%",  rtAprsStatus);
        html.replace("%VOL%",  String(nvs_load_int(NVS_KEY_BUZZER_VOLUME, BUZZER_DEFAULT_VOLUME)));
        html.replace("%MEL%",  String(nvs_load_int(NVS_KEY_BOOT_MELODY, 2)));
        html.replace("%DREF%", String((int)(rtDisplayUpdateMs / 1000)));
        html.replace("%WX%",   String((int)(rtWeatherIntervalMs / 60000)));
        html.replace("%ST%",   String((int)(rtStatusIntervalMs / 60000)));
        html.replace("%OWM%",  nvs_load_string(NVS_KEY_OWM_KEY, ""));
        otaServer.send(200, "text/html; charset=utf-8", html);
    });
    otaServer.on("/config", HTTP_POST, []() {
        // WiFi APs
        auto saveWifi = [](const char* sk, const char* pk, const char* an, const char* pn) {
            if (otaServer.hasArg(an)) { String v = otaServer.arg(an); if (v.length()>0) nvs_save_string(sk, v.c_str()); }
            if (otaServer.hasArg(pn)) { String v = otaServer.arg(pn); if (v.length()>0) nvs_save_string(pk, v.c_str()); }
        };
        saveWifi(NVS_KEY_WIFI_SSID,  NVS_KEY_WIFI_PASS,  "w1s", "w1p");
        saveWifi(NVS_KEY_WIFI2_SSID, NVS_KEY_WIFI2_PASS, "w2s", "w2p");
        saveWifi(NVS_KEY_WIFI3_SSID, NVS_KEY_WIFI3_PASS, "w3s", "w3p");
        // Profilo attivo
        if (otaServer.hasArg("active_prof")) {
            int p = otaServer.arg("active_prof").toInt();
            if (p >= 0 && p < NUM_PROFILES) { activeProfile = p; nvs_save_int(NVS_KEY_PROFILE, p); }
        }
        // Dati per-profilo 0/1/2
        for (int i = 0; i < NUM_PROFILES; i++) {
            char ck[16], pk[16], sk[16], lk[16];
            snprintf(ck, sizeof(ck), "call_%d", i);
            snprintf(pk, sizeof(pk), "pass_%d", i);
            snprintf(sk, sizeof(sk), "ssid_%d", i);
            snprintf(lk, sizeof(lk), "lbl_%d",  i);
            String ci = String(i);
            if (otaServer.hasArg("c" + ci) && otaServer.arg("c" + ci).length() > 0)
                nvs_save_string(ck, otaServer.arg("c" + ci).c_str());
            if (otaServer.hasArg("k" + ci) && otaServer.arg("k" + ci).length() > 0)
                nvs_save_string(pk, otaServer.arg("k" + ci).c_str());
            if (otaServer.hasArg("a" + ci)) {
                int s = otaServer.arg("a" + ci).toInt();
                if (s >= 0 && s <= 15) nvs_save_int(sk, s);
            }
            if (otaServer.hasArg("lbl" + ci) && otaServer.arg("lbl" + ci).length() > 0)
                nvs_save_string(lk, otaServer.arg("lbl" + ci).c_str());
        }
        // Ricarica variabili runtime per il profilo attivo
        {
            char pkey[16];
            snprintf(pkey, sizeof(pkey), "call_%d", activeProfile);
            String s = nvs_load_string(pkey, "");
            if (s.length() == 0) s = nvs_load_string(NVS_KEY_CALLSIGN, profiles[activeProfile].callsign);
            strncpy(rtCallsign, s.c_str(), sizeof(rtCallsign)-1); rtCallsign[sizeof(rtCallsign)-1]='\0';

            snprintf(pkey, sizeof(pkey), "pass_%d", activeProfile);
            s = nvs_load_string(pkey, "");
            if (s.length() == 0) s = nvs_load_string(NVS_KEY_PASSCODE, profiles[activeProfile].passcode);
            strncpy(rtPasscode, s.c_str(), sizeof(rtPasscode)-1); rtPasscode[sizeof(rtPasscode)-1]='\0';

            snprintf(pkey, sizeof(pkey), "ssid_%d", activeProfile);
            rtSsidAprs = nvs_load_int(pkey, nvs_load_int(NVS_KEY_SSID_APRS, profiles[activeProfile].ssid));
        }
        // Globale
        if (otaServer.hasArg("locator") && otaServer.arg("locator").length() > 0) {
            String v = otaServer.arg("locator");
            nvs_save_string(NVS_KEY_LOCATOR, v.c_str());
            strncpy(rtLocator, v.c_str(), sizeof(rtLocator)-1); rtLocator[sizeof(rtLocator)-1]='\0';
        }
        if (otaServer.hasArg("symbol") && otaServer.arg("symbol").length() >= 2) {
            String v = otaServer.arg("symbol");
            nvs_save_string(NVS_KEY_SYMBOL, v.c_str());
            rtSymbolTable = v.charAt(0); rtSymbolCode = v.charAt(1);
        }
        if (otaServer.hasArg("status") && otaServer.arg("status").length() > 0) {
            String v = otaServer.arg("status");
            nvs_save_string(NVS_KEY_APRS_STATUS, v.c_str());
            strncpy(rtAprsStatus, v.c_str(), sizeof(rtAprsStatus)-1); rtAprsStatus[sizeof(rtAprsStatus)-1]='\0';
        }
        if (otaServer.hasArg("buzz_vol")) { int v = otaServer.arg("buzz_vol").toInt(); if (v>=0&&v<=100) { nvs_save_int(NVS_KEY_BUZZER_VOLUME,v); buzzer_set_volume(v); } }
        if (otaServer.hasArg("melody"))   { int v = otaServer.arg("melody").toInt();   if (v>=0&&v<=5)   nvs_save_int(NVS_KEY_BOOT_MELODY,v); }
        if (otaServer.hasArg("disp_ref")) { int v = otaServer.arg("disp_ref").toInt(); if (v>=10&&v<=300){ nvs_save_int(NVS_KEY_DISPLAY_REFRESH,v); rtDisplayUpdateMs=v*1000UL; } }
        if (otaServer.hasArg("wx_intv"))  { int v = otaServer.arg("wx_intv").toInt();  if (v>=1&&v<=60)  { nvs_save_int(NVS_KEY_WEATHER_INTERVAL,v); rtWeatherIntervalMs=v*60000UL; } }
        if (otaServer.hasArg("st_intv"))  { int v = otaServer.arg("st_intv").toInt();  if (v>=10&&v<=180){ nvs_save_int(NVS_KEY_STATUS_INTERVAL,v); rtStatusIntervalMs=v*60000UL; } }
        // OWM API key
        if (otaServer.hasArg("owm_key")) {
            String v = otaServer.arg("owm_key");
            v.trim();
            nvs_save_string(NVS_KEY_OWM_KEY, v.c_str());
            // Re-init OWM con la nuova key
            owm_init();
        }
        Serial.printf("[Config] Salvato profilo %d: call=%s ssid=%d loc=%s sym=%c%c\n",
                      activeProfile, rtCallsign, rtSsidAprs, rtLocator, rtSymbolTable, rtSymbolCode);
        otaServer.send(200, "text/html; charset=utf-8",
            "<html><head><meta charset='utf-8'><meta http-equiv='refresh' content='2;url=/config'></head>"
            "<body><h3>&#10003; Salvato!</h3><a href='/config'>Torna</a></body></html>");
    });
    otaServer.on("/data", HTTP_GET, []() {
#if DATA_LOGGER_ENABLED
        logger_serve_csv(&otaServer);
#else
        otaServer.send(200, "text/plain", "Data logger non disponibile in questa build");
#endif
    });
    otaServer.begin();
    Serial.printf("[Web] http://%s:%d/update | /config | /data\n", WiFi.localIP().toString().c_str(), OTA_WEB_PORT);
}
#endif
