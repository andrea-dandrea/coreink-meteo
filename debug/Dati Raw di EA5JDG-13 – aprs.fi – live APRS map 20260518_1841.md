# Analisi dati raw APRS — EA5JDG-13

**Fonte**: `Dati Raw di EA5JDG-13 – aprs.fi – live APRS map 20260518_1841.mhtml`
**Periodo**: 2026-05-17 23:59 CEST → 2026-05-18 ~18:41 CEST
**Firmware**: v1.2.6
**Server APRS-IS**: rotate.aprs2.net (gateways rilevati: T2TURKIYE, T2CSNGRAD, T2SPAIN, T2UKRAINE, T2CHILE, T2AUSTRIA, T2RADOM, T2KA, T2ERFURT, T2DENMARK, T2PANAMA, T2TAS)

---

## Sessioni identificate

| Sessione | Ora inizio | Coordinate | Modalità |
|----------|-----------|------------|---------|
| A | 23:59 CEST | 39°28.75'N 0°22.50'W | ENV+WiFi (casa) |
| B | ~00:48 CEST | 39°27.91'N 0°21.43'W | GPS+WiFi (mobile ~1.5km da casa) |
| C | ~01:47 CEST | 39°28.75'N 0°22.50'W | ENV+WiFi (rientro casa) |

---

## Pacchetti trasmessi (decodificati)

### Sessione A — 23:59 CEST (boot)

```
23:59:20  WX   @172159z 3928.75N/00022.50W_ .../... t032 h00 b00000   ← cold start!
23:59:20  POS  !3928.75N/00022.50W_ Vbat=4.14V 1.2.6
23:59:21  TEL  PARM.Vbat,RSSI,Uptime,Sat,Rsvd,GPS,WiFi,Chg,TX,Err,R1,R2,R3
23:59:21  TEL  UNIT.mV,dBm,min,n,,,,,,,,
23:59:22  TEL  EQNS.0,1,0,0,1,-100,0,1,0,0,1,0,0,0,0
23:59:22  STA  >CoreInk-Meteo v1.2.6 by EA5JDG/IZ3ARR

00:07:02  WX   @172207z t077(25°C) h59% b10152(1015.2hPa)
00:07:03  POS  Vbat=4.10V
00:07:04  TEL  PARM+UNIT+EQNS  ← BUG: ripetuti al 2° ciclo
00:07:06  STA  >CoreInk-Meteo v1.2.6

00:17:07  WX   @172217z t097(36°C!) h90% b10152    ← spike temperatura
00:17:07  POS  Vbat=4.07V
00:17:07  T#   T#000,4060,036,010,000,000,01010000

00:27:07  WX   @172227z t078(26°C) h58% b10153
00:27:11  POS  Vbat=4.02V
00:27:11  T#   T#001,4022,026,020,000,000,01010000

00:37:07  STA  >CoreInk-Meteo v1.2.6
00:37:15  WX   @172237z t077(25°C) h58% b00000     ← pressione = 0 (bug QMP6988)
00:37:16  POS  Vbat=4.01V
00:37:21  T#   T#002,3992,026,030,000,000,01010000
```

### Sessione B — SmartBeacon mobile 00:48–01:40 CEST

```
00:48:21  POS  3927.91N/00021.43W Vbat=3.92V       ← posizione cambia!
00:48:41  POS  3927.91N/00021.42W Vbat=3.93V       ← SmartBeacon: 20s dopo
00:49:16  POS  3927.93N/00021.43W Vbat=3.93V       ← 35s dopo
00:52:35  POS  3927.93N/00021.63W Vbat=3.92V       ← 3min dopo
00:54:53  POS  3927.91N/00021.48W Vbat=3.91V       ← 2min dopo — 5 POS in 6 min!

00:57:09  WX   @172257z 3927.91N b00000 [Duplicate!]   ← aprs.fi: duplicato
00:57:09  T#   T#004,3906,026,050,005,000,11010000      ← GPS:1 WiFi:1 Sat:5

01:04:11  POS  3927.91N Vbat=3.88V [Duplicate!]
01:07:06  STA  >CoreInk-Meteo v1.2.6
01:07:09  WX   @172307z b00000 [Duplicate!]
01:07:09  T#   T#005,3873,033,060,010,000,11010000      ← Sat:10

01:17:10  WX   @172317z b00000 [Duplicate!]
01:17:10  T#   T#006,3854,032,070,009,000,11010000

01:27:09  WX   @172327z b00000
01:27:09  T#   T#007,3819,035,080,013,000,11010000      ← Sat:13

01:34:11  POS  Vbat=3.80V
01:37:06  STA  >CoreInk-Meteo v1.2.6
01:37:09  WX   @172337z b00000
01:37:10  T#   T#008,3772,036,090,014,000,11010000      ← Sat:14

01:40:30  POS  3927.91N Vbat=3.77V
01:40:59  POS  3927.88N Vbat=3.77V                      ← SmartBeacon ancora
```

