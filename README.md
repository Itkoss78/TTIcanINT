# CANM8 Universal Firmware
**True Chainers** — PCB3071-4

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

## Notes développement

### Bitrate CAN
Le firmware est configuré pour **250 kbps** avec crystal 6MHz + PLL x4.
Pour 500 kbps (standard moderne), ajuster BRGCON1/2/3 dans `can.c`.
Certains véhicules utilisent 125kbps (anciens) ou 1Mbps (récents).

### Pins LED D3
À confirmer sur le schéma réel : les pins RA0/RA1 sont des placeholders.
Vérifier le routage PCB pour les pins exactes de D3 (commune anode).

### OBD2 partage de trame
Le ring buffer CAN est partagé entre `obd2_task` et `learner_on_frame`.
Actuellement, `vehicle_id_scan` consomme les trames pendant le scan.
Pour les passes d'apprentissage, refactoriser en dispatcher depuis `main`.

## TODO
- [ ] Dispatcher centralisé (une seule lecture ring buffer → fan-out)
- [ ] Support 29-bit extended CAN IDs
- [ ] Watchdog timer activation
- [ ] Test sur banc CAN simulé
- [ ] Calibration bitrate par véhicule
