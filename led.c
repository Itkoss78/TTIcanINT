#include "led.h"
#include "config.h"

static LedPattern current_pattern = LED_PATTERN_BOOT;
static uint32_t last_toggle = 0;
static uint8_t led_state = 0;

void led_init(void) {
    LED_RED_TRIS   = 0;
    LED_GREEN_TRIS = 0;
    LED_RED_LAT    = LED_OFF;
    LED_GREEN_LAT  = LED_OFF;
}

static void set_color(uint8_t red, uint8_t green) {
    // TOUJOURS utiliser LATx pour écriture sur PIC18, jamais PORTx
    LED_RED_LAT   = red   ? LED_ON : LED_OFF;
    LED_GREEN_LAT = green ? LED_ON : LED_OFF;
}

void led_set_pattern(LedPattern pattern) {
    current_pattern = pattern;
}

void led_update(uint32_t tick_ms) {
    uint32_t elapsed = tick_ms - last_toggle;

    switch (current_pattern) {

        case LED_PATTERN_BOOT:
            /* Orange clignotant 200ms — démarrage */
            if (elapsed >= 200) {
                led_state ^= 1;
                if (led_state) set_color(1, 1); /* Orange = rouge+vert */
                else           set_color(0, 0); /* Off */
                last_toggle = tick_ms;
            }
            break;

        case LED_PATTERN_SEARCHING:
            /* Rouge clignotant 500ms — "Searching for CAN information" */
            if (elapsed >= 500) {
                led_state ^= 1;
                if (led_state) set_color(1, 0); /* Rouge */
                else           set_color(0, 0); /* Off */
                last_toggle = tick_ms;
            }
            break;

        case LED_PATTERN_IDENTIFIED:
            /* Rouge FIXE — "CAN received, vehicle not yet identified" */
            set_color(1, 0);
            break;

        case LED_PATTERN_RECOGNISED:
            /* Vert FIXE — "Vehicle type recognised" */
            set_color(0, 1);
            break;

        case LED_PATTERN_SPEED_DETECTED:
            /* Vert clignotant 500ms — "Vehicle speed detected" */
            if (elapsed >= 500) {
                led_state ^= 1;
                if (led_state) set_color(0, 1); /* Vert */
                else           set_color(0, 0); /* Off */
                last_toggle = tick_ms;
            }
            break;

        case LED_PATTERN_ERROR:
            /* Rouge FIXE */
            set_color(1, 0);
            break;
    }
}
