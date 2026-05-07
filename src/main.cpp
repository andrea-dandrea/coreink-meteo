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
#include <WiFiMulti.h>
#include <WiFiManager.h>
#include <time.h>

#if defined(BOARD_COREINK)
  #include <esp_adc_cal.h>
#endif

#include "config.h"
#include "M5UnitENV.h"
#include "aprs.h"
#include "aprs_is.h"
#include "aprs_telemetry.h"
#include "nvs_config.h"
#include "smartbeacon.h"

#if GPS_ENABLED
#include <TinyGPSPlus.h>
#endif

#if BLE_ENABLED
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#endif

// === Hardware ===
#if defined(BOARD_COREINK)
  #define LED_PIN 10                // LED verde integrato nel CoreInk
#elif defined(BOARD_STICKCPLUS2)
  #define LED_PIN -1                // StickCPlus2 non ha LED visibile utente
#endif

// === Display ===
#if defined(BOARD_COREINK)
  Ink_Sprite mainSprite(&M5.M5Ink);
#endif
// StickCPlus2 usa M5.Display (M5GFX) direttamente

// === WiFi Multi ===
WiFiMulti wifiMulti;

// === Sensori ENV III (SHT30 + QMP6988) ===
SHT3X sht3x;
QMP6988 qmp;

// === Profilo attivo ===
int activeProfile = 0;

// === GPS (opzionale) ===
#if GPS_ENABLED
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
bool gpsFixValid = false;
float gpsLat = 0.0;
float gpsLon = 0.0;
float gpsSpeed = 0.0;            // km/h
float gpsCourse = 0.0;           // gradi
int gpsSatellites = 0;

#if SMARTBEACON_ENABLED
SmartBeacon smartBeacon(SB_FAST_SPEED, SB_FAST_RATE,
                        SB_SLOW_SPEED, SB_SLOW_RATE,
                        SB_TURN_MIN_ANGLE, SB_TURN_SLOPE, SB_TURN_TIME);
#endif
#endif

// === Variabili globali ===
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;            // hPa
float batVoltage = 0.0;          // Volt

unsigned long lastWeatherTime = 0;
unsigned long lastPositionTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastTelemetryTime = 0;
unsigned long lastTelemetryDefTime = 0;
unsigned long uptimeStart = 0;
int telemetrySeq = 0;

bool lastTxOk = false;
char lastTxTime[6] = "--:--";

// === Prototipi ===
void selectProfile();
void connectWiFi();
void startWiFiManager();
void readSensors();
float readBattery();
void updateDisplay();
void drawProfileMenu();
void sendWeatherPacket();
void sendPositionPacket();
void sendTelemetry();
void sendTelemetryDefinitions();
void syncTime();
void ledFlash(int ms);
#if GPS_ENABLED
void readGps();
#endif

void setup() {
#if defined(BOARD_COREINK)
    M5.begin();
    Serial.begin(115200);
#elif defined(BOARD_STICKCPLUS2)
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    Serial.begin(115200);
#endif
    Serial.println("\n[CoreInk-Meteo] Avvio v1.0...");

    // LED TX
#if defined(BOARD_COREINK)
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED spento (attivo basso)
#endif

    // Inizializzare NVS per configurazione persistente
    nvs_config_init();

    // Caricare profilo salvato da NVS
    activeProfile = nvs_load_int(NVS_KEY_PROFILE, 0);
    if (activeProfile >= NUM_PROFILES) activeProfile = 0;

    // Inizializzare I2C per i sensori (Port A: G32=SDA, G33=SCL)
    Wire.begin(32, 33);

    // Inizializzare SHT30 (temperatura e umidità)
    if (!sht3x.begin(&Wire, SHT3X_I2C_ADDR, 32, 33, 400000U)) {
        Serial.println("[SENSORE] SHT30 non trovato!");
    } else {
        Serial.println("[SENSORE] SHT30 OK (0x44)");
    }

    // Inizializzare QMP6988 (pressione atmosferica)
    if (!qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, 32, 33, 400000U)) {
        Serial.println("[SENSORE] QMP6988 non trovato!");
    } else {
        Serial.println("[SENSORE] QMP6988 OK (0x70)");
    }