### Sessione C — Rientro casa 01:47 CEST

```
01:47:10  WX   @172347z 3928.75N b10153                  ← pressione OK di nuovo
01:47:11  T#   T#009,3787,063,100,000,000,01010000       ← RSSI=-37dBm (AP vicino!)
01:50:59  POS  Vbat=3.78V

01:57:09  WX   @172357z t076(24°C) h59% b10152
01:57:09  T#   T#010,3771,061,110,000,000,01010000

02:07:08  TEL  PARM+UNIT+EQNS  ← corretto: +2h dal boot
02:07:10  STA  >CoreInk-Meteo v1.2.6
02:07:11  WX   @180007z t075(24°C) h59% b10151
02:07:13  T#   T#011,3753,060,120,000,000,01010000

... (sessione stabile, pressione costante, temp ~24-25°C, h~59%) ...

02:37:09  WX   @180037z t075 h59 b10150
```

---

## Bug confermati dai dati

### BUG-1 ⚠️ Pressione = 0 intermittente (QMP6988)

| Quando | Pacchetto | Condizione |
|--------|-----------|------------|
| 23:59 (1° ciclo) | b00000 | Cold start — **atteso** |
| 00:37 (4° ciclo!) | b00000 | ENV attivo, dovrebbe essere stabile — **BUG** |
| 00:57–01:37 (GPS) | b00000 | GPS mode: pressione cached a 0 da ciclo precedente |
| 01:47+ (rientro) | b10153 | OK di nuovo dopo reinit ENV |

**Causa probabile**: `qmp.update()` restituisce `false` al 4° ciclo (I2C glitch?). La variabile `pressure` resta al valore precedente che era già 0 dal cold start (bug di reset locale). Quando si attiva GPS mode, la pressione rimane 0 perché `readSensors()` non viene chiamata.

**Fix proposto**:
```cpp
// Se pressure == 0 dopo i primi 2 cicli di warmup → non inviare b nel pacchetto WX
// oppure → retry qmp init con Wire.begin() + qmp.begin()
```

### BUG-2 ⚠️ Spike temperatura t=097°F (36°C) al 3° ciclo

| Ora | T misurata | Nota |
|----|-----------|------|
| 23:59 | 0°C | Cold start SHT30 |
| 00:07 | 25°C | Normale |
| 00:17 | **36°C** | Spike anomalo |
| 00:27 | 26°C | Rientro normale |

Il SHT30 restituisce valori errati durante il riscaldamento termico. L'ESP32 dissipa calore che influenza il sensore nei primi 15-20 minuti.

**Fix proposto**: scartare letture di temperatura >35°C nelle prime 15 minuti di uptime, oppure rate-limit di variazione max ±5°C/ciclo.

### BUG-3 ⚠️ SmartBeacon troppo aggressivo

5 pacchetti posizione in 6 minuti (00:48–00:55). Di questi, 2 marcati `[Duplicate position packet]` da aprs.fi (posizione identica, stessa finestra temporale).

**Causa**: `SB_TURN_TIME = 15s` + `SB_FAST_RATE = 60s` troppo bassi. Il GPS jitter (±20m) viene interpretato come movimento reale e triggerisce beacon.

**Fix proposto**: 
- `SB_FAST_RATE` → 120s
- Filtro posizione: ignora delta < 50m
- `SB_SLOW_RATE` rimane 1800s

### BUG-4 ℹ️ PARM/UNIT/EQNS ripetuti al 2° ciclo (noto)

Inviati a 23:59 (boot) e 00:07 (+8min). Poi correttamente a 02:07 (+2h). L'impatto è minimo — solo 3 pacchetti extra in più a inizio sessione.

---

## Analisi Vbat (da telemetria)

### Decodifica canali T#seq,A1,A2,A3,A4,A5,bits

