# Mappa Schermi — CoreInk-Meteo v1.3.0

> **Stato implementativo**: in v1.3.0 è attivo solo lo stato S3 NAV con navigazione
> lineare su 10 pagine. Gli stati S0-S2, S4-S8 sono documentati come design target
> per la v1.4 (WiFi FSM + buzzer feedback).

## Tutti gli schermi del sistema

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│  [S0] BOOT ──────────────────────────────► [S2] WiFi MENU  (WIFI_MENU_TIMEOUT_S, def 60s)│
│                                                 │                           │
│                   scelta AP / WiFi OFF ─────────► [S3] NAV                  │
│                                                 │                           │
│                          MID ────────────► [S5] MENU CONTESTUALE            │
│                                                 │                           │
│                          EXT long ───────► [S6] MENU EMERGENZA              │
│                                                 │                           │
│              (da S2 / menu) ────────────► [S2c] CONNECTING  (15s)          │
│                                                 │                           │
│                                          ok ────┴── fail                    │
│                                          │              │                   │
│                                     [S3] NAV      [S4] WAITING              │
│                                                         │                   │
│                                              (MID=menu, ha opzione AP)      │
│                                                         │                   │
│                          (da menu) ──────────────► [S7] AP INTERNO          │
│                                                                             │
│                          (da menu) ──────────────► [S8] CONFERMA            │
│                                                                             │
│  [S1] PROFILI: accessibile solo da menu pagina 3 (NON nel boot)             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Feedback sonoro (buzzer)

| Evento | Suono | Tono |
|--------|-------|------|
| Navigazione UP / DOWN | Bip corto (50 ms) | Acuto ~1500 Hz |
| Conferma azione MID | Bip lungo (150 ms) | Medio ~1000 Hz |
| Annulla / torna indietro EXT | Bip corto (80 ms) | Grave ~600 Hz |
| WiFi connesso | 2 toni ascendenti | — |
| WiFi fallito / errore | Bip doppio breve | Grave ~400 Hz |
| Boot completato | Melodia boot (esistente) | — |

> Il feedback sonoro conferma ogni comando anche quando il display e-ink
> non ha ancora aggiornato (~250 ms) o quando la luce è scarsa.
> I bip UP/DOWN possono essere disabilitati da config.

---

## [S0] BOOT

**Durata**: ~3 secondi (init hardware)

**Display**:
```
┌──────────────────────────────┐
│                              │
│   CoreInk-Meteo  v1.3       │
│                              │
│   Avvio in corso...          │
│                              │
│   ▪ NVS                     │
│   ▪ Sensori                 │
│   ▪ Buzzer/LED              │
│                              │
│                              │
│                              │
└──────────────────────────────┘
```

**Bottoni**: nessuno (auto-avanza)

**Transizioni**:
| Condizione | Destinazione |
|---|---|
| Init completato | → [S3] NAV pagina 0 (WiFi OFF) |

---

## [S1] PROFILI (selezione profilo — NON nel flusso di boot)

> **Nota**: la selezione profilo è stata rimossa dal boot. Si accede a questo schermo
> solo dal menu contestuale della pagina 3 (Profili + NVS), premendo MID.

**Durata**: indefinita (menu utente)

**Display**:
```
┌──────────────────────────────┐
│ Seleziona profilo:           │
│                              │
│ ▸ EA5JDG-13  Meteo casa     │
│   EA5JDG-9   Mobile         │
│   EA5JDG-13  Remota         │
│                              │
│ Porta: ENV III               │
│                              │
│                              │
│                              │
│ SU/GIU:scegli  MID:conferma │
│ Timeout 5s: auto-conferma   │
└──────────────────────────────┘
```

**Bottoni**:
| Bottone | Azione |
|---|---|
| UP | Profilo precedente |
| DOWN | Profilo successivo |
| MID | Conferma e torna a S3 NAV |
| EXT | Annulla e torna a S3 NAV |

**Transizioni**:
| Condizione | Destinazione |
|---|---|
| MID | → [S3] NAV (profilo cambiato, salvato NVS) |
| EXT short | → [S3] NAV (annullato) |

