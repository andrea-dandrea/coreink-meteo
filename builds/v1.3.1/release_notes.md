# Release Notes — v1.3.1 (2026-05-31)

## Bugfix: permanenza testo nel display (ghosting e-ink)

**Issue**: [#3 — permanenza del testo nel display](https://github.com/andrea-dandrea/coreink-meteo/issues/3)

### Problema

Dopo giorni di uso continuato con la libreria M5Unified, il display e-ink mostrava
permanenza del testo (ghosting): le schermate precedenti restavano visibili come
ombra grigia sotto il contenuto attuale, con 2-3 livelli di sovrapposizione.

**Causa root**: il port a M5Unified (v1.3.0) imposta `epd_mode_t::epd_fast` che
salta il ciclo completo di inversione dei pixel (nero→bianco→immagine). La vecchia
libreria M5Core-Ink eseguiva sempre un full refresh (0.82s) che impediva
l'accumulo di carica residua nei pixel.

### Soluzione

1. **Full refresh periodico anti-ghosting**: ogni 5 aggiornamenti automatici il
   display esegue un ciclo `epd_quality` (inversione completa), poi torna a
   `epd_fast`. Questo elimina la carica residua accumulata.

2. **Intervallo refresh aumentato**: da 60s (1 min) a 300s (5 min) di default.
   Un refresh ogni minuto è eccessivo per un display e-ink e accelera il
   degrado. Il valore resta configurabile (10-300s) via WiFiManager e web UI.

3. **Primo refresh post-boot in quality**: il contatore è inizializzato affinché
   il primo aggiornamento nel loop sia un full refresh, partendo "pulito".

4. **Navigazione manuale resta veloce**: i cambi pagina con joystick (UP/DOWN)
   usano sempre `epd_fast` per reattività — il full refresh avviene solo nel
   timer automatico.

### Menu emergenza (EXT long 3s)

Il long-press su EXT ora apre un menu con 3 opzioni (UP/DOWN per navigare, MID per confermare, EXT per uscire):

1. **WiFi emergenza** — connessione WiFi (come prima)
2. **Pulizia display** — 10 cicli nero(1s)→bianco(1s) per rimuovere ghosting accumulato
3. **Standby display** — nero→bianco poi resta in bianco fino a pressione tasto (risparmio e-ink)

### Modalità display

- `epd_quality` sempre attivo (no più alternanza fast/quality)
- `clearDisplay()` periodico ogni 5 aggiornamenti automatici
- `eink_deep_clean(10)` disponibile da menu per pulizia aggressiva

### File modificati

| File | Modifica |
|------|----------|
| `include/m5unified_compat.h` | Aggiunte `eink_deep_clean()`, `eink_clear_ghosting()`, setup in `epd_quality` |
| `include/config.h` | `DISPLAY_UPDATE_MS` 60000→300000, aggiunto `EINK_FULL_REFRESH_EVERY 5` |
| `src/main.cpp` | Menu emergenza 3 opzioni, refresh periodico, contatore anti-ghosting |
| `platformio.ini` | `FW_VERSION` → `"1.3.1"` |

### Build

| Metrica | Valore |
|---------|--------|
| Environment | `coreink_lite_m5u` |
| Flash | 98.7% (1,293,109 / 1,310,720 bytes) |
| RAM | ~17.6% |

### Note

- Il valore NVS salvato dall'utente ha precedenza sul nuovo default (backward-compatible)
- Il ghosting fisico accumulato durante le settimane in epd_fast non è completamente reversibile via software — la pulizia deep clean aiuta ma non elimina al 100%
- La frequenza del full refresh (ogni 5 = ~25 min con default 300s) è un
  compromesso tra pulizia display e velocità percepita
- `EINK_FULL_REFRESH_EVERY` è un `#define` in config.h, modificabile a compile-time
