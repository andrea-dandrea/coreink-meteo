# UI Button Model — CoreInk-Meteo

> Versione definitiva — decisions chiuse in sessione 18 mag 2026

## Hardware pulsanti (CoreInk)

**StickCPlus2: DEPRECATED** — Non più supportato. Tutto il codice `#ifdef BOARD_STICKCPLUS2` verrà rimosso nella prossima refactor.

## Hardware disponibile (CoreInk)

| Bottone | Posizione | Identificatore |
|---------|-----------|----------------|
| EXT | Superiore (fisico) | M5.BtnEXT |
| UP | Joy su | M5.BtnUP |
| DOWN | Joy giù | M5.BtnDOWN |
| MID | Joy centro (press) | M5.BtnMID |
| PWR | Laterale | Non usato per UI |

---

## Modello a due livelli

### Livello 1: NAVIGAZIONE (default)

Lo stato in cui il dispositivo passa la maggior parte del tempo.

| Bottone | Azione | Feedback |
|---------|--------|----------|
| UP | Pagina precedente | Buzzer PAGE, display aggiorna |
| DOWN | Pagina successiva | Buzzer PAGE, display aggiorna |
| MID (short) | **Entra nel MENU della pagina corrente** | Buzzer MENU, display mostra menu |
| EXT (short) | Nessuna azione | — |
| EXT (long 3s) | **→ MENU EMERGENZA** (via ISR, sempre funziona) | Buzzer EMERGENCY |

### Livello 2: MENU CONTESTUALE (per pagina)

Attivato da MID short in navigazione. Ogni pagina ha il suo set di opzioni.

| Bottone | Azione | Feedback |
|---------|--------|----------|
| UP | Opzione precedente (cursore ▸ si muove) | Nessun buzzer (rapido) |
| DOWN | Opzione successiva | Nessun buzzer |
| MID (short) | **Esegui opzione selezionata** | Buzzer CONFIRM |
| EXT (short) | **Esci dal menu → torna a NAVIGAZIONE** | Buzzer CANCEL (tono basso) |
| EXT (long 3s) | **→ MENU EMERGENZA** (via ISR) | Buzzer EMERGENCY |
| Timeout 10s | Auto-esci → torna a NAVIGAZIONE | Nessun feedback |

### Livello 3: MENU EMERGENZA

Attivato da EXT long (3s) in **qualsiasi** stato UI, tramite ISR hardware.
Garantisce accesso anche se il loop() è bloccato su un'operazione lunga.

| Bottone | Azione | Feedback |
|---------|--------|----------|
| UP | Opzione precedente | Nessun buzzer |
| DOWN | Opzione successiva | Nessun buzzer |
| MID (short) | **Esegui opzione selezionata** | Buzzer CONFIRM |
| EXT (short) | **Esci → torna a NAVIGAZIONE** | Buzzer CANCEL |
| Nessun timeout | — (rimane fino ad azione esplicita) | — |

**Opzioni del menu EMERGENZA:**

| # | Opzione | Azione |
|---|---------|--------|
| 1 | Connetti WiFi | wifiState → WIFI_ST_CONNECTING |
| 2 | Apri AP interno | wifiState → WIFI_ST_AP_CONFIG |
| 3 | WiFi OFF | WiFi.disconnect(), wifiState → WIFI_ST_OFF |
| 4 | Reset credenziali WiFi | → CONFERMA poi WiFiManager.resetSettings() |
| 5 | Reboot | → CONFERMA poi ESP.restart() |

### Livello 4: CONFERMA

Usato prima di azioni distruttive o irreversibili (reset credenziali, reboot).

| Bottone | Azione | Feedback |
|---------|--------|----------|
| MID (short) | **Sì, conferma** | Buzzer CONFIRM, esegui |
| EXT (short) | **No, annulla → torna al menu precedente** | Buzzer CANCEL |
| Timeout 10s | Auto-annulla → torna a NAVIGAZIONE | Nessun feedback |

---

## Rappresentazione visuale del menu

Quando si entra nel menu di una pagina, il display mostra:

```
┌──────────────────────────────┐
│ EA5JDG-13    MENU     v1.3   │  ← Header (come sempre)
│──────────────────────────────│
│                              │
│  ▸ Opzione 1                 │  ← Selezionata (cursore ▸)
│    Opzione 2                 │
│    Opzione 3                 │
│    Opzione 4                 │
│                              │
│                              │
│                              │
│──────────────────────────────│
│ SU/GIU:scegli MID:ok EXT:←  │  ← Footer hint (sempre visibile)
└──────────────────────────────┘
```

---

## Menu contestuali per pagina

### Pagina 0 — Principale (Meteo + posizione)

| # | Opzione | Azione |
|---|---------|--------|
| 1 | Leggi sensori | Forzare readSensors() + aggiorna display |
| 2 | Invia meteo ora | sendWeatherPacket() immediato |
| 3 | Invia posizione | sendPositionPacket() immediato |
| 4 | Cambia porta ENV/GPS | switchPortMode() |