---

## [S2] WiFi MENU (selezione connessione)

> **Nota**: raggiunto dal boot (S0→S2 automatico) e dal menu EMERGENZA.
> Permette di scegliere la rete o mantenere WiFi spento.

**Durata**: `WIFI_MENU_TIMEOUT_S` secondi (default 60s, configurabile in config.h — auto-timeout → riprende ultima impostazione NVS)

**Display**:
```
┌──────────────────────────────┐
│           WiFi               │
│──────────────────────────────│
│  Seleziona connessione:      │
│                              │
│ ▸ [MiCasa 2.4G]    (AP1)    │
│   [MiCasa 5G]      (AP2)    │
│   [Mobile HotSpot] (AP3)    │
│   Apri AP interno            │
│   Mantieni WiFi OFF          │
│                              │
│──────────────────────────────│
│ SU/GIU:scegli  MID:ok        │
│ Timeout 60s: → ultimo usato  │
└──────────────────────────────┘
```

**Bottoni**:
| Bottone | Azione | Suono |
|---|---|---|
| UP | Opzione precedente | bip corto |
| DOWN | Opzione successiva | bip corto |
| MID | Conferma scelta | bip lungo |
| EXT short | WiFi OFF → [S3] NAV | bip grave |
| EXT long (3s) | → [S6] EMERGENZA | — |
| Timeout (WIFI_MENU_TIMEOUT_S) | → [S3] NAV (ultima impostazione NVS) | — |

**Transizioni**:
| Condizione | Destinazione |
|---|---|
| AP1/AP2/AP3 selezionato + MID | → [S2c] CONNECTING |
| "Apri AP interno" + MID | → [S7] AP INTERNO |
| "Mantieni WiFi OFF" + MID | → [S3] NAV (offline) |
| EXT short | → [S3] NAV (offline) |
| Timeout (WIFI_MENU_TIMEOUT_S) | → [S3] NAV (usa ultima impostazione NVS) |
| EXT long | → [S6] EMERGENZA |

---

## [S2c] CONNECTING (tentativo connessione WiFi)

> **Nota**: raggiunto da [S2] WiFi MENU dopo selezione AP, o da [S6] EMERGENZA
> con "Connetti WiFi" (usa credenziali NVS, salta il menu S2).

**Durata**: max 15 secondi (WIFI_TIMEOUT_MS)

**Display**:
```
┌──────────────────────────────┐
│ EA5JDG-13 [WiFi...] v1.3.0  │
│──────────────────────────────│
│                              │
│   Connessione WiFi...        │
│                              │
│   Rete: [SSID salvato]       │
│                              │
│   (attendere 15s)            │
│                              │
│                              │
│──────────────────────────────│
│ EXT long: menu emergenza     │
└──────────────────────────────┘
```

> **Nota e-ink**: nessuna barra di progresso animata. Il display si aggiorna
> all'inizio e alla fine. Nessun refresh durante i 15s.

**Bottoni**:
| Bottone | Azione |
|---|---|
| UP/DOWN | — |
| MID | — |
| EXT short | — |
| EXT long (3s) | → [S6] EMERGENZA |

**Transizioni**:
| Condizione | Destinazione |
|---|---|
| WiFi connesso | → [S3] NAV pagina 0 (con buzzer WIFI_OK) |
| Timeout 15s | → [S4] WAITING |
| EXT long | → [S6] EMERGENZA |

---

## [S3] NAV — Navigazione pagine (stato principale)

**Durata**: indefinita (stato normale di funzionamento)

**Display**: pagina corrente (0-9, totale 10 pagine) con header e contenuto

```
┌──────────────────────────────┐
│ EA5JDG-13 [WiFi✓] 3/10 v1.3 │  ← Header con stato WiFi
│──────────────────────────────│
│                              │
│   (contenuto pagina)         │
│                              │
│                              │
│                              │
│                              │
│                              │
│                              │
│──────────────────────────────│
│           [MID:menu]         │  ← Footer hint
└──────────────────────────────┘
```

