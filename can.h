#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t id;
    uint8_t  data[8];
    uint8_t  dlc;
    uint32_t timestamp;
    bool     extended;
} CanFrame;

void     can_init(void);
void     can_rx_task(void);
bool     can_get_frame(CanFrame *frame);
bool     can_get_speed_frame(CanFrame *frame, uint32_t target_id);
bool     can_send(uint32_t id, uint8_t *data, uint8_t len);
uint16_t can_get_rx_count(void);

#endif /* CAN_H */
