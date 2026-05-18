# Release Notes — v1.2.8 (in sviluppo)

> Il contenuto pianificato per questa versione è stato incorporato in v1.2.7.
> Questa sezione verrà aggiornata al termine della v1.2.7 con i fix rimasti.

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