**Icone stato WiFi nell'header**:
- `[WiFi✓]` = connesso
- `[WiFi!]` = disconnesso, retry in corso
- `[WiFi✗]` = nessuna credenziale / waiting
- `[WiFi—]` = WiFi OFF
- `[WiFiO]` = AP interno attivo (SoftAP, S7)

**Bottoni**:
| Bottone | Azione |
|---|---|
| UP | Pagina precedente |
| DOWN | Pagina successiva |
| MID | → [S5] Menu contestuale della pagina |
| EXT short | — (nulla) |
| EXT long (3s) | → [S6] EMERGENZA |

**Transizioni automatiche (WiFi state machine in background)**:
| Condizione | Destinazione |
|---|---|
| WiFi perso | Resta in S3, icona cambia a [WiFi!], retry in background |
| Retry ok | Resta in S3, icona torna a [WiFi✓] |
| — | Il loop gira sempre, display aggiorna ogni rtDisplayUpdateMs |

### Pagine NAV (v1.3.0)

| # | Contenuto | Note |
|---|-----------|------|
| 0 | Sensori ENV: Temp, Umid, Press | Dati locali SHT30 + QMP6988 |
| 1 | Stato stazione: call, locator, IP, SSID | Info sistema |
| 2 | GPS: lat/lon, sat, HDOP, velocità | Solo con GPS attivo |
| 3 | APRS: ultimo TX, stato, contatori | — |
| 4 | WiFi: RSSI, SSID, IP, stato | — |
| 5 | Astronomia: alba, tramonto, fase luna | Calcoli NOAA |
| 6 | Telemetria: batteria, RSSI, uptime, bit | — |
| 7 | SmartBeacon: velocità, heading, intervallo | Solo con GPS |
| 8 | **OWM Current**: Temp, Umid, Press, Cond, Vento(range), Nubi, Pioggia 3h | Richiede WiFi + API key |
| 9 | **OWM Forecast**: 5 slot (3 oggi + 2 domani), 2 righe per slot | Richiede WiFi + API key |

---

## [S4] WAITING (nessuna connessione WiFi stabilita)

**Questo è S3 ma con schermata informativa la prima volta.**
In realtà dopo la prima visualizzazione, diventa S3 normale con icona [WiFi✗].

**Display** (mostrato una volta dopo S2 fallito):
```
┌──────────────────────────────┐
│ EA5JDG-13 [WiFi✗] 1/10 v1.3 │
│──────────────────────────────│
│                              │
│   WiFi non disponibile      │
│                              │
│   Funzionamento offline     │
│   Sensori/GPS attivi        │
│                              │
│   Per configurare WiFi:     │
│   MID → menu → "Apri AP"   │
│   oppure EXT long → Emerg.  │
│                              │
│           [MID:menu]         │
└──────────────────────────────┘
```

**Dopo 5 secondi** → transita a [S3] NAV pagina 0 con funzionamento normale (offline).

**Bottoni**: stessi di S3

**Transizioni**:
| Condizione | Destinazione |
|---|---|
| 5s auto | → [S3] NAV (offline, retry ogni 60s in background) |
| MID | → [S5] Menu |
| EXT long | → [S6] EMERGENZA |

---

## [S5] MENU CONTESTUALE (per pagina)

**Durata**: max 10 secondi (auto-timeout)

**Display**:
```
┌──────────────────────────────┐
│ EA5JDG-13   MENU      v1.3  │
│──────────────────────────────│
│                              │
│  ▸ Opzione 1                 │
│    Opzione 2                 │
│    Opzione 3                 │
│    Opzione 4                 │
│                              │
│                              │
│                              │
│──────────────────────────────│
│ SU/GIU:scegli MID:ok EXT:←  │
└──────────────────────────────┘
```

**Bottoni**:
| Bottone | Azione |
|---|---|
| UP | Opzione precedente |
| DOWN | Opzione successiva |
| MID | Esegui opzione selezionata |
| EXT short | Esci → [S3] NAV |
| EXT long (3s) | → [S6] EMERGENZA |
| Timeout 10s | Esci → [S3] NAV |

