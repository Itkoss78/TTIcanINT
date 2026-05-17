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
#pragma config FOSC     = HS1       // Crystal haute vitesse 4-16MHz (6MHz)
#pragma config PLLCFG   = ON        // PLL x4 activé → Fcy 24MHz
#pragma config FCMEN    = OFF
#pragma config IESO     = OFF
#pragma config XINST    = OFF       // Extended instruction set désactivé (XC8)
#pragma config PWRTEN   = ON        // Power-up Timer ON (stabilité démarrage)
#pragma config BOREN    = SBORDIS   // Brown-out Reset activé
#pragma config BORV     = 2         // Seuil BOR 2.0V

// Watchdog : activé, période ~2s (WDTPS=1:32768 avec LFINTOSC ~31kHz)
// → Reset automatique si le firmware freeze
#pragma config WDTEN    = ON
#pragma config WDTPS    = 32768

// Dans main.c, CLRWDT() est appelé à chaque itération de la boucle principale
// pour éviter un reset intempestif pendant les phases normales

#pragma config MCLRE    = ON        // MCLR pin activé (ICSP), RE3 désactivé
#pragma config STVREN   = ON        // Stack overflow/underflow reset
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
// PCB3071-4 : cathode rouge → R15 1K → RB5, cathode verte → R14 1K → RB4
#define LED_ON  0   // Cathode à GND → LED ON
#define LED_OFF 1   // Cathode à VCC → LED OFF

#define LED_RED_TRIS    TRISBbits.TRISB5   // PCB3071-4 confirmé
#define LED_GREEN_TRIS  TRISBbits.TRISB4   // PCB3071-4 confirmé
#define LED_RED_LAT     LATBbits.LATB5     // TOUJOURS LATx, jamais PORTx
#define LED_GREEN_LAT   LATBbits.LATB4     // TOUJOURS LATx, jamais PORTx

// ─── SIGNAL W — SORTIE VITESSE (J2 broche 3) ─────────────────────────────────
// PCB3071-4 : RD4 → R11 4.7K → Base T5 (BC817 NPN) → J2-3
// RD4 HIGH = T5 saturé = J2 BAS ; RD4 LOW = T5 bloqué = J2 HAUT
#define SIGNAL_W_TRIS   TRISDbits.TRISD4   // TOUJOURS TRISx
#define SIGNAL_W_LAT    LATDbits.LATD4     // TOUJOURS LATx, jamais PORTx

// Mode fréquence — source : canm8.com/cannect-pulse
// 1 Hz/MPH standard (taximetres, chronotachygraphes)
#define SIGNAL_W_MODE_1HZ   1
#define SIGNAL_W_MODE_4HZ   4
#define SIGNAL_W_MODE_10HZ  10
#ifndef SIGNAL_W_MODE
#define SIGNAL_W_MODE       SIGNAL_W_MODE_1HZ   // ← changer ici selon équipement
#endif

#endif /* CONFIG_H */
