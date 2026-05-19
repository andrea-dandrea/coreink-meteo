# Buglist v1.2.8 — Basata su osservazioni campo post-v1.2.7

> **Build di riferimento:** `coreink_lite` (DATA_LOGGER_ENABLED=0, GPS_EXTRA_ENABLED=0)
> **Data compilazione buglist:** 2026-05-19
> **Ultima verifica build:** 2026-05-19 — Flash 84.0% (1,100,621/1,310,720) RAM 16.1% — SUCCESS ✅

## Stato fix v1.2.8

| ID      | Priorità | Stato | Nota |
|---------|----------|-------|------|
| BUG-01  | Alta     | ✅ FIXATO | Password mascherata con `***` |
| BUG-02  | Alta     | ✅ FIXATO | Rimosso timer fallback sbActive |
| BUG-03  | Alta     | ✅ FIXATO | Filtro distanza speed-independent + SB_MIN_DIST_M=100 |
| BUG-04  | Media    | ✅ FIXATO | Ora in header top-right (case 0) |
| BUG-05  | Media    | ✅ FIXATO | hPa → mbar |
| BUG-06  | Media    | ✅ FIXATO | `"\xb0""C"` — verifica visiva dopo flash necessaria |
| BUG-07  | Media    | ✅ FIXATO | Warning ` LOW` / ` !!!` in case 0 e case 2 |
| BUG-08  | Media    | ✅ FIXATO | Messaggio EXT→MID corretto in case 1 |
| BUG-09  | Media    | ✅ FIXATO | Uptime hh:mm + ora ultimo TX in case 2 |
| BUG-10  | Media    | ✅ FIXATO | Hint `MID: menu` + `EXT 3s: emergenza` in case 6 |
| BUG-11  | Media    | ✅ FIXATO | `lastDisplayTime = millis()` dopo UP/DOWN |
| BUG-12  | Alta     | ✅ FIXATO | WiFi round-robin AP cycling |
| FEA-01..05 | Bassa | ⏭ v1.3 | Rinviato versione successiva |

> **Note post-build:**
> - BUG-06: il simbolo `\xb0` (grado) dipende dal font `AsciiFont8x16` — verificare visivamente dopo flash. Se non si vede, usare `"degC"`.
> - EXT long: soglia portata a 3s (era 2s) in accordo con `ui_button_model.md`.
> - `showPage6Menu()` aggiunto: MID short su pagina 6 apre menu con `1. Forza lettura` e `2. Commuta ENV/GPS`.

---

## PRIORITÀ ALTA — Fix obbligatori v1.2.8

---

### BUG-01 · Schermo 4 (case 3): password visibile — dato sensibile
**Sintomo:** `rtPasscode` (passcode APRS-IS) viene stampato in chiaro su schermo 4 (Profili/NVS).
**Causa:** `main.cpp` – `drawPage()` `case 3`, riga:
```cpp
snprintf(buf, sizeof(buf), "Pass: %s", rtPasscode);
mainSprite.drawString(5, 136, buf, &fonts::AsciiFont8x16);
```
**Fix:** Rimuovere la riga con la password. Sostituire con asterischi ("Pass: \*\*\*\*\*") o con la sola indicazione di stato ("Pass: configurato / NON configurato"). Rimuovere anche il callsign dalla sezione "NVS attivi" perché è già nell'header della pagina e nella lista profili sopra.

---

### BUG-02 · APRS: [Duplicate position packet]
**Sintomo:** Il server APRS-IS riceve pacchetti di posizione duplicati a breve distanza temporale.
**Causa:** Nel `loop()` di `main.cpp` ci sono **due percorsi distinti** che chiamano `sendPositionPacket()` in modalità GPS:
1. SmartBeacon: `if (gpsFixValid && smartBeacon.shouldBeacon(...))` — triggered da velocità/rotta
2. Timer fallback: `if (sbActive && now - lastPositionTime >= WEATHER_INTERVAL_MS)` — ogni 5 min

