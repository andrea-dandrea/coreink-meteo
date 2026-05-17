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
  #include <M5CoreInk.h>
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
unsigned long lastLogTime = 0;
unsigned long uptimeStart = 0;
int telemetrySeq = 0;

bool lastTxOk = false;
char lastTxTime[6] = "--:--";

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
String latlon_to_maidenhead(float lat, float lon);
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

    // Selezione profilo al boot
    selectProfile();

    // Caricare TUTTI i parametri da NVS
    {
        String s;
        s = nvs_load_string(NVS_KEY_CALLSIGN, profiles[activeProfile].callsign);
        strncpy(rtCallsign, s.c_str(), sizeof(rtCallsign) - 1);
        rtCallsign[sizeof(rtCallsign) - 1] = '\0';

        s = nvs_load_string(NVS_KEY_PASSCODE, profiles[activeProfile].passcode);
        strncpy(rtPasscode, s.c_str(), sizeof(rtPasscode) - 1);
        rtPasscode[sizeof(rtPasscode) - 1] = '\0';

        rtSsidAprs = nvs_load_int(NVS_KEY_SSID_APRS, profiles[activeProfile].ssid);

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

        // APRS Status
        s = nvs_load_string(NVS_KEY_APRS_STATUS, APRS_STATUS_DEFAULT);
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
        WiFi.mode(WIFI_STA);

        // DOWN premuto al boot = reset credenziali WiFi
        M5.update();
        if (M5.BtnDOWN.isPressed()) {
            Serial.println("[WiFi] Reset credenziali (DOWN premuto)");
            WiFiManager wmReset;
            wmReset.resetSettings();
            buzzer_play_event(BUZZ_CONFIRM);
        }

        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) {
            startWiFiManager();
        }
        if (WiFi.status() == WL_CONNECTED) {
            if (strcmp(rtCallsign, "NOCALL") != 0 && strcmp(rtPasscode, "-1") != 0) {
                led_set_state(LED_SOLID);
            } else {
                led_set_state(LED_SLOW);  // WiFi ok ma APRS non configurato
            }
            buzzer_play_event(BUZZ_WIFI_OK);
        }
    } else {
        WiFi.mode(WIFI_OFF);
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

    // Posizione: SmartBeacon se GPS, altrimenti non ripetere (è fissa)
    bool sbActive = false;
#if SMARTBEACON_ENABLED
    sbActive = (portMode == PORT_MODE_GPS);
#endif
    if (sbActive && (now - lastPositionTime >= WEATHER_INTERVAL_MS)) {
        if (canTx && gpsFixValid) sendPositionPacket();
        lastPositionTime = now;
    }

    // Telemetria ogni TELEMETRY_INTERVAL_MS (default 10 min)
    if (now - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
        batVoltage = readBattery();
        if (canTx) sendTelemetry();
        lastTelemetryTime = now;
    }

    // Definizioni telemetria
    if (now - lastTelemetryDefTime >= TELEMETRY_DEFINITION_INTERVAL_MS) {
        if (canTx) sendTelemetryDefinitions();
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

    // Display
    if (now - lastDisplayTime >= rtDisplayUpdateMs) {
        if (portMode == PORT_MODE_ENV) readSensors();
        batVoltage = readBattery();
        coordShowMaidenhead = !coordShowMaidenhead;
        updateDisplay();
        lastDisplayTime = now;
    }

    // === Pulsanti ===
    M5.update();
#if defined(BOARD_COREINK)
    if (M5.BtnMID.pressedFor(3000)) {
        startWiFiManager();
    }
    if (M5.BtnUP.wasPressed()) {
        currentPage = (currentPage - 1 + NUM_PAGES) % NUM_PAGES;
        buzzer_play_event(BUZZ_PAGE);
        updateDisplay();
    }
    if (M5.BtnDOWN.wasPressed()) {
        currentPage = (currentPage + 1) % NUM_PAGES;
        buzzer_play_event(BUZZ_PAGE);
        updateDisplay();
    }
    if (M5.BtnEXT.wasPressed()) {
        int newMode = (portMode == PORT_MODE_ENV) ? PORT_MODE_GPS : PORT_MODE_ENV;
        switchPortMode(newMode);
        nvs_save_int(NVS_KEY_PORT_MODE, portMode);
        buzzer_play_event(BUZZ_CONFIRM);
        drawPortModeNotice();
        delay(2000);
        updateDisplay();
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

    // Batteria bassa
    if (batVoltage > 0.5f && batVoltage < 3.3f) {
        led_set_state(LED_FAST);
        static unsigned long lastBatWarn = 0;
        if (now - lastBatWarn > 60000) {
            buzzer_play_event(BUZZ_BAT_LOW);
            lastBatWarn = now;
        }
    }

    // WiFi reconnect
    if (wifiEnabled && WiFi.status() != WL_CONNECTED) {
        led_set_state(LED_SLOW);
        static unsigned long lastReconnect = 0;
        if (now - lastReconnect > 60000) {
            connectWiFi();
            if (WiFi.status() == WL_CONNECTED) {
                if (strcmp(rtCallsign, "NOCALL") != 0 && strcmp(rtPasscode, "-1") != 0) {
                    led_set_state(LED_SOLID);
                } else {
                    led_set_state(LED_SLOW);
                }
                buzzer_play_event(BUZZ_WIFI_OK);
            }
            lastReconnect = now;
        }
    }

    delay(50);
}

// ============================================================================
// WIFI
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

    Serial.println("\n[WiFi] Provo reti config.h...");
    WiFi.disconnect();
    for (int i = 0; i < NUM_WIFI_APS; i++) {
        if (strlen(wifiNetworks[i].ssid) > 1 && strcmp(wifiNetworks[i].ssid, "RETE_1") != 0) {
            WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);
            start = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS / NUM_WIFI_APS) {
                delay(500);
                Serial.print(".");
            }
            if (WiFi.status() == WL_CONNECTED) {
                Serial.printf("\n[WiFi] Connesso: %s IP: %s\n",
                              WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
                return;
            }
            WiFi.disconnect();
        }
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
    wm.setConfigPortalTimeout(300);  // 5 minuti — sicuro contro watchdog
    wm.setConnectTimeout(10);        // Max 10s per tentativo di connessione
    wm.setBreakAfterConfig(true);    // Chiudi solo dopo salvataggio

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

    bool wifiOk = wm.startConfigPortal(WIFIMANAGER_AP_NAME);

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
        temperature = sht3x.cTemp;
        humidity = sht3x.humidity;
    }
    if (qmp.update()) {
        pressure = qmp.pressure / 100.0f;
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
        }
        Serial.printf("[PORT] ENV III (I2C) attivo, QMP6988: %s\n", qmpOk ? "OK" : "FAIL");
    } else if (newMode == PORT_MODE_GPS) {
        gpsFixValid = false;
        gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        Serial.println("[PORT] GPS (UART) attivo");
    }
    portMode = newMode;
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

    // Header: nominativo | pagina | versione
    snprintf(buf, sizeof(buf), "%s-%d", rtCallsign, rtSsidAprs);
    mainSprite.drawString(2, 0, buf, &fonts::AsciiFont8x16);
    snprintf(buf, sizeof(buf), "%d/%d", page + 1, NUM_PAGES);
    mainSprite.drawString(90, 0, buf, &fonts::AsciiFont8x16);
    snprintf(buf, sizeof(buf), "v%s", FW_VERSION);
    int vx = 200 - (strlen(buf) * 8);
    mainSprite.drawString(vx, 0, buf, &fonts::AsciiFont8x16);

    switch (page) {

    case 0: { // === Principale: meteo + posizione ===
        struct tm ti;
        if (getLocalTime(&ti)) {
            snprintf(buf, sizeof(buf), "Ora %02d:%02d", ti.tm_hour, ti.tm_min);
            mainSprite.drawString(2, 188, buf, &fonts::AsciiFont8x16);
        }
        snprintf(buf, sizeof(buf), "T: %.1f C", temperature);
        mainSprite.drawString(5, 20, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "H: %.1f %%", humidity);
        mainSprite.drawString(5, 38, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "P: %.1f hPa", pressure);
        mainSprite.drawString(5, 56, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Bat: %.2fV", batVoltage);
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
            snprintf(buf, sizeof(buf), "Sat:%d", gpsSatellites);
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
            mainSprite.drawString(5, 80, "Premi EXT per", &fonts::AsciiFont8x16);
            mainSprite.drawString(5, 100, "attivare modo GPS", &fonts::AsciiFont8x16);
            break;
        }
#if GPS_EXTRA_ENABLED
        snprintf(buf, sizeof(buf), "Fix: %s  Q:%d", gpsFixValid ? "SI" : "NO", gpsExtra.getFixQuality());
        mainSprite.drawString(5, 40, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Vista:%d  Uso:%d", gpsExtra.getSatsInView(), gpsExtra.getSatsTracked());
        mainSprite.drawString(5, 58, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "HDOP:%.1f PDOP:%.1f", gpsExtra.getHdop(), gpsExtra.getPdop());
        mainSprite.drawString(5, 76, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Alt: %.1f m", gpsExtra.getAltitude());
        mainSprite.drawString(5, 94, buf, &fonts::AsciiFont8x16);
#else
        snprintf(buf, sizeof(buf), "Fix: %s  Sat:%d", gpsFixValid ? "SI" : "NO", gpsSatellites);
        mainSprite.drawString(5, 40, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Alt: %.0f m", gpsAlt);
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
        mainSprite.drawString(5, 20, "=== GPS ===", &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Sat: %d  Fix: %s", gpsSatellites, gpsFixValid ? "SI" : "NO");
        mainSprite.drawString(5, 50, buf, &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 80, "(Dettaglio SAT non", &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 100, " disponibile)", &fonts::AsciiFont8x16);
#endif
        break;
    }

    case 3: { // === Profili + parametri NVS ===
        mainSprite.drawString(5, 20, "=== Profili ===", &fonts::AsciiFont8x16);
        for (int i = 0; i < NUM_PROFILES; i++) {
            snprintf(buf, sizeof(buf), "%s %s-%d %s",
                     i == activeProfile ? ">" : " ",
                     profiles[i].callsign, profiles[i].ssid, profiles[i].label);
            mainSprite.drawString(5, 40 + i * 18, buf, &fonts::AsciiFont8x16);
        }
        mainSprite.drawString(5, 100, "--- NVS attivi ---", &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Call: %s", rtCallsign);
        mainSprite.drawString(5, 118, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Pass: %s", rtPasscode);
        mainSprite.drawString(5, 136, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Loc: %s  Sym: %c%c", rtLocator, rtSymbolTable, rtSymbolCode);
        mainSprite.drawString(5, 154, buf, &fonts::AsciiFont8x16);
        mainSprite.drawString(5, 172, "MID 3s: configura", &fonts::AsciiFont8x16);
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
            snprintf(buf, sizeof(buf), "CSV: :%d/data", OTA_WEB_PORT);
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
        mainSprite.drawString(5, 150, "EXT: cambia ENV/GPS", &fonts::AsciiFont8x16);
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

    case 8: { // === Data Logger ===
#if DATA_LOGGER_ENABLED
        mainSprite.drawString(5, 20, "=== Recorder ===", &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Stato: %s", logger_is_enabled() ? "REC" : "STOP");
        mainSprite.drawString(5, 40, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Record: %u / %u", logger_get_count(), logger_get_capacity());
        mainSprite.drawString(5, 58, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Uso: %.1f %%", logger_get_usage_percent());
        mainSprite.drawString(5, 76, buf, &fonts::AsciiFont8x16);
        snprintf(buf, sizeof(buf), "Intervallo: %us", logger_get_interval());
        mainSprite.drawString(5, 94, buf, &fonts::AsciiFont8x16);
#if OTA_ENABLED
        if (wifiEnabled && WiFi.status() == WL_CONNECTED) {
            snprintf(buf, sizeof(buf), "CSV: :%d/data", OTA_WEB_PORT);
            mainSprite.drawString(5, 120, buf, &fonts::AsciiFont8x16);
        }
#endif
#else
        mainSprite.drawString(5, 20, "=== Info ===", &fonts::AsciiFont8x16);
#endif
        int upMin = (millis() - uptimeStart) / 60000;
        snprintf(buf, sizeof(buf), "Uptime: %dh %dm", upMin / 60, upMin % 60);
        mainSprite.drawString(5, 150, buf, &fonts::AsciiFont8x16);
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
        if (WiFi.status() != WL_CONNECTED) return;
    }
    const StationProfile& prof = profiles[activeProfile];
    char comment[48];
    snprintf(comment, sizeof(comment), "Vbat=%.2fV %s", batVoltage, FW_VERSION);
    String packet;
    if (portMode == PORT_MODE_GPS && gpsFixValid) {
        packet = aprs_build_position_packet(rtCallsign, rtSsidAprs,
            gpsLat, gpsLon, rtSymbolTable, rtSymbolCode, comment);
    } else {
        packet = aprs_build_position_packet(rtCallsign, rtSsidAprs,
            rtLocator, rtSymbolTable, rtSymbolCode, comment);
    }
    Serial.printf("[APRS-POS] %s\n", packet.c_str());
    AprsIs txClient(APRS_SERVER, APRS_PORT, rtCallsign, rtSsidAprs, rtPasscode);
    if (txClient.connect()) {
        txClient.sendPacket(packet);
        txClient.disconnect();
    }
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
    if (portMode == PORT_MODE_GPS && gpsFixValid) bits |= 0x80;
    if (WiFi.status() == WL_CONNECTED) bits |= 0x40;
    if (lastTxOk) bits |= 0x10;
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
    while (millis() - start < 5000) {
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
    Serial.printf("[PROFILO] %s-%d (%s)\n",
                  profiles[activeProfile].callsign, profiles[activeProfile].ssid,
                  profiles[activeProfile].label);
}

void drawProfileMenu() {
    char buf[40];
#if defined(BOARD_COREINK)
    M5.M5Ink.clear();
    mainSprite.clear();
    mainSprite.drawString(5, 5, "Seleziona profilo:", &fonts::AsciiFont8x16);
    for (int i = 0; i < NUM_PROFILES; i++) {
        snprintf(buf, sizeof(buf), "%s %s-%d %s",
                 i == activeProfile ? ">" : " ",
                 profiles[i].callsign, profiles[i].ssid, profiles[i].label);
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
<a href="/data">Scarica dati CSV</a>
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
    otaServer.on("/data", HTTP_GET, []() {
#if DATA_LOGGER_ENABLED
        logger_serve_csv(&otaServer);
#else
        otaServer.send(200, "text/plain", "Data logger non disponibile in questa build");
#endif
    });
    otaServer.begin();
    Serial.printf("[Web] http://%s:%d/update | /data\n", WiFi.localIP().toString().c_str(), OTA_WEB_PORT);
}
#endif
