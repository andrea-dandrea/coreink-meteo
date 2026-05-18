# WiFi State Machine — CoreInk-Meteo

## Problemi attuali (v1.2.7)

1. **AP automatico nel setup**: se `connectWiFi()` fallisce al boot, apre subito `startWiFiManager()` (portale captive). Questo è **bloccante** e non ritorna al loop finché non scade il timeout (5 min) o l'utente configura.
2. **Reconnect nel loop primitivo**: ogni 60s chiama `connectWiFi()` che prova le credenziali salvate, poi le reti hardcoded di config.h (placeholder "RETE_1/2/3"). Se una rete reale si chiama simile, tenta di connettersi con password sbagliate.
3. **Nessuno stato esplicito**: il codice controlla solo `WiFi.status() == WL_CONNECTED` senza tracciare *perché* è disconnesso né *cosa stava facendo prima*.
4. **Display incoerente**: durante il portale AP non c'è ritorno al loop → schermo fisso. Dopo timeout del portale, il display alterna profili/AP senza logica chiara.
5. **Corruzione dati AP**: WiFiManager salva SSID/password nell'NVS interno di ESP32. Se il portale scade senza salvare, le credenziali precedenti possono essere sovrascritte o corrotte.

---

## Nuova State Machine

```
┌──────────────────────────────────────────────────────────────────────┐
│                          BOOT                                        │
│  Stato iniziale: WIFI_ST_OFF (sempre, indipendente da NVS)           │
│  Il WiFi si attiva SOLO su azione esplicita dell'utente              │
│  dalla pagina 4 (menu WiFi) o dal menu EMERGENZA.                    │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│  WIFI_OFF                                                            │
│  - WiFi.mode(WIFI_OFF)                                               │
│  - Display: icona WiFi barrata                                       │
│  - Nessun retry, nessun AP automatico                                │
│  - Transizioni:                                                      │
│      → WIFI_CONNECTING  da menu pagina 4 → "Connetti AP 1/2/3"       │
│      → WIFI_AP_CONFIG   da menu EMERGENZA → "Apri AP interno"        │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│  WIFI_CONNECTING                                                     │
│  - WiFi.mode(WIFI_STA)                                               │
│  - Tenta connessione su slot AP scelto dall'utente (NVS, 3 slot)     │
│  - NON prova reti hardcoded — wifiNetworks[] ELIMINATO               │
│  - Timeout: WIFI_TIMEOUT_MS (15s)                                    │
│  - Display: icona WiFi lampeggiante (full refresh, no progress bar)  │
│  - LED: SLOW blink                                                   │
│  - Transizioni:                                                      │
│      → WIFI_CONNECTED   se WL_CONNECTED                              │
│      → WIFI_WAITING     se timeout                                   │
│      → WIFI_AP_CONFIG   da menu EMERGENZA (EXT long 3s)              │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│  WIFI_CONNECTED                                                      │
│  - WiFi.mode(WIFI_STA) — connesso                                    │
│  - OTA attivo, APRS-IS attivo (se canTx)                             │
│  - Display: icona WiFi solida + SSID/IP (in pagina info)             │
│  - LED: SOLID (se APRS configurato) / SLOW (se NOCALL)              │
│  - Transizioni:                                                      │
│      → WIFI_DISCONNECTED  se WiFi.status() != WL_CONNECTED           │
│      → WIFI_AP_CONFIG     da menu EMERGENZA (EXT long 3s)            │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│  WIFI_DISCONNECTED                                                   │
│  - WiFi caduto dopo essere stato connesso                            │
│  - Timer backoff: 15s → 30s → 60s → 60s (max)                       │
│  - Display: icona WiFi con "!" + countdown al prossimo retry         │
│  - LED: SLOW blink                                                   │
│  - Transizioni:                                                      │
│      → WIFI_CONNECTING   quando scade il timer di retry              │
│      → WIFI_AP_CONFIG    da menu EMERGENZA (EXT long 3s)             │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│  WIFI_WAITING                                                        │
│  - Mai stato connesso (tentativo utente fallito, timeout 15s)        │
│  - Nessun retry automatico — l'utente decide quando riprovare        │
│  - Il loop GIRA normalmente (display, sensori, GPS)                  │
│  - Display: icona [WiFi✗] + "EXT long: menu emergenza"              │
│  - LED: OFF o SLOW                                                   │
│  - Transizioni:                                                      │
│      → WIFI_CONNECTING   da menu pagina 4 → "Connetti"              │
│      → WIFI_AP_CONFIG    da menu EMERGENZA (EXT long 3s)             │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│  WIFI_AP_CONFIG                                                      │
│  - Portale captive SOLO su richiesta esplicita (da menu EMERGENZA)   │
│  - WiFi.mode(WIFI_AP_STA) — AP per config + STA per test connessione│
│  - Timeout: 300s (5 min) poi → stato precedente                      │
│  - Display: schermata dedicata AP con SSID/IP (non alterna pagine!)  │
│  - LED: FAST blink                                                   │
│  - Transizioni:                                                      │
│      → WIFI_CONNECTING   dopo salvataggio credenziali (wm ok)        │
│      → stato_precedente  dopo timeout senza salvataggio              │
└──────────────────────────────────────────────────────────────────────┘
```