#if GPS_ENABLED
    // Inizializzare porta seriale GPS (AT6558, 9600 baud)
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("[GPS] Seriale GPS inizializzata");
#endif

    // Creare sprite per il display
#if defined(BOARD_COREINK)
    mainSprite.creatSprite(0, 0, 200, 200);
#elif defined(BOARD_STICKCPLUS2)
    StickCP2.Display.setRotation(1);  // Orizzontale
    StickCP2.Display.setTextSize(1);
#endif

    // Selezione profilo al boot (con timeout di 5 secondi)
    selectProfile();

    // Connettere WiFi (con fallback a WiFiManager portale captive)
    WiFi.mode(WIFI_STA);
    connectWiFi();

    // Se WiFi non connesso, avviare portale captive
    if (WiFi.status() != WL_CONNECTED) {
        startWiFiManager();
    }

    // Sincronizzare l'ora tramite NTP
    syncTime();

#if BLE_ENABLED
    // Inizializzare BLE (coesiste con WiFi tramite time-division multiplexing)
    BLEDevice::init(BLE_DEVICE_NAME);
    Serial.printf("[BLE] Inizializzato: %s\n", BLE_DEVICE_NAME);
#endif

    // Prima lettura sensori e batteria
    readSensors();
    batVoltage = readBattery();
    updateDisplay();

    // Primo invio pacchetti
    sendWeatherPacket();
    sendPositionPacket();
    sendTelemetryDefinitions();

    uptimeStart = millis();
    lastWeatherTime = millis();
    lastPositionTime = millis();
    lastDisplayTime = millis();
    lastTelemetryTime = millis();
    lastTelemetryDefTime = millis();
}

void loop() {
    unsigned long now = millis();

#if GPS_ENABLED
    // Leggere dati GPS continuamente
    readGps();

#if SMARTBEACON_ENABLED
    // SmartBeaconing: decidere se inviare posizione in base a velocità/rotta
    if (gpsFixValid && smartBeacon.shouldBeacon(gpsSpeed, gpsCourse)) {
        sendPositionPacket();
        lastPositionTime = now;
    }
#endif
#endif

    // Invio weather report ogni SEND_INTERVAL_MS (10 min default)
    if (now - lastWeatherTime >= SEND_INTERVAL_MS) {
        readSensors();
        sendWeatherPacket();
        lastWeatherTime = now;
    }

    // Invio posizione (se non gestito da SmartBeaconing)
#if !(GPS_ENABLED && SMARTBEACON_ENABLED)
    if (now - lastPositionTime >= SEND_INTERVAL_MS) {
        sendPositionPacket();
        lastPositionTime = now;
    }
#endif

    // Invio telemetria ogni 10 minuti (insieme al meteo)
    if (now - lastTelemetryTime >= SEND_INTERVAL_MS) {
        batVoltage = readBattery();
        sendTelemetry();
        lastTelemetryTime = now;
    }

    // Invio definizioni telemetria PARM/UNIT/EQNS ogni 2 ore
    if (now - lastTelemetryDefTime >= TELEMETRY_DEFINITION_INTERVAL_MS) {
        sendTelemetryDefinitions();
        lastTelemetryDefTime = now;
    }

    // Aggiornare display periodicamente
    if (now - lastDisplayTime >= DISPLAY_UPDATE_MS) {
        readSensors();
        batVoltage = readBattery();
        updateDisplay();
        lastDisplayTime = now;
    }

    // Controllare pulsante lungo per WiFiManager
    M5.update();
#if defined(BOARD_COREINK)
    if (M5.BtnMID.pressedFor(3000)) {
        startWiFiManager();
    }
#elif defined(BOARD_STICKCPLUS2)
    if (M5.BtnA.pressedFor(3000)) {
        startWiFiManager();
    }
#endif

    delay(100);
}

