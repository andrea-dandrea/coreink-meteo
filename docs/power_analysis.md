# Analisi Power Management — CoreInk M5Stack

**Hardware**: M5Stack CoreInk
**MCU**: ESP32-PICO-D4 @ 240MHz
**Batteria**: LiPo 390mAh @ 3.7V nominale
**Boost converter**: SY7088 (3.7V → 5V)
**RTC**: BM8563 (I²C)
**USB**: CH9102 (solo UART/carica, nessun output di stato)

---

## Circuito di alimentazione

> Schema di riferimento: [docs/HW/coreink_sch_page_01.png](HW/coreink_sch_page_01.png)

```
USB VBUS (5V)
    │
    ├──► TP40P57 (caricatore lineare, variante TP4054/MCP73831)
    │         IN:  USB VBUS
    │         BAT: SYS_BAT — regolato a 4.2V (fine carica)
    │         Corrente di carica: ~200mA (R45 = 5.1kΩ sul pin PROG)
    │         CHḠ: LED rosso indicatore carica (non collegato a GPIO)
    │         STDBY: non collegato
    │
    └──► PWR_SW&ADC (Q4 SI2301 P-MOS, Q5 SI2302 N-MOS)
              Trigger accensione: GPIO12 (PS_EN/HOLD) OR USB_5V
                via diodi D14/D19 → gate Q5 → gate Q4
              │
              Q4 source = SYS_BAT (LiPo, ≤4.2V)
              Q4 drain  = SY7088 VIN  ←── nodo misurato da GPIO35
              │
              ├──► GPIO35 via R41(20kΩ top) / R42(5.1kΩ bottom) + C30(1µF)
              │
              └──► SY7088 (boost sincrono, abilitato da PS_EN)
                        IN:  SYS_BAT 3.0–4.2V
                        OUT: SYS_P050 5V ◄─── anche USB VBUS (path condiviso)
                        │
                        └──► SY8089 (buck, EN auto-attivo)
                                  IN:  SYS_P050 5V
                                  OUT: MCU_VDD 3.3V → ESP32
```

### SY7088 — Cosa NON fa
Il SY7088 è un **boost converter sincrono** (elevatore di tensione), non gestisce la carica.
Non ha output di stato, non segnala fine carica, non disconnette il carico.
La carica è gestita dal TP40P57 collegato a USB.
`PS_EN` controlla l'abilitazione del SY7088 (pad non collegato a GPIO ESP32 nella scheda attuale).

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

## Misura tensione batteria — circuito PWR_SW&ADC e ADC ESP32

> Schema di riferimento: [docs/HW/coreink_sch_page_01.png](HW/coreink_sch_page_01.png)

### Partitore resistivo — formula e valori reali

| Componente | Valore | Ruolo |
|---|---|---|
| R41 | 20 kΩ | Resistenza serie (top) |
| R42 | 5.1 kΩ | Resistenza a GND (bottom) |
| R_totale | 25.1 kΩ | |
| C30 | 1 µF | Filtro anti-rumore in parallelo a R42 |

$$V_{GPIO35} = V_{nodo} \times \frac{5.1}{25.1}$$

Il codice implementa correttamente l'inverso:
```cpp
float voltage = float(voltageMv) * 25.1f / 5.1f / 1000.0f;
// = V_GPIO35 × (R_totale / R_bottom) ← formula corretta
```
Il commento in `config.h` ("partitore 25.1/5.1") è fuorviante (indica totale/basso,
non top/basso) ma il **codice è matematicamente corretto**.

### Letture attese in condizioni normali (solo batteria)

| V_LiPo | V_GPIO35 | Lettura codice |
|---|---|---|
| 4.2 V (piena) | 854 mV | 4.20 V |
| 3.7 V (nominale) | 751 mV | 3.70 V |
| 3.3 V (cut-off) | 670 mV | 3.30 V |

### Effetto backdrive SY7088 con USB collegata

