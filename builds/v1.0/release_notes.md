# Release Notes - CoreInk Meteo

---

## v1.0 — Stazione meteo completa (2026-05-05)

Prima versione stabile con tutte le funzionalità base operative.

### Funzionalità

- Lettura sensore ENV III (SHT30 + QMP6988): temperatura, umidità, pressione
- Pubblicazione pacchetti APRS weather report via APRS-IS
- WiFi Multi (più reti memorizzate, connessione automatica)
- 3 profili stazione selezionabili al boot (joystick)
- Simbolo APRS configurabile per profilo
- Posizione in formato Maidenhead
- GPS opzionale (AT6558) con sovrascrittura coordinate
- SmartBeaconing (intervallo adattivo basato su velocità/direzione GPS)
- Display e-ink: dati sensore, IP, nominativo, stato GPS, ultimo TX
- LED verde lampeggio conferma invio pacchetto
- Telemetria APRS (batteria, RSSI, uptime, satelliti GPS)
- Separazione pacchetto posizione e pacchetto meteo
- Salvataggio configurazione in NVS (persistente tra riavvii)
- Sincronizzazione orario NTP
- Supporto dual-board: CoreInk + StickCPlus2

---

## v0.1 — Prototipo iniziale (2026-05-01)

### Funzionalità

- Lettura sensore ENV III base
- Invio pacchetto APRS weather report
- Display e-ink con dati
- Connessione WiFi singola
