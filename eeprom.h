#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool     valid;
    uint32_t speed_can_id;
    uint8_t  speed_byte_offset;
    uint8_t  speed_factor;
    uint32_t vehicle_signature;
} EepromData;

void eeprom_init(void);
void eeprom_read(EepromData *data);
void eeprom_write(EepromData *data);
void eeprom_clear(void);

#endif /* EEPROM_H */