Se SmartBeacon si attiva pochi secondi prima del timer da 5 minuti, entrambi inviano. Inoltre APRS-IS considera "duplicate" qualsiasi pacchetto identico ricevuto entro ~30s.
**Fix:** Il timer fallback non dovrebbe essere necessario in modalità GPS se SmartBeacon è attivo. Rimuovere il secondo blocco `sbActive && WEATHER_INTERVAL_MS` quando SmartBeacon è abilitato. In alternativa, impostare `WEATHER_INTERVAL_MS` come soglia di sicurezza solo se il SmartBeacon non ha inviato nell'intervallo.

---

### BUG-03 · APRS: [Location changes too fast] — jitter GPS da fermo
**Sintomo:** APRS-IS produce l'avviso "Location changes too fast (adaptive limit)" anche da stazione ferma.
**Causa:**
- Il filtro distanza in `sendPositionPacket()` si applica **solo se** `gpsSpeed < SB_SLOW_SPEED (5 km/h)`.
- Il jitter del modulo AT6558 può generare velocità GPS spurie > 5 km/h anche da fermo (oscillazioni di posizione di 10-50m su periodi brevi = velocità apparente elevata).
- SmartBeacon si attiva anche per variazioni di rotta (`SB_TURN_MIN_ANGLE=30°`): da fermo, il jitter di direzione è casuale e supera spesso 30°, causando beacon ogni ~15s (`SB_TURN_TIME`).
```cpp
// in sendPositionPacket():
if (gpsSpeed < SB_SLOW_SPEED && dist < SB_MIN_DIST_M) { return; }
// NON filtra se gpsSpeed >= 5 km/h (anche da fermo!)
```
**Fix:**
1. Invertire la logica: filtrare se `dist < SB_MIN_DIST_M` **indipendentemente** dalla velocità (con `sbLastTxValid`).
2. Aumentare `SB_MIN_DIST_M` da 50m a 100m.
3. Aumentare `SB_SLOW_SPEED` da 5 a 10 km/h (soglia di riconoscimento del "fermo").
4. In SmartBeacon: aggiungere soglia minima di velocità per trigger di svolta (es. niente turn-beacon sotto 10 km/h).

---

## PRIORITÀ MEDIA — Display e UX

---

### BUG-04 · Schermo 1 (case 0): ora in fondo a sinistra, poco visibile
**Sintomo:** L'ora è stampata in y=188 (ultima riga, sinistra). Poco visibile per il layout affollato.
**Causa:** `drawPage()` `case 0`:
```cpp
snprintf(buf, sizeof(buf), "Ora %02d:%02d", ti.tm_hour, ti.tm_min);
mainSprite.drawString(2, 188, buf, &fonts::AsciiFont8x16);
```
**Fix:** Spostare l'ora in alto a destra. Il header già usa `x=2`, `x=90`, `x=vx (destra)`. Inserire l'ora nella riga header al posto della versione firmware, o usare una riga dedicata a y=20 sul lato destro.

---

### BUG-05 · Schermo 1 (case 0): unità pressione "hPa" invece di "mbar"
**Sintomo:** La pressione viene mostrata come `P: 1012.3 hPa`. L'utente preferisce `mbar` (numericamente identico).
**Causa:** `drawPage()` `case 0`:
```cpp
snprintf(buf, sizeof(buf), "P: %.1f hPa", pressure);
```
**Fix:** Cambiare `"hPa"` in `"mbar"`. Cambiamento puramente cosmetico.

---