**Transizioni post-azione**:
| Azione eseguita | Destinazione |
|---|---|
| Azione semplice (leggi sensori, invia pkt) | → [S3] NAV |
| "Apri AP interno" | → [S7] AP INTERNO |
| Azione distruttiva (cancella log, reset) | → [S8] CONFERMA |
| "Connetti ora" | → [S2c] CONNECTING |
| EXT / timeout | → [S3] NAV |

---

## [S6] MENU EMERGENZA (interrupt, sempre accessibile)

**Accesso**: EXT long (3s) da QUALSIASI schermo (interrupt hardware)

**Display**:
```
┌──────────────────────────────┐
│         ⚠ EMERGENZA ⚠        │
│──────────────────────────────│
│                              │
│  ▸ Connetti WiFi             │
│    Apri AP interno           │
│    WiFi OFF                  │
│    Reboot                    │
│                              │
│                              │
│──────────────────────────────│
│ SU/GIU:scegli MID:ok EXT:←  │
└──────────────────────────────┘
```

**Bottoni**:
| Bottone | Azione |
|---|---|
| UP | Opzione precedente |
| DOWN | Opzione successiva |
| MID | Esegui opzione |
| EXT short | Esci → [S3] NAV |
| EXT long | — (già in emergenza) |

**Azioni del menu emergenza**:
| # | Opzione | Effetto | Destinazione |
|---|---------|---------|---|
| 1 | Connetti WiFi | WiFi.begin() con credenziali NVS (salta S2) | → [S2c] CONNECTING |
| 2 | Apri AP interno | Avvia WiFiManager SoftAP | → [S7] AP INTERNO |
| 3 | WiFi OFF | WiFi.disconnect() + mode(OFF) | → [S3] NAV (offline) |
| 4 | Reboot | ESP.restart() | (riavvio) |

---

## [S7] AP INTERNO (WiFiManager captive portal)

**Durata**: max 300 secondi (5 min), poi timeout

**Display** (FISSO, non alterna con altre pagine):
```
┌──────────────────────────────┐
│       AP interno             │
│──────────────────────────────│
│                              │
│  Connettiti a:               │
│  CoreInkMeteo-Setup          │
│                              │
│  Poi vai a:                  │
│  192.168.4.1                 │
│                              │
│  Timeout: 280s               │  ← countdown a -10s
│                              │
│ EXT long: forza uscita       │
└──────────────────────────────┘
```

**Bottoni**:
| Bottone | Azione |
|---|---|
| UP/DOWN | — |
| MID | — |
| EXT short | — |
| EXT long (3s) | **FORZA USCITA** → interrompe WiFiManager → [S3] NAV |

**Nota critica**: WiFiManager è bloccante. Il check di EXT long deve essere fatto:
- Via interrupt ISR che setta flag `emergencyFlag`
- WiFiManager custom callback controlla flag e chiama `wm.stopConfigPortal()`

**Transizioni**:
| Condizione | Destinazione |
|---|---|
| Utente salva config | → [S2c] CONNECTING (con nuove credenziali) |
| Timeout 300s | → [S3] NAV (credenziali invariate) |
| EXT long (interrupt) | → [S6] EMERGENZA oppure → [S3] NAV |

---

## [S8] CONFERMA (azioni distruttive)

**Durata**: max 10 secondi, poi annulla

**Display**:
```
┌──────────────────────────────┐
│        ⚠ CONFERMA ⚠          │
│──────────────────────────────│
│                              │
│  Sei sicuro?                 │
│                              │
│  [descrizione azione]        │
│  es. "Reset credenziali WiFi"│
│                              │
│                              │
│                              │
│──────────────────────────────│
│      MID: Sì    EXT: No     │
└──────────────────────────────┘
```

**Bottoni**:
| Bottone | Azione |
|---|---|
| UP/DOWN | — |
| MID | Conferma → esegui azione |
| EXT | Annulla → torna a schermo precedente |
| Timeout 10s | Annulla → torna a schermo precedente |

