# Roadmap - CoreInk Meteo

## Versioni rilasciate

| Versione | Descrizione | Data |
|----------|-------------|------|
| v0.1 | Prototipo iniziale | 2026-05-01 |
| v1.0 | Stazione meteo APRS completa | 2026-05-05 |
| v1.1 | OTA WiFi + BLE + WiFiManager | 2026-05-16 |
| v1.2 | Fix NVS callsign/locator, versione FW a schermo | 2026-05-17 |
| v1.2.5 | Refactor build LITE (partizione default.csv, senza BLE/logger/gps_extra) | 2026-05-17 |
| v1.2.6 | Fix WiFiManager (watchdog, salvataggio, timeout), display, LED, intervalli TX | 2026-05-17 |
| v1.2.7 | Fix locator Maidenhead 8-char, display 10-char, prima APRS-IS operativa | 2026-05-18 |
| v1.2.8 | Bug fixes post-campo (BUG-01..12), boot splash, WiFi round-robin e timeout 8 s | 2026-05-20 |
| v1.2.9 | Fix GPS→ENV sticky, meteo immediato, allarme batteria critica | 2026-05-20 |
| v1.3.0 | Port M5Unified, OWM current+forecast, web config /config, 10 pagine display | 2026-05-23 |

---

## v1.3.0 — Rilasciata (2026-05-23)

### Completato in questa versione

- [x] **Port a M5Unified 0.2.15** — sostituisce M5Core-Ink 1.0.0 deprecata
- [x] Shim di compatibilità `m5unified_compat.h` per transizione graduale
- [x] **OpenWeatherMap current weather** — temp, feels_like, humidity, pressure, wind, clouds, rain_3h
- [x] **OWM Forecast 5 slot** — +3h, +6h, +9h, domani mattina, domani pomeriggio
- [x] Parsing JSON manuale (senza ArduinoJson, risparmio flash)
- [x] **Pagina 9 OWM** — dati meteo correnti: Temp, Umid, Press, Cond, Vento(range), Nubi, Pioggia 3h
- [x] **Pagina 10 Forecast** — previsioni 2 righe per slot (titolo + min-max + descrizione)
- [x] **Web server `/config`** — form per chiave OWM API, salvata in NVS
- [x] `NUM_PAGES=10` via build flag
- [x] Build env `coreink_lite_m5u` in platformio.ini
- [x] Flash 98.6% (1,292,037 / 1,310,720 bytes) su partizione default.csv

### Non implementato (rinviato a versioni future)

- WiFi state machine a 6 stati (documento di design pronto, non implementata)
- Feedback sonoro buzzer (header pronto, non integrato nel loop)
- Screen map completa S0-S8 (solo S3 NAV attiva)
- Bit telemetria Chg/Err/LoRa (riservati, sempre 0)

---

## v1.4 — Prossima versione pianificata

### Priorità alta — Stabilità e partizione

- [ ] **Partizione custom 1.94 MB** — necessaria per future aggiunte (flash attuale al 98.6%)
- [ ] Verifica stabilità multi-giorno (uptime > 72h)
- [ ] Fix PARM/UNIT/EQNS duplicati dopo reboot (B5/W3)
- [ ] Skip invio weather se sensore ENV scollegato (B6)

### WiFi state machine

> Design in `docs/wifi_state_machine.md` e `docs/screen_map.md`

- [ ] FSM WiFi a 6 stati (`WIFI_ST_OFF` default al boot)
- [ ] Screen map S0→S8 con buzzer feedback
- [ ] Pagina S1 PROFILI accessibile da menu (non dal boot)

### Web server esteso

- [ ] Pagina `/config` completa: callsign, locator, SSID, passcode, intervalli TX
- [ ] Profili stazione editabili via web
- [ ] Accessibile in STA mode e SoftAP (192.168.4.1)

### Batteria e alimentazione

- [ ] `isOnUsb()`: soglia ADC `batVoltage > 4.4f`
- [ ] `detectChargeState()`: slope algorithm
- [ ] Bit telemetria Chg (0x20), Err (0x08)
- [ ] Shutdown critico: `M5.shutdown()` se `!isOnUsb() && batVoltage < 3.2V`

### Documentazione

- [ ] README in inglese (o bilingue)
- [ ] Traduzione manuale utente in inglese

### Bug aperti

| # | Problema | Priorità |
|---|----------|----------|
| B5 | PARM/UNIT/EQNS ripetuti a ogni reboot | Media |
| B6 | Skip invio weather se sensore ENV non aggiornato | Media |
| B7 | Commento posizione APRS ridondante | Bassa |
| B9 | Satelliti BeiDou non conteggiati nel display | Bassa |

---

## v2.0 — Espansione hardware, radio e connettività