### BUG-06 · Schermo 1 (case 0): unità temperatura "C" invece di "°C"
**Sintomo:** La temperatura viene mostrata come `T: 23.4 C` senza il simbolo di grado.
**Causa:** `drawPage()` `case 0`:
```cpp
snprintf(buf, sizeof(buf), "T: %.1f C", temperature);
```
**Fix:** Cambiare `"C"` in `"\xb0""C"` (ISO-8859-1 grado) o `"gC"` se il font ASCII 8x16 non supporta il grado Unicode. Verificare se `AsciiFont8x16` renderizza correttamente `\xb0` o `\xc2\xb0` (UTF-8). In alternativa usare `"degC"`.

---

### BUG-07 · Schermi 1 e 3 (case 0 e 2): assenza warning testuale batteria bassa
**Sintomo:** Quando la batteria scende sotto la soglia di warning, il buzzer suona ma non c'è indicazione visiva sul display.
**Causa:** In `case 0` e `case 2` (lite, schermo Stato), la tensione è mostrata senza alcun marker:
```cpp
snprintf(buf, sizeof(buf), "Bat: %.2fV", batVoltage);
```
Il controllo batteria nel `loop()` imposta `LED_FAST` e suona il buzzer ma non aggiorna il display.
**Fix:** Nel format string della batteria, aggiungere `" LOW"` se `batVoltage < BAT_LOW_THRESHOLD_V (3.5V)` e `" !!!"` se `batVoltage < BAT_CRITICAL_THRESHOLD_V + 0.1f (3.3V)`. Esempio:
```cpp
const char* batWarn = "";
if      (batVoltage < 3.3f)                    batWarn = " !!!";
else if (batVoltage < BAT_LOW_THRESHOLD_V)     batWarn = " LOW";
snprintf(buf, sizeof(buf), "Bat: %.2fV%s", batVoltage, batWarn);
```

---

### BUG-08 · Schermo 2 (case 1): messaggio EXT ingannevole in modo ENV
**Sintomo:** In schermo 2 (GPS Dettaglio, modo ENV), appare il testo "Premi EXT per attivare modo GPS", ma premere EXT non fa nulla perché EXT corto funziona solo da **pagina 7** (case 6).
**Causa:** `drawPage()` `case 1`:
```cpp
mainSprite.drawString(5, 80, "Premi EXT per", &fonts::AsciiFont8x16);
mainSprite.drawString(5, 100, "attivare modo GPS", &fonts::AsciiFont8x16);
```
Nel `loop()`, EXT corto è vincolato a `currentPage == 6`:
```cpp
if (!extLongFired && currentPage == 6) { ... switchPortMode ... }
```
**Fix (opzione A):** Estendere EXT corto a qualsiasi pagina (non solo pagina 6).
**Fix (opzione B):** Cambiare il messaggio in "Vai a pag.7 + EXT per GPS" oppure eliminarlo e lasciare solo la scritta "GPS non attivo".

---

### BUG-09 · Schermo 3 (case 2 lite): uptime in minuti invece di hh:mm
**Sintomo:** Uptime mostrato come "Uptime: 347m". Lo schermo 9 (case 8) mostra già "Uptime: 5h 47m".
**Causa:** `drawPage()` `case 2` (GPS_EXTRA_ENABLED=0):
```cpp
snprintf(buf, sizeof(buf), "Uptime: %lum", (millis() - uptimeStart) / 60000UL);
```
**Fix:** Uniformare il formato al `case 8`:
```cpp
int upMin = (millis() - uptimeStart) / 60000;
snprintf(buf, sizeof(buf), "Uptime: %dh %dm", upMin / 60, upMin % 60);
```
**Aggiunte:** Aggiungere anche l'ora dell'ultimo TX (`lastTxTime`) nello stesso schermo.

---

### BUG-10 · Schermo 7 (case 6): manca istruzione per EXT lungo
**Sintomo:** Schermo 7 (Meteo) mostra solo "EXT: cambia ENV/GPS" ma non indica che EXT tenuto 2s apre il menu WiFi di emergenza.
**Causa:** `drawPage()` `case 6`:
```cpp
mainSprite.drawString(5, 150, "EXT: cambia ENV/GPS", &fonts::AsciiFont8x16);
```
**Fix:** Aggiungere una riga sotto: `"EXT 2s: menu WiFi"`.

