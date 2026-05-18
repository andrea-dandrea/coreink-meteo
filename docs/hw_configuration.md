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
| I2C SDA (Grove Port A) | GPIO32 | Wire.begin(32,33) — ENV III SHT30/QMP6988 |
| I2C SCL (Grove Port A) | GPIO33 | Wire.begin(32,33) — ENV III SHT30/QMP6988 |
| I2C SDA (interno RTC) | GPIO21 | Gestito internamente dalla libreria M5CoreInk — BM8563 |
| I2C SCL (interno RTC) | GPIO22 | Gestito internamente dalla libreria M5CoreInk — BM8563 |
| UART GPS RX (da GPS) | GPIO33 | Shared con I2C SCL: attivo solo in PORT_MODE_GPS |
| UART GPS TX (verso GPS) | GPIO32 | Shared con I2C SDA: attivo solo in PORT_MODE_GPS |
| Batteria ADC | GPIO35 | Input-only (ADC1_CH7) — `analogRead` (Vbat) |
| Pulsante UP (Dial Up) | GPIO37 | `M5.BtnUP` |
| Pulsante MID (Dial Middle) | GPIO38 | `M5.BtnMID` |
| Pulsante DOWN (Dial Down) | GPIO39 | `M5.BtnDOWN` |
| Pulsante EXT (User Button) | GPIO5 | Pannello frontale programmabile — `M5.BtnEXT` |
| Pulsante RST | EN/RST | Reset hardware diretto al pin EN dell'ESP32 |
| Pulsante PWR | GPIO12 | Laterale — BM8563 via PS_EN (`M5.BtnPWR`) |

> **Nota**: GPIO32 e GPIO33 sono condivisi tra I2C (ENV III) e UART (GPS) nel firmware attuale.
> La commutazione avviene via `switchPortMode()` che chiama `Wire.end()` o `gpsSerial.end()` prima
> di reinizializzare il bus. Questo vincolo viene eliminato nella **mod hardware v1.3** (vedi sotto).

---

## Connettore Grove (Ext.PORT) — pinout fisico reale

Da etichetta serigrafata e schematico J3:

| Pos | Colore | Segnale | GPIO | Protezione | Uso firmware attuale |
|-----|--------|---------|------|------------|----------------------|
| 1 | Nero | GND | — | — | massa |
| 2 | Rosso | 5V | — | — | alimentazione moduli |
| 3 | Giallo | G32 | GPIO32 | **47Ω serie** | I2C SDA (ENV III) / UART TX (GPS) |
| 4 | Bianco | G33 | GPIO33 | **47Ω serie** | I2C SCL (ENV III) / UART RX (GPS) |

> I **47Ω in serie** su G32/G33 limitano la corrente in caso di cortocircuito o
> inversione accidentale dei segnali, proteggendo il GPIO dell'ESP32.
> Questo protegge anche in caso di hot-swap del modulo con bus I2C attivo.

> **Colori seriale incrociati**: il cavo Grove fa crossover automatico TX↔RX.
> Sul GPS Unit: giallo = GPS TX (NMEA uscita) → si collega al bianco G33 (ESP32 RX).

---

## Analisi tecniche correlate

- [eink_analysis.md](eink_analysis.md) — Refresh e-ink, partial update, deep sleep display
- [power_analysis.md](power_analysis.md) — Gestione alimentazione, duty cycle, charging detection

---

## Mod hardware v1.3 — HAT dual-sensor (ENV III + GPS simultanei)

> **Stato**: pianificata per v1.3. Richiede cavo adattatore custom (connettore HAT 8-pin → 2×Grove).
> Elimina il meccanismo di hot-swap (`portMode` / `switchPortMode()`) e libera Grove Port A.

### Motivazione

Nel firmware attuale (v1.2.x) GPIO32/GPIO33 sono condivisi tra I2C (ENV III) e UART (GPS):
l'utente deve fisicamente scollegare un modulo e collegare l'altro, con rischio di corruzione
dati se la sequenza non è rispettata (vedi bug fix in v1.2.7).

Connettendo entrambi i moduli al connettore HAT J1 (FPC) si ottengono tre bus indipendenti
e Grove Port A diventa disponibile per un terzo modulo (es. LoRa UART in v2.0).

### Connettore HAT — pinout fisico reale (da etichetta serigrafata CoreInk)

Connettore a 8 pin, da sinistra a destra:

