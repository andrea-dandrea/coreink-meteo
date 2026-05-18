# Mappa Schermi — CoreInk-Meteo

## Tutti gli schermi del sistema

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│  [S0] BOOT ──────────────────────────────► [S3] NAV  (WiFi OFF)             │
│                                                 │                           │
│                          MID ────────────► [S5] MENU CONTESTUALE            │
│                                                 │                           │
│                          EXT long ───────► [S6] MENU EMERGENZA              │
│                                                 │                           │
│                    (da menu pagina 4) ──► [S2] CONNECTING                   │
│                                                 │                           │
│                                          ok ────┴── fail (timeout 15s)      │
│                                          │              │                   │
│                                     [S3] NAV      [S4] WAITING              │
│                                                         │                   │
│                                              (MID=menu, ha opzione AP)      │
│                                                         │                   │
│                          (da menu) ──────────────► [S7] AP PORTAL           │
│                                                                             │
│                          (da menu) ──────────────► [S8] CONFERMA            │
│                                                                             │
│  [S1] PROFILI: accessibile solo da menu pagina 3 (NON nel boot)             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

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

## [S2] CONNECTING (tentativo connessione WiFi)

> **Nota**: non è più raggiunto dal boot. Si entra da menu pagina 4
> ("Connetti AP 1/2/3") o da menu EMERGENZA ("Connetti WiFi").

**Durata**: max 15 secondi (WIFI_TIMEOUT_MS)

**Display**:
```
┌──────────────────────────────┐
│ EA5JDG-13 [WiFi...] v1.3  │
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

> **Nota e-ink**: nessuna barra di progresso animata. L'aggiornamento del display
> avviene all'inizio ("Connessione...") e alla fine (ok/fail). Nessun refresh durante i 15s.

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

**Display**: pagina corrente (0-8) con header e contenuto

```
┌──────────────────────────────┐
│ EA5JDG-13  [WiFi✓] 3/9 v1.3 │  ← Header con stato WiFi
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

---

## [S4] WAITING (nessuna connessione WiFi stabilita)

**Questo è S3 ma con schermata informativa la prima volta.**
In realtà dopo la prima visualizzazione, diventa S3 normale con icona [WiFi✗].

**Display** (mostrato una volta dopo S2 fallito):
```
┌──────────────────────────────┐
│ EA5JDG-13  [WiFi✗] 1/9 v1.3 │
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
| "Apri portale AP" | → [S7] AP PORTAL |
| Azione distruttiva (cancella log, reset) | → [S8] CONFERMA |
| "Connetti ora" | → [S2] CONNECTING |
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
│    Apri portale AP           │
│    WiFi OFF                  │
│    Reset credenziali WiFi    │
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
| 1 | Connetti WiFi | WiFi.begin() con credenziali salvate | → [S2] CONNECTING |
| 2 | Apri portale AP | Avvia WiFiManager | → [S7] AP PORTAL |
| 3 | WiFi OFF | WiFi.disconnect() + mode(OFF) | → [S3] NAV (offline) |
| 4 | Reset credenziali | wm.resetSettings() | → [S8] CONFERMA prima |
| 5 | Reboot | ESP.restart() | (riavvio) |

---

## [S7] AP PORTAL (WiFiManager captive portal)

**Durata**: max 300 secondi (5 min), poi timeout

**Display** (FISSO, non alterna con altre pagine):
```
┌──────────────────────────────┐
│       WiFi Setup             │
│──────────────────────────────│
│                              │
│  Connettiti a:               │
│  CoreInkMeteo-Setup          │
│                              │
│  Poi vai a:                  │
│  192.168.4.1                 │
│                              │
│  Timeout: 287s               │  ← countdown
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
| Utente salva config | → [S2] CONNECTING (con nuove credenziali) |
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
        POWER ON───►│  S0  │ BOOT (3s auto)
                    │ BOOT │
                    └──┬───┘
                       │
                       ▼
                    ┌──────┐
                    │  S1  │ PROFILI (5s / MID)
                    │ PROF │
                    └──┬───┘
                       │
              ┌────────┴────────┐
              │wifi_enabled     │!wifi_enabled
              ▼                 ▼
         ┌─────────┐      ┌─────────┐
         │   S2    │      │   S3    │
         │CONNECTNG│      │  NAV    │◄─────────────────────────┐
         └────┬────┘      └────┬────┘                          │
              │                │                               │
         ┌────┴────┐          │                               │
         │ok       │fail      │                               │
         ▼         ▼          │                               │
    ┌─────────┐ ┌──────┐     │                               │
    │   S3    │ │  S4  │     │MID                            │
    │  NAV    │ │WAITNG│─5s─►│                               │
    └─────────┘ └──────┘     │                               │
         │                    ▼                               │
         │              ┌──────────┐    EXT short             │
         │              │    S5    │───────────────────────────┘
         │MID           │   MENU   │
         │              └────┬─────┘
         │                   │
         └───────────────────┤
                             │MID (esegui)
                ┌────────────┼────────────────┐
                │            │                │
                ▼            ▼                ▼
           ┌─────────┐ ┌─────────┐     ┌─────────┐
           │   S2    │ │   S7    │     │   S8    │
           │CONNECTNG│ │AP PORTAL│     │CONFERMA │
           └─────────┘ └────┬────┘     └────┬────┘
                            │                │
                            │saved/timeout   │MID/EXT
                            ▼                ▼
                       ┌─────────┐      (azione o annulla)
                       │ S2/S3   │
                       └─────────┘


   ╔══════════════════════════════════════════════╗
   ║  EXT LONG (3s) = INTERRUPT da QUALSIASI S   ║
   ║                                              ║
   ║  S0,S1,S2,S3,S4,S5,S7,S8 ──────► S6        ║
   ║                                  (EMERGENZA) ║
   ║                                              ║
   ║  S6 ── EXT short ──► S3 (NAV)               ║
   ╚══════════════════════════════════════════════╝
```

---

## Cosa succede al REBOOT (il dubbio di stamattina)

Dopo un RST o un reboot:
1. **S0 BOOT** (3s) — mostra "Avvio..."
2. **S1 PROFILI** (5s) — conferma o auto-conferma profilo
3. **S2 CONNECTING** (15s) — **mostra chiaramente cosa sta facendo**:
   - "Connessione a: MiRedDeCasa..."
   - Barra progresso con countdown
   - Se successo → S3 con buzzer
   - Se fallisce → S4 "WiFi non disponibile, funzionamento offline"

**Il problema di stamattina**: dopo RST andava diretto in S2 ma senza display → non sapevi cosa stava facendo. Con questo schema, S2 ha un display dedicato che ti dice esattamente: "sto provando a connettermi a [SSID], 8s/15s...".

---

## Riepilogo: da dove si può uscire da ogni blocco

| Sei bloccato in... | Via d'uscita |
|---|---|
| S2 CONNECTING (non si connette) | Aspetta 15s → va a S4/S3 automaticamente |
| S4 WAITING (offline) | MID → menu → "Apri AP" oppure EXT long → Emergenza |
| S7 AP PORTAL (nessuno si connette) | EXT long (interrupt) → Emergenza |
| S7 AP PORTAL (timeout) | Auto-esce dopo 300s → S3 |
| Loop freezato | RST (hardware) → riparte da S0 |
| Qualsiasi stato con WiFi sbagliato | EXT long → Emergenza → "Reset credenziali" → Reboot |