---

### BUG-11 · Qualsiasi schermo: refresh automatico subito dopo cambio pagina manuale
**Sintomo:** L'utente cambia pagina con SU/GIU; quasi subito dopo il display viene aggiornato di nuovo dal timer periodico (da 1 min), causando un refresh doppio ravvicinato.
**Causa:** Nel `loop()`, dopo UP/DOWN, si chiama `updateDisplay()` ma `lastDisplayTime` non viene azzerato:
```cpp
if (M5.BtnUP.wasPressed()) {
    currentPage = (currentPage - 1 + NUM_PAGES) % NUM_PAGES;
    buzzer_play_event(BUZZ_PAGE);
    updateDisplay();
    // ← manca: lastDisplayTime = millis();
}
```
Il timer controlla `now - lastDisplayTime >= rtDisplayUpdateMs`, quindi se il cambio manuale avviene quando `lastDisplayTime` è prossimo alla scadenza, il refresh periodico parte pochi secondi dopo.
**Fix:** Aggiungere `lastDisplayTime = millis();` dopo ogni `updateDisplay()` nel handler dei tasti UP/DOWN (e anche dopo EXT quando aggiorna il display).

---

## PRIORITÀ BASSA — Feature e ottimizzazioni

---

### FEA-01 · GPS: ora da satellite non mostrata da nessuna parte
**Sintomo:** L'ora del modulo GPS (UTC) non è visibile. Sarebbe utile come fonte di tempo indipendente da NTP.
**Causa:** `gps.time` da TinyGPSPlus non è mai mostrato nel display. `gps.time.hour()`, `.minute()`, `.second()` sono disponibili.
**Fix (v1.2.8 o v1.3):** Aggiungere in `case 1` (GPS Dettaglio) una riga `"GPS UTC: HH:MM:SS"` quando il fix è valido. Usare `gps.time.hour()`, `gps.time.minute()`, `gps.time.second()`.

---

### FEA-02 · GPS: satelliti BeiDou e HDOP in telemetria APRS (canale 4)
**Nota:** Canale 4 telemetria (`Sat`) trasmette `gpsSatellites` (solo GPS). Con GPS_EXTRA_ENABLED si potrebbe usare il canale 4 per totale satelliti visibili e il campo `Rsvd`/bits per conteggio BDS separato. Valutare in v1.3 con build full.

---

### FEA-03 · OpenWeather su schermo 8 (case 7 Astro)
**Sintomo:** Schermo 8 mostra dati NOAA offline (alba/tramonto/luna) ma nessuna previsione meteo online.
**Causa:** `NVS_KEY_OWM_KEY` e `OWM_DEFAULT_INTERVAL` sono definiti in `config.h` ma non implementati nel firmware.
**Fix (v1.3):** Implementare chiamata HTTP GET a `api.openweathermap.org/data/2.5/weather` con chiave OWM e parsing JSON per temperatura/condizioni/icona. Mostrare in testo nello schermo 8 o in una nuova pagina 9 dedicata.

---

### FEA-04 · Batteria: soglia bip urgente sotto 3.3V
**Sintomo:** Il bip di batteria bassa parte a 3.5V (BAT_LOW_THRESHOLD_V) ma la batteria precipita velocemente solo sotto 3.3V. Sarebbe utile un bip più insistente nella fase critica.
**Causa:** Solo due soglie: `BAT_LOW_THRESHOLD_V=3.5V` (LED_FAST + 1 bip/min) e `BAT_CRITICAL_THRESHOLD_V=3.2V` (shutdown). Manca una soglia intermedia.
**Fix:** Aggiungere `BAT_URGENT_THRESHOLD_V 3.35f` in `config.h`. Nel `loop()`, sotto questa soglia usare un evento buzzer più insistente (es. `BUZZ_BAT_LOW` ogni 30s invece di 60s) e aggiungere il warning `" !!!"` sul display (già coperto da BUG-07).