// === Connessione WiFi Multi ===
void connectWiFi() {
    Serial.println("[WiFi] Connessione in corso...");

    for (int i = 0; i < NUM_WIFI_APS; i++) {
        wifiMulti.addAP(wifiNetworks[i].ssid, wifiNetworks[i].password);
    }

    unsigned long start = millis();
    while (wifiMulti.run() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connesso a: %s  IP: %s\n",
                      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WiFi] Connessione fallita!");
    }
}

// === Portale captive WiFiManager ===
void startWiFiManager() {
    Serial.println("[WiFiManager] Avvio portale captive...");

    // Mostrare info sul display
#if defined(BOARD_COREINK)
    M5.M5Ink.clear();
    mainSprite.clear();
    mainSprite.drawString(10, 30, "WiFi Setup", &fonts::AsciiFont8x16);
    mainSprite.drawString(10, 55, "Connettiti a:", &fonts::AsciiFont8x16);
    mainSprite.drawString(10, 80, WIFIMANAGER_AP_NAME, &fonts::AsciiFont8x16);
    mainSprite.drawString(10, 110, "poi vai a:", &fonts::AsciiFont8x16);
    mainSprite.drawString(10, 135, "192.168.4.1", &fonts::AsciiFont8x16);
    mainSprite.pushSprite();
#elif defined(BOARD_STICKCPLUS2)
    StickCP2.Display.fillScreen(BLACK);
    StickCP2.Display.setTextColor(WHITE);
    StickCP2.Display.setCursor(5, 10);
    StickCP2.Display.println("WiFi Setup");
    StickCP2.Display.println();
    StickCP2.Display.println("Connettiti a:");
    StickCP2.Display.println(WIFIMANAGER_AP_NAME);
    StickCP2.Display.println();
    StickCP2.Display.println("poi vai a:");
    StickCP2.Display.println("192.168.4.1");
#endif

    WiFiManager wm;
    wm.setConfigPortalTimeout(WIFIMANAGER_TIMEOUT);

    // Parametri personalizzati
    WiFiManagerParameter param_call("callsign", "Callsign", 
        nvs_load_string(NVS_KEY_CALLSIGN, profiles[activeProfile].callsign).c_str(), 10);
    WiFiManagerParameter param_pass("passcode", "APRS Passcode",
        nvs_load_string(NVS_KEY_PASSCODE, profiles[activeProfile].passcode).c_str(), 6);
    WiFiManagerParameter param_loc("locator", "Locatore Maidenhead",
        nvs_load_string(NVS_KEY_LOCATOR, STATION_LOCATOR).c_str(), 7);

    wm.addParameter(&param_call);
    wm.addParameter(&param_pass);
    wm.addParameter(&param_loc);

    if (wm.startConfigPortal(WIFIMANAGER_AP_NAME)) {
        Serial.println("[WiFiManager] Connesso!");

        // Salvare parametri in NVS se modificati
        if (strlen(param_call.getValue()) > 0) {
            nvs_save_string(NVS_KEY_CALLSIGN, param_call.getValue());
        }
        if (strlen(param_pass.getValue()) > 0) {
            nvs_save_string(NVS_KEY_PASSCODE, param_pass.getValue());
        }
        if (strlen(param_loc.getValue()) > 0) {
            nvs_save_string(NVS_KEY_LOCATOR, param_loc.getValue());
        }
    } else {
        Serial.println("[WiFiManager] Timeout portale");
    }
}

