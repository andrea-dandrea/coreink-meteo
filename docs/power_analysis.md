# Analisi Power Management — CoreInk M5Stack

**Hardware**: M5Stack CoreInk
**MCU**: ESP32-PICO-D4 @ 240MHz
**Batteria**: LiPo 390mAh @ 3.7V nominale
**Boost converter**: SY7088 (3.7V → 5V)
**RTC**: BM8563 (I²C)
**USB**: CH9102 (solo UART/carica, nessun output di stato)

---

## Circuito di alimentazione

```
LiPo 3.7V
    │
    ├──► TP4057 (o equivalente) ← USB 5V (CH9102 VBUS)
    │         Caricabatterie LiPo
    │         - Corrente di carica: ~500mA (configurabile con R)
    │         - Nessun pin di stato carica accessibile via GPIO
    │
    └──► SY7088 (boost converter)
              │  IN: 3.0–4.2V (da LiPo)
              │  OUT: 5V @ 500mA
              │  NON è un caricabatterie
              └──► alimenta ESP32, display, periferiche 5V
```

### SY7088 — Cosa NON fa
Il SY7088 è un **boost converter** (elevatore di tensione), non gestisce la carica.
Non ha output di stato, non segnala fine carica, non disconnette il carico.
La carica è gestita da un chip separato (TP4057 o compatibile) collegato a USB.

---

## Rilevamento stato carica (senza pin dedicato)

Il TP4057 non ha un output di stato accessibile via firmware.
**Soluzione**: rilevare charging/discharging analizzando la **derivata della tensione batteria**.

### Algoritmo slope Vbat

```cpp
#define VBAT_SAMPLES 8          // Finestra media mobile
#define VBAT_SLOPE_THRESHOLD 0.002f  // V/campione (≈2mV per ciclo di misura)

float batSamples[VBAT_SAMPLES];
int batSampleIdx = 0;

void updateBattery(float newVoltage) {
    batSamples[batSampleIdx] = newVoltage;
    batSampleIdx = (batSampleIdx + 1) % VBAT_SAMPLES;
}

enum ChargeState { CHARGING, DISCHARGING, STABLE };

ChargeState detectChargeState() {
    // Confronta il campione più vecchio con il più recente
    float oldest = batSamples[batSampleIdx];           // oldest = next slot (circolare)
    float newest = batSamples[(batSampleIdx - 1 + VBAT_SAMPLES) % VBAT_SAMPLES];
    float slope = (newest - oldest) / VBAT_SAMPLES;   // V/campione

    if (slope > VBAT_SLOPE_THRESHOLD)  return CHARGING;
    if (slope < -VBAT_SLOPE_THRESHOLD) return DISCHARGING;
    return STABLE;
}
```

### Calibrazione soglia
- I dati APRS (v1.2.6) mostrano drain di ~3–5 mV/min in uso normale
- Una soglia di 2 mV/campione (con campioni ogni ~10 min) è appropriata
- In carica rapida la tensione sale ~15–20 mV/min → ampiamente detectabile
- **Falsi positivi**: evitare misurazioni durante spike di corrente (WiFi TX, GPS fix)
  → misurare Vbat subito dopo il TX, non durante

### Bit telemetria "Chg"
Il bit `Chg` nei pacchetti T# APRS deve riflettere questo stato:
```cpp
bool chgBit = (detectChargeState() == CHARGING);
// Incluso nei bits di telemetria: T#seq,A1,A2,A3,A4,A5,xxxxxxxx
// bit 2 (da destra) = Chg
```

---

## Consumo stimato per modalità

Dati ricavati da misurazioni reali (sessione APRS 18 mag 2026, v1.2.6):

| Modalità | Drain Vbat | Corrente stimata* | Autonomia (390mAh) |
|---------|-----------|------------------|-------------------|
| WiFi + ENV (idle) | ~30–35 mV/10min | ~15–18 mA | ~6–7 ore |
| WiFi + ENV (TX attivo) | ~35–40 mV/10min | ~18–20 mA | ~5–6 ore |
| WiFi + GPS | ~40–47 mV/10min | ~20–24 mA | ~4–5 ore |
| WiFi OFF + M5.shutdown | ~2–4 mV/10min | ~1–2 mA equiv. | ~50–80 ore |

*\*Stima da: V_drain × C_batteria / V_nominale × 1/(V_drain rate)*

---

## Strategie power save

### 1. M5.shutdown(seconds) — massimo risparmio
```cpp
// Dopo aggiornamento display e invio APRS:
M5.shutdown(600);  // 10 minuti di spegnimento totale
// Al risveglio: cold boot, setup() viene rieseguito
```
- Consumo: ~50µA (solo BM8563 RTC attivo)
- Risveglio: garantito dall'alarm RTC BM8563
- **Usare quando**: WiFi OFF, modalità solo-ENV, nessun evento asincrono atteso

