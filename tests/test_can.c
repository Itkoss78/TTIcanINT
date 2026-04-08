/**
 * test_can.c — Tests ring buffer CAN (can_get_frame / can_get_speed_frame)
 *
 * Stratégie : on contourne can_rx_task() (qui lit des registres hardware) et on
 * injecte directement des trames dans le ring buffer via une fonction de test
 * friend déclarée ici. Le ring buffer et can_get_frame() sont de la logique
 * pure — aucun registre PIC18 impliqué.
 */

/* Remplacer <xc.h> par le stub avant tout include du projet */
#include "xc_stub.h"
#define _XTAL_FREQ 24000000UL

/* Inclusion directe du .c pour accéder aux statics internes */
#include "../can.c"

#include "test_framework.h"

/* ── Helpers ──────────────────────────────────────────────────────────────── */

/* Tick factice (requis par can.c : extern uint32_t get_tick) */
uint32_t _tick = 0;
uint32_t get_tick(void) { return _tick; }

/* Réinitialiser le ring buffer entre les tests */
static void reset_ring(void) {
    rx_head = rx_tail = rx_count = 0;
}

/* Injecter une trame directement dans le ring buffer */
static void inject_frame(uint32_t id, uint8_t b0, uint8_t b1,
                         uint8_t b2, uint8_t b3, uint32_t ts) {
    uint8_t next = (rx_head + 1) % CAN_RX_BUFFER_SIZE;
    if (next == rx_tail) return; /* buffer plein — ignoré */
    CanFrame *f = &rx_buffer[rx_head];
    f->id = id; f->dlc = 4; f->timestamp = ts; f->extended = false;
    f->data[0]=b0; f->data[1]=b1; f->data[2]=b2; f->data[3]=b3;
    f->data[4]=0;  f->data[5]=0;  f->data[6]=0;  f->data[7]=0;
    rx_head = next;
    rx_count++;
}

/* ── Tests ────────────────────────────────────────────────────────────────── */

int main(void) {
    printf("CANM8 — Tests ring buffer CAN\n");

    SECTION("can_get_frame : buffer vide");

    TEST("buffer vide → retourne false") {
        reset_ring();
        CanFrame f;
        ASSERT(can_get_frame(&f) == false);
    } END_TEST;

    SECTION("can_get_frame : lecture FIFO");

    TEST("1 trame injectée → lue correctement") {
        reset_ring();
        inject_frame(0x100, 0xAA, 0xBB, 0xCC, 0xDD, 42);
        CanFrame f;
        ASSERT(can_get_frame(&f) == true);
        ASSERT_EQ(f.id, 0x100U);
        ASSERT_EQ(f.data[0], 0xAA);
        ASSERT_EQ(f.data[3], 0xDD);
        ASSERT_EQ(f.timestamp, 42U);
    } END_TEST;

    TEST("après lecture, buffer vide à nouveau") {
        reset_ring();
        inject_frame(0x200, 1, 2, 3, 4, 0);
        CanFrame f;
        can_get_frame(&f);
        ASSERT(can_get_frame(&f) == false);
    } END_TEST;

    TEST("ordre FIFO préservé sur N trames") {
        reset_ring();
        inject_frame(0x101, 1, 0, 0, 0, 0);
        inject_frame(0x102, 2, 0, 0, 0, 0);
        inject_frame(0x103, 3, 0, 0, 0, 0);
        CanFrame f;
        can_get_frame(&f); ASSERT_EQ(f.data[0], 1);
        can_get_frame(&f); ASSERT_EQ(f.data[0], 2);
        can_get_frame(&f); ASSERT_EQ(f.data[0], 3);
        ASSERT(can_get_frame(&f) == false);
    } END_TEST;

    SECTION("can_get_frame : remplissage complet");

    TEST("remplir 31 slots (max = 32-1 pour distinguer plein/vide)") {
        reset_ring();
        for (uint8_t i = 0; i < CAN_RX_BUFFER_SIZE - 1; i++)
            inject_frame(0x300 + i, i, 0, 0, 0, i);
        CanFrame f;
        uint8_t count = 0;
        while (can_get_frame(&f)) count++;
        ASSERT_EQ(count, CAN_RX_BUFFER_SIZE - 1);
    } END_TEST;

    TEST("trame injectée après overflow n'écrase pas les données existantes") {
        reset_ring();
        for (uint8_t i = 0; i < CAN_RX_BUFFER_SIZE; i++) /* remplit + déborde */
            inject_frame(0x400 + i, i, 0, 0, 0, 0);
        CanFrame f;
        /* La première trame doit être 0x400 (pas 0x41F) */
        ASSERT(can_get_frame(&f) == true);
        ASSERT_EQ(f.id, 0x400U);
    } END_TEST;

    SECTION("can_get_speed_frame");

    TEST("retourne false si ID cible absent") {
        reset_ring();
        inject_frame(0x100, 10, 0, 0, 0, 0);
        inject_frame(0x200, 20, 0, 0, 0, 0);
        CanFrame f;
        ASSERT(can_get_speed_frame(&f, 0x300) == false);
    } END_TEST;

    TEST("retourne la trame avec l'ID cible et consomme le buffer") {
        reset_ring();
        inject_frame(0x100, 0xAA, 0, 0, 0, 0);
        inject_frame(0x1A0, 0x42, 0, 0, 0, 0);
        inject_frame(0x200, 0xBB, 0, 0, 0, 0);
        CanFrame f;
        ASSERT(can_get_speed_frame(&f, 0x1A0) == true);
        ASSERT_EQ(f.data[0], 0x42);
        /* can_get_speed_frame consomme les trames AVANT la cible et la cible,
         * mais les trames APRÈS la cible restent dans le buffer (0x200) */
        ASSERT(can_get_frame(&f) == true);
        ASSERT_EQ(f.id, 0x200U);
        ASSERT(can_get_frame(&f) == false); /* maintenant vide */
    } END_TEST;

    TEST("retourne la première occurrence si plusieurs trames avec l'ID cible") {
        reset_ring();
        inject_frame(0x1A0, 11, 0, 0, 0, 0);
        inject_frame(0x1A0, 22, 0, 0, 0, 0);
        CanFrame f;
        ASSERT(can_get_speed_frame(&f, 0x1A0) == true);
        ASSERT_EQ(f.data[0], 11); /* première, pas la deuxième */
    } END_TEST;

    return test_summary();
}
