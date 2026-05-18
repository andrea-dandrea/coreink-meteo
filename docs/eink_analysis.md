# Analisi E-ink — GDEW0154M09 / JD79653

**Display**: GDEW0154M09 (Good Display)
**Driver IC**: JD79653
**Risoluzione**: 200×200 pixel, 1.54"
**Libreria**: M5Core-Ink@1.0.0 + M5GFX@0.2.20

---

## Caratteristiche fisiche

| Parametro | Valore |
|-----------|--------|
| Risoluzione | 200 × 200 px |
| Diagonale | 1.54" |
| Profondità colore | 1 bit (bianco/nero) |
| Driver IC | JD79653A |
| Interfaccia | SPI |
| Tensione operativa | 3.3V |

---

## Tempi di refresh

| Tipo | Durata | Note |
|------|--------|------|
| **Full refresh** | ~0.82 s | Ciclo completo: bianco→nero→bianco→immagine. Flickering visibile. |
| **Partial refresh** | ~0.24 s | Solo i pixel che cambiano, no flickering. |
| **Deep sleep exit** | ~0.82 s | Equivalente a full refresh (re-init display) |

### Regola partial refresh
- Dopo **5 partial refresh consecutivi** → eseguire almeno 1 full refresh (altrimenti ghosting permanente)
- Intervallo minimo tra refresh: **15s** (raccomandazione M5Stack), **180s** (raccomandazione Good Display per massima longevità)
- Per la navigazione manuale (utente preme pulsanti) la cadenza naturale è sicura

---

## M5Core-Ink — comportamento libreria

La libreria M5Core-Ink@1.0.0 **non espone partial update** all'utente.
Ogni chiamata a `M5.Display.pushSprite()` o `M5.Display.display()` esegue un **full refresh** (0.82s).

```cpp
// Attuale — sempre full refresh:
canvas.pushSprite(0, 0);  // → JD79653 full update sequence

// Partial refresh NON disponibile via API M5Core-Ink
// Richiederebbe accesso diretto via SPI ai comandi JD79653
```

**Implicazione per il progetto**: ogni cambio di pagina, menu o aggiornamento stato
costa 0.82s di flickering. Accettabile per UX manuale, non adatto a clock/animazioni.

---

## Deep sleep del display

Il JD79653 ha una modalità deep sleep propria (command `0x10`) che riduce il consumo
del pannello tra i refresh, indipendente dal deep sleep dell'ESP32.

### Deep sleep display (JD79653 command 0x10)
- Risparmio: ~5–20 mA (consumo panel in idle)
- Attivare dopo ogni `pushSprite()` completato
- Risveglio: automatico al prossimo comando SPI (init sequence)

```cpp
// Attivare deep sleep display dopo aggiornamento schermo
void displaySleep() {
    // Invia command 0x10 (Deep Sleep Mode 1) via SPI diretto
    // M5Core-Ink non espone questo comando, occorre chiamata diretta
    M5.Display.sleep();  // Se disponibile nella versione usata
}
```

### Deep sleep ESP32 + hardware power-off (M5.shutdown)
`M5.shutdown(int seconds)` fa ben di più del sleep ESP32:
- Taglia `POWER_HOLD_PIN` via BM8563 RTC → **hardware power-off completo**
- Il BM8563 RTC sveglia il dispositivo dopo `seconds` secondi (alarm RTC)
- Consumo in shutdown: ~µA (solo RTC attivo)
- Al risveglio: cold boot completo (setup() → loop())

```cpp
// Spegni tutto per 600s (10 minuti), poi riavvia automaticamente
M5.shutdown(600);
// NON ritorna — il dispositivo si spegne fisicamente
```

---

## Sensore di temperatura JD79653

Il JD79653 ha un sensore di temperatura interno usato per la compensazione della
tensione di azionamento del pannello e-ink (i tempi di risposta variano con T).

**Non è accessibile via M5Core-Ink** — è gestito internamente dal driver IC.
Non usarlo come fonte di temperatura ambiente.

---

## Strategia duty cycle per CoreInk-Meteo

### WiFi ON (modalità normale, ciclo 10 min)
```
[loop attivo]
  ├── leggi sensori (~100ms)
  ├── aggiorna display (~820ms full refresh)
  ├── invia pacchetti APRS (~200ms)
  ├── OTA server polling
  └── modem sleep WiFi tra invii (ESP32 modem sleep, ~10mA vs ~80mA)
```

### WiFi OFF (risparmio massimo)
```
[ciclo 10 min]:
  1. wake (cold boot da M5.shutdown)        ~3s
  2. leggi sensori ENV                      ~200ms
  3. aggiorna display                       ~820ms
  4. M5.shutdown(600)                       → spegni tutto per 10 min
```
Consumo stimato: ~50µA × 600s + ~100mA × 4s ≈ **~0.4 mAh/ciclo**
vs WiFi continuo: ~15mA × 600s ≈ **~2.5 mAh/ciclo**

**Guadagno**: ~6× autonomia in WiFi OFF mode

### Deep sleep leggero (GPS + display periodico)
```
ESP32 light sleep tra invii (mantiene WiFi):
  - Risparmio: ~30% vs attivo
  - Risveglio su: timer, interrupt pulsante, GPS data
```

---

## Refresh in navigazione utente

**Domanda**: è un problema fare refresh ad ogni cambio pagina?

**Risposta**: No, nelle condizioni di utilizzo normali.

- Full refresh = 0.82s → accettabile per feedback visivo manuale
- L'utente non premerà UP/DOWN più velocemente di ~1s tra un tap e l'altro
- Il limite 15s si riferisce a refresh *automatici e ripetuti* (es. clock che si aggiorna ogni secondo)
- Per navigazione manuale la cadenza naturale è abbondantemente oltre i 15s
- **Attenzione**: non mettere refresh automatico nel loop con intervallo < 15s

---

## Raccomandazioni implementative

1. **Mai** aggiornare il display in un `while()` senza uscita (blocca il loop)
2. Aggiornare solo quando lo stato visivo è effettivamente cambiato (flag `displayDirty`)
3. Separare il refresh del display dal ciclo sensori (non ogni loop iteration)
4. In WiFi OFF: usare `M5.shutdown()` invece di `delay()` o `vTaskDelay()` — risparmio energetico enormemente superiore
5. Considerare di inviare display in sleep dopo ogni refresh (se la libreria lo supporta)
