# Release Notes — v1.2.5 "Lite" (2026-05-17)

Build ottimizzato per la partizione **default** (app 1.25 MB) attualmente sul dispositivo.
Obiettivo: mandare in onda APRS-IS senza dipendere da USB per cambio partizioni.

---

## Differenze tra versioni

| Versione | Partizione | Dimensione | Caratteristiche principali |
|----------|-----------|------------|---------------------------|
| **v1.2** | default (1.25 MB) | ~1.1 MB | Core APRS + OTA + display base |
| **v1.2.5** | default (1.25 MB) | **1.079 MB** | v1.3 senza BLE/logger/GPS extra |
| **v1.3** | custom (1.94 MB) | 1.94 MB | Tutti i moduli (richiede flash USB) |

---

## Cosa c'è nella v1.2.5

### Core APRS (tutto presente)
- Pacchetti weather report (temperatura, umidità, pressione)
- Pacchetti posizione (da locatore Maidenhead o GPS)
- Telemetria APRS (batteria, RSSI, uptime, satelliti)
- Status APRS ogni 30 minuti
- Connessione APRS-IS (rotate.aprs2.net:14580)
- Guard `canTx`: nessun TX senza callsign/passcode configurati

### Configurazione
- WiFiManager completo (callsign, passcode, locator, simbolo, status, volume, melodia)
- 3 profili stazione selezionabili al boot
- NVS persistente per tutti i parametri
- Simbolo APRS configurabile a runtime

### Display (9 pagine)
- Pagina 0: meteo + posizione + TX status
- Pagina 1: GPS dettaglio (base, senza DOP/costellazioni)
- Pagina 2: GPS satelliti (semplificato)
- Pagina 3: Profili + NVS
- Pagina 4: WiFi info
- Pagina 5: Bluetooth (mostra "BLE OTA: Off")
- Pagina 6: Meteo avanzato
- Pagina 7: Astro (alba/tramonto/luna)
- Pagina 8: Uptime (senza data logger)

### OTA
- ArduinoOTA (porta 3232)
- Web OTA (porta 8080/update)
- NO BLE OTA (rimosso per risparmiare ~150 KB)

### Hardware
- Buzzer: 10 eventi + 6 melodie boot
- LED status: 4 stati + flash TX
- SmartBeaconing GPS
- Hot-swap porta ENV III ↔ GPS
- Fix QMP6988 cold boot (retry ×3 + doppio warmup)

---

## Cosa NON c'è (rispetto a v1.3)

| Modulo rimosso | Risparmio | Motivo |
|----------------|-----------|--------|
| BLE OTA | ~150 KB | Lo stack BLE è il più pesante; Web OTA basta |
| Data Logger (LittleFS) | ~35 KB | Registrazione CSV non essenziale ora |
| GPS Extra (GSV/GGA/GSA parser) | ~12 KB | Info DOP/costellazioni non critiche |

---

## Note di build

| Metrica | Valore |
|---------|--------|
| Flash | 1.073.325 byte (81.9% di 1.310.720) |
| RAM | 52.688 byte (16.1%) |
| Margine flash | **237 KB liberi** (18.1%) |
| Partizione | `default.csv` (app0=app1=1.25 MB, spiffs=1.5 MB) |
| Ambiente PlatformIO | `coreink_lite` / `coreink_lite_ota` |
| CORE_DEBUG_LEVEL | 0 |
| FW_VERSION | "1.2.5" (mostrata su display e in APRS status) |

---

## File binari

| File | Dimensione | Note |
|------|-----------|------|
| `firmware_coreink_v1.2.5.bin` | 1.079.904 byte | Firmware da flashare via Web OTA |

---

## Come flashare

1. Aprire http://192.168.1.92:8080/update nel browser
2. Selezionare `firmware_coreink_v1.2.5.bin`
3. Premere "Carica Firmware"
4. Attendere riavvio (~10 secondi)
5. Verificare versione "v1.2.5" in alto a destra sul display

---

## Prossimi passi

1. Verificare invio APRS-IS (configurare callsign/passcode via WiFiManager)
2. Quando pronto, risolvere USB per flashare partizioni custom
3. Poi upgrade a v1.3 completa via OTA
