#include "eeprom.h"
#include "config.h"

static uint8_t eeprom_read_byte(uint8_t addr) {
    EEADR  = addr;
    EECON1 = 0x00; // Sélectionner data EEPROM
    EECON1bits.RD = 1;
    return EEDATA;
}

static void eeprom_write_byte(uint8_t addr, uint8_t data) {
    EEADR  = addr;
    EEDATA = data;
    EECON1bits.EEPGD = 0; // Data EEPROM
    EECON1bits.CFGS  = 0;
    EECON1bits.WREN  = 1;

    // Séquence obligatoire
    uint8_t gie = INTCONbits.GIE;
    INTCONbits.GIE = 0;
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    INTCONbits.GIE = gie;

    while (EECON1bits.WR); // Attendre fin écriture
    EECON1bits.WREN = 0;
}

static uint8_t compute_crc(uint8_t *buf, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x07;
            else            crc <<= 1;
        }
    }
    return crc;
}

void eeprom_init(void) {
    // Rien à faire sur PIC18
}

void eeprom_read(EepromData *data) {
    uint8_t magic = eeprom_read_byte(EEPROM_MAGIC_ADDR);

    if (magic != EEPROM_MAGIC_VAL) {
        data->valid = false;
        return;
    }

    uint8_t addr = EEPROM_DATA_ADDR;
    // Layout: [can_id 4B][byte_offset 1B][factor 1B][signature 4B][crc 1B]
    uint8_t buf[11];
    for (uint8_t i = 0; i < 11; i++) {
        buf[i] = eeprom_read_byte(addr++);
    }

    uint8_t stored_crc = buf[10];
    uint8_t calc_crc   = compute_crc(buf, 10);

    if (stored_crc != calc_crc) {
        data->valid = false;
        return;
    }

    data->speed_can_id      = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
                            | ((uint32_t)buf[2] << 8)  | buf[3];
    data->speed_byte_offset = buf[4];
    data->speed_factor      = buf[5];
    data->vehicle_signature = ((uint32_t)buf[6] << 24) | ((uint32_t)buf[7] << 16)
                            | ((uint32_t)buf[8] << 8)  | buf[9];
    data->valid             = true;
}

void eeprom_write(EepromData *data) {
    uint8_t buf[11];
    buf[0]  = (uint8_t)(data->speed_can_id >> 24);
    buf[1]  = (uint8_t)(data->speed_can_id >> 16);
    buf[2]  = (uint8_t)(data->speed_can_id >> 8);
    buf[3]  = (uint8_t)(data->speed_can_id);
    buf[4]  = data->speed_byte_offset;
    buf[5]  = data->speed_factor;
    buf[6]  = (uint8_t)(data->vehicle_signature >> 24);
    buf[7]  = (uint8_t)(data->vehicle_signature >> 16);
    buf[8]  = (uint8_t)(data->vehicle_signature >> 8);
    buf[9]  = (uint8_t)(data->vehicle_signature);
    buf[10] = compute_crc(buf, 10);

    eeprom_write_byte(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VAL);
    for (uint8_t i = 0; i < 11; i++) {
        eeprom_write_byte(EEPROM_DATA_ADDR + i, buf[i]);
    }
}

void eeprom_clear(void) {
    eeprom_write_byte(EEPROM_MAGIC_ADDR, 0x00);
}
