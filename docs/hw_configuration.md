# Configurazione Hardware — CoreInk Meteo

Indice di tutta la documentazione hardware del progetto: datasheet, schemi elettrici e pagine prodotto.

---

## Moduli principali

| Modulo | Ruolo nel progetto | Documentazione |
|--------|--------------------|----------------|
| M5Stack CoreInk (K048) | Unità principale: ESP32, display e-ink, batteria, RTC, pulsanti | [coreink_datasheet.md](coreink_datasheet.md) · [Schematico PNG](HW/coreink_sch_page_01.png) · [Pagina prodotto](HW/CoreInk.mhtml) |
| M5Stack Unit ENV-III (U001-C) | Sensore temperatura, umidità, pressione | [enviii_datasheet.md](enviii_datasheet.md) · [Schematico](HW/envIII_sch_01.webp) · [Pagina prodotto](HW/Unit%20ENV-III.mhtml) |
| M5Stack Unit GPS (U032) | Posizione GPS + SmartBeaconing | [Schematico](HW/gps_sch_01.webp) · [Pagina prodotto](HW/Unit%20GPS.mhtml) |

---

## Chip e componenti — Datasheet

### Microcontrollore

| Chip | Funzione | Datasheet |
|------|----------|-----------|
| ESP32-PICO-D4 | SoC principale — dual-core 240 MHz, WiFi, BLE, 520KB SRAM, 4MB Flash | [esp32_datasheet_en.pdf](HW/esp32_datasheet_en.pdf) |

### Display

| Chip | Funzione | Datasheet |
|------|----------|-----------|
| GDEW0154M09 (driver JD79653) | E-ink 200×200 @ 1.54" — refresh completo 0.82s, parziale 0.24s | [CoreInk-K048-GDEW0154M09 V2.0 Specification.pdf](HW/CoreInk-K048-GDEW0154M09%20V2.0%20Specification.pdf) |

### Sensori

| Chip | Funzione | Datasheet |
|------|----------|-----------|
| SHT30 | Temperatura (±0.2°C) e umidità (±2% RH) — I2C 0x44 | [SHT3x_Datasheet_digital.pdf](HW/SHT3x_Datasheet_digital.pdf) |
| QMP6988 | Pressione barometrica 300–1100 hPa (±3.9 Pa) — I2C 0x70 | [QMP6988 Datasheet.pdf](HW/QMP6988%20Datasheet.pdf) |

### Alimentazione

| Chip | Funzione | Datasheet |
|------|----------|-----------|
| SY7088 | Boost converter 3.7V → 5V (NON è un charger) | [SY7088-Silergy.pdf](HW/SY7088-Silergy.pdf) |

> **Nota**: il circuito di ricarica LiPo (TP4057 o equivalente) non espone pin di stato carica accessibili via GPIO. Il rilevamento charge/discharge è implementato via slope della media mobile di Vbat — vedi [power_analysis.md](power_analysis.md).

### RTC

| Chip | Funzione | Datasheet |
|------|----------|-----------|
| BM8563 | RTC I2C — allarme per wake-up programmato (`M5.shutdown(seconds)`) | [BM8563_V1.1_cn.pdf](HW/BM8563_V1.1_cn.pdf) *(cinese)* |

### GPS

| Chip | Funzione | Datasheet |
|------|----------|-----------|
| AT6558 | Ricevitore GPS/BeiDou/GLONASS multi-costellazione | [AT6558_en.pdf](HW/AT6558_en.pdf) · [Multimode_satellite_navigation_receiver_cn.pdf](HW/Multimode_satellite_navigation_receiver_cn.pdf) |
| MAX2659 | LNA GPS — amplificatore a basso rumore per antenna passiva | [MAX2659_en.pdf](HW/MAX2659_en.pdf) |

---

## Schemi elettrici

| Schema | Formato |
|--------|---------|
| [CoreInk — schema completo](HW/coreink_sch_page_01.png) | PNG alta risoluzione |
| [CoreInk — anteprima](HW/coreink_sch_01.webp) | WebP |
| [ENV III Unit](HW/envIII_sch_01.webp) | WebP |
| [GPS Unit](HW/gps_sch_01.webp) | WebP |

---

## Pin mapping (CoreInk)

| Segnale | GPIO | Note |
|---------|------|------|
| LED verde | GPIO10 | Output digitale — flash ad ogni TX |
| Buzzer | GPIO2 | PWM canale 0 |
| POWER_HOLD | GPIO12 | Tenere HIGH per mantenere alimentazione |
| I2C SDA | GPIO21 | Bus ENV III, RTC BM8563 |
| I2C SCL | GPIO22 | Bus ENV III, RTC BM8563 |
| UART GPS TX | GPIO19 | Seriale verso GPS (RX lato GPS) |
| UART GPS RX | GPIO18 | Seriale da GPS (TX lato GPS) |
| Batteria ADC | GPIO35 | Lettura tensione batteria (media mobile) |
| Pulsante UP | — | Navigazione pagine / opzione su |
| Pulsante MID | — | Conferma / entra nel menu |
| Pulsante DOWN | — | Navigazione pagine / opzione giù |
| Pulsante EXT | — | Uscita menu / emergenza (ISR 3s) |

---

## Analisi tecniche correlate

- [eink_analysis.md](eink_analysis.md) — Refresh e-ink, partial update, deep sleep display
- [power_analysis.md](power_analysis.md) — Gestione alimentazione, duty cycle, charging detection
