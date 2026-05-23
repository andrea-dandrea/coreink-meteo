# CoreInk-Meteo — Manuale Utente

**Versione**: v1.3.0 (coreink_lite_m5u)  
**Autore**: IZ3ARR / EA5JDG  
**Hardware**: M5Stack CoreInk + ENV III + GPS/BDS (AT6558)  
**Libreria**: M5Unified v0.2.15

> Questo manuale documenta il firmware **v1.3.0**.

---

## Indice

1. [Descrizione](#descrizione)
2. [Hardware necessario](#hardware-necessario)
3. [Installazione firmware](#installazione-firmware)
4. [Primo avvio e configurazione WiFi](#primo-avvio)
5. [Navigazione display](#navigazione-display)
6. [Pagine display](#pagine-display)
7. [Configurazione web](#configurazione-web)
8. [OpenWeatherMap](#openweathermap)
9. [APRS-IS](#aprs-is)
10. [GPS](#gps)
11. [Buzzer e audio](#buzzer)
12. [Gestione energia](#gestione-energia)
13. [Aggiornamento OTA](#aggiornamento-ota)
14. [Troubleshooting](#troubleshooting)

---

## 1. Descrizione <a name="descrizione"></a>

CoreInk-Meteo è una stazione meteorologica portatile e tracker APRS basata su
M5Stack CoreInk (ESP32). Rileva temperatura, umidità e pressione atmosferica
tramite sensore ENV III, le trasmette alla rete APRS-IS, e integra previsioni
meteo da OpenWeatherMap.

---

## 2. Hardware necessario <a name="hardware-necessario"></a>

| Componente | Descrizione | Obbligatorio |
|------------|-------------|:---:|
| M5Stack CoreInk | Controller con display e-ink 200×200 | ✓ |
| M5Stack ENV III | Sensore SHT30 + QMP6988 | ✓ |
| M5Stack GPS/BDS Unit | Modulo GPS AT6558 + MAX2659 | Opzionale |
| Cavo Grove HY2.0-4P | Collegamento I2C/UART | ✓ |

---

## 3. Installazione firmware <a name="installazione-firmware"></a>

### Prima installazione (USB)

```bash
pio run -e coreink_lite_m5u -t upload
```

### Aggiornamento OTA (dopo prima installazione)

1. Aprire `http://<IP>:8080/update` nel browser
2. Selezionare `firmware.bin`
3. Attendere il riavvio (~30 s)

---

## 4. Primo avvio e configurazione WiFi <a name="primo-avvio"></a>

Al primo avvio (o se nessun AP configurato è raggiungibile) il dispositivo
crea un hotspot: **`CoreInk-AP`**.

1. Connettiti all'hotspot dal telefono/PC
2. Si apre il portale captive → inserisci SSID e password della tua rete
3. Il dispositivo si riavvia e si connette

---

## 5. Navigazione display <a name="navigazione-display"></a>

| Pulsante | Azione breve | Azione lunga (3-5s) |
|----------|-------------|---------------------|
| **UP** (↑) | Pagina precedente | — |
| **DOWN** (↓) | Pagina successiva | — |
| **MID** (●) | Menu contestuale pagina | WiFiManager (portale config) |
| **EXT** (hat) | Azione pagina | Emergenza (3s) |
| **PWR** | — | Spegnimento |

---

## 6. Pagine display <a name="pagine-display"></a>

### Header (tutte le pagine)
```
EA5JDG-13   5/10   v1.3.0
```
Nominativo-SSID | Pagina corrente/totale | Versione firmware

### Pagina 1 — Meteo locale + posizione

| Riga | Contenuto |
|------|-----------|
| Temp | Temperatura ENV III (°C) |
| Umid | Umidità relativa (%) |
| Press | Pressione atmosferica (mbar) |
| Bat | Tensione batteria + warning |
| Pos | Coordinate o locatore Maidenhead |

### Pagina 2 — GPS Dettaglio

Fix, satelliti, HDOP/PDOP, altitudine, velocità, rotta, coordinate, locatore.
Se GPS non attivo: messaggio con istruzioni.

### Pagina 3 — SNR Satelliti

Grafico a barre SNR per ogni satellite tracciato (GPS, GLONASS, BeiDou, Galileo).

### Pagina 4 — Stato

Batteria, WiFi SSID, IP, RSSI, ultimo TX con esito.

### Pagina 5 — WiFi

Dettagli connessione: SSID, IP, gateway, DNS, RSSI, MAC.

### Pagina 6 — Bluetooth

Stato BLE (abilitato/disabilitato in questa build).

### Pagina 7 — Meteo avanzato

Dati meteo dal sensore locale con formattazione estesa.
MID = menu (commutazione GPS↔ENV).

### Pagina 8 — Astronomia

Alba, tramonto, durata giorno, fase lunare, prossima luna piena/nuova.

### Pagina 9 — Meteo OWM

Dati meteo da OpenWeatherMap (stazione più vicina alle coordinate del locatore):

| Riga | Contenuto |
|------|-----------|
| Temp | Temperatura + percepita |
| Umid | Umidità % |
| Press | Pressione hPa |
| Cond | Descrizione condizioni (es. "cielo sereno") |
| Vento | Range velocità min-max (m/s) |
| Nubi | Copertura nuvolosa % |
| Pioggia 3h | Precipitazioni ultime 3 ore (mm) |
| Agg | Tempo dall'ultimo aggiornamento |
| Up | Uptime dispositivo |

### Pagina 10 — Previsioni

Previsioni da OWM (forecast 5-day/3h):

```
-- Previsioni --
Oggi 18:00
 20-24C Cielo sereno
Oggi 21:00
 18-22C Nubi sparse
Oggi 00:00
 16-19C Poco nuvoloso
Dom. mattina
 14-21C Pioggia leggera
Dom. pomeriggio
 18-25C Sereno
```

---

## 7. Configurazione web <a name="configurazione-web"></a>

Accedere a `http://<IP>:8080/config` dal browser. Campi disponibili:

| Sezione | Parametri |
|---------|-----------|
| WiFi | SSID + password per 3 AP |
| Profili | Nominativo, passcode, SSID APRS, etichetta (×3) |
| Generale | Locatore, simbolo APRS, messaggio status |
| Display | Volume buzzer, melodia boot, refresh display |
| Intervalli TX | Meteo (min), Status (min) |
| **OpenWeatherMap** | API Key (32 char hex) |

Dopo il salvataggio i parametri sono attivi immediatamente (non serve riavvio).

---

## 8. OpenWeatherMap <a name="openweathermap"></a>

### Registrazione

1. Crea account gratuito su https://openweathermap.org
2. Vai a https://home.openweathermap.org/api_keys
3. Copia la API key (32 caratteri esadecimali)
4. Inseriscila nel form `/config` → sezione OpenWeatherMap

### Funzionamento

- La posizione usata è quella del **locatore Maidenhead** configurato
- Aggiornamento ogni 30 minuti (configurabile 5-180 min)
- Free tier: 1000 chiamate/giorno — il dispositivo ne usa ~96 (current + forecast)
- La prima fetch avviene immediatamente dopo l'inserimento della key

### Dati disponibili

- **Current Weather**: temperatura, umidità, pressione, vento, nuvolosità, pioggia
- **Forecast**: previsioni ogni 3h per i prossimi 5 giorni (mostrati 5 slot rilevanti)

---

## 9. APRS-IS <a name="aprs-is"></a>

Il dispositivo si connette a `rotate.aprs2.net:14580` e trasmette:

- **Pacchetto meteo** (formato WX): temperatura, umidità, pressione — ogni N minuti
- **Pacchetto posizione**: coordinate dal GPS o dal locatore — SmartBeaconing o timer
- **Pacchetto status**: messaggio di stato — ogni N minuti
- **Telemetria**: batteria, RSSI, uptime — ogni 15 minuti

Visibile su https://aprs.fi cercando il proprio nominativo.

---

## 10. GPS <a name="gps"></a>

- **Attivazione**: Pagina 7 → MID → seleziona "GPS"
- **SmartBeaconing**: beacon adattivo (velocità + cambio rotta)
- **Multi-costellazione**: GPS + GLONASS + BeiDou + Galileo (parser GSV)
- **Sticky position**: dopo commutazione GPS→ENV, l'ultimo fix rimane attivo

---

## 11. Buzzer e audio <a name="buzzer"></a>

| Evento | Suono |
|--------|-------|
| Boot | Melodia configurabile (0=muto, 1-5=melodie) |
| TX pacchetto | Bip breve |
| Batteria LOW | 1 bip 600 Hz ogni 60s |
| Batteria CRITICA | 3 bip 800 Hz ogni 30s |

Volume configurabile via `/config` (0-100%).

---

## 12. Gestione energia <a name="gestione-energia"></a>

| Stato batteria | Tensione | Indicatore |
|----------------|----------|------------|
| Normale | > 3.5 V | — |
| LOW | 3.3 - 3.5 V | `LOW` su display |
| Critica | < 3.3 V | `!!!` + buzzer |

Spegnimento: tenere premuto PWR per 3 secondi.

---

## 13. Aggiornamento OTA <a name="aggiornamento-ota"></a>

### Via pagina web (raccomandato)

1. Aprire `http://<IP>:8080/update`
2. Selezionare il file `.bin`
3. Attendere upload e riavvio automatico

### Via PlatformIO

```bash
pio run -e coreink_lite_m5u -t upload --upload-port <IP>
```

### Via ArduinoOTA

Qualsiasi client OTA ESP32 compatibile. Hostname: `coreink-meteo`.

---

## 14. Troubleshooting <a name="troubleshooting"></a>

| Problema | Soluzione |
|----------|-----------|
| Display nero dopo OTA | Normale: primo refresh e-ink richiede ~3s |
| "In attesa dati..." su pagina OWM | Attendere 30s dopo inserimento key; verificare WiFi connesso |
| "API key non config." | Inserire key in `/config` → OpenWeatherMap |
| WiFi non si connette | Tenere MID 5s → portale WiFiManager |
| Previsioni di domani mancanti | Normale se è tardi la sera (l'API potrebbe non avere slot "domani" entro i 16 blocchi richiesti) |
| Flash piena (OTA fallisce) | Il bin v1.3.0 è 98.6% della partizione default; se OTA fallisce, usare USB |
| GPS non trova fix | Portare all'esterno; primo fix richiede 1-5 minuti |