Salto qualitativo dell'applicazione: nuovi hardware, nuove reti, nuove funzionalità
che trasformano la stazione in un nodo della rete APRS e delle reti meteo globali.

### Sensori esterni wireless RF433

- Ricezione dati da sensori meteo wireless 433 MHz (Oregon Scientific, Acurite, ecc.)
- Decodifica protocolli comuni (temperatura, umidità, pioggia esterna)
- Sensore effetto Hall per anemometro o pluviometro
- Hardware: M5Stack RF433R, Hall Effect Unit, Hub Grove 1→3

### iGate / Digipeater LoRa

- Modulo EBYTE E32-433T20D o E22-400M30S (UART su M5-Bus G13/G14)
- Inoltro a APRS-IS (iGate RX)
- Digipeater locale (ripetizione senza internet)
- Filtro pacchetti per area/nominativo

### Integrazione reti meteo alternative

- CWOP, Weather Underground, Windy.com, MQTT broker custom
- Selezione reti attive da web config
- Credenziali per ogni rete in NVS
- Invio simultaneo multi-rete con intervalli indipendenti

### Ordine di implementazione suggerito

1. RF433 — estende la funzione meteo principale, impatto hardware limitato
2. Multi-rete meteo — estende la connettività, solo software
3. LoRa iGate/Digipeater — maggiore complessità hardware

---

## Idee future (non pianificate)

- App BLE per configurazione da smartphone
- Sensori aggiuntivi I2C (UV, luminosità)
- Font selezionabile dall'utente (8/12/16/24 px)
- Deep sleep con wake-up RTC per risparmio estremo
- Dashboard web locale con grafici storici
- Integrazione MQTT in parallelo ad APRS
- Supporto multi-lingua interfaccia display


## Versioni rilasciate

| Versione | Descrizione | Data |
|----------|-------------|------|
| v0.1 | Prototipo iniziale | 2026-05-01 |
| v1.0 | Stazione meteo APRS completa | 2026-05-05 |
| v1.1 | OTA WiFi + BLE + WiFiManager | 2026-05-16 |
| v1.2 | Fix NVS callsign/locator, versione FW a schermo | 2026-05-17 |

### Bug noti v1.2
- Locator Maidenhead troncato a 7 caratteri (buffer troppo piccolo, max 8+ per locator esteso)
- Lettura batteria fluttuante (singolo campione ADC, rumore ESP32)
- BLE OTA non visibile: il blocco BLE OTA era annidato dentro BLE_ENABLED=0, mai inizializzato

---

## v1.3 — Navigazione, Display Multi-Pagina e Data Logger ← PROSSIMO

Versione maggiore: sistema di navigazione completo con display paginato, registrazione dati,
informazioni astro-nautiche, e ottimizzazione firmware.

### Mod hardware HAT dual-sensor (prerequisito per v2.0 LoRa)

Cavo adattatore custom FPC 10-pin → 2×Grove che collega ENV III e GPS al connettore HAT J1,
liberando Grove Port A per il modulo LoRa UART in v2.0.

- ENV III (SHT30 + QMP6988) su **Wire1** (G25=SDA, G26=SCL)
- GPS (AT6558 NMEA) su **Serial2** (G36=RX, TX non connesso)
- Grove Port A (G32/G33) **libero** → riservato LoRa v2.0
- Rimozione completa di `portMode`, `switchPortMode()`, hot-swap e relativi menu
- Dettaglio tecnico: [hw\_configuration.md — Mod hardware v1.3](hw_configuration.md)

### Gestione energia e autonomia (deep sleep schedulato)

Il dispositivo attualmente gira in loop continuo (~50–80 mA con WiFi): autonomia ~6h su 390mAh.
Con deep sleep schedulato via BM8563 l'autonomia sale a ~48h o più.

- Architettura: wake → misura → TX → `M5.shutdown(SLEEP_INTERVAL_S)` → sleep (0,5mA)
- Intervallo sleep normale: 270s (4m50s sleep + ~10s attivo = 5 min ciclo completo)
- Integrazione GPS: wake, attendi fix (max 30s), TX, sleep
- Schermata e-ink "sleeping" opzionale prima dello shutdown (aggiorna immagine)
- NVS flag `deep_sleep_enabled` (default 0 = disabilitato, retrocompatibile)
- Prerequisito: rimozione loop continuo WiFi; FSM WiFi già implementata in v1.2.7

### Modalità solare notturna (solar night mode)

Uso tipico: piccolo pannello solare (1–5W) che ricarica di giorno ma non copre il consumo
notturno. Obiettivo: ridurre il consumo nelle ore buie affinché la batteria residua a fine
giornata arrivi all'alba successiva.

