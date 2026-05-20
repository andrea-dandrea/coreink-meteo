# Release Notes — v1.2.9

Versione di fix campo post-v1.2.8: risolve il salto di posizione GPS→ENV e garantisce
l'invio immediato del pacchetto meteo con dati validi dopo la commutazione di porta.

**Build:** `coreink_lite` — Flash 84.0% (1,101,193/1,310,720) · RAM 16.1% · ESP32 240MHz

---

## Bug fixati

| ID | Problema | Fix applicato |
|----|----------|---------------|
| BUG-13 | Dopo commuta GPS→ENV la posizione tornava al locatore statico NVS invece di usare l'ultima posizione GPS nota | Aggiunte variabili sticky `lastGpsLat/Lon/PosValid`; `readGps()` le aggiorna ad ogni fix valido; `sendWeatherPacket()` e `sendPositionPacket()` usano la posizione sticky come fallback intermedio (GPS attivo + fix → posizione sticky → locatore NVS) |
| BUG-14 | Dopo commuta GPS→ENV il primo pacchetto meteo poteva arrivare fino a 5 min dopo la commutazione | `lastWeatherTime = 0` in `showPage6Menu()` dopo la commutazione: il loop invia il pacchetto meteo nel ciclo successivo |
| BUG-15 | Dopo commuta GPS→ENV i valori `temperature/humidity/pressure` potevano restare a 0 fino alla prima chiamata ordinaria di `readSensors()`, causando lo skip del pacchetto (guard `pressure == 0`) | `switchPortMode()` chiama `readSensors()` automaticamente subito dopo aver impostato `portMode = PORT_MODE_ENV` |

---

## Miglioramenti

| ID | Descrizione | Implementazione |
|----|-------------|-----------------|
| ENH-01 | Allarme batteria `!!!` (3.2–3.3 V) privo di buzzer distinto — stessa tonalità del livello `LOW` | Aggiunto `BUZZ_BAT_CRITICAL`: 3 bip 800 Hz ogni 30 s (vs 1 bip ogni 60 s del livello `LOW`). Soglia estratta in `BAT_WARN_THRESHOLD_V 3.3f` in `config.h` |

---

## Note operative

- Il locatore statico NVS viene usato **solo** se non è mai stato acquisito un fix GPS
  nella sessione corrente (o se il device non è mai stato in modalità GPS).
- In modalità GPS il comportamento SmartBeacon è invariato.

---

## File inclusi

| File | Descrizione |
|------|-------------|
| `coreink_lite_v1.2.9.bin` | Firmware completo (app + bootloader + partizioni fuse, default.csv 1.25MB) |
| `bootloader.bin` | Bootloader ESP32 separato |
| `partitions.bin` | Tabella partizioni default |
