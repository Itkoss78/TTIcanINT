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
            // Orange clignotant rapide 100ms
            if (elapsed >= 100) {
                led_state ^= 1;
                if (led_state) set_color(1, 1); // Orange
                else           set_color(0, 0); // Off
                last_toggle = tick_ms;
            }
            break;

        case LED_PATTERN_RED_SLOW:
            // Rouge clignotant lent 500ms
            if (elapsed >= 500) {
                led_state ^= 1;
                if (led_state) set_color(1, 0); // Rouge
                else           set_color(0, 0); // Off
                last_toggle = tick_ms;
            }
            break;

        case LED_PATTERN_RED_FAST:
            // Rouge clignotant rapide 150ms
            if (elapsed >= 150) {
                led_state ^= 1;
                if (led_state) set_color(1, 0); // Rouge
                else           set_color(0, 0); // Off
                last_toggle = tick_ms;
            }
            break;

        case LED_PATTERN_ORANGE:
            // Orange fixe (rouge + vert simultané)
            set_color(1, 1);
            break;

        case LED_PATTERN_GREEN_SOLID:
            // Vert fixe
            set_color(0, 1);
            break;

        case LED_PATTERN_RED_SOLID:
            // Rouge fixe
            set_color(1, 0);
            break;
    }
}
