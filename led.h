#ifndef LED_H
#define LED_H

#include <stdint.h>

typedef enum {
    LED_PATTERN_BOOT,            /* Orange clignotant 200ms — démarrage interne            */
    LED_PATTERN_SEARCHING,       /* Rouge clignotant 500ms  — Searching for CAN            */
    LED_PATTERN_IDENTIFIED,      /* Rouge FIXE              — CAN reçu, véhicule non identifié */
    LED_PATTERN_RECOGNISED,      /* Vert FIXE               — Véhicule reconnu (transition)  */
    LED_PATTERN_SPEED_DETECTED,  /* Vert clignotant 500ms   — Vitesse détectée (production)  */
    LED_PATTERN_ERROR            /* Rouge FIXE              — Erreur (même rendu que IDENTIFIED) */
} LedPattern;

void led_init(void);
void led_startup_flash(void);   /* 5 clignotements orange au démarrage */
void led_set_pattern(LedPattern pattern);
void led_update(uint32_t tick_ms);

#endif /* LED_H */