- **A1** = Vbat (mV)
- **A2** = RSSI+100 (dBm+100)
- **A3** = Uptime (minuti)
- **A4** = Satelliti GPS
- **A5** = Rsvd (0)
- **bits** = GPS, WiFi, Chg, TX, Err, R1, R2, R3

| Seq | Uptime | Vbat (mV) | Delta/10min | RSSI (dBm) | Sat | GPS | WiFi | Chg | TX |
|-----|--------|-----------|------------|------------|-----|-----|------|-----|----|
| 000 | 10min | 4060 | — | -64 | 0 | 0 | 1 | 0 | 1 |
| 001 | 20min | 4022 | -38 | -74 | 0 | 0 | 1 | 0 | 1 |
| 002 | 30min | 3992 | -30 | -74 | 0 | 0 | 1 | 0 | 1 |
| 004 | 50min | 3906 | -43* | -74 | 5 | **1** | 1 | 0 | 1 |
| 005 | 60min | 3873 | -33 | -67 | 10 | 1 | 1 | 0 | 1 |
| 006 | 70min | 3854 | -19 | -68 | 9 | 1 | 1 | 0 | 1 |
| 007 | 80min | 3819 | -35 | -65 | 13 | 1 | 1 | 0 | 1 |
| 008 | 90min | 3772 | -47 | -64 | 14 | 1 | 1 | 0 | 1 |
| 009 | 100min | 3787 | +15* | -37 | 0 | 0 | 1 | 0 | 1 |
| 010 | 110min | 3771 | -16 | -39 | 0 | 0 | 1 | 0 | 1 |
| 011 | 120min | 3753 | -18 | -40 | 0 | 0 | 1 | 0 | 1 |
| 012 | 130min | 3747 | -6 | -40 | 0 | 0 | 1 | 0 | 1 |
| 013 | 140min | 3735 | -12 | -37 | 0 | 0 | 1 | 0 | 1 |

*\* seq 003 mancante (gap sessione B), +15mV al seq009 = ricarica breve da USB*

### Consumo stimato

| Modalità | Drain medio | Corrente stimata* |
|---------|------------|------------------|
| WiFi + ENV (casa) | ~30–40 mV/10min | ~15–20 mA |
| WiFi + GPS (mobile) | ~35–47 mV/10min | ~18–24 mA |
| WiFi + ENV (sessione C) | ~12–18 mV/10min | ~6–9 mA (?) |

*\*stima grezza: batteria 390mAh@3.7V, capacità ~1.4Wh*

**Autonomia stimata** (range operativo 3900mV → 3400mV):
- WiFi+ENV continuo: ~120–150 minuti
- Con power save (da implementare): stimato 8–12 ore

### Bit "Chg" sempre = 0

Il bit `Chg` è sempre 0 in tutti i pacchetti. Non è implementato il rilevamento di carica. La proposta (slope della media mobile Vbat) è valida — conferma dei dati: il +15mV tra T#008 e T#009 non è rilevato come charging.

---

## Analisi RSSI

| Sessione | RSSI tipico | AP |
|---------|------------|-----|
| A (casa, 23:59–00:37) | -64 / -74 dBm | AP domestico distante / attraverso muri |
| B (mobile) | -64 / -74 dBm | AP domestico al limite copertura |
| C (casa rientro) | **-37 / -40 dBm** | AP domestico vicino |

Il salto da -74 a -37 dBm al rientro conferma che l'AP domestico fornisce ottimo segnale quando il CoreInk è in prossimità.

---

## Coordinate

| Pos | Lat | Lon | Note |
|-----|-----|-----|------|
| Casa | 39°28.75'N | 0°22.50'W | IM99TL (QTH) |
| Mobile | 39°27.91'N | 0°21.43'W | ~1.5km a SE |

---

## Checklist bug da fixare (priorità)

- [ ] **P1** QMP6988: gestire `pressure==0` dopo cold start — non trasmettere b=0 dopo ciclo 1
- [ ] **P1** QMP6988: retry init se `qmp.update()` fallisce consecutivamente
- [ ] **P2** SmartBeacon: `SB_FAST_RATE=120s`, filtro delta posizione <50m
- [ ] **P3** SHT30: rate-limit variazione temperatura (scarta spike >35°C nei primi 15min uptime)
- [ ] **P4** Bit Chg telemetria: implementare rilevamento charge/discharge da slope Vbat
- [ ] **P5** PARM/UNIT/EQNS: fix invio al 2° ciclo (già noto, bassa priorità)
