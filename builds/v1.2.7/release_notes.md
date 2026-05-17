# Release Notes — v1.2.7 (2026-05-18)

Fix precisione locator, display 10-char Maidenhead, preparazione web config e profili.

---

## Fix implementati (da buglist v1.2.6)

| # | Bug v1.2.6 | Fix v1.2.7 | Stato |
|---|------------|------------|-------|
| 1 | Locator 8-char ignorato | `maidenhead_to_latlon()` supporta 4/6/8 char con 4ª coppia | ✅ Fatto |
| 8 | Locator 6 char su display | `latlon_to_maidenhead()` calcola 10 char per visualizzazione | ✅ Fatto |
| — | WiFiManager label locator | Indica "max 8 char", maxlength=10 | ✅ Fatto |

---

## Nuove funzionalità previste

### Web Config (porta 8080)
Pagina HTML accessibile su WiFi domestico con form per:
- Callsign, SSID APRS, Passcode
- Locator Maidenhead (8 char)
- Simbolo APRS
- Messaggio Status
- Intervalli (meteo, telemetria, status, display refresh)
- Volume/melodia buzzer
- Porta mode (ENV/GPS)
- **Scarica config** (JSON export)
- **Carica config** (JSON import)

### Profili → Configurazione singola
- Eliminare il concetto di profili multipli
- Una sola configurazione stazione, editabile via web o WiFiManager
- NVS come storage unico

### Pulsanti ridefiniti
| Pulsante | Azione |
|----------|--------|
| UP | Pagina precedente |
| DOWN | Pagina successiva |
| MID breve | Commutazione ENV↔GPS |
| MID lungo 3s | WiFiManager (AP mode) |
| EXT (sopra) | Libero / Forzare TX immediato? |
| DOWN al boot | Reset credenziali WiFi |

---

## Note tecniche
- Buffer locator: `rtLocator[11]` — già sufficiente per 8+null
- Conversione 8-char: aggiunta 4ª coppia (cifre 0-9) → precisione ~500m
- Web server: riuso `otaServer` (WebServer su porta 8080), aggiunta routes `/config` GET/POST
