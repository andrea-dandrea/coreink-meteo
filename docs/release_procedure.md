# Procedura Release — CoreInk Meteo

> Regola fondamentale: **il tag si crea solo dopo aver flashato e verificato sul dispositivo fisico.**
> Il tag è la garanzia che quella versione funziona.

---

## FASE 1 — Sviluppo

1. Implementa tutto il previsto per la versione
2. Aggiorna `FW_VERSION` in `platformio.ini` (es. `"1.2.8"`)
3. Build: `pio run -e coreink_lite`

## FASE 2 — Verifica sul dispositivo ⚠️

> Questa fase è obbligatoria e blocca tutto il resto.

4. Flash via USB o OTA
5. Test completo: display, sensori ENV, WiFi/APRS-IS, telemetria, SmartBeacon
6. Controlla su aprs.fi che i pacchetti arrivino corretti
7. **Solo se tutto OK → procedi. Altrimenti torna alla FASE 1.**

## FASE 3 — Archiviazione

8. Copia i binari compilati in `builds/vX.Y/`
9. Scrivi `builds/vX.Y/release_notes.md` con tutte le modifiche rispetto alla versione precedente

## FASE 4 — Commit + Tag

```powershell
git add -A
git commit -m "vX.Y: descrizione breve"
git tag -a vX.Y -m "Release vX.Y"
```

## FASE 5 — Push

```powershell
git push origin master --tags
```

## FASE 6 — GitHub Release

```powershell
# Archivio sorgente (obbligatorio da v1.2.7 in poi)
git archive vX.Y --format=zip --output="builds/vX.Y/source_coreink_vX.Y.zip"

# Crea la release con binari + sorgente
$files = Get-ChildItem "builds/vX.Y/*.bin", "builds/vX.Y/*.zip"
gh release create vX.Y --title "vX.Y - titolo breve" --notes-file "builds/vX.Y/release_notes.md" @files

# Marca come Latest (sempre sull'ultima versione stabile)
gh release edit vX.Y --latest
```

---

## Errori da evitare (lezione appresa da v1.2.7)

| Errore | Conseguenza | Regola |
|--------|-------------|--------|
| Creare il tag prima di flashare | Release pubblica di firmware non verificato | **Flash prima, tag dopo** |
| Creare release retroattive su versioni più vecchie | Ordine errato sulla pagina releases di GitHub | Ricordarsi di `--latest` sulla versione corrente |
| Sovrascrivere una release già pubblicata | Confusione su quale binario è stato flashato | Bumpa la versione invece di sovrascrivere |
