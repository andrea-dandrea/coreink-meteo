# Release Notes — v1.2.7 (2026-05-18)

Fix precisione locator, display 10-char Maidenhead. Prima versione APRS-IS verificata operativa.

---

## Fix implementati (da buglist v1.2.6)

| # | Bug v1.2.6 | Fix v1.2.7 | Stato |
|---|------------|------------|-------|
| 1 | Locator 8-char ignorato | `maidenhead_to_latlon()` supporta 4/6/8 char con 4ª coppia | ✅ |
| 8 | Locator 6 char su display | `latlon_to_maidenhead()` calcola 10 char per visualizzazione | ✅ |
| — | WiFiManager label locator | Indica "max 8 char", maxlength=10 | ✅ |

---

## Bug noti v1.2.7 (da fixare in v1.2.8)

| # | Bug | Impatto | Priorità |
|---|-----|---------|----------|
| 1 | Nessuna pagina web di configurazione | Solo OTA + CSV su :8080, no config via WiFi domestico | Alta |
| 2 | Profili stazione hardcoded NOCALL | Non editabili, funzionalità morta | Alta |
| 3 | SmartBeacon troppo aggressivo | Jitter GPS scatena TX ogni 20s da fermo | Alta |
| 4 | MID breve non assegnato | Commutazione ENV↔GPS ancora solo su EXT | Media |
| 5 | PARM/UNIT/EQNS ripetuti | Inviati ai primi 2 cicli meteo, non solo al boot | Media |
| 6 | Weather inviato senza ENV III | Dati cached trasmessi anche con sensore scollegato | Media |
| 7 | Commento posizione ridondante | Vbat+FW_VERSION già in telemetria/status | Bassa |
| 8 | Schermo GPS ridondante | Mostra sia sat count che fix sì/no | Bassa |
| 9 | BDS non mostrato | Satelliti BeiDou non visualizzati | Bassa |

---

## Note tecniche
- Buffer locator: `rtLocator[11]` — sufficiente per 10+null
- Conversione 8-char: aggiunta 4ª coppia (cifre 0-9) → precisione ~500m
- Display 10-char: aggiunta 5ª coppia (lettere a-x) → precisione ~20m
- GPS AT6558: fix confermato in 1-2 min (warm start), LED bianco = 1PPS = fix
- Web server: riuso `otaServer` (WebServer su porta 8080), aggiunta routes `/config` GET/POST
