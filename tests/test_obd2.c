/**
 * test_obd2.c — Tests parsing réponse OBD2 PID 0x0D (vitesse)
 *
 * Couvre :
 *   - obd2_on_frame() : trame valide → speed mise à jour
 *   - obd2_on_frame() : mauvais PID ignoré
 *   - obd2_on_frame() : ID hors plage 0x7E8–0x7EF ignoré
 *   - obd2_on_frame() : DLC insuffisant ignoré
 *   - obd2_get_speed_raw() : retourne la dernière valeur parsée
 *   - obd2_task() : envoie une requête dès que l'intervalle est écoulé
 */

#include "xc_stub.h"
#define _XTAL_FREQ 24000000UL

/* Stub get_tick() et can_send() avant d'inclure obd2.c */
static uint32_t _tick = 0;
uint32_t get_tick(void) { return _tick; }

/* Capturer les envois CAN pour vérifier que obd2_task() envoie la bonne trame */
static uint32_t last_sent_id   = 0;
static uint8_t  last_sent_data[8];
static uint8_t  last_sent_len  = 0;
static int      can_send_calls = 0;

bool can_send(uint32_t id, uint8_t *data, uint8_t len) {
    last_sent_id  = id;
    last_sent_len = len;
    for (uint8_t i = 0; i < len && i < 8; i++) last_sent_data[i] = data[i];
    can_send_calls++;
    return true;
}

#include "../obd2.c"

#include "test_framework.h"
#include <string.h>

/* ── Helpers ──────────────────────────────────────────────────────────────── */

static CanFrame make_obd2_response(uint32_t id, uint8_t len_byte,
                                   uint8_t service, uint8_t pid,
                                   uint8_t value) {
    CanFrame f;
    memset(&f, 0, sizeof(f));
    f.id = id; f.dlc = 8; f.extended = false;
    f.data[0] = len_byte;
    f.data[1] = service;
    f.data[2] = pid;
    f.data[3] = value;
    return f;
}

/* Réinitialiser l'état interne entre tests                                  */
static void reset_obd2(void) {
    last_request_tick   = 0;
    last_response_tick  = 0;
    current_speed.speed_kmh = 0;
    current_speed.timestamp = 0;
    current_speed.valid     = false;
    waiting_response        = false;
    can_send_calls          = 0;
    _tick = 0;
}

/* ── Tests ────────────────────────────────────────────────────────────────── */