// === Sincronizzazione NTP ===
void syncTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("[NTP] Sincronizzazione ora...");

    struct tm timeinfo;
    int retries = 0;
    while (!getLocalTime(&timeinfo) && retries < 10) {
        delay(1000);
        retries++;
    }

    if (retries < 10) {
        Serial.println("[NTP] Ora sincronizzata");
    } else {
        Serial.println("[NTP] Sincronizzazione fallita");
    }
}

// === Lettura sensori ENV III ===
void readSensors() {
    if (sht3x.update()) {
        temperature = sht3x.cTemp;
        humidity = sht3x.humidity;
    }

    if (qmp.update()) {
        pressure = qmp.pressure / 100.0;  // Pa -> hPa
    }

    Serial.printf("[SENSORE] T=%.1f°C  H=%.1f%%  P=%.1fhPa\n",
                  temperature, humidity, pressure);
}

// === Lettura tensione batteria ===
float readBattery() {
#if defined(BOARD_COREINK)
    analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);
    esp_adc_cal_characteristics_t *adc_chars =
        (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                             3600, adc_chars);
    uint16_t adcValue = analogRead(BAT_ADC_PIN);
    uint32_t voltageMv = esp_adc_cal_raw_to_voltage(adcValue, adc_chars);
    float voltage = float(voltageMv) * 25.1 / 5.1 / 1000.0;
    free(adc_chars);

    Serial.printf("[BAT] %.2fV (ADC raw=%d)\n", voltage, adcValue);
    return voltage;
#elif defined(BOARD_STICKCPLUS2)
    float voltage = StickCP2.Power.getBatteryVoltage() / 1000.0;
    Serial.printf("[BAT] %.2fV\n", voltage);
    return voltage;
#endif
}

// === Aggiornamento display ===
void updateDisplay() {
    const StationProfile& prof = profiles[activeProfile];
    char buf[32];

#if defined(BOARD_COREINK)
    M5.M5Ink.clear();
    mainSprite.clear();

    // Riga 1: nominativo e stato
    snprintf(buf, sizeof(buf), "%s-%d", prof.callsign, prof.ssid);
    mainSprite.drawString(10, 5, buf, &fonts::AsciiFont8x16);

#if GPS_ENABLED
    mainSprite.drawString(150, 5, gpsFixValid ? "GPS" : "---", &fonts::AsciiFont8x16);
#endif

    snprintf(buf, sizeof(buf), "Temp: %.1f C", temperature);
    mainSprite.drawString(10, 30, buf, &fonts::AsciiFont8x16);

    snprintf(buf, sizeof(buf), "Umid: %.1f %%", humidity);
    mainSprite.drawString(10, 50, buf, &fonts::AsciiFont8x16);

    snprintf(buf, sizeof(buf), "Pres: %.1f hPa", pressure);
    mainSprite.drawString(10, 70, buf, &fonts::AsciiFont8x16);

    snprintf(buf, sizeof(buf), "Bat: %.2fV", batVoltage);
    mainSprite.drawString(10, 95, buf, &fonts::AsciiFont8x16);

    if (WiFi.status() == WL_CONNECTED) {
        snprintf(buf, sizeof(buf), "IP:%s", WiFi.localIP().toString().c_str());
    } else {
        snprintf(buf, sizeof(buf), "IP: non connesso");
    }
    mainSprite.drawString(10, 115, buf, &fonts::AsciiFont8x16);

#if GPS_ENABLED
    if (gpsFixValid) {
        snprintf(buf, sizeof(buf), "%.4f, %.4f", gpsLat, gpsLon);
    } else {
        snprintf(buf, sizeof(buf), "Loc: %s", STATION_LOCATOR);
    }
#else
    snprintf(buf, sizeof(buf), "Loc: %s", STATION_LOCATOR);
#endif
    mainSprite.drawString(10, 140, buf, &fonts::AsciiFont8x16);

    snprintf(buf, sizeof(buf), "TX: %s %s", lastTxTime, lastTxOk ? "OK" : "FAIL");
    mainSprite.drawString(10, 165, buf, &fonts::AsciiFont8x16);

    mainSprite.pushSprite();

#elif defined(BOARD_STICKCPLUS2)
    auto &lcd = StickCP2.Display;
    lcd.fillScreen(BLACK);
    lcd.setTextColor(WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(5, 5);

    snprintf(buf, sizeof(buf), "%s-%d", prof.callsign, prof.ssid);
    lcd.print(buf);
#if GPS_ENABLED
    lcd.setCursor(200, 5);
    lcd.print(gpsFixValid ? "GPS" : "---");
#endif

    lcd.setCursor(5, 25);
    snprintf(buf, sizeof(buf), "T:%.1fC  H:%.1f%%", temperature, humidity);
    lcd.print(buf);

    lcd.setCursor(5, 45);
    snprintf(buf, sizeof(buf), "P:%.1fhPa", pressure);
    lcd.print(buf);

    lcd.setCursor(5, 65);
    snprintf(buf, sizeof(buf), "Bat:%.2fV", batVoltage);
    lcd.print(buf);

    lcd.setCursor(5, 85);
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(buf, sizeof(buf), "%s", WiFi.localIP().toString().c_str());
    } else {
        snprintf(buf, sizeof(buf), "No WiFi");
    }
    lcd.print(buf);

    lcd.setCursor(5, 105);
#if GPS_ENABLED
    if (gpsFixValid) {
        snprintf(buf, sizeof(buf), "%.4f,%.4f", gpsLat, gpsLon);
    } else {
        snprintf(buf, sizeof(buf), "Loc:%s", STATION_LOCATOR);
    }
#else
    snprintf(buf, sizeof(buf), "Loc:%s", STATION_LOCATOR);
#endif
    lcd.print(buf);

    lcd.setCursor(5, 125);
    snprintf(buf, sizeof(buf), "TX:%s %s", lastTxTime, lastTxOk ? "OK" : "FAIL");
    lcd.print(buf);
#endif
}

