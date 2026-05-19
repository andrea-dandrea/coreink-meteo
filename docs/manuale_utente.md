# CoreInk-Meteo — Manuale Utente

**Versione**: v1.2.8 (coreink_lite)  
**Autori**: EA5JDG / IZ3ARR  
**Hardware**: M5Stack CoreInk + ENV III + GPS/BDS (AT6558)

---

## Indice

1. [Descrizione](#descrizione)
2. [Hardware necessario](#hardware-necessario)
3. [Installazione firmware](#installazione-firmware)
4. [Primo avvio e configurazione WiFi](#primo-avvio)
5. [Navigazione display](#navigazione-display)
6. [Pagine display](#pagine-display)
7. [Parametri configurabili](#parametri-configurabili)
8. [APRS-IS](#aprs-is)
9. [GPS](#gps)
10. [Data Logger](#data-logger)
11. [Buzzer e audio](#buzzer)
12. [Gestione energia](#gestione-energia)
13. [API esterne](#api-esterne)
14. [Aggiornamento OTA](#aggiornamento-ota)
15. [Troubleshooting](#troubleshooting)

---

## 1. Descrizione <a name="descrizione"></a>

CoreInk-Meteo è una stazione meteorologica portatile e tracker APRS basata su
M5Stack CoreInk (ESP32). Rileva temperatura, umidità e pressione atmosferica,
le trasmette alla rete APRS-IS, e offre funzionalità di navigazione GPS,
registrazione dati, e informazioni astro-nautiche.

---

## 2. Hardware necessario <a name="hardware-necessario"></a>

| Componente | Descrizione | Obbligatorio |
|------------|-------------|:---:|
| M5Stack CoreInk | Controller con display e-ink 200×200 | ✓ |
| M5Stack ENV III | Sensore temperatura/umidità/pressione (SHT30 + QMP6988) | ✓ |
| M5Stack GPS/BDS Unit | Modulo GPS AT6558 + MAX2659 | Opzionale |
| Cavo Grove HY2.0-4P | Collegamento I2C/UART | ✓ |

### Connessioni

- ENV III → Porta Grove (G32=SDA, G33=SCL) via I2C
- GPS → Porta Grove (G32=RX, G33=TX) via UART (alternativo a ENV III sullo stesso connettore)

---

## 3. Installazione firmware <a name="installazione-firmware"></a>

### Flash USB (prima installazione)

1. Collegare CoreInk al PC via USB-C
2. Installare driver CH9102 (se necessario)
3. Da PlatformIO: `pio run -e coreink -t upload`

### Aggiornamento OTA WiFi (consigliato)

1. Aprire nel browser: `http://<IP>:8080/update`
2. Selezionare il file `.pio/build/coreink/firmware.bin`
3. Premere "Carica Firmware"
4. Attendere riavvio automatico

### Aggiornamento OTA BLE

1. Installare nRF Connect sul telefono
2. Cercare dispositivo "CoreInkMeteo"
3. Caricare firmware via servizio BLE OTA

---

## 4. Primo avvio e configurazione WiFi <a name="primo-avvio"></a>

Al primo avvio (o se nessuna rete WiFi è configurata):

1. Il dispositivo crea un AP: **CoreInkMeteo-Setup**
2. Connettersi con il telefono a quell'AP
3. Aprire `192.168.4.1` nel browser
4. Selezionare la rete WiFi e inserire la password
5. Compilare i campi: Callsign, APRS Passcode, Locatore Maidenhead
6. Premere "Save" → il dispositivo si connette alla rete

Per riaprire il portale di configurazione WiFi: **tenere premuto MID per 3 secondi**.

---

## 5. Navigazione display <a name="navigazione-display"></a>

### Comandi pulsanti

| Azione | Funzione |
|--------|----------|
| SU | Pagina precedente |
| GIÙ | Pagina successiva |
| MID (pressione breve) | Funzione contestuale: pagina 7 → apre menu; altre → non assegnato |
| MID (pressione lunga 3 s) | Apre portale WiFiManager (da pagina 5) o selezione profilo (da pagina 4, 5 s) |
| EXT (pressione breve) | Nessuna azione |
| EXT (pressione lunga 3 s) | Modo emergenza (segnale APRS di aiuto) |
| DOWN al boot | Reset credenziali WiFiManager |

### Indicatore pagina

Il numero della pagina corrente appare in alto al centro del display.

---

## 6. Pagine display <a name="pagine-display"></a>

Il display e-ink mostra 9 pagine, navigabili con SU/GIÙ. Il numero pagina
corrente (es. `3/9`) appare nell'header in alto a sinistra.

### Pagina 1 — Principale (case 0)

Dati meteo e stato stazione in tempo reale:
- Temperatura, umidità, pressione atmosferica (in mbar)
- Batteria con warning visivo (`LOW` <3.5 V, `!!!` <3.3 V)
- Localizzatore Maidenhead o coordinate GPS (alternano ad ogni aggiornamento)
- Altitudine, velocità, rotta (solo se GPS attivo e fix valido)
- Satelliti e HDOP (solo se GPS)
- Stato WiFi: IP address o "WiFi OFF"
- Stato ultimo TX: orario + OK/FAIL
- Header fisso: callsign-SSID a sinistra, versione FW e ora in alto a destra

### Pagina 2 — GPS Dettaglio (case 1)

Informazioni dettagliate GPS:
- Fix SI/NO, numero satelliti, altitudine, HDOP
- Velocità e rotta
- Coordinate decimali e locatore Maidenhead
- Se GPS non attivo: messaggio "Vai a pag.7, premi MID per GPS"

### Pagina 3 — Stato (case 2)

Diagnostica stazione:
- Batteria con warning visivo
- Uptime (formato `Xh Ym`)
- SSID WiFi connesso / "non connesso"
- IP address, RSSI
- Orario e stato ultimo TX (OK/FAIL)

### Pagina 4 — Profili + NVS (case 3)

Visualizza la configurazione attiva:
- Lista dei 3 profili con indicatore del profilo attivo (`>`)
- Locatore Maidenhead attivo (da NVS)
- Simbolo APRS attivo (tabella + codice)
- `MID 5s`: riapre la selezione profilo

### Pagina 5 — WiFi (case 4)

Stato connettività WiFi:
- Stato ON/OFF
- SSID, IP, RSSI (se connesso)
- mDNS hostname (`coreink-meteo.local`)
- URL aggiornamento OTA (`http://<IP>:8080/update`)
- `MID 3s`: apre portale WiFiManager

### Pagina 6 — Bluetooth (case 5)

Stato BLE:
- In build `coreink_lite`: "BLE OTA: Off" (modulo disabilitato)
- In build completa: stato OTA BLE, nome dispositivo, istruzioni nRF Connect

### Pagina 7 — Meteo (case 6)

Dati meteo con controlli:
- Temperatura, umidità, pressione (etichette estese)
- Porta attiva: ENV III o GPS
- `MID`: apre menu contestuale (timeout 10 s):
  - `1. Forza lettura` — rilettura immediata sensori ENV (doppio campionamento)
  - `2. Commuta ENV/GPS` — toggle porta con salvataggio NVS
- Hint: `EXT 3s: emergenza`

### Pagina 8 — Astro (case 7)

Calcoli astronomici (offline, da posizione NVS o GPS):
- Alba e tramonto (ora locale)
- Durata del giorno
- Fase lunare (nome e numero giorno 0–29)
- Data corrente

### Pagina 9 — Info (case 8)

Informazioni sistema:
- Uptime totale (`Xh Ym`)
- In build con data logger: stato registrazione, record memorizzati, URL download CSV

---

## 7. Parametri configurabili <a name="parametri-configurabili"></a>

Tutti i parametri sono salvati in NVS (persistenti tra riavvii e aggiornamenti OTA).

> **Nota build `coreink_lite`**: i parametri Buzzer, Data Logger, BLE e API esterne
> non sono attivi nella build corrente (v1.2.8). Saranno disponibili in v1.3.

### Stazione APRS

| Parametro | Chiave NVS | Default | Descrizione |
|-----------|-----------|---------|-------------|
| Callsign | `callsign` | "NOCALL" | Nominativo radioamatoriale |
| SSID | (da profilo) | 13 | SSID APRS (0-15) |
| Passcode | `passcode` | "-1" | Codice autenticazione APRS-IS |
| Locatore | `locator` | "JN61fw" | Maidenhead max 8 char (input manuale). Display mostra 10 char calcolati da GPS |
| Status | `aprs_status` | "CoreInk-Meteo v1.3 by EA5JDG/IZ3ARR" | Commento stazione |
| Intervallo status | `status_interval` | 1800000 (30 min) | ms tra invii status |
| Profilo attivo | `profile` | 0 | Indice profilo (0-2) |

### WiFi e connettività

| Parametro | Chiave NVS | Default | Descrizione |
|-----------|-----------|---------|-------------|
| WiFi abilitato | `wifi_enabled` | 1 | 0=off, 1=on |
| WiFi al boot | `wifi_on_boot` | 1 | Riattiva WiFi al reboot |
| BT abilitato | `bt_enabled` | 1 | 0=off, 1=on |
| BT al boot | `bt_on_boot` | 1 | Riattiva BT al reboot |

### Buzzer

| Parametro | Chiave NVS | Default | Descrizione |
|-----------|-----------|---------|-------------|
| Buzzer master | `buzzer_enabled` | 1 | 0=off, 1=on |
| Volume | `buzzer_volume` | 50 | Duty cycle PWM (0-100) |
| Melodia boot | `boot_melody` | 2 | 0=bip, 1=Weather Chime, 2=Sailor's Call, 3=Sunrise, 4=Radio Check, 5=Jingle |
| TX OK | `buzz_tx_ok` | 1 | Bip su invio riuscito |
| TX FAIL | `buzz_tx_fail` | 1 | Bip su invio fallito |
| GPS fix | `buzz_gps_fix` | 1 | Bip su acquisizione fix |
| GPS perso | `buzz_gps_lost` | 1 | Bip su perdita fix |
| WiFi OK | `buzz_wifi_ok` | 1 | Bip su connessione WiFi |
| WiFi perso | `buzz_wifi_lost` | 1 | Bip su disconnessione |
| Batteria bassa | `buzz_bat_low` | 1 | Bip periodico sotto soglia |
| Cambio pagina | `buzz_page` | 1 | Click su navigazione |
| Conferma | `buzz_confirm` | 1 | Bip su pressione MID |

### Data Logger

| Parametro | Chiave NVS | Default | Descrizione |
|-----------|-----------|---------|-------------|
| Logger attivo | `log_enabled` | 1 | 0=stop, 1=registra |
| Intervallo | `log_interval` | 600 | Secondi tra campioni |
| Campo qualità | `log_qual_type` | 0 | 0=n° satelliti, 1=HDOP |

### LED

| Parametro | Chiave NVS | Default | Descrizione |
|-----------|-----------|---------|-------------|
| LED abilitato | `led_enabled` | 1 | 0=off, 1=on |

### Batteria

| Parametro | Chiave NVS | Default | Descrizione |
|-----------|-----------|---------|-------------|
| Campioni ADC | `bat_samples` | 5 | Media mobile su N letture |

### API esterne

| Parametro | Chiave NVS | Default | Descrizione |
|-----------|-----------|---------|-------------|
| OWM API key | `owm_api_key` | "" | Chiave OpenWeatherMap |
| OWM intervallo | `owm_interval` | 30 | Minuti tra aggiornamenti |
| Porto maree | `tide_port` | "" | ID porto WorldTides |
| Tide API key | `tide_api_key` | "" | Chiave WorldTides |

---

## 8. APRS-IS <a name="aprs-is"></a>

### Pacchetti inviati

| Tipo | Contenuto | Intervallo |
|------|-----------|-----------|
| Weather Report | Temperatura, umidità, pressione | 10 min |
| Position | Coordinate (GPS o locatore) + batteria | 10 min |
| Telemetry | Batteria mV, RSSI, uptime, satelliti, flags | 10 min |
| Telemetry Def | PARM/UNIT/EQNS definizioni | 2 ore |
| Status | Commento stazione configurabile | 30 min |

### Ottenere il passcode APRS

Il passcode si genera dal callsign su: https://apps.magicbug.co.uk/passcode/

---

## 9. GPS <a name="gps"></a>

### Modulo supportato

AT6558 + MAX2659 (M5Stack GPS/BDS Unit), comunicazione UART 9600 baud.

### Costellazioni

- GPS (USA): ~12 satelliti visibili
- BeiDou (Cina): ~8 satelliti visibili

### Tempi di acquisizione fix

| Tipo | Tempo |
|------|-------|
| Cold start | ~35 secondi |
| Warm start | ~32 secondi |
| Hot start | ~1 secondo |

### Dati estratti

- Posizione (lat, lon), altitudine, velocità, rotta
- Numero satelliti, HDOP
- SNR per satellite (parser GSV custom)

---

## 10. Data Logger *(v1.3)* <a name="data-logger"></a>

> *Funzionalità non disponibile in `coreink_lite` (v1.2.8). Disponibile in v1.3.*

### Formato record (30 bytes)

| Campo | Tipo | Bytes |
|-------|------|-------|
| Timestamp | uint32 (Unix) | 4 |
| Temperatura | int16 (×10) | 2 |
| Umidità | uint16 (×10) | 2 |
| Pressione | uint16 (×10, offset 900) | 2 |
| Batteria | uint16 (mV) | 2 |
| Latitudine | int32 (×1e6) | 4 |
| Longitudine | int32 (×1e6) | 4 |
| Altitudine | int16 (m) | 2 |
| Velocità | uint16 (×10 km/h) | 2 |
| Rotta | uint16 (×10 gradi) | 2 |
| Sat/HDOP | uint8 | 1 |
| Flags | uint8 | 1 |
| **Totale** | | **30** |

### Capacità

| Intervallo | Record in 1.5 MB | Durata |
|------------|-----------------|--------|
| 1 min | ~52.000 | ~36 giorni |
| 5 min | ~52.000 | ~180 giorni |
| 10 min | ~52.000 | ~360 giorni |

### Download

- Web: `http://<IP>:8080/data` → file CSV con header e timestamp ISO 8601
- BLE: via Nordic UART Service (JSON commands)
- USB: dump seriale

---

## 11. Buzzer e audio *(v1.3)* <a name="buzzer"></a>

> *Funzionalità non disponibile in `coreink_lite` (v1.2.8). Disponibile in v1.3.*

Buzzer passivo su GPIO2. Suoni via PWM con frequenza e durata variabili.

### Melodie di avvio

| ID | Nome | Descrizione |
|----|------|-------------|
| 0 | Bip singolo | Un bip a 2kHz, 100ms |
| 1 | Weather Chime | Do-Mi-Sol-Do (arpeggio ascendente) |
| 2 | Sailor's Call | Sol-Do-Mi + Sol-Do (richiamo marinaro) |
| 3 | Sunrise | Do-Re-Mi-Sol-Do (scala crescendo) |
| 4 | Radio Check | La-La-La-La alta (stile CW) |
| 5 | Jingle minimale | Do-Mi-Sol (tre note secche) |

---

## 12. Gestione energia <a name="gestione-energia"></a>

### Consumi stimati

| Modalità | Consumo |
|----------|---------|
| WiFi + GPS + display | ~150 mA |
| WiFi OFF | ~70 mA |
| Display sleep (e-ink ritiene immagine) | -5 mA |

### Batteria

Batteria interna: 390 mAh @ 3.7V. Autonomia stimata:
- Tutto attivo: ~2 ore
- Solo GPS + display: ~4 ore
- Offline (no WiFi, no BT): ~6 ore

### Toggle WiFi/BT

Disattivabili dalla rispettiva pagina display. I parametri `wifi_on_boot` e `bt_on_boot`
determinano se si riattivano automaticamente dopo un reboot.

---

## 13. API esterne *(v1.3)* <a name="api-esterne"></a>

> *Funzionalità non disponibile in `coreink_lite` (v1.2.8). Disponibile in v1.3.*

### OpenWeatherMap (previsioni meteo)

1. Registrarsi su https://openweathermap.org/
2. Creare una API key gratuita (sezione "API keys")
3. Configurare la chiave nel parametro `owm_api_key`
4. Free tier: 1000 chiamate/giorno (ampiamente sufficiente)

### WorldTides (maree)

1. Registrarsi su https://www.worldtides.info/
2. Ottenere API key
3. Cercare l'ID del porto desiderato
4. Configurare `tide_api_key` e `tide_port`

---

## 14. Aggiornamento OTA <a name="aggiornamento-ota"></a>

### Via Web (consigliato)

1. Verificare che il dispositivo sia connesso alla stessa rete
2. Aprire `http://<IP>:8080/update` nel browser
3. Selezionare il file `firmware.bin`
4. Attendere il completamento e il riavvio automatico

### Via PlatformIO

```
pio run -e coreink_ota -t upload
```

### Via BLE

Usare nRF Connect o app compatibile con BLE OTA DFU.

---

## 15. Troubleshooting <a name="troubleshooting"></a>

| Problema | Soluzione |
|----------|----------|
| Non si accende | Tenere premuto PWR per 2 secondi |
| WiFi non si connette | Long press MID (3s) per WiFiManager |
| Callsign non appare | Verificare configurazione in WiFiManager |
| Locatore troncato | Max 7 caratteri in v1.2, corretto in v1.3 |
| Pressione a 0 | Riavviare, il sensore necessita warmup |
| Batteria fluttua | Normale in v1.2, media mobile in v1.3 |
| BLE non visibile | Bug v1.2, corretto in v1.3 |
| OTA fallisce | Verificare stesso WiFi, provare http://<IP>:8080/update |
| Display non aggiorna | Attendere ciclo refresh (ogni 30s) |
| GPS no fix | Posizionare all'aperto, attendere 35s cold start |

---

*CoreInk-Meteo — Stazione meteorologica APRS portatile*  
*© 2026 EA5JDG / IZ3ARR*
