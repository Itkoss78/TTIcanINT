# TTICANINT6 Universal Firmware
**Trusted Technology Instruments** — PCB3071-4

> Copyright © 2026 Itsik H. — Tous droits réservés.

## Concept
Boîtier universel de lecture vitesse CAN natif avec phase d'apprentissage automatique.
Élimine le besoin d'une variante firmware par constructeur.

## Hardware
- MCU : PIC18F46K80 @ 24MHz (6MHz xtal + PLL x4)
- Transceiver CAN : MCP2562
- LED RGB : D3 (commune anode)
- EEPROM interne : 1KB

## États LED

| Pattern          | Signification                              |
|------------------|--------------------------------------------|
| Orange clignotant rapide | Boot / initialisation              |
| Rouge lent (500ms)      | Scan véhicule (10s)                |
| Rouge rapide (150ms)    | **Passe 1** — Accélérer et freiner ! |
| Orange fixe             | **Passe 2** — Corrélation en cours  |
| Vert fixe               | ✅ Trouvé — Mode production          |
| Rouge fixe              | ❌ Échec — Nouveau essai dans 5s    |

## Flow automatique

```
Branchement OBD2
      │
      ▼
   BOOT (2s)
      │
      ▼
SCAN VÉHICULE (10s)
  Fingerprint CAN bus
      │
      ├─ Même véhicule qu'EEPROM → PRODUCTION directe (LED verte)
      │
      └─ Nouveau véhicule / EEPROM vide
             │
             ▼
        PASSE 1 (30s min)
        Accélérer + freiner pendant cette phase !
        Filtrage grossier des candidats monotones
             │
             ▼
        PASSE 2 (30s)
        Corrélation Pearson sur candidats
             │
             ├─ Corrélation > 0.95 → Sauvegarder EEPROM → PRODUCTION
             └─ Échec → ERROR (retry 5s)
```

## Changement de véhicule
Automatique. Au branchement sur un nouveau véhicule, le fingerprint CAN
diffère → EEPROM effacée → apprentissage relancé. Aucune intervention requise.

## Paramètres EEPROM sauvegardés
- CAN ID de la trame vitesse
- Offset du byte vitesse dans la trame (0-7)
- Facteur de conversion (Q8 fixe point)
- Signature véhicule (fingerprint 4 bytes)
- CRC intégrité

## Architecture firmware

### Dispatcher centralisé
`can_rx_task()` pousse les trames dans le ring buffer. Un seul `can_get_frame()`
dans la boucle principale distribue en fan-out selon l'état actif :

| État | Destinataires |
|------|---------------|
| `SCAN_VEHICLE` | `vehicle_id_on_frame` + `obd2_on_frame` |
| `LEARN_PASS1/2` | `obd2_on_frame` + `learner_on_frame` + injection vitesse |
| `PRODUCTION` | `can_get_speed_frame` sur l'ID sauvegardé |

### Bitrate CAN
Configuré à **500 kbps** (standard véhicules modernes) :
`BRGCON1=0x01`, `BRGCON2=0x9C`, `BRGCON3=0x02` — 6 MHz / 12 TQ, sample point 75 %.
Pour **250 kbps** (anciens véhicules) : commenter BRP=1 et décommenter BRP=3 dans `can.c`.

### Filtre OBD2
Trames OBD2 exclues de l'apprentissage en 11-bit **et** 29-bit (ISO 15765-4) :
- `0x7DF` / `0x7E8–0x7EF` (11-bit standard)
- `0x18DB33F1` / `0x18DAF100–0x18DAF1FF` (29-bit étendu)

### LED D3 — anode commune
Logique inversée : `LOW = LED ON`, `HIGH = LED OFF`.  
Pins par défaut : RA0 (rouge cathode), RA1 (vert cathode) — **à confirmer dans le schéma Protel avant flash**.  
Un `#warning` de compilation rappelle la vérification.

### Watchdog Timer
Activé avec période ~2 s (`WDTPS=32768`). `CLRWDT()` appelé à chaque itération
de la boucle principale — un freeze firmware déclenche un reset propre.

## Tests unitaires

Suite de tests host (gcc/clang, macOS/Linux) — aucune dépendance, pas de XC8 requis.

```
cd tests && make run
```

### Résultats — 46 / 46 PASS ✅

| Suite | Tests | Couverture |
|-------|------:|------------|
| `test_can` | 9 / 9 ✅ | Ring buffer FIFO, overflow, `can_get_speed_frame` |
| `test_vehicle_id` | 13 / 13 ✅ | Filtrage OBD2 11+29-bit, fréquences, signature XOR top-5 |
| `test_eeprom` | 9 / 9 ✅ | CRC-8, round-trip encode/decode, magic byte, corruption |
| `test_obd2` | 14 / 14 ✅ | Parsing PID 0x0D, IDs/DLC ignorés, timing `obd2_task` |
| `test_learner` | 11 / 11 ✅ | Filtre OBD2, pass1 durée/variation, pass2 corrélation Pearson |
| **Total** | **46 / 46** | |

### Bugs firmware corrigés par les tests

| Fichier | Bug | Correction |
|---------|-----|------------|
| `learner.c` | `pass2_on_frame()` définie mais jamais appelée → passe 2 n'accumulait aucune donnée | Ajout flag `pass2_active`, appel dans `learner_on_frame()` |
| `learner.c` | `(uint8_t)(factor * 256)` déborde à 0 pour un ratio 1:1 (256 → 0 en uint8) | Clip à 255 via `uint32_t` intermédiaire |

## TODO restant
- [ ] Bitrate fixé à **500 kbps** — véhicules modernes uniquement

---

## Licence

Copyright © 2026 **Itsik H.**  
Tous droits réservés.

Ce logiciel et le code source associé sont la propriété exclusive d'Itsik H.
Toute reproduction, distribution, modification ou utilisation, en tout ou en partie,
sans l'autorisation écrite préalable du propriétaire est strictement interdite.

---

*Firmware développé et maintenu par **Itsik H.***