// === Invio pacchetto meteo APRS ===
void sendWeatherPacket() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[APRS] Nessun WiFi, riconnessione...");
        if (wifiMulti.run(WIFI_TIMEOUT_MS) != WL_CONNECTED) {
            lastTxOk = false;
            return;
        }
    }

    const StationProfile& prof = profiles[activeProfile];
    String packet;

#if GPS_ENABLED
    if (gpsFixValid) {
        packet = aprs_build_weather_packet(
            prof.callsign, prof.ssid,
            gpsLat, gpsLon,
            prof.symbolTable, prof.symbolCode,
            temperature, humidity, pressure);
    } else {
        packet = aprs_build_weather_packet(
            prof.callsign, prof.ssid,
            STATION_LOCATOR,
            prof.symbolTable, prof.symbolCode,
            temperature, humidity, pressure);
    }
#else
    packet = aprs_build_weather_packet(
        prof.callsign, prof.ssid,
        STATION_LOCATOR,
        prof.symbolTable, prof.symbolCode,
        temperature, humidity, pressure);
#endif

    Serial.printf("[APRS-WX] Pacchetto: %s\n", packet.c_str());

    AprsIs txClient(APRS_SERVER, APRS_PORT, prof.callsign, prof.ssid, prof.passcode);
    if (txClient.connect()) {
        if (txClient.sendPacket(packet)) {
            lastTxOk = true;
            ledFlash(200);
            Serial.println("[APRS-WX] Inviato OK");
        } else {
            lastTxOk = false;
            Serial.println("[APRS-WX] Invio fallito");
        }
        txClient.disconnect();
    } else {
        lastTxOk = false;
    }

    // Salvare ora dell'ultimo TX
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        snprintf(lastTxTime, sizeof(lastTxTime), "%02d:%02d",
                 timeinfo.tm_hour, timeinfo.tm_min);
    }
}