---

## Diagramma transizioni (riassunto)

```
              ┌─────────┐
    boot ────►│CONNECTING├───ok──►┌─────────┐
              └────┬────┘         │CONNECTED │
                   │fail          └────┬─────┘
                   ▼                   │lost
              ┌────────┐               ▼
              │WAITING │◄──────── ┌────────────┐
              └────┬───┘  retry   │DISCONNECTED│
                   │              └─────┬──────┘
                   │                    │retry
                   │              ┌─────▼──────┐
                   └──retry──────►│ CONNECTING │
                                  └────────────┘

         Da QUALSIASI stato:
              EXT long (3s) → MENU EMERGENZA → scelta "Apri AP interno" → WIFI_AP_CONFIG
              EXT long (3s) → MENU EMERGENZA → scelta "Connetti WiFi"   → WIFI_CONNECTING
              EXT long (3s) → MENU EMERGENZA → scelta "WiFi OFF"        → WIFI_OFF
              AP_CONFIG timeout → stato precedente
              AP_CONFIG salvato → WIFI_CONNECTING

         WIFI_OFF: isolato, entra solo se NVS wifi_enabled=0
```

---

## Regole fondamentali

| Regola | Descrizione |
|--------|-------------|
| **R1** | L'AP di configurazione si apre SOLO da menu EMERGENZA (EXT long 3s → "Apri AP interno"). MAI automaticamente. |
| **R2** | Il loop() non si blocca MAI. Anche senza WiFi, display/sensori/GPS funzionano. |
| **R3** | Le reti hardcoded in config.h vengono ELIMINATE. Solo WiFiManager gestisce le credenziali. |
| **R4** | Dopo disconnessione, retry con backoff progressivo (15s, 30s, 60s max). |
| **R5** | Lo stato WiFi è tracciato con una variabile enum globale `wifiState`. |
| **R6** | Il display mostra SEMPRE lo stato WiFi come icona/testo nella barra di stato. |
| **R7** | Il portale AP ha display FISSO dedicato (non alterna con altre pagine). |
| **R8** | Dopo timeout del portale AP si torna allo stato precedente senza corrompere nulla. |

---

## Schermate display per stato WiFi

| Stato | Barra superiore | Pagina corrente | Note |
|-------|----------------|-----------------|------|
| WIFI_OFF | `[WiFi OFF]` | Pagine normali 0-8 | Tutto funziona tranne TX |
| WIFI_CONNECTING | `[WiFi...]` lampeggiante | Pagine normali 0-8 | Loop gira, display aggiorna |
| WIFI_CONNECTED | `[WiFi ✓]` + RSSI bar | Pagine normali 0-8 | Operazione piena |
| WIFI_DISCONNECTED | `[WiFi !]` + countdown | Pagine normali 0-8 | Sensori/GPS attivi |
| WIFI_WAITING | `[No WiFi]` | Pagine normali + hint MID 3s | Primo boot senza credenziali |
| WIFI_AP_CONFIG | Schermata FISSA dedicata | "AP: CoreInkMeteo-Setup / 192.168.4.1 / timeout Xs" | **Non si alterna** con altre pagine |

---

## Eliminazione reti hardcoded

Il vettore `wifiNetworks[]` in config.h è **ELIMINATO**. Al suo posto: 3 slot AP salvati in NVS.

- `nvsWifi[0..2]` — SSID + password per 3 reti configurabili
- Configurazione via web `:8080/config` o portale captive (AP_CONFIG)
- L'utente sceglie quale slot usare dal menu pagina 4
- Nessuna rete hardcoded nel firmware — zero tentativi di connessione a placeholder ("RETE_1/2/3")
- WiFiManager rimane per la configurazione iniziale (primo avvio senza credenziali)

