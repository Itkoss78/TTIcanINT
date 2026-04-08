/**
 * test_vehicle_id.c — Tests fingerprint CAN (vehicle_id_on_frame / get_signature)
 *
 * Couvre :
 *   - Filtrage OBD2 11-bit et 29-bit dans vehicle_id_on_frame()
 *   - Comptage de fréquence des IDs
 *   - Stabilité de la signature : même set d'IDs → même signature
 *   - Différentiation : deux sets différents → signatures différentes
 */

#include "xc_stub.h"
#define _XTAL_FREQ 24000000UL

#include "../vehicle_id.c"

#include "test_framework.h"
#include <string.h>

/* ── Helpers ──────────────────────────────────────────────────────────────── */

/* Reset de l'état interne (les statics ne se remettent pas à zéro seuls) */
static void reset_vehicle_id(void) {
    memset(freq_table, 0, sizeof(freq_table));
    freq_count = 0;
    signature  = 0;
    scan_done  = false;
}

static void inject_id(uint32_t id, uint16_t count) {
    CanFrame f;
    memset(&f, 0, sizeof(f));
    f.id  = id;
    f.dlc = 8;
    for (uint16_t i = 0; i < count; i++)
        vehicle_id_on_frame(&f);
}

/* ── Tests ────────────────────────────────────────────────────────────────── */

int main(void) {
    printf("CANM8 — Tests vehicle_id fingerprint\n");

    /* ------------------------------------------------------------------ */
    SECTION("Filtrage OBD2");

    TEST("0x7DF ignoré — ne rentre pas dans freq_table") {
        reset_vehicle_id();
        inject_id(0x7DF, 100);
        ASSERT_EQ(freq_count, 0);
    } END_TEST;

    TEST("0x7E8 ignoré") {
        reset_vehicle_id();
        inject_id(0x7E8, 50);
        ASSERT_EQ(freq_count, 0);
    } END_TEST;

    TEST("0x7EF ignoré (borne haute 11-bit)") {
        reset_vehicle_id();
        inject_id(0x7EF, 50);
        ASSERT_EQ(freq_count, 0);
    } END_TEST;

    TEST("0x18DB33F1 ignoré (broadcast 29-bit)") {
        reset_vehicle_id();
        inject_id(0x18DB33F1UL, 50);
        ASSERT_EQ(freq_count, 0);
    } END_TEST;

    TEST("0x18DAF1A0 ignoré (response 29-bit)") {
        reset_vehicle_id();
        inject_id(0x18DAF1A0UL, 50);
        ASSERT_EQ(freq_count, 0);
    } END_TEST;

    TEST("ID natif 0x1A0 est compté") {
        reset_vehicle_id();
        inject_id(0x1A0, 10);
        ASSERT(freq_count > 0);
        ASSERT_EQ(freq_table[0].id,    0x1A0U);
        ASSERT_EQ(freq_table[0].count, 10);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("Comptage fréquences");

    TEST("plusieurs IDs distincts créent plusieurs entrées") {
        reset_vehicle_id();
        inject_id(0x100, 5);
        inject_id(0x200, 3);
        inject_id(0x300, 8);
        ASSERT_EQ(freq_count, 3);
    } END_TEST;

    TEST("même ID injecté deux fois → même entrée incrémentée") {
        reset_vehicle_id();
        inject_id(0x1A0, 7);
        inject_id(0x1A0, 3); /* même ID → count doit être 10 */
        ASSERT_EQ(freq_count, 1);
        ASSERT_EQ(freq_table[0].count, 10);
    } END_TEST;

    TEST("table sature à FREQ_TABLE_SIZE (32) sans crash") {
        reset_vehicle_id();
        for (uint32_t i = 0; i < 40; i++)
            inject_id(0x100 + i, 1); /* 40 IDs distincts, table max=32 */
        ASSERT_EQ(freq_count, FREQ_TABLE_SIZE);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("Stabilité de la signature");

    TEST("même set d'IDs → même signature") {
        reset_vehicle_id();
        inject_id(0x100, 100); inject_id(0x200, 80);
        inject_id(0x300, 60);  inject_id(0x400, 40); inject_id(0x500, 20);
        uint32_t sig1 = vehicle_id_get_signature();

        reset_vehicle_id();
        inject_id(0x100, 100); inject_id(0x200, 80);
        inject_id(0x300, 60);  inject_id(0x400, 40); inject_id(0x500, 20);
        uint32_t sig2 = vehicle_id_get_signature();

        ASSERT_EQ(sig1, sig2);
    } END_TEST;

    TEST("set d'IDs différent → signature différente") {
        reset_vehicle_id();
        inject_id(0x100, 100); inject_id(0x201, 80); /* 0x201 ≠ 0x200 */
        inject_id(0x300, 60);  inject_id(0x400, 40); inject_id(0x500, 20);
        uint32_t sig_a = vehicle_id_get_signature();

        reset_vehicle_id();
        inject_id(0x100, 100); inject_id(0x200, 80);
        inject_id(0x300, 60);  inject_id(0x400, 40); inject_id(0x500, 20);
        uint32_t sig_b = vehicle_id_get_signature();

        ASSERT_NEQ(sig_a, sig_b);
    } END_TEST;

    TEST("signature non nulle avec des données réelles") {
        reset_vehicle_id();
        inject_id(0x100, 100); inject_id(0x200, 80);
        inject_id(0x300, 60);  inject_id(0x400, 40); inject_id(0x500, 20);
        uint32_t sig = vehicle_id_get_signature();
        ASSERT_NEQ(sig, 0U);
    } END_TEST;

    TEST("ordre d'injection ne change pas la signature (top-5 triés)") {
        reset_vehicle_id();
        inject_id(0x500, 20); inject_id(0x100, 100); inject_id(0x300, 60);
        inject_id(0x400, 40); inject_id(0x200, 80);
        uint32_t sig1 = vehicle_id_get_signature();

        reset_vehicle_id();
        inject_id(0x100, 100); inject_id(0x200, 80); inject_id(0x300, 60);
        inject_id(0x400, 40);  inject_id(0x500, 20);
        uint32_t sig2 = vehicle_id_get_signature();

        ASSERT_EQ(sig1, sig2);
    } END_TEST;

    return test_summary();
}
