# CoreInk Meteo - Stazione Meteorologica APRS

Stazione meteorologica basata su M5Stack CoreInk con sensore ENV III che pubblica i dati in formato APRS via WiFi alla rete APRS-IS.

## Autore e crediti di sviluppo

**ADAsoft** — Andrea D'Andrea

| Ruolo                  | Responsabile    |
|------------------------|-----------------|
| Design e architettura  | Andrea D'Andrea |
| Codice sorgente        | Claude          |
| Verifica e validazione | Andrea D'Andrea |

## ⚠️ Disclaimer — Uso legale della rete APRS

La rete APRS (Automatic Packet Reporting System) opera su frequenze radio amatoriali regolamentate.
L'uso di questa applicazione per trasmettere dati sulla rete APRS-IS **richiede**:
- Il possesso di una **licenza radioamatoriale valida** nel proprio paese di operazione;
- L'uso **esclusivo del proprio nominativo** assegnato dall'autorità competente del paese in cui si opera;
- Il rispetto delle **normative vigenti** in materia di telecomunicazioni nel proprio paese di operazione.

L'uso di nominativi altrui, di nominativi fittizi o l'operatività senza licenza costituisce **violazione di legge**.
L'autore declina ogni responsabilità per un utilizzo non conforme alle normative applicabili.

## Versione
     
| Versione | Data       | Note                                                   |
|----------|------------|--------------------------------------------------------|
| v1.2.7   | 2026-05-18 | Fix locator 8-char, display 10-char, APRS-IS operativo |
| v1.2.6   | 2026-05-17 | Fix WiFiManager, display, intervalli TX configurabili  |
| v1.2.5   | 2026-05-17 | Build LITE: partizione default.csv senza BLE/logger    |
| v1.3     | 2026-05-17 | Moduli avanzati: buzzer, LED, data logger, astro       |
| v1.2     | 2026-05-17 | Fix NVS, versione FW su display                        |
| v1.1     | 2026-05-16 | OTA WiFi + BLE (no USB)                                |
| v1.0     | 2026-05-05 | Stazione meteo completa                                |
| v0.1     | 2026-05-01 | Prototipo iniziale                                     |

## Hardware

- **M5Stack CoreInk** - Microcontrollore ESP32 con display e-ink, 3 pulsanti e LED verde
- **ENV III Unit** - Sensore con SHT30 (temperatura + umidità) e QMP6988 (pressione barometrica)
- **Modulo GPS** (opzionale) - Per posizione dinamica e SmartBeaconing

## Funzionalità

- Lettura di temperatura, umidità e pressione atmosferica
- Pubblicazione dei dati in formato pacchetto APRS weather report
- Connessione ad APRS-IS via TCP/IP su WiFi
- **WiFi Multi**: memorizzazione di più punti di accesso, connessione automatica al migliore disponibile
- **3 profili stazione** selezionabili al boot con i pulsanti, ognuno con nominativo e icona APRS propri
- **Simbolo APRS configurabile** per profilo (visibile su aprs.fi)
- **GPS opzionale**: le coordinate GPS sovrascrivono il locatore Maidenhead
- **SmartBeaconing**: intervallo di beacon adattivo basato su velocità e cambi di rotta (con GPS)
- **OTA WiFi**: aggiornamento firmware via ArduinoOTA (da PlatformIO) o pagina web (`http://<IP>:8080/update`)
- **OTA BLE**: aggiornamento firmware via Bluetooth da smartphone (nRF Connect o app custom)
- **Bluetooth Low Energy** (opzionale): coesiste con WiFi per funzioni aggiuntive future
- LED verde lampeggiante ad ogni invio pacchetto
- Display e-ink con: dati sensore, nominativo, IP, stato GPS, ultimo TX con orario e stato
- Sincronizzazione oraria tramite NTP
- Posizione in formato locatore Maidenhead

## Configurazione

Modificare `include/config.h` con i propri dati:

1. **WiFi**: fino a 3 reti memorizzate (SSID + password), si connette alla prima disponibile
2. **Profili**: 3 profili con nominativo, SSID APRS, passcode, etichetta e simbolo APRS
3. **Posizione**: Locatore Maidenhead (4 o 6 caratteri)
4. **GPS**: Abilitare/disabilitare e configurare i pin seriale
5. **SmartBeaconing**: Parametri di velocità, intervalli e angolo di svolta
6. **BLE**: Abilitare/disabilitare il Bluetooth Low Energy
7. **OTA**: Abilitare/disabilitare aggiornamento via WiFi e/o BLE
8. **Intervalli**: Frequenza di invio (usata senza SmartBeaconing) e aggiornamento display

## Aggiornamento firmware OTA

Dopo il primo flash (via USB o Bruce firmware), il dispositivo supporta aggiornamento over-the-air:

### WiFi (3 metodi)

1. **PlatformIO CLI**: `pio run -e coreink_ota -t upload` (usa mDNS `coreink-meteo.local`)
2. **Pagina web**: aprire `http://<IP>:8080/update` nel browser e caricare il file `.bin`
3. **ArduinoOTA**: compatibile con qualunque client OTA ESP32

### Bluetooth (BLE OTA)

1. Aprire **nRF Connect** (Android/iOS) e connettersi al dispositivo `CoreInkMeteo`
2. Individuare il servizio UUID `fb1e4001-...`
3. Inviare alla caratteristica CMD: `0x01` + 4 byte dimensione firmware (little-endian)
4. Scrivere i chunk del .bin sulla caratteristica DATA (write-no-response)
5. Inviare alla caratteristica CMD: `0x02` per finalizzare

### Bootstrap iniziale (Bruce firmware)

Se il CoreInk ha già installato il firmware **Bruce**, è possibile usare la sua funzione OTA
per caricare il primo .bin di coreink-meteo senza necessità di USB funzionante.

## Profili

Al boot il display mostra i 3 profili configurati. Usare i pulsanti SU/GIÙ per selezionare e il pulsante centrale per confermare. Dopo 5 secondi senza interazione, viene usato il primo profilo.

Ogni profilo include un simbolo APRS indipendente per la visualizzazione su aprs.fi:

| Tabella | Codice | Icona                 |
|---------|--------|-----------------------|
| `/`     | `_`    | Stazione meteo (blu)  |
| `\`     | `_`    | Stazione meteo (verde)|
| `/`     | `W`    | Sito NWS              |
| `\`     | `W`    | WX supersite          |
| `/`     | `-`    | Casa / QTH            |

## SmartBeaconing

Quando il GPS è abilitato e `SMARTBEACON_ENABLED = 1`, l'algoritmo adatta il beacon:

- **Fermo/lento** (< 5 km/h): beacon ogni 30 minuti
- **Veloce** (> 100 km/h): beacon ogni 60 secondi
- **Cambio di rotta**: beacon immediato se la svolta supera la soglia dinamica
- Interpolazione lineare degli intervalli per velocità intermedie

## Bluetooth

WiFi e BLE coesistono sull'ESP32 tramite time-division multiplexing (confermato dalla documentazione Espressif). Impostando `BLE_ENABLED = 1` il dispositivo si rende visibile come periferica BLE per funzioni aggiuntive future (es. lettura dati da smartphone, configurazione remota, notifiche).

## Formato APRS

Il pacchetto generato segue lo standard APRS weather report:

```
CALLSIGN-13>APRS,TCPIP*:@DDHHMMzLAT[T]LON[S].../...g...tXXXhXXbXXXXX
```

Dove:
- `[T]` = carattere tabella simbolo (`/` o `\`)
- `[S]` = codice simbolo (es. `_` per WX)
- `t` = temperatura (°F)
- `h` = umidità relativa (%)
- `b` = pressione barometrica (decimi di hPa)

## Compilazione

Progetto PlatformIO. Per compilare e caricare:

```bash
pio run -t upload
pio device monitor
```

## Struttura

```
├── platformio.ini              # Configurazione PlatformIO (USB + OTA)
├── scripts/
│   └── fix_i2c_guard.py        # Script pre-build: patch include guard M5Unit-ENV
├── include/
│   ├── config.h                # Configurazione utente (WiFi, profili, GPS, OTA, BLE)
│   ├── aprs.h                  # Formato dei pacchetti APRS
│   ├── aprs_is.h               # Client APRS-IS
│   ├── aprs_telemetry.h        # Telemetria APRS (batteria, RSSI, uptime)
│   ├── astro.h                 # Calcoli astronomici (alba/tramonto, fase lunare)
│   ├── ble_ota.h               # OTA firmware via Bluetooth Low Energy
│   ├── buzzer.h                # Gestione buzzer (eventi sonori, melodie boot)
│   ├── data_logger.h           # Data logger su LittleFS (ring buffer CSV)
│   ├── gps_extra.h             # Parser NMEA esteso (GSV, GGA, GSA multi-costellazione)
│   ├── led_status.h            # Gestione LED stato (SLOW/SOLID/FAST/OFF)
│   ├── nvs_config.h            # Configurazione persistente NVS
│   └── smartbeacon.h           # Algoritmo SmartBeaconing
├── src/
│   ├── main.cpp                # Programma principale + OTA WiFi/Web
│   ├── aprs.cpp                # Implementazione formato APRS
│   ├── aprs_is.cpp             # Implementazione client APRS-IS
│   ├── aprs_telemetry.cpp      # Implementazione telemetria APRS
│   ├── astro.cpp               # Implementazione calcoli astronomici
│   ├── ble_ota.cpp             # Implementazione BLE OTA (servizio GATT)
│   ├── buzzer.cpp              # Implementazione buzzer PWM
│   ├── data_logger.cpp         # Implementazione data logger LittleFS
│   ├── gps_extra.cpp           # Implementazione parser NMEA esteso
│   ├── led_status.cpp          # Implementazione LED stato
│   ├── nvs_config.cpp          # Implementazione storage NVS
│   └── smartbeacon.cpp         # Implementazione SmartBeaconing
├── docs/
│   ├── release_notes.md        # Changelog cumulativo tutte le versioni
│   ├── roadmap.md              # Funzionalità pianificate
│   ├── manuale_utente.md       # Guida utente
│   ├── build_notes_v1.md       # Note tecniche di build
│   ├── coreink_datasheet.md    # Datasheet M5Stack CoreInk
│   └── enviii_datasheet.md     # Datasheet ENV III Unit
└── builds/
    ├── v1.3/                   # Firmware v1.3 + release notes
    ├── v1.2/                   # Release notes (firmware sul dispositivo)
    ├── v1.1/                   # Firmware v1.1 + release notes
    ├── v1.0/                   # Release notes (firmware non disponibile)
    └── v0.1/                   # Release notes (prototipo)
```

## Note

- Il passcode APRS si calcola a partire dal nominativo. Esistono calcolatori online.
- SSID 13 è lo standard per le stazioni meteorologiche APRS.
- L'intervallo di invio consigliato è 10 minuti per stazioni fisse.
- Il LED verde (GPIO10) lampeggia brevemente ad ogni pacchetto inviato con successo.

## Licenza e crediti

Il codice originale di questo progetto è distribuito sotto licenza **MIT** — vedi [LICENSE](LICENSE).

Questo progetto è realizzato nello spirito di condivisione della comunità radioamatoriale: sei libero di usarlo, modificarlo e redistribuirlo, nel rispetto della licenza e citando l'autore originale.

### Librerie di terze parti

| Libreria                                                              | Autore            | Licenza                |
|-----------------------------------------------------------------------|-------------------|------------------------|
| [M5Core-Ink](https://github.com/m5stack/M5Core-Ink)                   | M5Stack           | MIT                    |
| [M5GFX](https://github.com/m5stack/M5GFX)                             | M5Stack           | MIT                    |
| [M5StickCPlus2](https://github.com/m5stack/M5StickCPlus2)             | M5Stack           | MIT                    |
| [M5Unit-ENV](https://github.com/m5stack/M5Unit-ENV)                   | M5Stack           | MIT                    |
| [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)               | Mikal Hart        | GNU LGPL v2.1          |
| [WiFiManager](https://github.com/tzapu/WiFiManager)                   | tzapu             | MIT                    |
| [ESP32 Arduino Framework](https://github.com/espressif/arduino-esp32) | Espressif Systems | LGPL v2.1 / Apache 2.0 |
| NOAA Solar Calculator                                                 | U.S. NOAA / ESRL  | Pubblico dominio       |