- NVS flag `solar_mode` (default 0 = disabilitato)
- NVS parametri `night_start_h` e `night_end_h` (default 22:00–07:00, ora locale BM8563)
- Durante ore notturne:
  - Intervallo sleep esteso: `SLEEP_INTERVAL_S × night_multiplier` (default ×6 → 30 min)
  - WiFi disabilitato (nessun TX APRS-IS, nessun reconnect)
  - Display aggiornato solo ogni N cicli (riduce consumo picco EPD ~26 mA)
  - LED spento
- Durante ore diurne: operatività normale (intervalli standard, WiFi on)
- Sorgente orario: BM8563 RTC (disponibile offline, già usato per `M5.shutdown()`)
- Dipendenza: `deep_sleep_enabled=1` (prerequisito — il timer RTC è il meccanismo di wake)
- Pagina display dedicata: icona luna/sole + stato modalità + prossima transizione
- NVS `night_multiplier` configurabile via web (`/config`) o joystick

### Bugfix da v1.2

- Fix BLE OTA: rendere indipendente da BLE_ENABLED (init BLE autonomo)
- Fix locator Maidenhead troncato: buffer da 8 a 11 caratteri + WiFiManager max length
- Fix lettura batteria: media mobile su N campioni ADC (N configurabile, default 5)
- Fix pressione QMP6988: verifica warmup sensore all'avvio

### Display multi-pagina (joystick SU/GIÙ = cambia pagina, MID = azione)

- Pagina 1 — Principale: meteo + posizione + altitudine + velocità + rotta + satelliti
- Pagina 2 — Satelliti GPS: dettaglio fix, HDOP/PDOP, costellazioni GPS+BDS
- Pagina 3 — Segnale satelliti: SNR per satellite (parser GSV custom), barre grafiche
- Pagina 4 — Profili: selezione profilo attivo, visualizzazione callsign/passcode/locator NVS
- Pagina 5 — WiFi: stato, AP connesso, RSSI, lista AP disponibili, connessione, toggle on/off
- Pagina 6 — Bluetooth: stato BLE on/off, toggle, dispositivo collegato, BLE OTA
- Pagina 7 — Meteo e Venti: previsione locale (OpenWeatherMap), vento, raffica, UV, trend pressione
- Pagina 8 — Astro e Maree: alba/tramonto, durata giorno, fase lunare, maree (WorldTides)
- Pagina 9 — Recorder: stato registrazione, n° record, spazio usato, download
- Indicatore numero pagina in alto al centro

### GPS completo (AT6558 + MAX2659)

- Tutti i dati NMEA: posizione, altitudine, velocità, rotta, satelliti, HDOP
- Parser GSV custom per SNR individuale per satellite (GPS + BeiDou)
- HDOP o n° satelliti selezionabile come campo qualità (configurabile)
- Integrazione dati GPS nel display pagina 1 e nelle pagine dedicate

### Data Logger (flash LittleFS ~1.5 MB)

- Registrazione ciclica su flash (ring buffer, sovrascrive i più vecchi)
- Record binario 30 bytes: timestamp + temp + hum + press + bat + lat + lon + alt + vel + rotta + sat/hdop + flags
- Header file: magic "CMRK", versione formato, bitmask campi, intervallo, tipo campo 8 (sat/hdop)
- Intervallo campionamento configurabile (default 600s = 10 min, ~360 giorni di storage)
- Download via web http://<IP>:8080/data → conversione CSV on-the-fly con timestamp ISO 8601
- Download via BLE (Nordic UART) e Seriale USB
- Indicatore "REC" a schermo con % spazio usato
- Trend pressione calcolato dallo storico log (↑↓→)

### Informazioni astro-nautiche

- Alba/tramonto: algoritmo solare NOAA locale (offline, da lat/lon + data)
- Fase lunare: calcolo ciclo sinodale (offline)
- Durata del giorno
- Previsione meteo locale: OpenWeatherMap API (free tier, 1000 call/giorno, ogni 30 min)
- Vento: direzione, velocità, raffica (da OWM)
- Indice UV (da OWM)
- Maree: WorldTides API (porto configurabile)

### Buzzer ed eventi sonori

