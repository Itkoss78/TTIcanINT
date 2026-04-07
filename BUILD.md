# CANM8 PCB3071-4 — Guide de build et de flash

> Ce fichier **n'est pas un Makefile**. La compilation se fait dans **MPLAB X IDE**.

---

## Prérequis logiciels

| Outil | Version | Lien |
|-------|---------|------|
| **MPLAB X IDE** | 6.x (gratuit) | [microchip.com/mplab/mplab-x-ide](https://www.microchip.com/mplab/mplab-x-ide) |
| **XC8 Compiler** | 2.x (mode gratuit OK) | [microchip.com/xc8](https://www.microchip.com/xc8) |

> Le mode gratuit de XC8 suffit. L'optimisation `-O1` est active par défaut et
> reste compatible avec les contraintes RAM du PIC18F46K80 (3.6 KB RAM, 64 KB Flash).

---

## Fichiers du projet

| Fichier | Rôle |
|---------|------|
| `main.c` | Machine à états (BOOT / SCAN / LEARN\_PASS1 / LEARN\_PASS2 / PRODUCTION / ERROR) |
| `can.c` / `can.h` | Driver ECAN PIC18 + ring buffer RX 32 trames |
| `obd2.c` / `obd2.h` | Requêtes PID 0x0D (vitesse OBD2) toutes les 100 ms |
| `learner.c` / `learner.h` | Algorithme corrélation 2 passes (Pearson entier, sans float) |
| `vehicle_id.c` / `vehicle_id.h` | Fingerprint bus CAN (XOR top-5 IDs fréquents) |
| `eeprom.c` / `eeprom.h` | Save/load config en EEPROM interne (CRC-8) |
| `led.c` / `led.h` | Patterns LED RGB D3 (anode commune) |
| `config.h` | Fuses, defines timing, pins, constantes apprentissage |

---

## Création du projet MPLAB X

1. **File → New Project → Microchip Embedded → Standalone Project**
2. Device : `PIC18F46K80`
3. Tool : PICkit 3 / PICkit 4 / ICD4 / SNAP
4. Compiler : XC8 v2.x
5. Project Name : `CANM8_PCB3071`
6. Ajouter **tous les `.c`** dans *Source Files* et **tous les `.h`** dans *Header Files*
7. **Project Properties → XC8 Global Options → C Standard : `C99`**
8. **Project Properties → XC8 Compiler → Optimizations : `-O1`**

---

## Paramètres à ajuster avant le flash

### ⚠️ Pins LED D3 — à confirmer obligatoirement

Les pins RA0 / RA1 dans `config.h` sont des **placeholders**. Avant de flasher :

1. Ouvrir `PCB3071-4.Sch` dans Altium / Protel
2. Tracer les nets cathode rouge et cathode verte de D3
3. Mettre à jour `LED_RED_TRIS` / `LED_RED_LAT` et `LED_GREEN_TRIS` / `LED_GREEN_LAT` dans `config.h`
4. Définir `LED_RED_PIN_CONFIRMED` et `LED_GREEN_PIN_CONFIRMED` pour supprimer les `#warning`

### Bitrate CAN

> ⚠️ **Le bitrate est fixé à la compilation.** Le PIC18F46K80 ECAN n'a pas
> d'auto-baud hardware, et une détection logicielle (essais successifs) est
> incompatible avec les contraintes du projet (pas de malloc, 3.6 KB RAM,
> ISR légère). **Il faut connaître le bitrate du véhicule cible avant de flasher.**

Réglage dans `can.c` (modifier les valeurs BRGCON) :

| Bitrate | BRGCON1 | BRGCON2 | BRGCON3 | Véhicules typiques |
|---------|---------|---------|---------|-------------------|
| **500 kbps** | `0x01` | `0x9C` | `0x02` | Voitures post-2008, OBD2 standard |
| **250 kbps** | `0x03` | `0x9C` | `0x02` | Anciens véhicules, J1939 camions |
| **125 kbps** | `0x07` | `0x9C` | `0x02` | Très anciens, bus/carrossiers |

> Calcul : crystal 6 MHz × PLL×4 = 24 MHz. BRP=n → Fq = 24 / (2×(n+1)) MHz. 12 TQ fixes.

Les trois jeux de valeurs sont déjà présents en commentaires dans `can.c` — il suffit d'activer le bon bloc.

---

## Compiler et flasher

1. Cliquer sur **Build** (icône marteau) ou `F11` — vérifier 0 erreur, 0 warning critique
2. Brancher le PICkit sur **J1** (header 6 broches ICSP, pas 2.54 mm)
3. Cliquer sur **Make and Program Device** (flèche verte)

---

## Matériel de programmation

### Programmateur

| Option | Prix indicatif | Remarque |
|--------|---------------|----------|
| PICkit 3.5 clone | ~16 € | Meilleur rapport qualité/prix |
| PICkit 3 clone | ~18 € | Compatible, très répandu |
| PICkit 4 clone | ~30–35 € | Standard actuel (réf. DV164005) |
| PICkit 4 original | ~60 € | Chez Mouser / Digikey |

Recherches AliExpress :
- [PICkit 3.5 programmer](https://fr.aliexpress.com/w/wholesale-pickit-3.5-programmer.html)
- [MPLAB PICkit 4](https://fr.aliexpress.com/w/wholesale-mplab-pickit-4.html)

### Câble
Nappe **6 broches femelle au pas 2.54 mm** — livrée avec le PICkit, sinon ~2 € sur AliExpress.

### Alimentation pendant le flash
- **12 V via J2** (connecteur OBD2 — alim de labo ou batterie 12 V)
- **5 V via PICkit** : cocher *"Power target circuit from PICkit"* dans MPLAB X — plus pratique au bureau
