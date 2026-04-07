#ifndef VEHICLE_ID_H
#define VEHICLE_ID_H

#include <stdint.h>
#include <stdbool.h>
#include "can.h"

// Appelé par le dispatcher dans main.c — ne consomme PAS can_get_frame()
void     vehicle_id_on_frame(CanFrame *frame);

// Wrapper vide conservé pour compatibilité — ne pas appeler can_get_frame()
void     vehicle_id_scan(uint32_t tick_ms);

uint32_t vehicle_id_get_signature(void);

#endif /* VEHICLE_ID_H */