- Master on/off + volume (duty cycle PWM)
- Eventi configurabili singolarmente: TX ok, TX fail, GPS fix, GPS perso, WiFi ok, WiFi perso, batteria bassa, cambio pagina, conferma, boot
- Melodia di avvio selezionabile (0=bip singolo, 1=Weather Chime, 2=Sailor's Call, 3=Sunrise, 4=Radio Check, 5=Jingle minimale)
- Persistenza NVS per tutti i parametri buzzer

### WiFi e BT gestione energia

- WiFi toggle on/off da pagina 5 (risparmio ~80mA)
- BT toggle on/off da pagina 6 (risparmio ~30mA)
- Parametro NVS `wifi_on_boot`: riattiva WiFi al reboot (default 1)
- Parametro NVS `bt_on_boot`: riattiva BT al reboot (default 1)
- Modalità offline: GPS + meteo locale + log senza connettività

### LED di stato (verde, GPIO10)

- Spento: offline / standby
- Acceso fisso: WiFi connesso, tutto OK
- Lampeggio lento (1s on/off): in attesa (connessione, GPS searching)
- Lampeggio rapido (100ms on/off): errore / batteria bassa
- Flash TX 200ms: override momentaneo su qualsiasi stato
- Parametro NVS: `led_enabled` (0=off, 1=on)

### APRS Status Packet

- Invio periodico pacchetto status con commento descrittivo della stazione
- Stringa configurabile in NVS (`aprs_status`)
- Default: "CoreInk-Meteo v1.3 by EA5JDG/IZ3ARR"
- Intervallo invio: ogni 30 minuti (configurabile, `status_interval`)
- Formato APRS: `>CoreInk-Meteo v1.3 by EA5JDG/IZ3ARR`

### Documentazione

- Manuale utente completo (docs/manuale_utente.md)
- Configurazione: tutti i parametri NVS con descrizione e valori default
- Navigazione display: schema pagine e comandi joystick
- Installazione: flash USB + OTA + BLE
- API keys: come ottenere e configurare OpenWeatherMap e WorldTides
- Troubleshooting: problemi comuni e soluzioni

### Ottimizzazione firmware e partizioni

- Flag compilazione: -Os, -ffunction-sections, -fdata-sections, --gc-sections
- Disabilitare log ESP-IDF (CORE_DEBUG_LEVEL=0)
- Schema partizioni custom: app0/app1 ridotte, SPIFFS/LittleFS massimizzato
- Compilazione condizionale per moduli non usati
- Target: firmware < 1 MB per garantire OTA anche con feature complete

### Stabilizzazione

- Gestione errori di rete (riconnessione automatica, timeout, retry)
- Watchdog timer per recovery da crash
- Verifica stabilità multi-giorno
- Pulizia codice e rimozione debug

---

## v1.4 — Sensori esterni wireless (RF433 + Hall)

Espansione stazione con sensori aggiuntivi via radio e magnetici.

- Ricezione dati da sensori meteo wireless 433MHz (Oregon Scientific, Acurite, ecc.)
- Decodifica protocolli comuni (temperatura, umidità, pioggia esterna)
- Sensore effetto Hall per anemometro o pluviometro
- Integrazione dati nel pacchetto APRS weather report
- Hardware: M5Stack RF433R, Hall Effect Unit, Hub 1→3

---

## v1.5 — Miglioramenti interfaccia

- Font selezionabile dall'utente (diversi stili e dimensioni)
- Configurazione font dal portale web
- Libreria font estesa (monospace, sans-serif, 8/12/16/24 px)

---

## v2.0 — iGate / Digipeater LoRa + Multi-rete

Modulo LoRa per ricezione pacchetti APRS-RF e inoltro a APRS-IS.
Integrazione con reti meteo alternative per massima visibilità dati.

- Ricezione pacchetti APRS via LoRa (430 MHz)
- Inoltro a APRS-IS (iGate RX)
- Digipeater locale (ripetizione senza internet)
- Filtro pacchetti per area/nominativo
- Visualizzazione stazioni ascoltate su display
- Hardware: RA-01 (SX1278) SPI o E32-433T UART

### Integrazione reti meteo alternative

- **CWOP** (Citizen Weather Observer Program): invio dati meteo a rete CWOP via APRS-IS (server cwop.aprs.net:14580), registrazione CW/DW number
- **Weather Underground** (WU): upload HTTP/HTTPS a wunderground.com, API key + station ID configurabili via WiFiManager
- **MADIS/NOAA**: inoltro automatico dei dati CWOP verso il sistema MADIS (Meteorological Assimilation Data Ingest System)
- **PWSweather / AerisWeather**: upload dati via API REST
- **OpenWeatherMap Station API**: invio dati alla propria stazione OWM registrata
- **MQTT broker custom**: pubblicazione dati su topic configurabile (Home Assistant, Node-RED, Grafana)
- **Windy.com**: upload via API stations (station ID + API key)
- Selezione reti attive da WiFiManager (checkbox per ogni rete)
- Credenziali e ID stazione per ogni rete salvati in NVS
- Invio simultaneo multi-rete con intervalli indipendenti
- Log errori per ogni rete (ultima risposta, ultimo invio OK)

---

## Idee future (non pianificate)

- App BLE per configurazione da smartphone
- Sensori aggiuntivi I2C (UV, luminosità)
- Deep sleep con wake-up RTC per risparmio estremo
- Dashboard web locale
- Integrazione MQTT in parallelo ad APRS
