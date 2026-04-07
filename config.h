/**
 * CANM8 Universal - Configuration PIC18F46K80
 * PCB: PCB3071-4
 * MCU: PIC18F46K80 @ 6MHz crystal, PLL x4 → 24MHz
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// ─── FUSES ────────────────────────────────────────────────────────────────────
#pragma config FOSC     = HS        // Crystal haute vitesse (6MHz)
#pragma config PLLCFG   = ON        // PLL x4 activé → Fcy 24MHz
#pragma config PRICLKEN = ON        // Clock primaire activée
#pragma config FCMEN    = OFF
#pragma config IESO     = OFF
#pragma config PWRTEN   = ON        // Power-up Timer ON (stabilité démarrage)
#pragma config BOREN    = SBORDIS   // Brown-out Reset activé
#pragma config BORV     = 190       // Seuil BOR 1.9V

// Watchdog : activé, période ~2s (WDTPS=1:32768 avec LFINTOSC ~31kHz)
// → Reset automatique si le firmware freeze
#pragma config WDTEN    = ON
#pragma config WDTPS    = 32768

// Dans main.c, CLRWDT() est appelé à chaque itération de la boucle principale
// pour éviter un reset intempestif pendant les phases normales

#pragma config MCLRE    = EXTMCLR   // MCLR pin activé (ICSP)
#pragma config STVREN   = ON        // Stack overflow/underflow reset
#pragma config LVP      = OFF       // Low-voltage programming désactivé
#pragma config CP0      = OFF       // Code protection désactivée
#pragma config CP1      = OFF
#pragma config CPB      = OFF
#pragma config CPD      = OFF

// ─── TIMING ──────────────────────────────────────────────────────────────────
#define _XTAL_FREQ   24000000UL   // 24MHz après PLL (pour __delay_ms XC8)

// ─── DURATIONS MACHINE À ÉTATS ───────────────────────────────────────────────
#define SCAN_DURATION_MS    10000UL   // 10s scan véhicule
#define PASS1_DURATION_MS   30000UL   // 30s passe 1 (filtrage monotone)
#define PASS2_DURATION_MS   30000UL   // 30s passe 2 (corrélation Pearson)

// ─── PARAMÈTRES LEARNER ──────────────────────────────────────────────────────
#define MAX_CANDIDATES      16        // Candidats max (RAM limitée : 3.6KB)
#define MIN_SPEED_RANGE     30        // Variation min km/h requise en passe 1
#define CORRELATION_THRESH  200       // Seuil corrélation Pearson (0-255, 200≈0.78)
#define MIN_SAMPLES         50        // Échantillons min pour calcul corrélation

// ─── PARAMÈTRES CAN ──────────────────────────────────────────────────────────
#define CAN_RX_BUFFER_SIZE  32        // Ring buffer RX (32 × sizeof(CanFrame) ≈ 800B)

// ─── PARAMÈTRES OBD2 ─────────────────────────────────────────────────────────
#define OBD2_REQUEST_INTERVAL_MS  100UL   // Requête PID 0x0D toutes les 100ms
#define OBD2_TIMEOUT_MS           200UL   // Timeout réponse OBD2

// ─── PARAMÈTRES SIGNATURE VÉHICULE ───────────────────────────────────────────
#define SIGNATURE_TOP_IDS   5         // XOR des 5 IDs les plus fréquents

// ─── EEPROM MAP ──────────────────────────────────────────────────────────────
#define EEPROM_MAGIC_ADDR   0x00      // Adresse octet magic
#define EEPROM_MAGIC_VAL    0xA5      // Valeur magic = données valides
#define EEPROM_DATA_ADDR    0x01      // Début des données (11 octets)

// ─── LED D3 — ANODE COMMUNE (commune → VCC 5V) ───────────────────────────────
// Cathode → pin PIC → LOW = LED allumée, HIGH = LED éteinte
#define LED_ON  0   // Cathode à GND → LED ON  ✓
#define LED_OFF 1   // Cathode à VCC → LED OFF ✓

// ⚠️ PINS À CONFIRMER dans Protel/Altium avant de flasher
// Valeurs par défaut : RA0 (cathode rouge), RA1 (cathode verte)
// Ouvrir PCB3071-4.Sch dans Altium/Protel et tracer les nets cathode de D3
#ifndef LED_RED_PIN_CONFIRMED
#warning "TODO HARDWARE: Confirmer pin cathode rouge de D3 dans le schema Protel"
#endif
#ifndef LED_GREEN_PIN_CONFIRMED
#warning "TODO HARDWARE: Confirmer pin cathode verte de D3 dans le schema Protel"
#endif

#define LED_RED_TRIS    TRISAbits.TRISA0   // A confirmer
#define LED_GREEN_TRIS  TRISAbits.TRISA1   // A confirmer
#define LED_RED_LAT     LATAbits.LATA0     // A confirmer — TOUJOURS LATx, jamais PORTx
#define LED_GREEN_LAT   LATAbits.LATA1     // A confirmer — TOUJOURS LATx, jamais PORTx

#endif /* CONFIG_H */