---

### FEA-05 · WiFi handover: backoff + timeout ancora lenti
**Nota:** L'handover tra AP richiede 4-5 minuti a causa del backoff max 60s + 3×5s di timeout per rete. Già noto da v1.2.7. Rimandato a v1.3.
**Opzione v1.2.8:** Ridurre `WIFI_TIMEOUT_MS / 3` (attuale 5s per AP) a 3s; abbassare il backoff massimo a 30s.

---

---

### BUG-12 · WiFi FSM: dopo perdita AP, non cicla tra gli AP disponibili ✅ FIXATO
**Sintomo:** Quando l'AP corrente cade (es. AP2 spento), la stazione non passa ad AP1 o AP3 e rimane disconnessa indefinitamente. L'unico rimedio era entrare nel menu WiFi di emergenza (EXT) e forzare la riconnessione manuale.
**Causa:** In `wifi_update()`, case `WIFI_ST_DISCONNECTED`, il retry usava `WiFi.begin()` senza argomenti, che ritenta sempre e solo **l'ultimo AP salvato dall'SDK**, mai gli altri:
```cpp
WiFi.begin();  // ← usa SSID/pass dell'ultimo AP: se è giù, rimane giù per sempre
if (wifiRetryBackoff < 60000) wifiRetryBackoff *= 2;
```
Il backoff raddoppiava fino a 60s, ma il ciclo tornava sempre sullo stesso AP caduto.
**Fix applicato:** Sostituito `WiFi.begin()` con ciclo round-robin `AP1 → AP2 → AP3 → AP1 ...` leggendo le credenziali da NVS. Backoff ridotto a 2s tra AP consecutivi; dopo ogni ciclo completo aumenta esponenzialmente fino a 60s. Al momento della disconnessione, `wifiRetryBackoff` resettato a 2000ms (era 15000ms) e `wifiApIdx = 0` per ripartire da AP1.
**Comportamento post-fix:**
- Caso migliore (AP1 disponibile, ero su AP2): ~2s attesa + ~2-5s connessione = **<10s**
- Caso peggiore (serve AP3, AP1 e AP2 down): ~2+15+2+15+2+connect ≈ **~35s**
- Senza nessun AP disponibile: cicla indefinitamente con backoff inter-ciclo 2→4→8→...→60s

---

## Riepilogo priorità per v1.2.8

| ID | Componente | Tipo | Effort |
|----|-----------|------|--------|
| BUG-01 | Display schermo 4 | Fix sicurezza | Basso |
| BUG-02 | APRS posizione duplicata | Fix logica | Medio |
| BUG-03 | APRS location too fast | Fix SmartBeacon | Medio |
| BUG-04 | Schermo 1: posizione ora | Cosmetic | Basso |
| BUG-05 | Schermo 1: unità pressione | Cosmetic | Basso |
| BUG-06 | Schermo 1: unità temperatura | Cosmetic | Basso |
| BUG-07 | Warning batteria bassa visivo | UX | Basso |
| BUG-08 | Schermo 2: EXT ingannevole | UX | Basso |
| BUG-09 | Schermo 3: uptime hh:mm + last TX | UX | Basso |
| BUG-10 | Schermo 7: hint EXT lungo | UX | Basso |
| BUG-11 | Reset lastDisplayTime su cambio pagina | Fix logica | Basso |
| BUG-12 | WiFi FSM: no ciclo AP round-robin | **Fix critico ✅** | Medio |
| FEA-01 | GPS: ora da satellite | Feature | Basso |
| FEA-02 | Telemetria: BDS/HDOP | Feature | Medio |
| FEA-03 | OpenWeather schermo 8 | Feature | Alto |
| FEA-05 | WiFi backoff più veloce | Tuning | Basso |
