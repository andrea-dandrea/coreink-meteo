# Release Notes — v1.2.8 (in sviluppo)

Web config, eliminazione profili, SmartBeacon rate limiting, riassegnazione pulsanti.

---

## TODO

### Priorità alta
- [ ] Pagina `/config` su web server (porta 8080): form HTML per tutti i parametri NVS
- [ ] Eliminare array `profiles[]` hardcoded — config singola NVS editabile via web
- [ ] SmartBeacon rate limiting: min 60s tra posizioni, ignorare spostamenti <50m

### Priorità media
- [ ] MID breve → commutazione ENV↔GPS (spostare da EXT)
- [ ] PARM/UNIT/EQNS: inviare solo al boot + ogni 2h (non ai primi N cicli meteo)
- [ ] Non inviare weather se ENV III non collegata (rilevare assenza I2C)
- [ ] EXT: assegnare a TX forzato immediato (o altro)

### Priorità bassa
- [ ] Rimuovere Vbat e FW_VERSION dal commento posizione
- [ ] Schermo GPS: rimuovere ridondanza "fix sì/no" (Sat>0 implica fix)
- [ ] Aggiungere conteggio satelliti BeiDou nel display GPS

---

## Specifiche Web Config

Endpoint: `http://<IP>:8080/config` (GET = form HTML, POST = salva in NVS)

### Campi form:
- Callsign + SSID APRS (0-15)
- Passcode APRS-IS
- Locatore Maidenhead (max 8 char)
- Simbolo APRS (2 char: table + code)
- Messaggio Status APRS
- Intervallo meteo (sec)
- Intervallo status (sec)
- Intervallo telemetria (sec)
- Refresh display (sec)
- Volume buzzer (0-100)
- Melodia boot (0-5)
- Porta mode (ENV/GPS)

### Extra:
- Scarica config (JSON export via `/config/export`)
- Carica config (JSON import via `/config/import`)
- Riavvia dispositivo (POST `/reboot`)

---

## Pulsanti ridefiniti

| Pulsante | Azione |
|----------|--------|
| UP | Pagina precedente |
| DOWN | Pagina successiva |
| MID breve | Commutazione ENV↔GPS |
| MID lungo 3s | WiFiManager (AP mode) |
| EXT (sopra) | TX forzato immediato |
| DOWN al boot | Reset credenziali WiFi |
