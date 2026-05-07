# CoreInk Meteo - Stazione Meteorologica APRS

Stazione meteorologica basata su M5Stack CoreInk con sensore ENV III che pubblica i dati in formato APRS via WiFi alla rete APRS-IS.

## Autore

**Andrea D'Andrea** — IZ3ARR / EA5JDG

## Versione

| Versione | Data       | Note                         |
|----------|------------|------------------------------|
| v0.1     | 2026-05-05 | Prima versione funzionante   |

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
7. **Intervalli**: Frequenza di invio (usata senza SmartBeaconing) e aggiornamento display

## Profili

Al boot il display mostra i 3 profili configurati. Usare i pulsanti SU/GIÙ per selezionare e il pulsante centrale per confermare. Dopo 5 secondi senza interazione, viene usato il primo profilo.

Ogni profilo include un simbolo APRS indipendente per la visualizzazione su aprs.fi:

| Tabella | Codice | Icona               |
|---------|--------|---------------------|
| `/`     | `_`    | Stazione meteo (blu)|
| `\`     | `_`    | Stazione meteo (verde)|
| `/`     | `W`    | Sito NWS            |
| `\`     | `W`    | WX supersite        |
| `/`     | `-`    | Casa / QTH          |

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
├── platformio.ini          # Configurazione PlatformIO
├── include/
│   ├── config.h            # Configurazione utente (WiFi, profili, GPS, SmartBeacon, BLE)
│   ├── aprs.h              # Formato dei pacchetti APRS
│   ├── aprs_is.h           # Client APRS-IS
│   └── smartbeacon.h       # Algoritmo SmartBeaconing
└── src/
    ├── main.cpp            # Programma principale
    ├── aprs.cpp            # Implementazione formato APRS
    ├── aprs_is.cpp         # Implementazione client APRS-IS
    └── smartbeacon.cpp     # Implementazione SmartBeaconing
```

## Note

- Il passcode APRS si calcola a partire dal nominativo. Esistono calcolatori online.
- SSID 13 è lo standard per le stazioni meteorologiche APRS.
- L'intervallo di invio consigliato è 10 minuti per stazioni fisse.
- Il LED verde (GPIO10) lampeggia brevemente ad ogni pacchetto inviato con successo.