---

## Diagramma transizioni completo

```
                    ┌──────┐
        POWER ON───►│  S0  │ BOOT (3s, carica profilo NVS)
                    │ BOOT │
                    └──┬───┘
                       │
                       ▼
                    ┌──────┐
                    │  S2  │ WiFi MENU (60s timeout → NVS)
                    │ WiFi │
                    └──┬───┘
              ┌────────┴──────────────────┐
              │AP selezionato             │WiFi OFF / timeout
              ▼                           ▼
         ┌──────┐                   ┌─────────┐
         │ S2c  │ CONNECTING (15s)  │   S3    │
         │CONN. │                   │  NAV    │◄─────────────────┐
         └──┬───┘                   └────┬────┘                  │
         ┌────┴────┐                    │MID                     │
         │ok  fail │                    ▼                        │
         ▼    ▼                  ┌──────────┐   EXT short        │
    ┌───┐ ┌──────┐               │    S5    │────────────────────┘
    │S3 │ │  S4  │               │   MENU   │
    │NAV│ │WAITNG│─5s►S3         └────┬─────┘
    └───┘ └──────┘                    │MID (esegui)
                       ┌──────────────┼──────────────┐
                       │              │               │
                       ▼              ▼               ▼
                  ┌─────────┐  ┌─────────┐    ┌─────────┐
                  │   S2c   │  │   S7    │    │   S8    │
                  │CONNECT. │  │AP INTERN│    │CONFERMA │
                  └─────────┘  └────┬────┘    └────┬────┘
                                    │               │MID/EXT
                                    │saved/timeout  ▼
                                    ▼         (azione/annulla)
                               ┌─────────┐
                               │ S2c/S3  │
                               └─────────┘

   ╔══════════════════════════════════════════════╗
   ║  EXT LONG (3s) = INTERRUPT da QUALSIASI S   ║
   ║                                              ║
   ║  S0,S2,S2c,S3,S4,S5,S7,S8 ──────► S6       ║
   ║                                  (EMERGENZA) ║
   ║                                              ║
   ║  S6 ── EXT short ──► S3 (NAV)               ║
   ╚══════════════════════════════════════════════╝
```

---

## Cosa succede al REBOOT

Dopo un RST o un reboot:
1. **S0 BOOT** (3s) — mostra "Avvio...", carica l'ultimo profilo NVS (o profilo 1 se primo avvio)
2. **S2 WiFi MENU** (`WIFI_MENU_TIMEOUT_S` = 60s timeout, configurabile) — schermata di selezione WiFi:
   - Utente presente: sceglie AP1/AP2/AP3, AP interno, o WiFi OFF
   - Nessun input entro il timeout: riprende automaticamente l'ultima impostazione WiFi NVS
3. **Se AP selezionato → S2c CONNECTING** (15s):
   - Mostra "Connessione a: [SSID]..."
   - Se successo → S3 con buzzer OK
   - Se fallisce → S4 "WiFi non disponibile"
4. **S3 NAV** — stato operativo normale

> S1 PROFILI non è nel flusso di boot. Il profilo persiste tra i reboot via NVS.
> Per cambiare profilo: naviga fino a pagina 3 → MID → menu → Cambia profilo.

---

## Riepilogo: da dove si può uscire da ogni blocco

| Sei bloccato in... | Via d'uscita |
|---|---|
| S2 WiFi MENU (nessun input) | Timeout (WIFI_MENU_TIMEOUT_S, def 60s) → riprende ultima impostazione NVS |
| S2c CONNECTING (non si connette) | Aspetta 15s → va a S4 automaticamente |
| S4 WAITING (offline) | MID → menu → "Apri AP interno" oppure EXT long → Emergenza |
| S7 AP INTERNO (nessuno si connette) | EXT long (interrupt) → Emergenza |
| S7 AP INTERNO (timeout) | Auto-esce dopo 300s → S3 |
| Loop freezato | RST (hardware) → riparte da S0 |
| Qualsiasi stato con WiFi sbagliato | EXT long → Emergenza → Reboot |
