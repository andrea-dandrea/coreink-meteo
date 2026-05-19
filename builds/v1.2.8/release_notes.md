# Release Notes — v1.2.8

Versione di consolidamento post-v1.2.7: fix di tutti i bug osservati durante la prima giornata operativa sul campo (EA5JDG-13, 19/05/2026).

**Build:** `coreink_lite` — Flash 84.0% (1,100,621/1,310,720) · RAM 16.1% · ESP32 240MHz

---

## Bug fixati

### Alta priorità

| ID | Problema | Fix applicato |
|----|----------|---------------|
| BUG-12 | WiFi FSM non ciclava tra gli AP dopo perdita connessione — rimaneva bloccato sul primo AP | Round-robin AP1→AP2→AP3, backoff iniziale 2s (era 15s), reset indice all'aggancio |
| BUG-02 | Doppio pacchetto posizione APRS: SmartBeacon + timer fallback `sbActive` inviavano entrambi | Rimosso il timer fallback ridondante; SmartBeacon gestisce tutti i TX GPS |
| BUG-03 | "Location changes too fast" su APRS-IS da stazione ferma: jitter GPS causava velocità apparente >5 km/h e bypassava il filtro distanza | Filtro distanza ora applicato indipendentemente dalla velocità; `SB_MIN_DIST_M` 50→100m |

### Media priorità

| ID | Problema | Fix applicato |
|----|----------|---------------|
| BUG-01 | Password APRS-IS visibile in chiaro sullo schermo 4 | Mascherata con `***` |
| BUG-04 | Ora in fondo a sinistra (poco visibile) su schermo 1 | Spostata in alto a destra nell'header |
| BUG-05 | Pressione mostrata come "hPa" | Cambiato in "mbar" (numericamente identico) |
| BUG-06 | Simbolo temperatura senza grado ("C" invece di "°C") | Aggiunto `\xb0` — verifica visiva dopo flash |
| BUG-07 | Nessun warning visivo batteria bassa sul display | Aggiunto ` LOW` (Vbat<3.5V) e ` !!!` (Vbat<3.3V) in schermo 1 e 3 |
| BUG-08 | Messaggio "Premi EXT per attivare modo GPS" ingannevole in schermo 2 | Cambiato in "Vai a pag.7, premi MID per GPS" |
| BUG-09 | Uptime in minuti ("347m") invece che ore:minuti in schermo 3 | Formato unificato "Xh Ym" + aggiunta ora ultimo TX |
| BUG-10 | Schermo 7 (Meteo): mancava indicazione EXT lungo e uso del MID | Aggiunti hint "MID: menu" e "EXT 3s: emergenza" |
| BUG-11 | Refresh automatico immediato dopo cambio pagina manuale | Aggiunto `lastDisplayTime = millis()` dopo UP/DOWN |

---

## Allineamento modello UI (`ui_button_model.md`)

- **EXT short**: rimosso comportamento contestuale (era commuta ENV/GPS su P6) → ora **nessuna azione** in navigazione, conforme al modello
- **EXT long**: soglia portata da 2s a **3s** (anti-accidentale), conforme al modello
- **MID short su pagina 6 (Meteo)**: apre il **menu contestuale P6** con due opzioni:
  - `1. Forza lettura` — riletttura immediata sensori ENV (doppio campionamento)
  - `2. Commuta ENV/GPS` — toggle porta con salvataggio NVS e conferma display

---

## Noto / Da verificare dopo flash

- **BUG-06**: il simbolo `°` (`\xb0`) dipende dal font `AsciiFont8x16`. Se non visibile, sarà fixato in v1.2.9 con stringa alternativa.
- I bug W2 (doppio riavvio WiFi/watchdog) e W3 (PARM duplicati dopo reboot) non sono stati affrontati in questa versione — rimandati a v1.3.

---

## File inclusi

| File | Descrizione |
|------|-------------|
| `coreink_lite_v1.2.8.bin` | Firmware completo (app + bootloader + partizioni fuse, default.csv 1.25MB) |
| `bootloader.bin` | Bootloader ESP32 separato |
| `partitions.bin` | Tabella partizioni default |


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
