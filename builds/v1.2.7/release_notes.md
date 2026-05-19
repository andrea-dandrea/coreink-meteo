# Release Notes — v1.2.7 (2026-05-18)

Fix precisione locator, display 10-char Maidenhead. Prima versione APRS-IS verificata operativa.

---

## Fix implementati

### Locator Maidenhead

| # | Bug v1.2.6 | Fix v1.2.7 | Stato |
|---|------------|------------|-------|
| 1 | Locator 8-char ignorato | `maidenhead_to_latlon()` supporta 4/6/8 char con 4ª coppia | ✅ |
| 8 | Locator 6 char su display | `latlon_to_maidenhead()` calcola 10 char per visualizzazione | ✅ |
| — | WiFiManager label locator | Indica "max 8 char", maxlength=10 | ✅ |

### Bug critico: QMP6988 I2C lockup dopo GPS→ENV switch

| # | Problema | Fix | Stato |
|---|----------|-----|-------|
| A | Dopo ore di uptime con switch GPS→ENV, QMP6988 restituisce pressione fissa `-33968` Pa (I2C risponde ma dati corrotti) | `readSensors()`: bounds check 500–1100 hPa; fuori range → `pressure=0` + `Wire.end/begin` + `qmp.begin()` | ✅ |
| B | Stesso lockup può manifestarsi subito dopo lo switch | `switchPortMode()`: lettura di verifica post-switch; secondo reinit se fuori range | ✅ |
| C | Pacchetti weather con `b-33968` trasmessi su APRS-IS | `aprs_build_weather_packet()`: campo `b` omesso se `pressure_hpa` fuori range | ✅ |

### Telemetria — bit digitali

| Bit | Maschera | Label PARM | Semantica | Stato |
|-----|----------|------------|-----------|-------|
| 7 | 0x80 | GPS | GPS fix valido | ✅ |
| 6 | 0x40 | WiFi | WiFi associato (WL_CONNECTED) | ✅ |
| 5 | 0x20 | Chg | Batteria in carica (slope>0 && !isOnUsb) | ⏳ |
| 4 | 0x10 | TX | Ultimo pacchetto APRS inviato con successo | ✅ |
| 3 | 0x08 | Err | Sensore ENV in errore (QMP reset attivo) | ⏳ |
| 2 | 0x04 | LoRa | Modulo LoRa attivo (sempre 0 — dichiarato per v2.0) | ⏳ |
| 1 | 0x02 | R2 | Riservato | — |
| 0 | 0x01 | R3 | Riservato | — |

### Vbat / rilevamento USB

| # | Problema | Fix | Stato |
|---|----------|-----|-------|
| D | `batVoltage` riporta 5–6V con certi caricatori (backdrive body-diode FET sincrono SY7088) | `isOnUsb()`: soglia `batVoltage > 4.4f` per rilevare USB senza GPIO dedicato | ⏳ |
| E | Bit Chg mai settato; slope detection non implementata | `detectChargeState()` su finestra `batSamples[]`; sospeso se `isOnUsb()==true` | ⏳ |

### Documentazione

| — | README: rimossi riferimenti a Bruce firmware; aggiunto link NOAA Solar Calculator | ✅ |
|---|---|---|
| — | `docs/power_analysis.md`: circuito reale con Q4/Q5, analisi backdrive SY7088, formula ADC | ✅ |

---

## Da completare prima del rilascio v1.2.7

### Telemetria e batteria

| # | Feature/Fix | Dettaglio | Stato |
|---|-------------|-----------|-------|
| D | `isOnUsb()` | `batVoltage > 4.4f`; backdrive SY7088 body-diode | ⏳ |
| E | `detectChargeState()` | Slope su `batSamples[]`; sospeso se `isOnUsb()` | ⏳ |
| F | Bit Chg (0x20) | `sendTelemetry()`: slope>0 && !isOnUsb | ⏳ |
| G | Bit Err (0x08) | `sendTelemetry()`: `pressure==0.0f` (QMP reset attivo) | ⏳ |
| H | PARM label R1→LoRa | Sempre 0; dichiarato per compatibilità v2.0 | ⏳ |
| R | **Shutdown critico** | `!isOnUsb() && Vbat < 3.2V` → `M5.shutdown()`; warning buzzer+LED a <3.5V (era <3.3V) | ⏳ |

### WiFi state machine e screen map (implementazione codice)

| # | Feature | Dettaglio | Stato |
|---|---------|-----------|-------|
| I | FSM WiFi 6 stati | `WIFI_ST_OFF` default al boot | ⏳ |
| J | Screen map aggiornata | S0→S3 NAV diretto; S1 solo da menu pagina 3 | ⏳ |
| K | Menu emergenza EXT long press | ISR per accesso rapido al menu | ⏳ |