// === Invio pacchetto posizione APRS (separato dal meteo) ===
void sendPositionPacket() {
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiMulti.run(WIFI_TIMEOUT_MS) != WL_CONNECTED) return;
    }

    const StationProfile& prof = profiles[activeProfile];
    String packet;

    // Commento con info batteria
    char comment[32];
    snprintf(comment, sizeof(comment), "Vbat=%.2fV", batVoltage);

#if GPS_ENABLED
    if (gpsFixValid) {
        packet = aprs_build_position_packet(
            prof.callsign, prof.ssid,
            gpsLat, gpsLon,
            prof.symbolTable, prof.symbolCode,
            comment);
    } else {
        packet = aprs_build_position_packet(
            prof.callsign, prof.ssid,
            STATION_LOCATOR,
            prof.symbolTable, prof.symbolCode,
            comment);
    }
#else
    packet = aprs_build_position_packet(
        prof.callsign, prof.ssid,
        STATION_LOCATOR,
        prof.symbolTable, prof.symbolCode,
        comment);
#endif

    Serial.printf("[APRS-POS] Pacchetto: %s\n", packet.c_str());

    AprsIs txClient(APRS_SERVER, APRS_PORT, prof.callsign, prof.ssid, prof.passcode);
    if (txClient.connect()) {
        txClient.sendPacket(packet);
        txClient.disconnect();
    }
}

// === Invio dati telemetria ===
void sendTelemetry() {
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiMulti.run(WIFI_TIMEOUT_MS) != WL_CONNECTED) return;
    }

    const StationProfile& prof = profiles[activeProfile];

    int batMv = (int)(batVoltage * 1000);
    int rssi = WiFi.RSSI();
    int uptimeMin = (int)((millis() - uptimeStart) / 60000);
    int sats = 0;
#if GPS_ENABLED
    sats = gpsSatellites;
#endif

    // Bit digitali: D1=GPS fix, D2=WiFi connesso, D3=in carica(TODO), D4=ultimo TX ok
    uint8_t bits = 0;
#if GPS_ENABLED
    if (gpsFixValid) bits |= 0x80;
#endif
    if (WiFi.status() == WL_CONNECTED) bits |= 0x40;
    if (lastTxOk) bits |= 0x10;

    String packet = aprs_build_telemetry_data(
        prof.callsign, prof.ssid,
        telemetrySeq, batMv, rssi, uptimeMin, sats, bits);

    Serial.printf("[APRS-TLM] Pacchetto: %s\n", packet.c_str());

    AprsIs txClient(APRS_SERVER, APRS_PORT, prof.callsign, prof.ssid, prof.passcode);
    if (txClient.connect()) {
        txClient.sendPacket(packet);
        txClient.disconnect();
    }

    telemetrySeq++;
}

// === Invio definizioni telemetria (PARM/UNIT/EQNS) ===
void sendTelemetryDefinitions() {
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiMulti.run(WIFI_TIMEOUT_MS) != WL_CONNECTED) return;
    }

    const StationProfile& prof = profiles[activeProfile];

    String parm = aprs_build_telemetry_parm(prof.callsign, prof.ssid);
    String unit = aprs_build_telemetry_unit(prof.callsign, prof.ssid);
    String eqns = aprs_build_telemetry_eqns(prof.callsign, prof.ssid);

    AprsIs txClient(APRS_SERVER, APRS_PORT, prof.callsign, prof.ssid, prof.passcode);
    if (txClient.connect()) {
        txClient.sendPacket(parm);
        delay(500);
        txClient.sendPacket(unit);
        delay(500);
        txClient.sendPacket(eqns);
        txClient.disconnect();
        Serial.println("[APRS-TLM] Definizioni PARM/UNIT/EQNS inviate");
    }
}