### Pagina 1 — GPS Dettaglio

| # | Opzione | Azione |
|---|---------|--------|
| 1 | Attiva/Disattiva GPS | switchPortMode() toggle |
| 2 | Cold start GPS | Reinit serial, reset fix |
| 3 | Invia posizione GPS | sendPositionPacket() |

### Pagina 2 — SNR Satelliti

| # | Opzione | Azione |
|---|---------|--------|
| 1 | Vista barre/lista | Toggle visualizzazione |
| 2 | Attiva GPS | switchPortMode(GPS) se non attivo |

### Pagina 3 — Profili + NVS

| # | Opzione | Azione |
|---|---------|--------|
| 1 | Profilo 1 (Meteo casa) | Cambia profilo attivo, salva NVS |
| 2 | Profilo 2 (Mobile) | Cambia profilo attivo, salva NVS |
| 3 | Profilo 3 (Remota) | Cambia profilo attivo, salva NVS |
| 4 | Ricarica NVS | Re-read tutti i parametri da NVS |

### Pagina 4 — WiFi ⭐ (la più importante per lo state machine)

| # | Opzione | Azione |
|---|---------|--------|
| 1 | Connetti ora | Forza WIFI_ST_CONNECTING immediato |
| 2 | Disconnetti | WiFi.disconnect(), stato → WAITING |
| 3 | Apri portale AP | Entra in WIFI_ST_AP_CONFIG |
| 4 | Info connessione | Mostra SSID/IP/RSSI dettagliato |
| 5 | WiFi ON/OFF | Toggle wifiEnabled, salva NVS |

### Pagina 5 — Bluetooth

| # | Opzione | Azione |
|---|---------|--------|
| 1 | BLE ON/OFF | Toggle, restart BLE se necessario |
| 2 | Restart BLE adv | Riavvia advertising |

### Pagina 6 — Meteo avanzato

| # | Opzione | Azione |
|---|---------|--------|
| 1 | Forza lettura | readSensors() doppia |
| 2 | Cambia porta | switchPortMode() toggle |
| 3 | Offset temp (future) | Calibrazione ±0.5°C |

### Pagina 7 — Astro

| # | Opzione | Azione |
|---|---------|--------|
| 1 | Ricalcola | Forza ricalcolo alba/tramonto |
| 2 | Giorno +1 | Preview giorno successivo |
| 3 | Giorno -1 | Preview giorno precedente |

### Pagina 8 — Data Logger

| # | Opzione | Azione |
|---|---------|--------|
| 1 | Start/Stop REC | Toggle logger_enable() |
| 2 | Cancella log | logger_clear() con conferma |
| 3 | Intervallo + | +60s |
| 4 | Intervallo - | -60s |

---

## Implementazione: variabili di stato UI

```cpp
enum UiMode {
    UI_NAVIGATE,    // Livello 1: scrolling pagine
    UI_MENU,        // Livello 2: menu contestuale
    UI_EMERGENCY,   // Livello 3: menu emergenza (da EXT long ISR)
    UI_CONFIRM      // Livello 4: conferma azione distruttiva
};

UiMode uiMode = UI_NAVIGATE;
int menuSelection = 0;          // Indice opzione selezionata (0-based)
int menuItemCount = 0;          // Numero opzioni nella pagina corrente
unsigned long menuEnteredAt = 0; // Per auto-timeout
#define MENU_TIMEOUT_MS 10000   // 10 secondi
#define CONFIRM_TIMEOUT_MS 10000 // 10 secondi
volatile bool extEmergencyFlag = false; // Settato da ISR EXT long

// Array di stringhe menu per pagina (in PROGMEM per risparmiare RAM)
// Ogni pagina ha max 5 opzioni
#define MAX_MENU_ITEMS 5
```

---

## Gestione pulsanti nel loop (pseudocodice)

```cpp
void handleButtons() {
    M5.update();

    if (uiMode == UI_NAVIGATE) {
        // --- NAVIGAZIONE ---
        if (M5.BtnUP.wasPressed()) {
            currentPage = (currentPage - 1 + NUM_PAGES) % NUM_PAGES;
            buzzer_play_event(BUZZ_PAGE);
            updateDisplay();
        }
        if (M5.BtnDOWN.wasPressed()) {
            currentPage = (currentPage + 1) % NUM_PAGES;
            buzzer_play_event(BUZZ_PAGE);
            updateDisplay();
        }
        if (M5.BtnMID.wasPressed()) {
            // Entra nel menu contestuale
            uiMode = UI_MENU;
            menuSelection = 0;
            menuItemCount = getMenuItemCount(currentPage);
            menuEnteredAt = millis();
            buzzer_play_event(BUZZ_MENU);
            drawMenu(currentPage);
        }
        // EXT short: nessuna azione in navigazione
        // EXT long: gestito da ISR (vedi sotto)

    } else if (uiMode == UI_MENU) {
        // --- MENU CONTESTUALE ---
        if (M5.BtnUP.wasPressed()) {
            menuSelection = (menuSelection - 1 + menuItemCount) % menuItemCount;
            drawMenu(currentPage);
            menuEnteredAt = millis(); // Reset timeout
        }
        if (M5.BtnDOWN.wasPressed()) {
            menuSelection = (menuSelection + 1) % menuItemCount;
            drawMenu(currentPage);
            menuEnteredAt = millis();
        }
        if (M5.BtnMID.wasPressed()) {
            // Esegui azione
            executeMenuAction(currentPage, menuSelection);
            buzzer_play_event(BUZZ_CONFIRM);
            uiMode = UI_NAVIGATE;
            updateDisplay();
        }
        if (M5.BtnEXT.wasPressed()) {
            // Annulla, torna a navigazione
            uiMode = UI_NAVIGATE;
            updateDisplay();
        }
        // Auto-timeout
        if (millis() - menuEnteredAt > MENU_TIMEOUT_MS) {
            uiMode = UI_NAVIGATE;
            updateDisplay();
        }
    }
}
```

