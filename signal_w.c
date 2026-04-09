/**
 * signal_w.c — Sortie vitesse Signal W (J2 broche 3)
 * PCB3071-4 : RD4 → R11 4.7K → Base T5 (BC817 NPN) → J2-3
 *
 * RD4 HIGH = T5 saturé = J2 BAS
 * RD4 LOW  = T5 bloqué = J2 HAUT (repos via pull-up externe)
 *
 * Formule : freq_Hz = speed_kmh × SIGNAL_W_MODE × 1000 / 1609
 * Calcul 100 % entier (uint32), zéro float, zéro malloc.
 *
 * Modes compatibles CANM8 CANNECT PULSE :
 *   SIGNAL_W_MODE_1HZ  (1)  — 1 Hz/MPH  standard (défaut)
 *   SIGNAL_W_MODE_4HZ  (4)  — 4 Hz/MPH  haute résolution
 *   SIGNAL_W_MODE_10HZ (10) — 10 Hz/MPH très haute résolution
 */
#include "signal_w.h"
#include "config.h"

static uint16_t target_freq_hz = 0;
static uint16_t half_period_ms = 0;
static uint32_t last_toggle_ms = 0;

void signal_w_init(void) {
    SIGNAL_W_TRIS  = 0;   /* sortie */
    SIGNAL_W_LAT   = 0;   /* T5 bloqué → repos J2 = HAUT */
    target_freq_hz = 0;
    half_period_ms = 0;
    last_toggle_ms = 0;
}

void signal_w_set_speed(uint8_t speed_kmh) {
    if (speed_kmh == 0) {
        target_freq_hz = 0;
        half_period_ms = 0;
        SIGNAL_W_LAT   = 0;
        return;
    }

    /* freq_Hz = speed_kmh × MODE × 1000 / 1609 — entier strict, zéro float */
    uint32_t freq = ((uint32_t)speed_kmh * (uint32_t)SIGNAL_W_MODE * 1000UL) / 1609UL;

    if (freq < 1UL)    freq = 1UL;
    if (freq > 2000UL) freq = 2000UL;   /* max 10 Hz/MPH × ~200 MPH */

    target_freq_hz = (uint16_t)freq;

    /* demi-période en ms = 500 / freq */
    uint16_t hp = (uint16_t)(500UL / freq);
    if (hp < 1) hp = 1;
    half_period_ms = hp;
}

void signal_w_task(uint32_t tick_ms) {
    if (target_freq_hz == 0) return;

    if (tick_ms - last_toggle_ms >= (uint32_t)half_period_ms) {
        SIGNAL_W_LAT  ^= 1;
        last_toggle_ms = tick_ms;
    }
}

void signal_w_stop(void) {
    target_freq_hz = 0;
    half_period_ms = 0;
    SIGNAL_W_LAT   = 0;
    last_toggle_ms = 0;
}

uint16_t signal_w_get_freq(void) {
    return target_freq_hz;
}
