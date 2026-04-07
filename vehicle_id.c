#include "vehicle_id.h"
#include "can.h"
#include "config.h"

/**
 * Fingerprint = XOR des 5 IDs CAN les plus fréquents
 * Stable pour un même véhicule (mêmes ECUs actifs)
 * Différent entre véhicules (IDs propriétaires par constructeur)
 */

typedef struct {
    uint32_t id;
    uint16_t count;
} IdFreq;

static IdFreq top[SIGNATURE_TOP_IDS];
static uint8_t top_count = 0;

// Table de fréquence compacte - on track les 32 premiers IDs vus
#define FREQ_TABLE_SIZE 32
static IdFreq freq_table[FREQ_TABLE_SIZE];
static uint8_t freq_count = 0;

static uint32_t signature = 0;
static bool     scan_done = false;

void vehicle_id_scan(uint32_t tick_ms) {
    if (scan_done) return;

    CanFrame frame;
    // Lire toutes les trames disponibles
    while (can_get_frame(&frame)) {
        // Ignorer OBD2
        if (frame.id == 0x7DF || (frame.id >= 0x7E8 && frame.id <= 0x7EF)) continue;

        // Chercher dans table
        bool found = false;
        for (uint8_t i = 0; i < freq_count; i++) {
            if (freq_table[i].id == frame.id) {
                freq_table[i].count++;
                found = true;
                break;
            }
        }
        if (!found && freq_count < FREQ_TABLE_SIZE) {
            freq_table[freq_count].id    = frame.id;
            freq_table[freq_count].count = 1;
            freq_count++;
        }
    }
}

uint32_t vehicle_id_get_signature(void) {
    // Trier pour trouver les top N IDs (tri à bulles simple, table petite)
    for (uint8_t i = 0; i < freq_count; i++) {
        for (uint8_t j = i + 1; j < freq_count; j++) {
            if (freq_table[j].count > freq_table[i].count) {
                IdFreq tmp  = freq_table[i];
                freq_table[i] = freq_table[j];
                freq_table[j] = tmp;
            }
        }
    }

    // XOR des top 5 IDs
    uint32_t sig = 0xDEADBEEF; // seed
    uint8_t  n   = freq_count < SIGNATURE_TOP_IDS ? freq_count : SIGNATURE_TOP_IDS;
    for (uint8_t i = 0; i < n; i++) {
        sig ^= freq_table[i].id * (i + 1); // pondéré par rang
    }

    return sig;
}
