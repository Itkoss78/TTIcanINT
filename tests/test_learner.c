/**
 * test_learner.c — Tests algorithme apprentissage 2 passes
 *
 * Couvre :
 *   - learner_init() remet à zéro
 *   - learner_on_obd2_speed() met à jour les accumulateurs
 *   - learner_on_frame() filtre OBD2 11-bit et 29-bit
 *   - learner_on_frame() accumule les candidats Pass1
 *   - learner_pass1_done() respecte la durée et la variation minimum
 *   - learner_has_candidates() après filtrage Pass1
 *   - learner_pass2_done() + learner_get_result() sur données corrélées
 */

#include "xc_stub.h"
#define _XTAL_FREQ 24000000UL

#include "../learner.c"

#include "test_framework.h"
#include <string.h>

/* ── Helpers ──────────────────────────────────────────────────────────────── */

/* Simule N paires (vitesse basse, vitesse haute) par blocs de 10km/h
 * pour déclencher la condition de score monotone dans pass1 :
 *   - On injecte d'abord quelques frames à vitesse_basse (établit le min)
 *   - Puis on monte à vitesse_haute (delta > 5) avec byte = vitesse
 *   → condition : current_obd2 > last_obd2 + 5  ET  val > min + 3 */
static void feed_correlated_frames(uint32_t can_id, uint8_t byte_offset,
                                   uint8_t speed_start, uint8_t speed_end,
                                   uint16_t n_samples) {
    uint8_t data[8] = {0};
    uint32_t tick = 1000;
    /* Alterner blocs de 3 frames basse vitesse / 3 frames haute vitesse
     * pour que le score monte suffisamment */
    uint8_t lo = speed_start;
    uint8_t hi = (uint8_t)(speed_start + 20); /* saut de 20km/h */
    if (hi > speed_end) hi = speed_end;

    for (uint16_t i = 0; i < n_samples; i++) {
        uint8_t phase = (uint8_t)((i / 6) % 2); /* alterne 0/1 toutes les 6 */
        uint8_t spd   = (phase == 0) ? lo : hi;

        /* Faire avancer les vitesses pour couvrir la plage complète */
        if (i > 0 && i % 30 == 0) {
            lo = (lo + 10 < speed_end - 20) ? (uint8_t)(lo + 10) : speed_start;
            hi = (lo + 20 <= speed_end)      ? (uint8_t)(lo + 20) : speed_end;
        }

        learner_on_obd2_speed(spd, tick);
        data[byte_offset] = spd;
        learner_on_frame(can_id, data, 8, tick);
        tick += 100;
    }
}

/* ── Tests ────────────────────────────────────────────────────────────────── */