// === Selezione profilo al boot ===
void selectProfile() {
    drawProfileMenu();

    // Attesa selezione (timeout 5 secondi)
    unsigned long start = millis();
    while (millis() - start < 5000) {
        M5.update();

#if defined(BOARD_COREINK)
        if (M5.BtnUP.wasPressed()) {
            activeProfile = (activeProfile - 1 + NUM_PROFILES) % NUM_PROFILES;
            start = millis();
            drawProfileMenu();
        }
        if (M5.BtnDOWN.wasPressed()) {
            activeProfile = (activeProfile + 1) % NUM_PROFILES;
            start = millis();
            drawProfileMenu();
        }
        if (M5.BtnMID.wasPressed()) {
            break;
        }
#elif defined(BOARD_STICKCPLUS2)
        if (M5.BtnA.wasClicked()) {
            activeProfile = (activeProfile + 1) % NUM_PROFILES;
            start = millis();
            drawProfileMenu();
        }
        if (M5.BtnB.wasClicked()) {
            break;  // Conferma
        }
#endif

        delay(50);
    }

    // Salvare profilo selezionato in NVS
    nvs_save_int(NVS_KEY_PROFILE, activeProfile);

    Serial.printf("[PROFILO] Selezionato: %s-%d (%s)\n",
                  profiles[activeProfile].callsign,
                  profiles[activeProfile].ssid,
                  profiles[activeProfile].label);
}

// === Disegno menu profilo (helper per evitare duplicazione) ===
void drawProfileMenu() {
    char buf[32];

#if defined(BOARD_COREINK)
    M5.M5Ink.clear();
    mainSprite.clear();
    mainSprite.drawString(10, 5, "Seleziona profilo:", &fonts::AsciiFont8x16);

    for (int i = 0; i < NUM_PROFILES; i++) {
        snprintf(buf, sizeof(buf), "%s %s-%d",
                 (i == activeProfile) ? ">" : " ",
                 profiles[i].callsign, profiles[i].ssid);
        mainSprite.drawString(10, 30 + i * 25, buf, &fonts::AsciiFont8x16);
    }
    mainSprite.drawString(10, 120, "SU/GIU: scegli", &fonts::AsciiFont8x16);
    mainSprite.drawString(10, 140, "MID: conferma", &fonts::AsciiFont8x16);
    mainSprite.pushSprite();

#elif defined(BOARD_STICKCPLUS2)
    auto &lcd = StickCP2.Display;
    lcd.fillScreen(BLACK);
    lcd.setTextColor(WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(5, 5);
    lcd.println("Seleziona profilo:");

    for (int i = 0; i < NUM_PROFILES; i++) {
        snprintf(buf, sizeof(buf), "%s %s-%d",
                 (i == activeProfile) ? ">" : " ",
                 profiles[i].callsign, profiles[i].ssid);
        lcd.println(buf);
    }
    lcd.println();
    lcd.println("BtnA: cambia");
    lcd.println("BtnB: conferma");
#endif
}

// === LED flash per indicare TX ===
void ledFlash(int ms) {
#if defined(BOARD_COREINK)
    digitalWrite(LED_PIN, LOW);   // LED acceso (attivo basso)
    delay(ms);
    digitalWrite(LED_PIN, HIGH);  // LED spento
#elif defined(BOARD_STICKCPLUS2)
    // Nessun LED visibile, breve flash sullo schermo
    StickCP2.Display.fillCircle(230, 5, 4, GREEN);
    delay(ms);
    StickCP2.Display.fillCircle(230, 5, 4, BLACK);
#endif
}

#if GPS_ENABLED
void readGps() {
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }

    if (gps.location.isUpdated() && gps.location.isValid()) {
        gpsLat = gps.location.lat();
        gpsLon = gps.location.lng();
        gpsFixValid = true;
    }

    if (gps.speed.isUpdated()) {
        gpsSpeed = gps.speed.kmph();
    }

    if (gps.course.isUpdated()) {
        gpsCourse = gps.course.deg();
    }

    if (gps.satellites.isUpdated()) {
        gpsSatellites = gps.satellites.value();
    }
}
#endif