### Web server

| # | Feature | Dettaglio | Stato |
|---|---------|-----------|-------|
| L | Web server su AP domestico | OTA + config su IP locale (STA mode) | ⏳ |
| M | Web server su AP interno | OTA + config su 192.168.4.1 (SoftAP) | ⏳ |
| N | Pagina `/config` | Form HTML GET/POST su `http://<IP>:8080/config` | ⏳ |
| O | Profili editabili | Risolve NOCALL hardcoded; config singola NVS | ⏳ |
| P | Export/Import config | JSON via `/config/export` e `/config/import` | ⏳ |
| Q | Reboot remoto | POST `/reboot` | ⏳ |

**Campi form `/config`:**
- Callsign + SSID APRS (0–15)
- Passcode APRS-IS
- Locatore Maidenhead (max 8 char)
- Simbolo APRS (2 char: table + code)
- Messaggio Status APRS
- Intervallo meteo, status, telemetria (sec)
- Refresh display (sec)
- Volume buzzer (0–100), melodia boot (0–5)
- Porta mode (ENV/GPS)

### Pulsanti ridefiniti

| Pulsante | Azione | Stato |
|----------|--------|-------|
| UP | Pagina precedente | ⏳ |
| DOWN | Pagina successiva | ⏳ |
| MID breve | Commutazione ENV↔GPS | ⏳ |
| MID lungo 3s | WiFiManager (AP mode) | ⏳ |
| EXT breve | TX forzato immediato | ⏳ |
| EXT lungo | Menu emergenza (WiFi FSM) | ⏳ |
| DOWN al boot | Reset credenziali WiFi | ⏳ |

### Bug noti v1.2.6 da risolvere

| # | Bug | Impatto | Stato |
|---|-----|---------|-------|
| 3 | SmartBeacon aggressivo | Filtro Δ<50m, `SB_FAST_RATE`=120s | ⏳ |
| 4 | MID breve non assegnato | Tasto centrale breve → ENV↔GPS | ⏳ |
| 5 | PARM/UNIT/EQNS ripetuti | Solo al boot | ⏳ |
| 6 | Weather senza ENV III | Skip se sensore scollegato | ⏳ |
| 7 | Commento posizione ridondante | Rimuovere/semplificare | ⏳ |
| 8 | Schermo GPS ridondante | Unificare sat count e fix | ⏳ |
| 9 | BDS non mostrato | Aggiungere BeiDou al conteggio | ⏳ |

---

## Fix aggiuntivi — build finale (test hardware live)

| # | Bug riscontrato | Fix | Stato |
|---|-----------------|-----|-------|
| F1 | TX:FAIL sempre mostrato in modalità GPS | `sendPositionPacket()`: cattura bool `txClient.sendPacket()`, aggiorna `lastTxOk` + `lastTxTime` + `led_flash_tx()` | ✅ |
| F2 | Status APRS mostra "v1.2.6" da NVS stale | `setup()`: se valore NVS inizia con "CoreInk-Meteo v", forza `APRS_STATUS_DEFAULT` e risalva su NVS | ✅ |
| F3 | MID long 3s → WiFiManager troppo sensibile | `pressedFor(3000)` → `pressedFor(5000)` | ✅ |
| F4 | EXT non aveva azione su schermo 2/3 | EXT long (2s) → `showWifiMenu()` (emergenza WiFi da qualsiasi pagina); EXT short → toggle ENV/GPS (solo pagina 6) | ✅ |
| F5 | WiFiManager bloccava device 5 minuti | Non-blocking (`setConfigPortalBlocking(false)` + `process()`), timeout 120s, EXT aborta, OTA disponibile durante AP | ✅ |
| F6 | /config non disponibile su STA | Aggiunto endpoint `/config` GET+POST con form HTML completo (callsign, SSID, passcode, locator, simbolo, status, vol, melodia, intervalli) | ✅ |
| F7 | NTP non risincronizzato dopo riconnessione WiFi | `wifi_update()` CONNECTING→CONNECTED: aggiunta chiamata `syncTime()` | ✅ |

---

## Note tecniche
- Buffer locator: `rtLocator[11]` — sufficiente per 10+null
- Conversione 8-char: aggiunta 4ª coppia (cifre 0-9) → precisione ~500m
- Display 10-char: aggiunta 5ª coppia (lettere a-x) → precisione ~20m
- GPS AT6558: fix confermato in 1-2 min (warm start), LED bianco = 1PPS = fix
- Web server: riuso `otaServer` (WebServer su porta 8080), aggiunta routes `/config` GET/POST
