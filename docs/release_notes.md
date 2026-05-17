# Release Notes - CoreInk Meteo

---

## v1.2.7 — Fix locator Maidenhead (2026-05-18)

Fix precisione posizione e display locator.

### Fix
- `maidenhead_to_latlon()`: supporto completo locator 4/6/8 caratteri (era troncato a 6)
- `latlon_to_maidenhead()`: calcola 10 caratteri per visualizzazione display (era 6)
- WiFiManager: label locator indica "max 8 char", maxlength corretto

### Note
- Prima versione con APRS-IS operativo e verificato su aprs.fi
- GPS testato con fix rapido (warm start AT6558)

---

## v1.2.6 — Fix WiFiManager e display (2026-05-17)

Fix critici portale configurazione, display e intervalli TX.

### Fix
- WiFiManager: watchdog disabilitato durante portale (non si chiude più dopo 30s)
- Parametri salvati SEMPRE (non solo su connessione WiFi riuscita)
- SSID APRS configurabile (0-15), refresh display e intervalli meteo/status configurabili
- Display: versione allineata a destra, rimossi "ENV" e simbolo APRS dalla riga TX
- LED: lento se WiFi ok ma APRS non configurato
- Primo TX: doppio readSensors() per cold-boot QMP6988
- DOWN al boot: reset credenziali WiFiManager

### Bug noti
- Locator 8-char ignorato (fix in v1.2.7)
- Nessuna pagina web di configurazione (solo OTA + CSV)
- Profili stazione hardcoded non editabili
- SmartBeacon troppo aggressivo con GPS fermo

---

## v1.2.5 — Build LITE (2026-05-17)

Refactor per partizione default.csv (1.25 MB app). Eliminati BLE, data logger, gps_extra.

---

## v1.3 — Moduli avanzati (2026-05-17)

Aggiunta di sei nuovi moduli e schema partizioni custom.

### Nuovi moduli
- `buzzer.h/cpp`: 10 eventi sonori, 6 melodie boot, PWM
- `led_status.h/cpp`: 4 stati LED + flash TX
- `data_logger.h/cpp`: ring buffer LittleFS, CSV via WebServer `/data`
- `gps_extra.h/cpp`: parser GSV multi-costellazione (GPS+BeiDou+GLONASS)
- `astro.h/cpp`: algoritmo solare NOAA, fase lunare
- `ble_ota.h/cpp`: BLE OTA indipendente da `BLE_ENABLED`

### Infrastruttura
- Schema partizioni custom `coreink_partitions.csv`: app 1.938 MB (+64 KB), LittleFS 64 KB
- Margine OTA ~88 KB rispetto a v1.3 (4.4%)

### Funzionalità
- 9 pagine display, runtime port switching ENV/GPS
- WiFiManager esteso con tutti i parametri configurabili
- Simbolo APRS configurabile a runtime, guard `canTx`
- Coordinate alternanti Maidenhead ↔ lat/lon, APRS Status packet ogni 30 min
- Batteria media mobile, WiFi auto-reconnect, SmartBeaconing GPS

---


Correzione caricamento parametri da NVS e miglioramenti display.

### Nuove funzionalità

- Versione firmware (`v1.2`) mostrata in alto a destra sul display
- Stringa di avvio seriale con versione dinamica da `FW_VERSION`

### Bugfix

- Fix callsign/passcode/locator: ora caricati da NVS all'avvio (fallback a config.h)
- Fix WiFiManager: le variabili runtime vengono aggiornate subito dopo il salvataggio NVS
- Fix WiFi credentials: `connectWiFi()` prova prima le credenziali salvate, poi fallback a config.h
- Fix joystick: aggiunto debounce 200ms + BtnEXT/BtnPWR come alternative per conferma

### Bug noti

- Locator Maidenhead troncato a 7 caratteri (buffer 8 byte, locator esteso ne richiede 9+)
- Lettura batteria fluttuante (singolo campione ADC)
- BLE OTA non visibile: blocco annidato dentro `BLE_ENABLED=0`, mai inizializzato

---

## v1.1 — OTA Update (2026-05-16)

Eliminazione della dipendenza dalla porta USB per aggiornamenti firmware.

### Nuove funzionalità

- ArduinoOTA: upload firmware da PlatformIO via WiFi (`pio run -e coreink_ota -t upload`)
- Web OTA: pagina http://<IP>:8080/update per upload .bin da browser
- BLE OTA: servizio GATT per upload firmware via Bluetooth (nRF Connect / app custom)
- Ambienti PlatformIO dedicati: `coreink_ota`, `stickcplus2_ota`
- WiFiManager: portale captive per configurazione WiFi al primo avvio

### Bugfix

- Fix timezone: display mostra ora locale italiana (CET/CEST automatico con ora legale)
- Fix timestamp APRS: usa `gmtime_r()` per garantire UTC nei pacchetti (suffisso `z`), indipendente dal timezone locale

### Test superati

- [x] Flash iniziale via USB completato con successo (CoreInk #1)
- [x] Web OTA verificato funzionante (http://192.168.1.92:8080/update)
- [x] Sensore ENV III: lettura temperatura, umidità, pressione OK
- [x] Connessione WiFi tramite WiFiManager OK
- [x] mDNS raggiungibile (coreink-meteo.local)
- [ ] BLE OTA con nRF Connect (da verificare)

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