### 2. ESP32 Light Sleep — risparmio moderato, WiFi mantenuto
```cpp
esp_sleep_enable_timer_wakeup(600 * 1000000ULL);  // 10 min
wifi_set_ps_mode(WIFI_PS_MIN_MODEM);              // Modem sleep
esp_light_sleep_start();
// Al risveglio: continua dal punto di sleep (no cold boot)
```
- Consumo: ~2–5 mA (WiFi in modem sleep + ESP32 light sleep)
- Mantiene: WiFi association, variabili RAM, OTA disponibile
- **Usare quando**: WiFi ON, necessità di rispondere a eventi (OTA, APRS-IS)

### 3. Modem Sleep semplice — risparmio minimo, compatibilità massima
```cpp
WiFi.setSleep(true);  // Abilita modem sleep automatico tra TX
```
- Consumo: ~10–15 mA vs ~80 mA WiFi attivo
- Trasparente al codice applicativo
- **Usare come baseline** — sempre attivare se WiFi è ON

---

## POWER_HOLD_PIN — gestione critica

Il CoreInk usa `POWER_HOLD_PIN` (GPIO12 o equivalente) per mantenere l'alimentazione accesa.
Se questo pin va LOW, l'alimentazione si taglia anche con batteria inserita.

```cpp
// Nel setup():
pinMode(POWER_HOLD_PIN, OUTPUT);
digitalWrite(POWER_HOLD_PIN, HIGH);  // Mantieni acceso

// Nel loop(): refresh periodico (watchdog HW)
// NON lasciare che il loop si blocchi per > 30s senza refresh POWER_HOLD
```

**Attenzione**: se il firmware va in stallo, il dispositivo potrebbe spegnersi
autonomamente (feature, non bug — evita batteria scarica per stallo infinito).

---

## Rilevamento tensione batteria — ADC ESP32

```cpp
// ADC ESP32 ha rumore significativo (~30–50mV variazione)
// Usare media mobile su N campioni:
#define ADC_SAMPLES 16

float readBattery() {
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        sum += analogRead(BAT_ADC_PIN);
        delayMicroseconds(100);
    }
    float raw = sum / ADC_SAMPLES;
    // Fattore di scala per partitore resistivo CoreInk (verifica con multimetro)
    return raw * (3.3f / 4095.0f) * 2.0f;  // Partitore 1:1 → ×2
}
```

**Nota**: non misurare durante WiFi TX (spike di corrente distorcono la lettura ADC).
Misurare almeno 100ms dopo l'ultimo invio.

---

## Duty cycle raccomandato (da implementare in v1.2.8)

### Modalità WiFi ON (default, ciclo 10 min)
```
wake → init → leggi ENV → aggiorna display → invia APRS → modem sleep → attendi 10min → ripeti
      └─ OTA server sempre attivo durante attesa ─┘
```

### Modalità WiFi OFF (batteria, ciclo 10 min)
```
wake (cold boot) → init → leggi ENV → aggiorna display → M5.shutdown(600)
                                                          ↑ 10 min spento
```

### Transizione automatica WiFi ON → OFF
- Se Vbat < 3.6V E WiFi disconnesso da > 30 min → auto-switch a modalità OFF
- Notifica su display prima dello switch

---

## Range operativo batteria

| Tensione | Stato | Azione |
|---------|-------|--------|
| > 4.1V | Piena / in carica | Normale |
| 3.7–4.1V | Normale | Normale |
| 3.5–3.7V | Bassa | Warning su display, icona batteria |
| 3.3–3.5V | Critica | Forza modalità WiFi OFF |
| < 3.3V | Cut-off | SY7088 non garantisce 5V → shutdown |

---

## Dati reali da sessione APRS (v1.2.6, 18 mag 2026)

| Ora | Vbat | Modalità | Note |
|-----|------|---------|------|
| 23:59 | 4.14V | WiFi+ENV | Boot |
| 00:07 | 4.10V | WiFi+ENV | -40mV in 8min (caldo) |
| 00:37 | 4.01V | WiFi+ENV | Stabile ~-35mV/10min |
| 00:57 | 3.91V | WiFi+GPS | GPS attivo |
| 01:37 | 3.77V | WiFi+GPS | -35mV/10min con GPS |
| 01:47 | 3.79V | WiFi+ENV | +15mV → possibile carica USB breve |
| 02:37 | 3.75V | WiFi+ENV | Stabile ~-15mV/10min (env cooling) |

**Autonomia osservata**: ~140 minuti WiFi+GPS continuo (4.1V → 3.75V = 350mV / ~2.5mV/min)
