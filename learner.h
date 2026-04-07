#ifndef LEARNER_H
#define LEARNER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t can_id;
    uint8_t  byte_offset;
    uint8_t  factor;
    uint8_t  correlation;
} VehicleConfig;

void learner_init(void);
void learner_on_obd2_speed(uint8_t speed_kmh, uint32_t tick_ms);
void learner_on_frame(uint32_t can_id, uint8_t *data, uint8_t dlc, uint32_t tick_ms);
void learner_pass1_task(uint32_t tick_ms);
bool learner_pass1_done(uint32_t tick_ms, uint32_t start_tick);
bool learner_has_candidates(void);
void learner_pass2_task(uint32_t tick_ms);
bool learner_pass2_done(uint32_t tick_ms, uint32_t start_tick);
bool learner_get_result(VehicleConfig *cfg);

#endif /* LEARNER_H */
