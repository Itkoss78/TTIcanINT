#ifndef LED_H
#define LED_H

#include <stdint.h>

typedef enum {
    LED_PATTERN_BOOT,
    LED_PATTERN_RED_SLOW,
    LED_PATTERN_RED_FAST,
    LED_PATTERN_ORANGE,
    LED_PATTERN_GREEN_SOLID,
    LED_PATTERN_RED_SOLID
} LedPattern;

void led_init(void);
void led_set_pattern(LedPattern pattern);
void led_update(uint32_t tick_ms);

#endif /* LED_H */
