# Release Notes — v1.3.0

**Data**: 2026-05-23  
**Build**: `coreink_lite_m5u` — Flash 98.6% (1,292,037 / 1,310,720) · RAM 17.6%  
**Libreria display**: M5Unified v0.2.15 (port da M5Core-Ink v1.0.0)  
**Partizione**: default.csv (1.25 MB app)

---

## Port a M5Unified

| Aspetto | Prima (v1.2.9) | Dopo (v1.3.0) |
|---------|----------------|---------------|
| Libreria | M5Core-Ink v1.0.0 (deprecata) | M5Unified v0.2.15 |
| Dimensione bin | 1,101,193 B (84%) | 1,292,037 B (98.6%) |
| Boot | ~1.5 s | ~2.5 s (init M5Unified più pesante) |
| API display | Ink_Sprite nativo | Compat shim `m5unified_compat.h` |

### Shim di compatibilità (`include/m5unified_compat.h`)

- `Ink_Sprite` → sottoclasse `M5Canvas` con `creatSprite()`, `pushSprite()`, `clear()`
- `#define M5Ink Display`
- Mapping pulsanti: BtnUP=BtnA(GPIO37), BtnMID=BtnB(GPIO38), BtnDOWN=BtnC(GPIO39)
- Setup e-ink: `epd_mode_t::epd_fast`, `setColorDepth(1)`, sfondo bianco, testo nero

---

## Nuove funzionalità

### OpenWeatherMap (OWM)

| Feature | Dettaglio |
|---------|-----------|
| Modulo `owm.h` / `owm.cpp` | Client HTTP per API OWM (free tier) |
| Current Weather | Temp, umidità, pressione, vento, nubi, pioggia 3h |
| Forecast 5-day/3h | 3 slot oggi (+3h, +6h, +9h) + 2 domani (mattina, pomeriggio) |
| Configurazione | Campo API Key nel form web `/config` |
| Intervallo | 30 min default, min 5 / max 180 (protezione rate-limit) |
| Prima fetch | Immediata dopo configurazione key (non aspetta intervallo) |
| Pagina 9 | Dati meteo OWM attuali + uptime |
| Pagina 10 | Previsioni (3 oggi + 2 domani) con temp range e descrizione |

### GPS Extra

| Feature | Dettaglio |
|---------|-----------|
| Parser NMEA esteso | GSV multi-costellazione (GPS, GLONASS, BeiDou, Galileo) |
| Dati aggiuntivi | HDOP, PDOP, altitudine, fix quality, sats in view/tracked |
| Pagina GPS | Informazioni dettagliate su fix, costellazioni, DOP |

---

## Bug fix

| ID | Problema | Soluzione |
|----|----------|-----------|
| B5/W3 | `telemetryDefSent = false` causava re-invio definizioni | Rimosso reset spurio |
| B6 | Skip pacchetto meteo con pressione 0 e altri valori validi | Skip solo se `pressure == 0.0f` |

---

## Modifiche UI

| Modifica | Dettaglio |
|----------|-----------|
| Status APRS default | `CoreInk-Meteo v1.3.0 by IZ3ARR` (rimosso `EA5JDG/`) |
| Titoli uniformati | `Temp:`, `Umid:`, `Press:` in tutte le pagine |
| Schermo 9 (OWM) | Temp, Umid, Press, Cond, Vento (range), Nubi, Pioggia 3h, Agg |
| Schermo 10 (Previsioni) | 3 slot oggi + 2 slot domani, formato 2 righe per slot |
| Schermo Info eliminato | Ex-schermo 9 rimosso; uptime spostato in pagina OWM |
| Vento OWM | Solo range velocità (rimosso gradi direzione — char non supportato) |
| NUM_PAGES | 10 (era 9 in v1.2.9) |

---

## Dimensioni

| Componente | Byte | Note |
|------------|------|------|
| Baseline v1.2.9 | 1,101,193 | 84.0% flash |
| M5Unified solo | 1,138,557 | +37 KB per la libreria |
| v1.3.0 completa | 1,292,037 | +191 KB totali (OWM, GPS_EXTRA, forecast) |

---

## Dipendenze

| Libreria | Versione |
|----------|----------|
| M5Unified | 0.2.15 |
| M5Unit-ENV | 1.0.1 |
| TinyGPSPlus | 1.1.0 |
| WiFiManager | 2.0.17 |
| ArduinoOTA | 2.0.0 |
| HTTPClient | 2.0.0 |
| ESP32 BLE Arduino | 2.0.0 |
| Espressif32 platform | 7.0.0 |
| Arduino ESP32 framework | 3.20017 |

---

## Note

- Flash al 98.6%: margine residuo ~18 KB. Ulteriori feature richiedono partizione custom (`coreink_partitions.csv`, 1.94 MB app) che richiede flash USB.
- Boot leggermente più lento (~1 s in più) per init M5Unified — non critico.
- BLE framework presente nell'ESP-IDF ma non rimovibile via `lib_ignore` (fa parte del core).