**ISR per EXT long (3s) — garantisce accesso all'emergenza anche con loop bloccato:**

```cpp
// Registrare in setup() — pin EXT su interrupt hardware
void IRAM_ATTR onExtLongPress() {
    // Flag settato dall'ISR, processato nel loop()
    extEmergencyFlag = true;
}

// Nel loop(), controllare prima di tutto il resto:
void loop() {
    if (extEmergencyFlag) {
        extEmergencyFlag = false;
        uiMode = UI_EMERGENCY;
        menuSelection = 0;
        buzzer_play_event(BUZZ_EMERGENCY);
        drawEmergencyMenu();
    }
    // ... resto del loop
}
```

---

## Interazione con WiFi State Machine

Il menu della pagina WiFi (pagina 4) è il punto dove l'utente **interviene manualmente** sullo stato WiFi:

```
┌─────────────────────────────────────────────────────┐
│  PAGINA 4 (WiFi) + MENU CONTESTUALE                │
│                                                     │
│  Opzione "Connetti ora"                             │
│    → wifiState = WIFI_ST_CONNECTING                 │
│    → wifiRetryTime = now                            │
│    → WiFi.begin()                                   │
│                                                     │
│  Opzione "Disconnetti"                              │
│    → WiFi.disconnect()                              │
│    → wifiState = WIFI_ST_WAITING                    │
│                                                     │
│  Opzione "Apri portale AP"                          │
│    → wifiStatePrev = wifiState                      │
│    → wifiState = WIFI_ST_AP_CONFIG                  │
│    → startWiFiManager()                             │
│                                                     │
│  Opzione "WiFi ON/OFF"                              │
│    → toggle wifiEnabled                             │
│    → nvs_save_int(NVS_KEY_WIFI_ENABLED, ...)        │
│    → wifiState = WIFI_ST_OFF / WIFI_ST_CONNECTING   │
└─────────────────────────────────────────────────────┘
```

Questo elimina completamente il bisogno di MID 3s come unico modo per aprire il portale.
L'utente naviga alla pagina WiFi, apre il menu, e sceglie cosa fare.

---

## Flusso visivo completo

```
BOOT → selectProfile (5s) → NAVIGAZIONE pagina 0
                                    │
         ┌──────────────────────────┼──────────────────────────┐
         │                          │                          │
     [UP/DOWN]                   [MID]                      [EXT]
    cambia pagina         entra menu pagina          quick refresh
         │                          │
         ▼                          ▼
    Pagina 0..8              ┌─── MENU ───┐
                             │ ▸ Opzione 1 │
                             │   Opzione 2 │
                             │   Opzione 3 │
                             └─────────────┘
                                    │
                       ┌────────────┼────────────┐
                       │            │            │
                    [UP/DOWN]    [MID]        [EXT]
                   naviga menu  esegui     torna indietro
                                    │
                                    ▼
                            Azione eseguita
                            → NAVIGAZIONE
```

---

## Compatibilità StickCPlus2

**DEPRECATED** — il supporto StickCPlus2 è rimosso. Tutto il codice `#ifdef BOARD_STICKCPLUS2`
verrà eliminato nella prossima refactor.

---

## Note di design

1. **E-ink e refresh**: ogni cambio pagina/menu = refresh completo (0.82s). Per la navigazione manuale è accettabile — l'utente non preme un tasto ogni <15s in modo continuativo. Non fare refresh automatici ravvicinati nel loop.
2. **Footer fisso**: in NAVIGAZIONE mostrare `[MID:menu]` in basso. In MENU mostrare `[SU/GIU MID:ok EXT:←]`.
3. **Nessuna azione distruttiva senza CONFERMA**: "Reset credenziali" e "Reboot" richiedono sempre conferma MID.
4. **Menu vuoto**: se una pagina non ha azioni, MID mostra "Nessuna azione" per 2s e torna.
5. **ISR EXT long**: registrare su interrupt hardware in `setup()`. L'ISR setta solo un flag booleano `extEmergencyFlag`; il menu si disegna nel loop() alla prima iterazione utile.
