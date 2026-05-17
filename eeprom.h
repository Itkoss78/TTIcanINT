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
/* Annule les macros XC8 legacy pic18.h qui entrent en conflit */
#ifdef eeprom_read
#undef eeprom_read
#endif
#ifdef eeprom_write
#undef eeprom_write
#endif
void eeprom_read(EepromData *data);
void eeprom_write(EepromData *data);
void eeprom_clear(void);

#endif /* EEPROM_H */
