# Roadmap - CoreInk Meteo

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