int main(void) {
    printf("CANM8 — Tests learner 2 passes\n");

    /* ------------------------------------------------------------------ */
    SECTION("learner_init");

    TEST("init remet les compteurs à zéro") {
        learner_init();
        ASSERT(learner_has_candidates() == false);
        VehicleConfig cfg;
        ASSERT(learner_get_result(&cfg) == false);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("Filtre OBD2 dans learner_on_frame");

    TEST("trame 0x7DF ignorée (OBD2 request 11-bit)") {
        learner_init();
        learner_on_obd2_speed(50, 0);
        uint8_t data[8] = {50,50,50,50,50,50,50,50};
        for (int i = 0; i < 20; i++)
            learner_on_frame(0x7DF, data, 8, i * 10);
        /* Aucun candidat ne doit avoir été créé */
        ASSERT(learner_has_candidates() == false);
    } END_TEST;

    TEST("trames 0x7E8–0x7EF ignorées (OBD2 response 11-bit)") {
        learner_init();
        learner_on_obd2_speed(60, 0);
        uint8_t data[8] = {60,60,60,60,60,60,60,60};
        for (uint32_t id = 0x7E8; id <= 0x7EF; id++)
            for (int i = 0; i < 10; i++)
                learner_on_frame(id, data, 8, i * 10);
        ASSERT(learner_has_candidates() == false);
    } END_TEST;

    TEST("trame 0x18DB33F1 ignorée (OBD2 broadcast 29-bit)") {
        learner_init();
        learner_on_obd2_speed(40, 0);
        uint8_t data[8] = {40,40,40,40,40,40,40,40};
        for (int i = 0; i < 20; i++)
            learner_on_frame(0x18DB33F1UL, data, 8, i * 10);
        ASSERT(learner_has_candidates() == false);
    } END_TEST;

    TEST("trame 0x18DAF110 ignorée (OBD2 response 29-bit)") {
        learner_init();
        learner_on_obd2_speed(40, 0);
        uint8_t data[8] = {40,40,40,40,40,40,40,40};
        for (int i = 0; i < 20; i++)
            learner_on_frame(0x18DAF110UL, data, 8, i * 10);
        ASSERT(learner_has_candidates() == false);
    } END_TEST;

    TEST("trame CAN native 0x1A0 n'est PAS filtrée") {
        learner_init();
        learner_on_obd2_speed(10, 0);
        uint8_t data[8] = {0};
        uint32_t tick = 0;
        /* Faire monter vitesse + byte pour créer un candidat */
        for (uint8_t s = 10; s < 60; s += 5) {
            learner_on_obd2_speed(s, tick);
            data[2] = s;
            learner_on_frame(0x1A0, data, 8, tick);
            tick += 200;
        }
        /* Au moins une entrée créée dans la table */
        bool found = false;
        for (uint8_t i = 0; i < candidate_count; i++)
            if (candidates[i].can_id == 0x1A0) { found = true; break; }
        ASSERT(found == true);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("learner_pass1_done — durée et variation");

    TEST("pass1_done=false si durée insuffisante") {
        learner_init();
        learner_on_obd2_speed(20, 0);
        learner_on_obd2_speed(80, 100);
        /* start=0, now=100 → bien inférieur à PASS1_DURATION_MS=30000 */
        ASSERT(learner_pass1_done(100, 0) == false);
    } END_TEST;

    TEST("pass1_done=false si variation vitesse insuffisante (<30km/h)") {
        learner_init();
        /* Variation de 20 km/h seulement */
        learner_on_obd2_speed(40, 0);
        learner_on_obd2_speed(55, 100);
        /* Durée OK (35000ms) mais variation < MIN_SPEED_RANGE */
        ASSERT(learner_pass1_done(35000, 0) == false);
    } END_TEST;

    TEST("pass1_done=true si durée et variation suffisantes") {
        learner_init();
        learner_on_obd2_speed(10, 0);
        learner_on_obd2_speed(80, 1000); /* variation = 70 km/h > 30 */
        ASSERT(learner_pass1_done(35000, 0) == true);
    } END_TEST;

    /* ------------------------------------------------------------------ */
    SECTION("Pass2 : corrélation et résultat");

    TEST("résultat valide avec données parfaitement corrélées") {
        learner_init();

        /* Injecter MIN_SAMPLES+50 trames avec corrélation parfaite byte=speed */
        feed_correlated_frames(0x1A0, 2, 10, 100, MIN_SAMPLES + 50);

        /* Simuler fin passe 1 : variation et durée OK */
        bool p1 = learner_pass1_done(PASS1_DURATION_MS + 1, 0);
        ASSERT(p1 == true);
        ASSERT(learner_has_candidates() == true);

        /* Injection pass 2 (même données) */
        learner_pass2_task(0);
        feed_correlated_frames(0x1A0, 2, 10, 100, MIN_SAMPLES + 50);

        bool p2 = learner_pass2_done(PASS2_DURATION_MS + 1, 0);
        ASSERT(p2 == true);

        VehicleConfig cfg;
        ASSERT(learner_get_result(&cfg) == true);
        ASSERT_EQ(cfg.can_id, 0x1A0U);
        ASSERT_EQ(cfg.byte_offset, 2);
        /* Facteur doit être non nul */
        ASSERT(cfg.factor > 0);
    } END_TEST;

    TEST("get_result=false si aucune corrélation trouvée") {
        learner_init();
        /* Passe 1 OK */
        learner_on_obd2_speed(10, 0);
        learner_on_obd2_speed(80, 1000);
        learner_pass1_done(PASS1_DURATION_MS + 1, 0);
        /* Passe 2 sans données → n < MIN_SAMPLES */
        learner_pass2_task(0);
        learner_pass2_done(PASS2_DURATION_MS + 1, 0);
        VehicleConfig cfg;
        ASSERT(learner_get_result(&cfg) == false);
    } END_TEST;

    return test_summary();
}