| Pos | Colore | Segnale | GPIO | Tipo | Uso nella mod |
|-----|--------|---------|------|------|---------------|
| 1 | Nero | GND | — | PWR | massa |
| 2 | Arancione | 5V↑ (5VIN) | — | PWR | ingresso 5V esterno (non usare) |
| 3 | Bianco | G26 | GPIO26 | I/O bidirezionale | **I2C SCL** (ENV III) |
| 4 | Grigio | G36 | GPIO36 | **INPUT ONLY** | **UART2 RX** (GPS NMEA) |
| 5 | Bianco | G25 | GPIO25 | I/O bidirezionale | **I2C SDA** (ENV III) |
| 6 | Fucsia | BAT | — | PWR | tensione batteria diretta (non usare) |
| 7 | Giallo | 3V3 | — | PWR | 3.3V logica (per sensori 3.3V) |
| 8 | Rosso | 5V↓ (5VOUT) | — | PWR | **alimentazione moduli** ← usare questo |

> **Nota**: l'etichetta fisica mostra 8 pin. I pin GPIO10 e GPIO5 presenti nello
> schematico J1 FPC interno non sono esposti sul connettore esterno.

> **GPIO36** (grigio): input-only (pin VP dell'ESP32). Funziona come UART RX
> (riceve NMEA dal GPS), operazione di sola lettura. GPIO25 e GPIO26 sono
> bidirezionali e supportano I2C.

### Schema cablaggio adattatore HAT → 2×Grove

```
HAT pos 1 (GND, nero)        ──► Grove ENV III  pin 1 (GND)
HAT pos 8 (5VOUT, rosso)     ──► Grove ENV III  pin 2 (5V)
HAT pos 5 (G25=SDA, bianco)  ──► Grove ENV III  pin 3 (SDA, filo bianco)
HAT pos 3 (G26=SCL, bianco)  ──► Grove ENV III  pin 4 (SCL, filo giallo)

HAT pos 1 (GND, nero)        ──► Grove GPS      pin 1 (GND)
HAT pos 8 (5VOUT, rosso)     ──► Grove GPS      pin 2 (5V)
HAT pos 4 (G36=RX, grigio)   ──► Grove GPS      pin 3 (GPS TX → ESP32 RX)
          (TX n.c.)               Grove GPS      pin 4 (GPS RX ← n.c.)
```

> **Nota ESD**: G25, G26, G36 sul HAT **non hanno** i 47Ω di protezione presenti
> su G32/G33 del Grove. Aggiungere resistori serie da **100Ω** su ciascuna linea
> segnale nel cavo adattatore (G25, G26, G36).

> **UART TX non necessario**: l'AT6558 emette NMEA a 9600 baud di default senza
> richiedere comandi di configurazione. Il GPS funziona in modalità RX-only lato ESP32.

### Modifiche firmware necessarie

```cpp
// Inizializzazione in setup() — entrambi i bus attivi contemporaneamente
Wire.begin(32, 33);                                    // libero per uso futuro o rimosso
Wire1.begin(25, 26);                                   // ENV III su HAT
sht3x.begin(&Wire1, SHT3X_I2C_ADDR, 25, 26, 400000U);
qmp.begin(&Wire1, QMP6988_SLAVE_ADDRESS_L, 25, 26, 400000U);
Serial2.begin(GPS_BAUD, SERIAL_8N1, 36, -1);           // GPS su HAT, RX-only

// Da rimuovere:
// - variabile portMode e costanti PORT_MODE_ENV / PORT_MODE_GPS
// - funzione switchPortMode()
// - funzione drawPortModeNotice()
// - voce menu "Cambia porta ENV/GPS"
// - tutto il codice #ifdef condizionato su portMode
```

### Architettura bus risultante (v1.3+)

| Bus | GPIO | Protocollo | Modulo | Stato |
|-----|------|------------|--------|-------|
| Wire (I2C interno) | G21/G22 | I2C | BM8563 RTC | libreria M5 |
| Wire1 (I2C HAT) | G25/G26 | I2C | ENV III SHT30+QMP6988 | sempre attivo |
| Serial2 (UART HAT) | G36 (RX-only) | UART 9600 | GPS AT6558 | sempre attivo |
| Serial1 (UART Grove) | G32/G33 | UART | **LoRa UART** (v2.0) | futuro |

### Compatibilità con HAT ENV III M5Stack (SKU U053)

L'HAT ENV III di M5Stack (SHT30 + QMP6988, stesso sensore del Grove ENV III) è progettato
per **M5StickC** (connettore pogo pin) e non è fisicamente compatibile con il CoreInk
(connettore FPC flat flex). La mod richiede un cavo adattatore costruito a mano.

---

## Connettore EPD1 (display FPC, interno) — analisi espansione

Il connettore EPD1 collega il pannello e-ink GDEW0154M09 al PCB via flat flex.
È **fisicamente interno** alla scocca e **non utilizzabile per espansione**.

| Pin | Segnale | GPIO | Stato |
|-----|---------|------|-------|
| 9 | BUSY | GPIO4 | ❌ SPI display |
| 10 | RST# | GPIO0 | ❌ SPI display + boot mode ESP32 |
| 11 | D/C | GPIO15 | ❌ SPI display |
| 12 | CS# | GPIO9 | ❌ SPI display |
| 13 | SCK | GPIO18 | ❌ SPI display |
| 14 | MOSI | GPIO23 | ❌ SPI display |
| 6 | TSCL | — | ⚠️ I2C master del JD79653 (non ESP32) |
| 7 | TSDA | — | ⚠️ I2C master del JD79653 (non ESP32) |
| altri | VGL/VGH/VCI… | — | ❌ alimentazioni interne e-ink |

**TSCL/TSDA**: il JD79653 usa questi pin come master I2C verso un sensore di temperatura
(NTC o digitale) per compensare le tensioni di pilotaggio al variare della temperatura.
Non sono controllati dall'ESP32 — inaccessibili per uso generale.

**Conclusione**: nessuna espansione pratica possibile tramite EPD1.

---

## GPIO liberi residui nel sistema completo

Dopo aver assegnato tutti i pin a display, I2C (Wire/Wire1), UART (Serial0/1/2),
ADC batteria, LED, buzzer, power hold e pulsanti, i soli GPIO bidirezionali liberi
dalla lista breakout del CoreInk sono:

| GPIO | Tipo | Possibile uso |
|------|------|---------------|
| **G13** | I/O bidirezionale | UART TX, SPI CS, GPIO generico |
| **G14** | I/O bidirezionale | UART RX, SPI CS, GPIO generico |
| G34 | **INPUT ONLY** | ADC, segnale esterno in ingresso |

G13 e G14 sono esposti sul connettore **M5-Bus** (vedi sezione successiva).

---

## Connettore M5-Bus (MI-BUS) — pinout completo

Connettore multifunzione sul retro/lato del CoreInk. Espone la maggior parte dei GPIO
del breakout su un connettore accessibile dall'esterno.

| Gruppo | Segnale M5-Bus | GPIO | Tipo | Stato nel progetto |
|--------|----------------|------|------|--------------------|
| SPI | MOSI | G23 | OUT | ❌ display SPI (condiviso) |
| SPI | MISO | G34 | **INPUT ONLY** | ⚠️ display SPI — G34 è input-only, SPI MISO funziona (master riceve) |
| SPI | SCK | G18 | OUT | ❌ display SPI (condiviso) |
| I2C | SDA | G21 | I/O | ❌ RTC BM8563 (Wire interno M5CoreInk) |
| I2C | SCL | G22 | I/O | ❌ RTC BM8563 (Wire interno M5CoreInk) |
| UART | TXD2 | G14 | OUT | ✅ **libero** |
| UART | RXD2 | G13 | IN | ✅ **libero** |
| GPIO | — | G25 | I/O | ⚠️ HAT mod v1.3: Wire1 SDA (ENV III) |
| GPIO | — | G26 | I/O | ⚠️ HAT mod v1.3: Wire1 SCL (ENV III) |
| ADC | — | G36 | **INPUT ONLY** | ⚠️ HAT mod v1.3: Serial2 RX (GPS) |
| RST | RST | EN | — | hardware reset ESP32 |
| Power | 3V3 | — | PWR | 3.3V logica |
| Power | 5VOUT | — | PWR | 5V da SY7088 boost |
| Power | 5VIN | — | PWR | ingresso 5V esterno |
| Power | BAT | — | PWR | tensione diretta batteria |
| Power | GND | — | GND | massa comune |

### Considerazioni importanti

**SPI condiviso con display (G18/G23/G34)**: tecnicamente un modulo SPI (es. LoRa SX1278)
potrebbe condividere il bus SPI del display usando un CS dedicato su G13 o G14.
Richiede serializzazione scrupolosa delle transazioni SPI (display e LoRa non possono
trasferire dati simultaneamente). Fattibile ma complesso. Alternativa UART più semplice.

**G25/G26/G36 sul M5-Bus**: sono gli stessi pin esposti sul HAT J1.
Dopo la mod hardware v1.3 questi pin sono occupati da ENV III e GPS —
**non usare sul M5-Bus** dopo quella modifica.

### Architettura bus completa dopo HAT mod v1.3

| Bus | GPIO | Protocollo | Modulo | Connettore fisico |
|-----|------|------------|--------|------------------|
| Wire (I2C interno) | G21/G22 | I2C | BM8563 RTC | interno PCB |
| Wire1 (I2C HAT) | G25(SDA)/G26(SCL) | I2C | ENV III SHT30+QMP6988 | HAT J1 FPC |
| Serial0 (UART0) | USB | UART | debug/flash | USB-C |
| Serial1 (UART1) | G13(RX)/G14(TX) | UART | **LoRa v2.0** | M5-Bus |
| Serial2 (UART2) | G36(RX only) | UART | GPS AT6558 | HAT J1 FPC |
| SPI display | G18/G23/G34/G9/G15/G4/G0 | SPI | GDEW0154M09 | EPD1 FPC |
| Grove Port A | G32/G33 | libero | **RF433 v2.0** | Grove HY2.0 |
