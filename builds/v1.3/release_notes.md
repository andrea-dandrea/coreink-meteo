# Release Notes - CoreInk Meteo

---

## v1.3 — Moduli avanzati (2026-05-17)

Aggiunta di sei nuovi moduli: buzzer, LED status, data logger, parser GPS esteso, calcoli astronomici, BLE OTA indipendente.

### Nuovi moduli

| Modulo | Descrizione |
|--------|-------------|
| `buzzer.h/cpp` | 10 eventi sonori, 6 melodie di boot, PWM canale 0, BUZZER_PIN=2 |
| `led_status.h/cpp` | 4 stati LED (SLOW/SOLID/FAST/OFF) + flash TX, GPIO10 |
| `data_logger.h/cpp` | Ring buffer LittleFS, LogRecord 30 byte, CSV via WebServer `/data` |
| `gps_extra.h/cpp` | Parser custom GSV (SNR/elevazione/azimuth), GPS + BeiDou + GLONASS, GGA, GSA |
| `astro.h/cpp` | Algoritmo solare NOAA (alba/tramonto offline), fase lunare (ciclo sinodale) |
| `ble_ota.h/cpp` | BLE OTA indipendente da `BLE_ENABLED` (fix bug v1.2) |

### Nuove funzionalità

- 9 pagine display (SU/GIU navigazione, EXT = cambio porta ENV/GPS)
- Runtime port switching ENV III ↔ GPS (hot-swap, salvato in NVS)
- WiFiManager esteso: tutti i parametri configurabili (callsign, passcode, locator, simbolo APRS, status, volume buzzer, melodia boot)
- Simbolo APRS configurabile a runtime (rtSymbolTable/rtSymbolCode salvati in NVS)
- Guard `canTx`: nessun TX se callsign == "NOCALL" o passcode == "-1"
- Coordinate alternanti ogni refresh display (Maidenhead ↔ lat/lon decimale)
- APRS Status packet ogni 30 minuti
- WiFi auto-reconnect ogni 60 secondi
- Batteria: media mobile su N campioni (fix rumore ADC, bug v1.2)
- Watchdog 30s, GPIO12 POWER_HOLD refresh ogni loop
- SmartBeaconing per modo GPS

### Bugfix

- BLE OTA ora sempre inizializzato indipendentemente da `BLE_ENABLED`
- Lettura batteria stabilizzata con media mobile

### Note di build (CoreInk)

- Flash: 98.4% (partizione `min_spiffs.csv` — 1.87MB app, necessaria per BLE+WiFi+tutti i moduli)
- RAM: 20.4%
- LittleFS data logger: ~64KB disponibili, ~2000 record da 30 byte
- `CORE_DEBUG_LEVEL` ridotto da 3 a 1 per risparmiare flash

### Stato build

| Board | Stato |
|-------|-------|
| coreink | SUCCESS |
| stickcplus2 | NON COMPILATO |

### File binari

| File | Dimensione | Note |
|------|-----------|------|
| firmware_coreink_v1.3.bin | ~1.9 MB | Firmware applicazione CoreInk |
| bootloader_coreink_v1.3.bin | ~17 KB | Bootloader ESP32 |
| partitions_coreink_v1.3.bin | ~3 KB | Tabella partizioni min_spiffs |

---

## v1.2 — Fix NVS e versione FW (2026-05-17)

Correzione caricamento parametri da NVS e miglioramenti display.

### Nuove funzionalità

- Versione firmware (`v1.2`) mostrata in alto a destra sul display
- Stringa di avvio seriale con versione dinamica da `FW_VERSION`

### Bugfix

- Fix callsign/passcode/locator: ora caricati da NVS all'avvio (fallback a config.h)
- Fix WiFiManager: le variabili runtime vengono aggiornate subito dopo il salvataggio NVS
- Fix WiFi credentials: `connectWiFi()` prova prima le credenziali salvate, poi fallback a config.h
- Fix joystick: aggiunto debounce 200ms + BtnEXT/BtnPWR come alternative per conferma

### Bug noti (risolti in v1.3)

- Lettura batteria fluttuante (singolo campione ADC)
- BLE OTA non visibile: blocco annidato dentro `BLE_ENABLED=0`, mai inizializzato

---

## v1.1 — OTA Update (2026-05-16)

Eliminazione della dipendenza dalla porta USB per aggiornamenti firmware.

### Nuove funzionalità

- ArduinoOTA: upload firmware da PlatformIO via WiFi (`pio run -e coreink_ota -t upload`)
- Web OTA: pagina http://\<IP\>:8080/update per upload .bin da browser
- BLE OTA: servizio GATT per upload firmware via Bluetooth (nRF Connect / app custom)
- Ambienti PlatformIO dedicati: `coreink_ota`, `stickcplus2_ota`
- WiFiManager: portale captive per configurazione WiFi al primo avvio

### Bugfix

- Fix timezone: display mostra ora locale italiana (CET/CEST automatico con ora legale)
- Fix timestamp APRS: usa `gmtime_r()` per garantire UTC nei pacchetti (suffisso `z`), indipendente dal timezone locale

---

## v1.0 — Stazione meteo completa (2026-05-05)

Prima versione stabile con tutte le funzionalità base operative.

### Funzionalità

- Lettura sensore ENV III (SHT30 + QMP6988): temperatura, umidità, pressione
- Pubblicazione pacchetti APRS weather report via APRS-IS
- WiFi Multi (più reti memorizzate, connessione automatica)
- 3 profili stazione selezionabili al boot (joystick)
- Simbolo APRS configurabile per profilo
- Posizione in formato Maidenhead
- GPS opzionale (AT6558) con sovrascrittura coordinate
- SmartBeaconing (intervallo adattivo basato su velocità/direzione GPS)
- Display e-ink: dati sensore, IP, nominativo, stato GPS, ultimo TX
- LED verde lampeggio conferma invio pacchetto
- Telemetria APRS (batteria, RSSI, uptime, satelliti GPS)
- Separazione pacchetto posizione e pacchetto meteo
- Salvataggio configurazione in NVS (persistente tra riavvii)
- Sincronizzazione orario NTP
- Supporto dual-board: CoreInk + StickCPlus2

---

## v0.1 — Prototipo iniziale (2026-05-01)

### Funzionalità

- Lettura sensore ENV III base
- Invio pacchetto APRS weather report
- Display e-ink con dati
- Connessione WiFi singola
