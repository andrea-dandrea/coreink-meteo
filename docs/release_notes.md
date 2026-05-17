# Release Notes - CoreInk Meteo

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