int main(void) {
    printf("CANM8 — Tests OBD2 parsing PID 0x0D\n");

    /* ------------------------------------------------------------------ */
    SECTION("obd2_on_frame : réponse valide");

    TEST("ID 0x7E8, PID 0x0D, valeur 80 km/h → speed=80") {
        reset_obd2();
        CanFrame f = make_obd2_response(0x7E8, 0x03, 0x41, 0x0D, 80);
        obd2_on_frame(&f);
        ASSERT_EQ(obd2_get_speed_raw(), 80);
    } END_TEST;

    TEST("ID 0x7EA (autre ECU) accepté, PID 0x0D → speed mis à jour") {
        reset_obd2();
        CanFrame f = make_obd2_response(0x7EA, 0x03, 0x41, 0x0D, 120);
        obd2_on_frame(&f);
        ASSERT_EQ(obd2_get_speed_raw(), 120);
    } END_TEST;

    TEST("valeur 0 km/h acceptée") {
        reset_obd2();
        CanFrame f = make_obd2_response(0x7E8, 0x03, 0x41, 0x0D, 0);
        obd2_on_frame(&f);
        Obd2Speed s;
        ASSERT(obd2_get_speed(&s) == true);
        ASSERT_EQ(s.speed_kmh, 0);
    } END_TEST;

    TEST("valeur 255 km/h acceptée (max uint8)") {
        reset_obd2();
        CanFrame f = make_obd2_response(0x7E8, 0x03, 0x41, 0x0D, 255);
        obd2_on_frame(&f);
        ASSERT_EQ(obd2_get_speed_raw(), 255);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("obd2_on_frame : trames ignorées");

    TEST("mauvais service byte (0x40 au lieu de 0x41) → ignoré") {
        reset_obd2();
        CanFrame f = make_obd2_response(0x7E8, 0x03, 0x40, 0x0D, 50);
        obd2_on_frame(&f);
        ASSERT(obd2_get_speed_raw() == 0); /* pas mis à jour */
    } END_TEST;

    TEST("mauvais PID (0x0C = RPM) → ignoré") {
        reset_obd2();
        CanFrame f = make_obd2_response(0x7E8, 0x03, 0x41, 0x0C, 50);
        obd2_on_frame(&f);
        ASSERT(obd2_get_speed_raw() == 0);
    } END_TEST;

    TEST("ID hors plage (0x700) → ignoré") {
        reset_obd2();
        CanFrame f = make_obd2_response(0x700, 0x03, 0x41, 0x0D, 60);
        obd2_on_frame(&f);
        ASSERT(obd2_get_speed_raw() == 0);
    } END_TEST;

    TEST("ID 0x7F0 (au-delà de 0x7EF) → ignoré") {
        reset_obd2();
        CanFrame f = make_obd2_response(0x7F0, 0x03, 0x41, 0x0D, 60);
        obd2_on_frame(&f);
        ASSERT(obd2_get_speed_raw() == 0);
    } END_TEST;

    TEST("DLC=3 (insuffisant, besoin de 4 bytes) → ignoré") {
        reset_obd2();
        CanFrame f = make_obd2_response(0x7E8, 0x03, 0x41, 0x0D, 70);
        f.dlc = 3; /* trop court */
        obd2_on_frame(&f);
        ASSERT(obd2_get_speed_raw() == 0);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("obd2_get_speed");

    TEST("get_speed retourne false si aucune réponse reçue") {
        reset_obd2();
        Obd2Speed s;
        ASSERT(obd2_get_speed(&s) == false);
    } END_TEST;

    TEST("get_speed retourne true et la bonne struct après réponse valide") {
        reset_obd2();
        _tick = 5000;
        CanFrame f = make_obd2_response(0x7E8, 0x03, 0x41, 0x0D, 95);
        f.timestamp = 5000;
        obd2_on_frame(&f);
        Obd2Speed s;
        ASSERT(obd2_get_speed(&s) == true);
        ASSERT_EQ(s.speed_kmh,  95);
        ASSERT_EQ(s.timestamp,  5000U);
        ASSERT(s.valid == true);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("obd2_task : envoi de requêtes");

    TEST("obd2_task envoie une requête après le premier intervalle") {
        reset_obd2();
        /* À t=0 last_request_tick=0 et tick_ms=0 → 0-0=0 < 100 → pas d'envoi */
        obd2_task(OBD2_REQUEST_INTERVAL_MS); /* t=100ms → 100-0=100 >= 100 → envoi */
        ASSERT_EQ(can_send_calls,  1);
        ASSERT_EQ(last_sent_id,    0x7DFUL);
        ASSERT_EQ(last_sent_data[1], 0x01); /* mode 01 */
        ASSERT_EQ(last_sent_data[2], 0x0D); /* PID vitesse */
    } END_TEST;

    TEST("obd2_task n'envoie pas une deuxième fois avant l'intervalle") {
        reset_obd2();
        obd2_task(OBD2_REQUEST_INTERVAL_MS);
        obd2_task(OBD2_REQUEST_INTERVAL_MS + 50); /* +50ms < 100ms */
        ASSERT_EQ(can_send_calls, 1); /* toujours 1 seul envoi */
    } END_TEST;

    TEST("obd2_task envoie à nouveau après le deuxième intervalle") {
        reset_obd2();
        obd2_task(OBD2_REQUEST_INTERVAL_MS);
        obd2_task(OBD2_REQUEST_INTERVAL_MS * 2);
        ASSERT_EQ(can_send_calls, 2);
    } END_TEST;

    return test_summary();
}
