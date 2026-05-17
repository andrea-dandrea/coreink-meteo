# Release Notes — v1.2.6 (2026-05-17)

Prima versione completamente operativa su APRS-IS. Fix portale di configurazione, display e intervalli TX.

---

## Fix rispetto a v1.2.5

| # | Problema | Fix |
|---|----------|-----|
| 1 | Portale AP si chiudeva in pochi secondi (watchdog 30s) | Watchdog disabilitato durante WiFiManager |
| 2 | Parametri APRS non salvati se WiFi non si connetteva | Salvataggio SEMPRE, indipendente da WiFi |
| 3 | SSID APRS (-13) non configurabile | Nuovo campo WiFiManager (0-15, suggerito 13=meteo) |
| 4 | Locator troncato a 7 char nel campo WiFiManager | Campo ampliato a 12 char |
| 5 | Versione "v1.2.5" troncata a "v1.2" sul display | Allineamento a destra dinamico |
| 6 | "ENV" inutile nell'header | Rimossa |
| 7 | "/_" (simbolo APRS) nella riga TX | Rimosso — ora mostra solo "TX:HH:MM OK/FAIL" |
| 8 | Due orologi indistinguibili | Etichette: "Ora HH:MM" (clock) e "TX:HH:MM" (ultimo invio) |
| 9 | LED fisso acceso senza APRS configurato | LED lento = WiFi ok ma APRS non pronto |
| 10 | Simbolo APRS nella pagina Meteo | Rimosso (non pertinente) |
| 11 | Data senza etichetta nella pagina Astro | Aggiunta "Data: dd/mm/yyyy" |
| 12 | Refresh display non configurabile | Nuovo campo WiFiManager (10-300 secondi) |
| 13 | Intervallo meteo e status non configurabili | Nuovi campi WiFiManager (meteo=5min, status=60min) |
| 14 | Posizione ripetuta ogni 5 min anche se fissa | Posizione solo al boot (locator fisso) o SmartBeacon (GPS) |
| 15 | Burst di pacchetti PARM/UNIT/EQNS ad ogni boot | Solo weather+position al primo TX |
| 16 | QMP6988 legge 0 al primo boot | Doppio readSensors() con 200ms delay |
| 17 | Pulsante DOWN non usato | Reset credenziali WiFiManager (premere al boot) |

---

## Bug noti v1.2.6

| # | Bug | Impatto | Priorità |
|---|-----|---------|----------|
| 1 | **Locator 8-char ignorato**: `maidenhead_to_latlon()` usa solo 6 char, "im99tl32" → centro di "IM99TL" = IM99TL55 | Posizione imprecisa (~1km) | Alta |
| 2 | **Nessuna pagina di configurazione via web**: il server HTTP offre solo OTA + CSV, non c'è modo di cambiare parametri senza riavviare in AP mode | Configurazione scomoda | Alta |
| 3 | **Profili stazione inutilizzabili**: array `profiles[]` hardcoded con NOCALL, non editabili da WiFiManager | Funzionalità morta | Alta |
| 4 | **SmartBeacon troppo aggressivo**: in modalità GPS fissa, invia 3 posizioni in 1 minuto — serve rate limiting | Traffico APRS eccessivo | Alta |
| 5 | **PARM/UNIT/EQNS ripetuti**: inviati ai primi 2 cicli meteo invece che solo al boot | Spreco traffico | Media |
| 6 | **Commento posizione** contiene "Vbat=X.XXV 1.2.6" — Vbat è già in telemetria, FW_VERSION ridondante | Spreco di spazio | Bassa |
| 7 | **Prima lettura temperatura**: QMP6988 cold-boot legge 0, SHT30 potenzialmente anche | Primo dato errato | Media |
| 8 | **Locator a 6 char su schermo 2**: anche con 8 char inseriti, il display mostra solo 6 | Cosmetico | Bassa |
| 9 | **Schermo 3 GPS ridondante**: mostra sia "n. satelliti" sia "fix sì/no" che è derivabile | UX migliorabile | Bassa |
| 10 | **MID breve non fa nulla**: il click della rotellina non ha azione assegnata | UX | Media |
| 11 | **BDS non mostrato**: il dato BeiDou non compare nella pagina GPS | Info mancante | Bassa |

---

## WiFiManager — campi disponibili

Dopo MID 3s → connetti a **CoreInkMeteo-Setup** → http://192.168.4.1 → "Configure WiFi" → scorri sotto le reti:

### Stazione APRS
- **Nominativo** (es. IZ3ARR)
- **SSID APRS** (0-15, 13=meteo)
- **Passcode APRS-IS** (5 cifre, da https://apps.magicbug.co.uk/passcode/)
- **Locatore Maidenhead** (6-8 char, es. JN61fw12)
- **Simbolo APRS** (2 char: /_ = WX, /- = Casa, /y = Antenna)

### Status e Buzzer
- **Messaggio Status APRS**
- **Volume buzzer** (0-100)
- **Melodia boot** (0-5)

### Display e Intervalli
- **Refresh display** (10-300 secondi)
- **Intervallo meteo** (60-3600 secondi, default 300 = 5min)
- **Intervallo status** (600-86400 secondi, default 3600 = 60min)

---

## LED status

| Stato | Significato |
|-------|-------------|
| Spento | WiFi disattivato |
| Lampeggio lento | WiFi ok, APRS non configurato |
| Fisso acceso | WiFi + APRS pronti |
| Lampeggio veloce | Batteria bassa |
| Flash breve | Pacchetto TX inviato |

---

## Note di build

| Metrica | Valore |
|---------|--------|
| Dimensione | 1.080.448 byte (82.4%) |
| Margine flash | 230 KB liberi |
| Partizione | default.csv (app 1.25 MB) |
| Ambiente | `coreink_lite` |

---

## Come flashare

1. http://192.168.1.92:8080/update
2. Selezionare `firmware_coreink_v1.2.6.bin`
3. Attendere riavvio
4. Verificare "v1.2.6" in alto a destra
