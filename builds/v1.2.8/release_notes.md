# Release Notes — v1.2.8 (in preparazione)

Versione di consolidamento basata sull'esperienza operativa di v1.2.7 sul campo.

---

## Bug noti da v1.2.7 (da confermare con i log)

### WiFi e connettività

| # | Problema osservato | Priorità |
|---|-------------------|----------|
| W1 | Handover AP1→AP2 richiede ~4-5 minuti (retry timer 60s + connectWiFi timeout ~22s totale) | Alta |
| W2 | Doppio riavvio durante transizione WiFi (watchdog?) — osservato a 10:50 e 11:18 nel test 19/05 | Alta |
| W3 | PARM/UNIT/EQNS inviati ad ogni riavvio (`telemetryDefSent` non persistito NVS) → duplicate warning su APRS-IS | Media |

### Display e menu

| # | Problema osservato | Priorità |
|---|-------------------|----------|
| D1 | Boot screen S0 non visibile — `selectProfile()` sovrascrive prima del refresh e-ink | Bassa |
| D2 | Navigazione menu contestuale non intuitiva — comportamento pulsanti varia per pagina senza feedback visivo chiaro | Media |
| D3 | Nessun feedback visivo sul display dopo salvataggio da `/config` web | Bassa |

### APRS e telemetria

| # | Problema osservato | Priorità |
|---|-------------------|----------|
| A1 | Doppio TX al boot: `sendWeatherPacket()` + `sendPositionPacket()` entrambe in `setup()` | Media |
| A2 | In mobilità, posizione statica (Maidenhead NVS) inviata finché GPS non ha fix valido | Info |

---

## Miglioramenti grafici proposti

| # | Feature | Dettaglio |
|---|---------|-----------|
| G1 | Indicatore pagina corrente | Es. "3/9" in angolo schermo |
| G2 | Feedback visivo cambio profilo | Splash con callsign attivo dopo selezione |
| G3 | Schermata WiFi: mostra AP connesso | Nome SSID corrente (AP1/2/3) |
| G4 | Indicatore handover in corso | "WiFi: cercando..." durante reconnect |

---

## Da raccogliere durante i giorni di test v1.2.7

- [ ] Frequenza riavvii spontanei (watchdog durante transizione WiFi?)
- [ ] Comportamento batteria a fine giornata (shutdown critico 3.2V attivo?)
- [ ] Qualità GPS in contesti diversi (urbano, aperto)
- [ ] Tempo effettivo handover con AP3 come fallback casa

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
