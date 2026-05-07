# Note di compilazione — v1.0

Registro dei problemi riscontrati e delle soluzioni applicate per compilare
con successo il progetto su PlatformIO (ESP32 / M5Stack CoreInk).

Build verificata: **7 maggio 2026**  
Piattaforma: `espressif32 7.0.0` — Framework Arduino  
Risultato: RAM 14.8 % · Flash 80.6 %

---

## 1. Nome libreria M5CoreInk errato

| | |
|---|---|
| **Errore** | `UnknownPackageError: Could not find the package with 'm5stack/M5-CoreInk @ ^0.0.4'` |
| **Causa** | Il pacchetto è stato rinominato nel registro PlatformIO. La versione 0.0.x non esiste più. |
| **Fix** | `m5stack/M5-CoreInk@^0.0.4` → `m5stack/M5Core-Ink@^1.0.0` |

La v1.0.0 mantiene la stessa API pubblica (M5CoreInk.h, Ink_Sprite, Button, ecc.)
ma internamente dipende da M5GFX per il driver e-ink.

---

## 2. Dipendenza transitiva M5GFX mancante

| | |
|---|---|
| **Errore** | `fatal error: M5GFX.h: No such file or directory` (da `Ink_eSPI.h`) |
| **Causa** | `M5Core-Ink` v1.0.0 include `M5GFX.h` ma non lo dichiara come dipendenza nel suo `library.json`. |
| **Fix** | Aggiungere `m5stack/M5GFX@^0.2.20` esplicitamente in `lib_deps`. |

---

## 3. M5Unit-ENV — versione e dipendenze indirette

| | |
|---|---|
| **Errore** | `'I2C_Class' does not name a type` su tutti i sensori (SHT3X, QMP6988, DHT12, BMP280, SCD4X, SHT4X) |
| **Causa** | Dalla v1.1.0, M5Unit-ENV aggiunge la dipendenza `m5stack/M5UnitUnified` che a sua volta trascina M5Unified, M5HAL, M5Utility, BME68x, bsec2 — librerie pesanti e non necessarie per questo progetto. |
| **Fix** | Pinnare la versione: `m5stack/M5Unit-ENV@1.0.1` (senza `^`). |

La v1.0.1 (27 KB) è autocontenuta, include il proprio `I2C_Class.h/.cpp` e
offre la stessa API usata dal progetto:
```cpp
SHT3X::begin(&Wire, addr, sda, scl, freq)
SHT3X::update()  →  .cTemp, .humidity
QMP6988::begin(&Wire, addr, sda, scl, freq)
QMP6988::update()  →  .pressure
```

---

## 4. Conflitto include guard tra M5Core-Ink e M5Unit-ENV

| | |
|---|---|
| **Errore** | `'I2C_Class' does not name a type` — **anche con la v1.0.1** |
| **Causa** | Bug upstream: due header diversi usano lo stesso guard `_I2C_DEVICE_BUS_`. |

Dettaglio:

| File | Definisce | Guard |
|---|---|---|
| `M5Core-Ink/src/utility/i2c_device.h` | classe `I2C_DEVICE` | `_I2C_DEVICE_BUS_` |
| `M5Unit-ENV/src/I2C_Class.h` | classe `I2C_Class` | `_I2C_DEVICE_BUS_` |

Quando `<M5CoreInk.h>` viene incluso **prima** di `"M5UnitENV.h"`, il guard è
già definito e il secondo header viene saltato completamente, lasciando
`I2C_Class` non dichiarato.

**Fix**: uno script PlatformIO (`fix_i2c_guard.py`) rinomina automaticamente il
guard in `M5Unit-ENV/src/I2C_Class.h` dopo l'installazione delle librerie.
Lo script è referenziato in `platformio.ini` con `extra_scripts`.

> Questo è un bug nelle librerie M5Stack. Potrebbe essere risolto in versioni
> future. Se si aggiorna M5Unit-ENV, verificare che il conflitto sia stato
> corretto prima di rimuovere lo script.

---

## 5. ADC_ATTEN_DB_11 deprecato

| | |
|---|---|
| **Warning** | `'ADC_ATTEN_DB_11' is deprecated [-Wdeprecated-declarations]` |
| **Causa** | Il framework Arduino-ESP32 ha rinominato la costante. |
| **Fix** | In `src/main.cpp`, funzione `readBattery()`: `ADC_ATTEN_DB_11` → `ADC_ATTEN_DB_12`. Comportamento identico. |

---

## Riepilogo dipendenze validate

```ini
lib_deps =
    m5stack/M5GFX@^0.2.20            ; driver grafico (richiesto da M5Core-Ink)
    m5stack/M5Core-Ink@^1.0.0         ; hardware CoreInk (e-ink + pulsanti + RTC)
    m5stack/M5Unit-ENV@1.0.1          ; sensori ENV III (SHT30 + QMP6988) — pinnata
    mikalhart/TinyGPSPlus@^1.0.3      ; parser NMEA per GPS opzionale
    https://github.com/tzapu/WiFiManager.git  ; portale captive WiFi
```

Librerie dal framework Arduino-ESP32 (non servono in `lib_deps`):
- `WiFi`, `WiFiClient`, `WiFiMulti` — networking
- `Wire` — I2C
- `Preferences` — NVS storage
- `ESP32 BLE Arduino` — Bluetooth Low Energy (condizionale)
- `esp_adc_cal` — calibrazione ADC (ESP-IDF)
