#ifndef OBD2_H
#define OBD2_H

#include <stdint.h>
#include <stdbool.h>
#include "can.h"

typedef struct {
    uint8_t  speed_kmh;
    uint32_t timestamp;
    bool     valid;
} Obd2Speed;

void    obd2_task(uint32_t tick_ms);
void    obd2_on_frame(CanFrame *frame);
bool    obd2_get_speed(Obd2Speed *out);
uint8_t obd2_get_speed_raw(void);
bool    obd2_is_responding(void);

#endif /* OBD2_H */
