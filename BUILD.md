# CANM8 PCB3071-4 — Guide de build et de flash

> Ce fichier **n'est pas un Makefile**. La compilation se fait dans **MPLAB X IDE**.

---

## Prérequis logiciels

| Outil | Version | Lien |
|-------|---------|------|
| **MPLAB X IDE** | 6.x (gratuit) | [microchip.com/mplab/mplab-x-ide](https://www.microchip.com/mplab/mplab-x-ide) |
| **XC8 Compiler** | 2.x (mode gratuit OK) | [microchip.com/xc8](https://www.microchip.com/xc8) |

> Le mode gratuit de XC8 suffit. L'optimisation `-O1` est active par défaut et
> reste compatible avec les contraintes RAM du PIC18F46K80.

---

## Création du projet MPLAB X

1. **File → New Project → Microchip Embedded → Standalone Project**
2. Device : `PIC18F46K80`
3. Tool : PICkit 3 / PICkit 4 / ICD4 / SNAP
4. Compiler : XC8 (latest)
5. Project Name : `CANM8_PCB3071`
6. Ajouter **tous les `.c`** dans *Source Files* et **tous les `.h`** dans *Header Files*
7. **Project Properties → XC8 Global Options → C Standard : `C99`**

---

## Sélection du protocole (définition à la compilation)

Dans **Project Properties → XC8 Compiler → Preprocessing and Messages → Define macros** :

| Macro | Cible |
|-------|-------|
| `PROTOCOL_OBDII` | **(défaut)** — voitures particuliers |
| `PROTOCOL_J1939` | Camions / bus — SAE J1939 / 250 kbps |
| `PROTOCOL_VW` | VW / Audi / Skoda / Seat MQB |
| `PROTOCOL_PSA` | Peugeot / Citroën |
| `PROTOCOL_MERCEDES` | Mercedes-Benz |

---

## Facteur W (impulsions par km)

Éditer `PULSES_PER_KM` dans `main.c` selon le tachygraphe :

| Valeur | Tachygraphe |
|--------|-------------|
| **4000** | Standard européen (le plus courant) |
| 6000 | Certains anciens Kienzle / VDO |
| 8000 | VDO DTCO 1381 |
| 16000 | Certains Stoneridge |

> La valeur W est gravée sur la plaque sig. du tachygraphe (ex : *"w = 4000 imp/km"*).

---

## Compiler et flasher

1. Cliquer sur **Build** (icône marteau) ou `F11`
2. Brancher le PICkit sur **J1** (header 6 broches ICSP)
3. Cliquer sur **Make and Program Device** (flèche verte)

---

## Matériel de programmation

### Programmateur recommandé

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
Deux options :
- **12 V via J2** (alim de labo ou batterie véhicule)
- **5 V via PICkit** : cocher *"Power target circuit from PICkit"* dans MPLAB X — plus pratique au bureau