Il SY7088 usa un **raddrizzatore sincrono** (FET interno invece di un diodo Schottky).
Quando `USB_VBUS > SYS_P050` (es. caricatore mal regolato o QC senza handshake),
la body diode del FET sincrono conduce in direzione inversa: VOUT → VIN,
elevando il nodo SY7088 VIN = nodo misurato da GPIO35.

$$V_{nodo\_ADC} \approx V_{USB\_VBUS} - V_{body\_diode} \approx V_{USB\_VBUS} - 0.4\,\text{V}$$

| Tipo caricatore | USB VBUS | Nodo ADC effettivo | Lettura codice |
|---|---|---|---|
| Standard 5V regolato | 5.0 V | 4.2 V (batteria, no backdrive) | ~4.2 V |
| Economico mal regolato | 6.0 V | 5.6 V | ~5.6 V |
| QC senza handshake | 9.0 V | 8.6 V | ~8.6 V |

> **Il TP40P57 isola correttamente USB VBUS dalla LiPo** (caricatore lineare).
> L'elevazione sul nodo ADC non si trasmette alla batteria, che rimane ≤ 4.2 V.
> Il backdrive non danneggia la LiPo; può però stressare il nodo SY7088 VIN
> se il caricatore eroga tensioni molto alte (QC 9V+ senza handshake).

### Rilevamento USB via soglia ADC (fix software)

Poiché la LiPo non può fisicamente superare 4.2 V, una lettura > 4.4 V indica
con certezza che USB è collegata e la misura non è affidabile come tensione batteria:

```cpp
bool isOnUsb() {
    return batVoltage > 4.4f;  // backdrive SY7088: VBUS > 5V → nodo ADC elevato
}
```

**Conseguenze per il codice**:
- `batVoltage` NON è la tensione LiPo reale quando `isOnUsb() == true`
- L'algoritmo slope-detection (sezione precedente) deve essere sospeso se su USB
- Display e pacchetti APRS T# dovrebbero mostrare "USB" invece di un valore non significativo

**Nota ADC**: non misurare durante WiFi TX (spike di corrente distorcono la lettura).
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

---

## Misure reali da campo — v1.2.9, maggio 2026

> Sessione APRS-IS prolungata (58h, 21–23 maggio 2026), stazione IZ3ARR-13/EA5JDG
> Valencia (IM99TL), WiFi+ENV attivo, 1 pacchetto ogni 10 min, OTA server attivo.

### Consumo USB misurato

Misure dirette con multimetro in serie sul cavo USB 5V:

| Stato sistema | Corrente USB (5V) | Potenza |
|---|---|---|
| Carica batteria (bassa/media carica) | 0.22–0.23 A | ~1.10–1.15 W |
| Regime normale (batteria piena) | 0.08–0.09 A | ~0.40–0.45 W |

**Note**:
- Il TP40P57 eroga ~190–200 mA alla LiPo durante la carica (resistenza PROG R45 = 5.1 kΩ → I_charge ≈ 500/5100 ≈ 98 mA, ma taratura effettiva misurata ~200 mA — verificare formula esatta dal datasheet).
- In regime con batteria piena: il sistema consuma ~0.40–0.45 W totali (ESP32 + WiFi + SHT30 + QMP6988 + display). Il TP40P57 è in trickle/maintenance.
- La batteria in stato "piena" sul display mostra 4.10–4.11 V (ADC: 4094–4135 mV in telemetria). La soglia di soglia di terminazione del TP40P57 è 4.2 V; il valore letto <4.2 V è normale (batteria in mantenimento/scarica naturale).

### Autonomia su batteria interna (390 mAh)

Da log APRS T#170–T#202 (04:00–09:25 May 23, 2026, modalità WiFi+ENV con GPS off):

