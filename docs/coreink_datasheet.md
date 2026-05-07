# M5Stack CoreInk - Documentazione Prodotto

Fonte: https://docs.m5stack.com/en/core/coreink
SKU: K048
Data consultazione: 2026-05-05

## Descrizione

CoreInk è un dispositivo controller con display E-Ink prodotto da M5Stack, basato
sull'ESP32-PICO-D4. Il lato frontale integra uno schermo E-Ink da 200x200 @ 1.54"
che supporta la visualizzazione in bianco/nero. Per l'interazione uomo-macchina,
dispone di un selettore a rotella e pulsanti fisici, un indicatore LED e un buzzer.
Ha una batteria al litio da 390mAh integrata che, combinata con l'RTC interno (BM8563),
consente funzioni di sleep e wake-up programmati.

## Specifiche Tecniche

| Parametro | Valore |
|-----------|--------|
| SoC | ESP32-PICO-D4 @ dual-core 240MHz |
| DMIPS | 600 |
| SRAM | 520KB |
| Flash | 4MB |
| Wi-Fi | 2.4 GHz |
| Tensione ingresso | 5V @ 500mA |
| Interfacce | USB Type-C x1, HY2.0-4P x1, M5-Bus, HAT expansion |
| Display | GDEW0154M09, SPI, 200x200 @1.54", 1-bit b/n |
| | DPI: 184, area visiva: 27.6x27.6mm |
| | Refresh completo: 0.82s, parziale: 0.24s |
| Pulsanti fisici | Programmabile x1, Reset x1, Power x1 |
| Selettore a rotella | UP, MIDDLE, DOWN |
| LED | Verde x1 (GPIO10) |
| RTC | BM8563 (I2C: SDA=G21, SCL=G22) |
| Buzzer | Passivo x1 |
| Antenna | 2.4G 3D |
| Batteria | 390mAh @ 3.7V |
| Temperatura operativa | 0 ~ 60°C |
| Dimensioni | 56.0 x 40.0 x 16.0mm |
| Peso | 31.5g |

## PinMap

### Display E-Ink (GDEW0154M09 - SPI)
| Segnale | BUSY | RST | D/C | CS | SCK | MOSI |
|---------|------|-----|-----|----|----|------|
| (da schema, SPI standard ESP32) |

### Selettore e Pulsanti
| Funzione | Dial Up | Dial Middle | Dial Down | User Button |
|----------|---------|-------------|-----------|-------------|
| (GPIO da schema) |

### LED e Buzzer
| Componente | GPIO |
|------------|------|
| LED verde | G10 |
| Buzzer | (da schema) |

### Controllo Alimentazione
| Funzione | GPIO |
|----------|------|
| HOLD | (mantiene alimentazione attiva) |

### RTC (BM8563)
| SDA | SCL |
|-----|-----|
| G21 | G22 |

### Porta HY2.0-4P (Port A)
| GND | 5V | SDA | SCL |
|-----|----|----|-----|
| GND | 5V | G32 | G33 |

### PIN Breakout disponibili
G25, G26, G36, G23, G34, G18, G21, G22, G14, G13

## Sensori Integrati

Il CoreInk **NON** ha sensori ambientali integrati. Dispone di:
- **RTC BM8563**: orologio in tempo reale (per sleep/wake programmati)
- **LED verde**: GPIO10 (attivo basso)
- **Buzzer passivo**: per feedback acustico
- La **batteria 390mAh** è collegata tramite il chip di gestione alimentazione

### Lettura Tensione Batteria
L'ESP32 può leggere la tensione della batteria tramite ADC su un pin dedicato
(consultare schema elettrico per il partitore di tensione). La libreria M5CoreInk
espone la funzione per leggere il livello di batteria.

## Note di Utilizzo

- Evitare refresh ad alta frequenza per lunghi periodi. Intervallo consigliato: ≥15s
- Non esporre a raggi UV per periodi prolungati (danno irreversibile all'E-Ink)
- Power ON: tenere premuto PWR per 2 secondi
- Power OFF: via software API o pulsante reset posteriore

## Datasheet Componenti
- ESP32: https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/644/esp32_datasheet_en.pdf
- BM8563 (RTC): https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/BM8563_V1.1_cn.pdf
- SY7088 (power): https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/SY7088-Silergy.pdf
- GDEW0154M09 (E-Ink): https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/CoreInk-K048-GDEW0154M09%20V2.0%20Specification.pdf

## Schema Elettrico
- PDF: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/coreink/coreink_sch.pdf

## Software
- Libreria Arduino: https://github.com/m5stack/M5Core-Ink
- M5Unified: https://github.com/m5stack/M5Unified
- Factory Test: https://github.com/m5stack/M5Core-Ink/tree/master/examples/Basics/FactoryTest
