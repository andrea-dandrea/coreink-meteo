# Roadmap - CoreInk Meteo

## v0.1 — Versione attuale (2026-05-05)

Stazione meteorologica APRS-IS completa via WiFi.

### Funzionalità implementate

- [x] Lettura sensore ENV III (temperatura, umidità, pressione)
- [x] Pubblicazione pacchetti APRS weather report via APRS-IS
- [x] WiFi Multi (più reti memorizzate, connessione automatica)
- [x] 3 profili stazione selezionabili al boot
- [x] Simbolo APRS configurabile per profilo
- [x] Posizione in formato Maidenhead
- [x] GPS opzionale con sovrascrittura coordinate
- [x] SmartBeaconing (intervallo adattivo con GPS)
- [x] Display e-ink con dati sensore, IP, nominativo, stato GPS, ultimo TX
- [x] LED verde lampeggio conferma invio
- [x] BLE predisposto (placeholder)
- [x] Sincronizzazione NTP

### Da completare per v1.0

- [ ] Portale captive WiFiManager per configurazione interattiva
- [ ] Telemetria APRS (batteria, RSSI, uptime, satelliti GPS)
- [ ] Separazione pacchetto posizione e pacchetto meteo
- [ ] Lettura tensione batteria e stato carica
- [ ] Salvataggio configurazione in NVS (sovrascrive valori di default)
- [ ] Test completo su hardware reale

---

## v1.0 — Stazione meteo completa

Tutte le funzionalità sopra funzionanti e testate.

---

## v1.1 — Miglioramenti interfaccia

### Funzionalità previste

- [ ] Font selezionabile dall'utente (diversi stili e dimensioni per il display e-ink)
- [ ] Configurazione font nel portale captive o tramite profilo
- [ ] Libreria font estesa (monospace, sans-serif, dimensioni 8/12/16/24 px)

---

## v2.0 — iGate / Digipeater LoRa

Aggiunta di un modulo LoRa per ricevere pacchetti APRS-RF (144.800 MHz) e
inoltrarli alla rete APRS-IS (funzione iGate), e viceversa (funzione digipeater).

### Hardware aggiuntivo necessario

- Modulo LoRa compatibile (es. RA-01, SX1278) operante a 430 MHz
- Connessione SPI o UART al CoreInk tramite GPIO breakout

### Funzionalità previste

- [ ] Ricezione pacchetti APRS via LoRa (demodulazione AX.25/APRS)
- [ ] Inoltro pacchetti ricevuti a APRS-IS (modalità iGate RX)
- [ ] Inoltro pacchetti APRS-IS a RF (modalità iGate TX, se autorizzato)
- [ ] Digipeater locale (ripetizione pacchetti RF senza internet)
- [ ] Filtro pacchetti per area/nominativo
- [ ] Visualizzazione sul display delle stazioni ascoltate
- [ ] Contatore pacchetti ricevuti/inoltrati
- [ ] Log locale delle stazioni sentite

### Note tecniche

- Frequenza operativa: 430 MHz (banda 70 cm radioamatoriale)
- Il formato pacchetto segue lo standard LoRa-APRS (header LoRa + payload AX.25)
- Il CoreInk ha GPIO sufficienti per collegare un modulo SPI LoRa
- La memoria e CPU dell'ESP32 sono più che sufficienti per gestire iGate + meteo

### Moduli LoRa compatibili (da valutare)

| Modulo | Interfaccia | Frequenza | Note |
|--------|-------------|-----------|------|
| RA-01 (SX1278) | SPI | 430 MHz | Economico, molto diffuso |
| LILYGO LoRa module | SPI | 430 MHz | Community attiva |
| E32-433T (SX1278) | UART | 430 MHz | Semplice integrazione |

---

## Idee future (non pianificate)

- App BLE per configurazione da smartphone
- Sensori aggiuntivi I2C (UV, luminosità, pluviometro)
- Deep sleep con wake-up RTC per risparmio energetico estremo
- Dashboard web locale (server HTTP sull'ESP32)
- Integrazione MQTT in parallelo ad APRS