| Parametro | Valore |
|---|---|
| Durata periodo su sola batteria | ~5.4 h |
| Vbat inizio | 4116 mV (T#170, 03:55) |
| Vbat fine | 3694 mV (T#202, 09:20) |
| Drain totale | 422 mV |
| Drain medio | ~78 mV/h (~1.3 mV/min) |
| Autonomia stimata a scarica completa (3.2V) | ~17–18 h da 4.1V |

> **Nota**: l'autonomia reale era ~17–18h perché la batteria era stata ricaricata
> durante la notte dal PB#2 fino a ~4.1V, quindi il ciclo 04:00–09:25 rappresenta
> solo una parte della scarica dal top.

**Correzione rispetto ai dati v1.2.6** (sezione precedente):
I valori "15–18 mA" nella tabella v1.2.6 erano sottostimati di ~5x per errore
nella formula. Il consumo reale stimato dalla batteria è ~70–80 mA @ 3.7V,
equivalente a ~0.40–0.45 W, coerente con le misure USB dirette.

### Tempo di ricarica

Da log APRS T#145–T#156 (23:50–01:45 May 22→23, 2026):

| Parametro | Valore |
|---|---|
| Vbat inizio carica | 3835 mV (T#145) |
| Vbat fine carica (plateau) | ~4135 mV |
| Durata ricarica | ~1h45m–2h |
| ΔV | ~300 mV |
| Corrente di carica stimata | ~190–200 mA |

Per una ricarica da ~3.6V a 4.1V completa: **~2.5 h** stimato.

### Hot-swap USB

Il dispositivo **sopravvive a una disconnessione breve del cavo USB** senza
riavviarsi. La batteria interna eroga corrente durante il gap; POWER_HOLD_PIN è
mantenuto HIGH dal firmware → il sistema non si spegne.

Osservato nel log T#202→T#203: Vbat salta da 3694→3803 mV (riconnessione USB),
ma nessun riavvio (il contatore T# continua in sequenza).

### Timeline annotata — sessione 21–23 maggio 2026

| Ora | T# | Vbat (mV) | Evento |
|-----|-----|-----------|--------|
| ~20:00 May 21 | T#001 | — | Boot sessione A (dati non inclusi nel log analizzato) |
| ~23:35 May 21 | T#001 B | — | Boot sessione B (log analizzato) |
| ~20:00 May 22 | T#123 | 4108 | PB#1 collegato (batteria quasi piena, Chg=1) |
| ~21:30 May 22 | T#132 | 4061 | PB#1 esaurito → Vbat inizia a scendere |
| ~23:45 May 22 | T#145 | 3835 | Batteria in scarica da ~2h20m |
| ~23:55 May 22 | T#146 | 3931 | **PB#2 collegato** (salto +96 mV in 10 min → carica) |
| ~01:15 May 23 | T#155 | 4117 | Batteria quasi piena |
| ~01:45 May 23 | T#158 | 4135 | Plateau di carica |
| ~04:00 May 23 | T#170 | 4116 | **PB#2 esaurito** → Vbat inizia a scendere |
| ~09:25 May 23 | T#202 | 3694 | Fine batteria interna (5.4h di scarica) |
| ~09:25 May 23 | T#203 | 3803 | **Trasformatore collegato** (salto +109 mV → ricarica) |

### Proiezione v1.3 con deep sleep (M5.shutdown)

Se il ciclo di 10 min fosse gestito con `M5.shutdown(600)` invece di WiFi+modem sleep:

| Componente | Corrente attiva | Corrente sleep |
|---|---|---|
| ESP32 | ~80 mA | ~50 µA |
| WiFi modem | ~0 mA (off) | — |
| SHT30+QMP6988 | ~2 mA | <10 µA |
| Display (EPD) | — (no refresh ogni ciclo) | — |
| BM8563 RTC | — | ~1.2 µA |

- Tempo attivo per ciclo 10 min: ~20–30 s (boot + lettura ENV + invio APRS)
- Duty cycle attivo: ~25–30 s / 600 s ≈ 4–5%
- **Corrente media**: 80 mA × 5% + 0.1 mA × 95% ≈ **4.1 mA**
- **Autonomia stimata (390 mAh)**: 390 / 4.1 ≈ **~95 h** (~4 giorni) vs ~17–18h attuali

> Guadagno realistico: **5–6x** (considerando overhead di boot e inefficienze).
> Il WiFi connection time domina il "tempo attivo per ciclo" — pre-connessione WiFi
> ottimizzata (fast connect con BSSID fisso) è fondamentale per massimizzare il risparmio.