---

## Implementazione proposta (struct)

```cpp
enum WifiState {
    WIFI_ST_OFF,          // WiFi spento da NVS
    WIFI_ST_CONNECTING,   // Tentativo connessione in corso
    WIFI_ST_CONNECTED,    // Connesso e operativo
    WIFI_ST_DISCONNECTED, // Era connesso, ha perso connessione
    WIFI_ST_WAITING,      // Mai connesso, attende retry o config utente
    WIFI_ST_AP_CONFIG     // Portale captive attivo (su richiesta utente)
};

WifiState wifiState = WIFI_ST_OFF;
WifiState wifiStatePrev = WIFI_ST_OFF;  // Per tornare dopo AP_CONFIG
unsigned long wifiRetryTime = 0;        // Prossimo retry
int wifiRetryBackoff = 15000;           // Backoff attuale (ms)
```

---

## Funzione wifi_update() — da chiamare nel loop()

```cpp
void wifi_update() {
    unsigned long now = millis();

    switch (wifiState) {
        case WIFI_ST_OFF:
            // Nessuna azione
            break;

        case WIFI_ST_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                wifiState = WIFI_ST_CONNECTED;
                wifiRetryBackoff = 15000;  // Reset backoff
                onWifiConnected();
            } else if (now - wifiRetryTime > WIFI_TIMEOUT_MS) {
                // Timeout connessione
                WiFi.disconnect();
                if (wifiStatePrev == WIFI_ST_CONNECTED || wifiStatePrev == WIFI_ST_DISCONNECTED) {
                    wifiState = WIFI_ST_DISCONNECTED;
                } else {
                    wifiState = WIFI_ST_WAITING;
                }
                wifiRetryTime = now;  // Timer per prossimo retry
            }
            break;

        case WIFI_ST_CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
                wifiState = WIFI_ST_DISCONNECTED;
                wifiRetryTime = now;
                wifiRetryBackoff = 15000;
                led_set_state(LED_SLOW);
            }
            break;

        case WIFI_ST_DISCONNECTED:
            if (now - wifiRetryTime >= (unsigned long)wifiRetryBackoff) {
                wifiStatePrev = WIFI_ST_DISCONNECTED;
                wifiState = WIFI_ST_CONNECTING;
                wifiRetryTime = now;
                WiFi.begin();  // Usa credenziali salvate
                // Incrementa backoff: 15s → 30s → 60s (max)
                if (wifiRetryBackoff < 60000) wifiRetryBackoff *= 2;
                if (wifiRetryBackoff > 60000) wifiRetryBackoff = 60000;
            }
            break;

        case WIFI_ST_WAITING:
            if (now - wifiRetryTime >= 60000) {
                wifiStatePrev = WIFI_ST_WAITING;
                wifiState = WIFI_ST_CONNECTING;
                wifiRetryTime = now;
                WiFi.begin();
            }
            break;

        case WIFI_ST_AP_CONFIG:
            // Gestito da startWiFiManager(), ritorna quando finisce
            break;
    }
}
```

---

## Sequenza Boot (nuova)

```
1. NVS init
2. Profilo (selectProfile) — display menu, 5s timeout
3. Carica parametri NVS (callsign, passcode, ecc.)
4. wifiEnabled = NVS
5. Se wifiEnabled:
     WiFi.mode(WIFI_STA)
     WiFi.begin()  ← credenziali salvate da WiFiManager
     wifiState = WIFI_ST_CONNECTING
     wifiRetryTime = now
   Altrimenti:
     WiFi.mode(WIFI_OFF)
     wifiState = WIFI_ST_OFF
6. Sensori, display, GPS — PROSEGUIRE SENZA ASPETTARE WiFi
7. Nel loop: wifi_update() gestisce tutto
```

**Differenza chiave**: al boot NON si blocca ad aspettare il WiFi e NON apre l'AP automaticamente.

---

## Domande aperte per la decisione

1. **Se al primissimo boot non ci sono credenziali salvate**, il dispositivo funziona normalmente in WIFI_WAITING e l'utente deve premere MID 3s per configurare. Va bene?
2. **DOWN al boot = reset credenziali**: manteniamo questa funzione di emergenza?
3. **Timeout portale**: 5 minuti o configurabile?
4. **Display durante CONNECTING**: mostrare un indicatore nella barra superiore basta, o serve una pagina dedicata "Connessione..."?
