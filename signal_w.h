/**
 * signal_w.h — Sortie vitesse Signal W (J2 broche 3)
 * PCB3071-4 : RD4 → R11 4.7K → Base T5 (BC817 NPN) → J2-3
 *
 * Fréquence : freq_Hz = speed_kmh × SIGNAL_W_MODE × 1000 / 1609
 * Compatible CANM8 CANNECT PULSE — source : canm8.com/cannect-pulse
 */
#ifndef SIGNAL_W_H
#define SIGNAL_W_H

#include <stdint.h>

void     signal_w_init(void);
void     signal_w_set_speed(uint8_t speed_kmh);
void     signal_w_task(uint32_t tick_ms);
void     signal_w_stop(void);
uint16_t signal_w_get_freq(void);   /* retourne la fréquence cible en Hz */

#endif /* SIGNAL_W_H */
