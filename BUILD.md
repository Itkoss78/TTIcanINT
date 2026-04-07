# =============================================================================
#  CANM8 PCB3071-4 Firmware Build Guide
# =============================================================================
#
#  This is NOT a standalone Makefile. Use MPLAB X IDE to build.
#  This file documents the build process.
#
# =============================================================================
#  PREREQUISITES
# =============================================================================
#
#  1. Download & install MPLAB X IDE (free):
#     https://www.microchip.com/mplab/mplab-x-ide
#
#  2. Download & install XC8 Compiler (free mode works):
#     https://www.microchip.com/xc8
#
# =============================================================================
#  PROJECT SETUP IN MPLAB X
# =============================================================================
#
#  1. File → New Project → Microchip Embedded → Standalone Project
#  2. Device: PIC18F46K80
#  3. Tool: PICkit3/4 or ICD4 (or SNAP)
#  4. Compiler: XC8 (latest)
#  5. Project Name: CANM8_PCB3071
#
#  6. Add main.c to Source Files
#
#  7. Project Properties → XC8 Global Options → C Standard: C99
#
# =============================================================================
#  PROTOCOL SELECTION (compile-time)
# =============================================================================
#
#  In MPLAB X: Project Properties → XC8 Compiler → Preprocessing and Messages
#  Add to "Define macros": one of the following:
#
#    PROTOCOL_OBDII        (default — most passenger cars)
#    PROTOCOL_J1939        (trucks, buses — SAE J1939 / 250kbps)
#    PROTOCOL_VW           (VW/Audi/Skoda/Seat MQB)
#    PROTOCOL_PSA          (Peugeot/Citroën)
#    PROTOCOL_MERCEDES     (Mercedes-Benz)
#
# =============================================================================
#  W FACTOR (PULSES PER KM)
# =============================================================================
#
#  Edit PULSES_PER_KM in main.c to match your tachograph:
#
#    4000  — Standard European (most common)
#    6000  — Some older Kienzle/VDO
#    8000  — VDO DTCO 1381 specific
#   16000  — Some Stoneridge
#
#  The W factor is printed on your tachograph nameplate (ex: "w = 4000 imp/km")
#
# =============================================================================
#  BUILD & FLASH
# =============================================================================
#
#  1. Click "Build" (hammer icon) or F11
#  2. Connect PICkit3/4 to J1 header (6-pin ICSP)
#  3. Click "Make and Program Device" (green arrow)
#
# =============================================================================

Le programmateur — un PICkit 4 (ref Microchip DV164005), c'est le standard actuel. Ça coûte environ 30-35€ sur AliExpress pour un clone, ou ~60€ pour l'original chez Mouser/Digikey. Un PICkit 3 marche aussi si t'en trouves un moins cher. Le PICkit se branche en USB sur ton PC et sur le connecteur J1 de ta board.
Le câble — une nappe 6-pin femelle au pas 2.54mm. C'est le header standard que t'as sur J1. Normalement le PICkit est livré avec, sinon c'est 2€ sur AliExpress.
Les logiciels (gratuits) — MPLAB X IDE + compilateur XC8, tous les deux téléchargeables sur microchip.com. Le mode gratuit du XC8 suffit largement, il optimise juste un peu moins que la version pro mais pour ce firmware c'est pas un souci.
L'alimentation — le module doit être alimenté pendant le flashage. Soit tu le branches en 12V via J2 (alim de labo ou batterie), soit tu coches l'option "Power target circuit from PICkit" dans MPLAB X et le PICkit alimente la board en 5V via J1 directement. La deuxième option est plus simple pour le dev sur le bureau.
Concrètement, ta commande AliExpress c'est juste un PICkit 4 clone et c'est tout — le reste c'est du logiciel gratuit. Tu veux que je te trouve des liens ?

Option 1 — PICkit 3.5 clone (~16€) — Le meilleur rapport qualité/prix, largement suffisant pour flasher un PIC18F46K80 :

https://www.aliexpress.com/item/33005417188.html

Option 2 — PICkit 3 clone (~18€) :

https://www.aliexpress.com/item/32794723685.html

Option 3 — PICkit 4 original/clone (~150€) — Overkill pour ce projet mais si tu veux le vrai deal :

https://www.aliexpress.com/item/32862769493.html

Option 4 — Page de recherche complète pour comparer les prix :

https://fr.aliexpress.com/w/wholesale-mplab-pickit-4.html
https://fr.aliexpress.com/w/wholesale-pickit-3.5-programmer.html