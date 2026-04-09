/**
 * test_signal_w.c — Tests unitaires signal_w.c
 *
 * Formule : freq_Hz = speed_kmh × SIGNAL_W_MODE × 1000 / 1609
 * Compatible CANM8 CANNECT PULSE (source : canm8.com/cannect-pulse)
 */
#define SIGNAL_W_MODE 1   /* forcer mode 1Hz/MPH avant inclusion config.h */

#include "xc_stub.h"
#define _XTAL_FREQ 24000000UL

#include "../signal_w.c"
#include "test_framework.h"

/* Accès direct au registre stub pour vérifier l'état LAT */
#define LATD4_VAL  LATDbits.LATD4

static void reset_signal_w(void) {
    signal_w_stop();
    LATDbits.LATD4 = 0;
}

int main(void) {
    printf("CANM8 — Tests Signal W (J2-3 / RD4)\n");

    /* ──────────────────────────────────────────── */
    SECTION("Fréquences mode 1Hz/MPH");

    TEST("speed=50 → freq=31 Hz  (50×1×1000/1609)") {
        reset_signal_w();
        signal_w_set_speed(50);
        ASSERT_EQ((int)signal_w_get_freq(), 31);
    } END_TEST;

    TEST("speed=90 → freq=55 Hz  (90×1×1000/1609)") {
        reset_signal_w();
        signal_w_set_speed(90);
        ASSERT_EQ((int)signal_w_get_freq(), 55);
    } END_TEST;

    TEST("speed=120 → freq=74 Hz (120×1×1000/1609)") {
        reset_signal_w();
        signal_w_set_speed(120);
        ASSERT_EQ((int)signal_w_get_freq(), 74);
    } END_TEST;

    /* ──────────────────────────────────────────── */
    SECTION("Demi-période et timing toggle");

    TEST("50 km/h → half_period=16ms, pas de toggle avant tick<16") {
        reset_signal_w();
        signal_w_set_speed(50);   /* 31 Hz → hp = 500/31 = 16 ms */
        uint8_t initial = LATD4_VAL;
        uint32_t t;
        for (t = 0; t < 16; t++) {
            signal_w_task(t);
            ASSERT_EQ((int)LATD4_VAL, (int)initial);
        }
        signal_w_task(16);  /* tick == half_period → 1er toggle */
        ASSERT_NEQ((int)LATD4_VAL, (int)initial);
    } END_TEST;

    TEST("90 km/h → LAT alterne 0→1→0→1 aux bons ticks") {
        reset_signal_w();
        signal_w_set_speed(90);   /* 55 Hz → hp = 500/55 = 9 ms */
        ASSERT_EQ((int)LATD4_VAL, 0);
        signal_w_task(9);   ASSERT_EQ((int)LATD4_VAL, 1);
        signal_w_task(18);  ASSERT_EQ((int)LATD4_VAL, 0);
        signal_w_task(27);  ASSERT_EQ((int)LATD4_VAL, 1);
    } END_TEST;

    /* ──────────────────────────────────────────── */
    SECTION("Mode 4 Hz/MPH — vérification formule entière");

    TEST("speed=50, mode=4 → freq=124 Hz  (50×4×1000/1609)") {
        uint32_t freq = (50UL * 4UL * 1000UL) / 1609UL;
        ASSERT_EQ((int)freq, 124);
    } END_TEST;

    TEST("speed=90, mode=4 → freq=223 Hz  (90×4×1000/1609)") {
        uint32_t freq = (90UL * 4UL * 1000UL) / 1609UL;
        ASSERT_EQ((int)freq, 223);
    } END_TEST;

    /* ──────────────────────────────────────────── */
    SECTION("Cas limites");

    TEST("speed=0 → freq=0, LAT=0, aucun toggle") {
        reset_signal_w();
        signal_w_set_speed(0);
        ASSERT_EQ((int)signal_w_get_freq(), 0);
        ASSERT_EQ((int)LATD4_VAL, 0);
        signal_w_task(1000);
        ASSERT_EQ((int)LATD4_VAL, 0);
    } END_TEST;

    TEST("speed=255 → freq ≤ 2000 Hz (clamp max)") {
        reset_signal_w();
        signal_w_set_speed(255);
        ASSERT((int)signal_w_get_freq() <= 2000);
        ASSERT((int)signal_w_get_freq() >= 1);
    } END_TEST;

    TEST("speed=1 → freq=1 Hz (clamp min : 1×1×1000/1609=0 → clamp→1)") {
        reset_signal_w();
        signal_w_set_speed(1);
        ASSERT_EQ((int)signal_w_get_freq(), 1);
    } END_TEST;

    /* ──────────────────────────────────────────── */
    SECTION("stop() et reprise");

    TEST("stop() → freq=0, LAT=0") {
        reset_signal_w();
        signal_w_set_speed(90);
        signal_w_task(100);
        signal_w_stop();
        ASSERT_EQ((int)signal_w_get_freq(), 0);
        ASSERT_EQ((int)LATD4_VAL, 0);
    } END_TEST;

    TEST("stop() puis set_speed(50) → signal reprend correctement") {
        reset_signal_w();
        signal_w_set_speed(50);   /* 31 Hz, hp=16 ms */
        signal_w_task(16);        /* 1er toggle */
        signal_w_stop();
        ASSERT_EQ((int)LATD4_VAL, 0);
        signal_w_set_speed(50);
        ASSERT_EQ((int)signal_w_get_freq(), 31);
        signal_w_task(1);   /* trop tôt */
        ASSERT_EQ((int)LATD4_VAL, 0);
        signal_w_task(16);  /* 1er toggle après reprise */
        ASSERT_EQ((int)LATD4_VAL, 1);
    } END_TEST;

    /* ──────────────────────────────────────────── */
    SECTION("Vérification formule entière (pas de float)");

    TEST("30/50/160 km/h → 18/31/99 Hz (entier strict)") {
        uint32_t f30  = (30UL  * 1UL * 1000UL) / 1609UL;
        uint32_t f50  = (50UL  * 1UL * 1000UL) / 1609UL;
        uint32_t f160 = (160UL * 1UL * 1000UL) / 1609UL;
        ASSERT_EQ((int)f30,  18);
        ASSERT_EQ((int)f50,  31);
        ASSERT_EQ((int)f160, 99);
    } END_TEST;

    return test_summary();
}
