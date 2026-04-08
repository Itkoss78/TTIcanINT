/**
 * test_eeprom.c — Tests EEPROM : CRC-8, encode/decode, magic, corruption
 *
 * Stratégie : réimplémentation locale des fonctions eeprom_* avec une EEPROM
 * simulée par un tableau RAM. On teste la logique pure (CRC-8, encode/decode,
 * magic, corruption) sans jamais écrire sur le PIC18.
 *
 * On inclut uniquement eeprom.h pour les types et constantes, PAS eeprom.c.
 */

#include "xc_stub.h"
#include "../eeprom.h"
#include "../config.h"

#include "test_framework.h"

/* ── Simulation EEPROM en RAM ─────────────────────────────────────────────── */
static uint8_t fake_eeprom[256];

static void eeprom_hw_reset(void) {
    for (int i = 0; i < 256; i++) fake_eeprom[i] = 0xFF;
}

/* ── CRC-8 (même algorithme que eeprom.c) ─────────────────────────────────── */
static uint8_t compute_crc(uint8_t *buf, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x07);
            else            crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* ── Réimplémentation locale des fonctions EEPROM avec fake_eeprom[] ──────── */
void eeprom_init(void) { /* rien */ }

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

    fake_eeprom[EEPROM_MAGIC_ADDR] = EEPROM_MAGIC_VAL;
    for (uint8_t i = 0; i < 11; i++)
        fake_eeprom[EEPROM_DATA_ADDR + i] = buf[i];
}

void eeprom_read(EepromData *data) {
    if (fake_eeprom[EEPROM_MAGIC_ADDR] != EEPROM_MAGIC_VAL) {
        data->valid = false;
        return;
    }

    uint8_t buf[11];
    for (uint8_t i = 0; i < 11; i++)
        buf[i] = fake_eeprom[EEPROM_DATA_ADDR + i];

    if (buf[10] != compute_crc(buf, 10)) {
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

void eeprom_clear(void) {
    fake_eeprom[EEPROM_MAGIC_ADDR] = 0x00;
}

/* ── Tests ────────────────────────────────────────────────────────────────── */

int main(void) {
    printf("CANM8 — Tests EEPROM (CRC-8 + encode/decode)\n");

    /* ------------------------------------------------------------------ */
    SECTION("eeprom_read : EEPROM vierge");

    TEST("EEPROM vierge → valid=false") {
        eeprom_hw_reset();
        EepromData d;
        eeprom_read(&d);
        ASSERT(d.valid == false);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("eeprom_write + eeprom_read : round-trip");

    TEST("écriture puis lecture retourne les mêmes valeurs") {
        eeprom_hw_reset();
        EepromData w;
        w.valid             = true;
        w.speed_can_id      = 0x1A0;
        w.speed_byte_offset = 3;
        w.speed_factor      = 128;
        w.vehicle_signature = 0xDEADBEEF;
        eeprom_write(&w);

        EepromData r;
        eeprom_read(&r);
        ASSERT(r.valid == true);
        ASSERT_EQ(r.speed_can_id,      0x1A0U);
        ASSERT_EQ(r.speed_byte_offset, 3);
        ASSERT_EQ(r.speed_factor,      128);
        ASSERT_EQ(r.vehicle_signature, 0xDEADBEEFUL);
    } END_TEST;

    TEST("valeurs limites : can_id=0xFFFFFFFF, offset=7, factor=255") {
        eeprom_hw_reset();
        EepromData w;
        w.valid             = true;
        w.speed_can_id      = 0xFFFFFFFFUL;
        w.speed_byte_offset = 7;
        w.speed_factor      = 255;
        w.vehicle_signature = 0x00000001UL;
        eeprom_write(&w);

        EepromData r;
        eeprom_read(&r);
        ASSERT(r.valid == true);
        ASSERT_EQ(r.speed_can_id,      0xFFFFFFFFUL);
        ASSERT_EQ(r.speed_byte_offset, 7);
        ASSERT_EQ(r.speed_factor,      255);
        ASSERT_EQ(r.vehicle_signature, 0x00000001UL);
    } END_TEST;

    TEST("valeurs zéros : can_id=0, offset=0, factor=0, signature=0") {
        eeprom_hw_reset();
        EepromData w;
        w.valid             = true;
        w.speed_can_id      = 0;
        w.speed_byte_offset = 0;
        w.speed_factor      = 0;
        w.vehicle_signature = 0;
        eeprom_write(&w);

        EepromData r;
        eeprom_read(&r);
        ASSERT(r.valid == true);
        ASSERT_EQ(r.speed_can_id,      0U);
        ASSERT_EQ(r.speed_byte_offset, 0);
        ASSERT_EQ(r.speed_factor,      0);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("Intégrité CRC-8");

    TEST("corruption d'un octet de données → valid=false") {
        eeprom_hw_reset();
        EepromData w;
        w.valid=true; w.speed_can_id=0x200; w.speed_byte_offset=2;
        w.speed_factor=100; w.vehicle_signature=0x12345678;
        eeprom_write(&w);

        /* Corrompre l'octet à EEPROM_DATA_ADDR + 2 (troisième octet des données) */
        fake_eeprom[EEPROM_DATA_ADDR + 2] ^= 0xFF;

        EepromData r;
        eeprom_read(&r);
        ASSERT(r.valid == false);
    } END_TEST;

    TEST("corruption du magic → valid=false") {
        eeprom_hw_reset();
        EepromData w;
        w.valid=true; w.speed_can_id=0x300; w.speed_byte_offset=1;
        w.speed_factor=200; w.vehicle_signature=0xABCD1234;
        eeprom_write(&w);

        fake_eeprom[EEPROM_MAGIC_ADDR] ^= 0x01; /* bit-flip sur magic */

        EepromData r;
        eeprom_read(&r);
        ASSERT(r.valid == false);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("eeprom_clear");

    TEST("clear puis read → valid=false") {
        eeprom_hw_reset();
        EepromData w;
        w.valid=true; w.speed_can_id=0x1A0; w.speed_byte_offset=0;
        w.speed_factor=128; w.vehicle_signature=0x11223344;
        eeprom_write(&w);

        eeprom_clear();

        EepromData r;
        eeprom_read(&r);
        ASSERT(r.valid == false);
    } END_TEST;

    TEST("double clear ne plante pas") {
        eeprom_hw_reset();
        eeprom_clear();
        eeprom_clear(); /* doit juste retourner sans crash */
        EepromData r;
        eeprom_read(&r);
        ASSERT(r.valid == false);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("Deux writes successifs");

    TEST("deuxième write écrase le premier") {
        eeprom_hw_reset();
        EepromData w1; w1.valid=true; w1.speed_can_id=0x100;
        w1.speed_byte_offset=0; w1.speed_factor=50; w1.vehicle_signature=0xAAAA;
        eeprom_write(&w1);

        EepromData w2; w2.valid=true; w2.speed_can_id=0x200;
        w2.speed_byte_offset=4; w2.speed_factor=200; w2.vehicle_signature=0xBBBB;
        eeprom_write(&w2);

        EepromData r;
        eeprom_read(&r);
        ASSERT(r.valid == true);
        ASSERT_EQ(r.speed_can_id, 0x200U);
        ASSERT_EQ(r.speed_factor, 200);
    } END_TEST;

    return test_summary();
}
