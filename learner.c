#include "learner.h"
#include "config.h"
#include <string.h>

/**
 * ALGORITHME 2 PASSES
 *
 * PASSE 1 - Filtrage monotone (30s)
 *   Pour chaque (ID, byte_offset) :
 *   → Vérifier que la valeur byte monte/descend avec la vitesse OBD2
 *   → Critère : au moins MIN_SPEED_RANGE km/h de variation observée
 *   → Sortie : liste de MAX_CANDIDATES candidats
 *
 * PASSE 2 - Corrélation Pearson incrémentale (30s)
 *   Pour chaque candidat uniquement :
 *   → Accumuler sum_x, sum_y, sum_xy, sum_x2, sum_y2, n
 *   → Calculer r = Pearson(byte_value, obd2_speed)
 *   → Trouver facteur de conversion : factor = mean(obd2) / mean(byte)
 *   → Retenir candidat avec r > CORRELATION_THRESH
 */

// ─── PASSE 1 ─────────────────────────────────────────────────────────────────

typedef struct {
    uint32_t can_id;
    uint8_t  byte_offset;
    uint8_t  min_val;
    uint8_t  max_val;
    uint8_t  last_obd2_speed;
    int8_t   direction;     // +1 montée, -1 descente, 0 indéfini
    uint8_t  score;         // nb fois monotone OK
    bool     active;
} Pass1Entry;

// On ne peut pas tracker tous les IDs CAN (trop de RAM)
// On observe les IDs qui apparaissent et on en garde MAX_CANDIDATES
static Pass1Entry candidates[MAX_CANDIDATES];
static uint8_t    candidate_count = 0;

// ─── PASSE 2 ─────────────────────────────────────────────────────────────────

typedef struct {
    uint32_t can_id;
    uint8_t  byte_offset;

    // Accumulateurs Pearson (int32 suffit pour 300 échantillons)
    int32_t  sum_x;   // sum(byte_val)
    int32_t  sum_y;   // sum(obd2_speed)
    int32_t  sum_xy;  // sum(byte_val * obd2_speed)
    int32_t  sum_x2;  // sum(byte_val^2)
    int32_t  sum_y2;  // sum(obd2_speed^2)
    uint16_t n;       // nombre d'échantillons

    uint8_t  best_correlation; // 0-255
    uint8_t  factor;           // Q8 fixe point
} Pass2Entry;

static Pass2Entry pass2[MAX_CANDIDATES];
static uint8_t    pass2_count = 0;

// OBD2 speed courant (mis à jour via learner_on_obd2_speed)
static uint8_t  current_obd2_speed = 0;
static uint32_t current_obd2_tick  = 0;
static uint8_t  obd2_min = 255;
static uint8_t  obd2_max = 0;

// Résultat final
static VehicleConfig result;
static bool          result_valid = false;
static bool          pass2_active = false; // true quand passe 2 initialisée

// Forward declaration (défini plus bas)
static void pass2_on_frame(uint32_t can_id, uint8_t *data, uint8_t dlc);

// ─── INIT ─────────────────────────────────────────────────────────────────────

void learner_init(void) {
    memset(candidates, 0, sizeof(candidates));
    memset(pass2,      0, sizeof(pass2));
    candidate_count  = 0;
    pass2_count      = 0;
    current_obd2_speed = 0;
    obd2_min = 255;
    obd2_max = 0;
    result_valid = false;
    pass2_active = false;
}

// ─── FEED OBD2 ────────────────────────────────────────────────────────────────

void learner_on_obd2_speed(uint8_t speed_kmh, uint32_t tick_ms) {
    current_obd2_speed = speed_kmh;
    current_obd2_tick  = tick_ms;
    if (speed_kmh < obd2_min) obd2_min = speed_kmh;
    if (speed_kmh > obd2_max) obd2_max = speed_kmh;
}

// ─── PASSE 1 ─────────────────────────────────────────────────────────────────

static Pass1Entry* find_or_create_candidate(uint32_t can_id, uint8_t byte_offset) {
    // Chercher existant
    for (uint8_t i = 0; i < candidate_count; i++) {
        if (candidates[i].can_id == can_id && 
            candidates[i].byte_offset == byte_offset) {
            return &candidates[i];
        }
    }
    // Créer nouveau si de la place
    if (candidate_count < MAX_CANDIDATES) {
        Pass1Entry *e = &candidates[candidate_count++];
        e->can_id       = can_id;
        e->byte_offset  = byte_offset;
        e->min_val      = 255;
        e->max_val      = 0;
        e->direction    = 0;
        e->score        = 0;
        e->active       = true;
        return e;
    }
    return NULL;
}

void learner_on_frame(uint32_t can_id, uint8_t *data, uint8_t dlc, uint32_t tick_ms) {
    // Filtrer OBD2 11-bit ET 29-bit (ISO 15765-4)
    bool is_obd2 = (can_id == 0x7DF) ||
                   (can_id >= 0x7E8 && can_id <= 0x7EF) ||
                   (can_id == 0x18DB33F1UL) ||
                   (can_id >= 0x18DAF100UL && can_id <= 0x18DAF1FFUL);
    if (is_obd2) return;
    // Ignorer si pas de speed OBD2 encore
    if (current_obd2_speed == 0 && obd2_max == 0) return;

    for (uint8_t b = 0; b < dlc && b < 8; b++) {
        uint8_t val = data[b];
        Pass1Entry *e = find_or_create_candidate(can_id, b);
        if (!e || !e->active) continue;

        // Mettre à jour min/max
        if (val < e->min_val) e->min_val = val;
        if (val > e->max_val) e->max_val = val;

        // Vérifier monotonie avec vitesse OBD2
        if (current_obd2_speed > e->last_obd2_speed + 5) {
            // Vitesse monte → byte doit monter
            if (val > (e->min_val + 3)) e->score++;
        } else if (current_obd2_speed + 5 < e->last_obd2_speed) {
            // Vitesse descend → byte doit descendre
            if (val < (e->max_val - 3)) e->score++;
        }

        e->last_obd2_speed = current_obd2_speed;
    }

    // Si passe 2 active, accumuler aussi pour la corrélation Pearson
    if (pass2_active) {
        pass2_on_frame(can_id, data, dlc);
    }
}

void learner_pass1_task(uint32_t tick_ms) {
    // Rien de spécial ici, tout se fait dans learner_on_frame
    // appelé depuis can_rx_task
    (void)tick_ms;
}

bool learner_pass1_done(uint32_t tick_ms, uint32_t start_tick) {
    if (tick_ms - start_tick < PASS1_DURATION_MS) return false;

    // Vérifier qu'on a eu assez de variation vitesse
    uint8_t speed_range = obd2_max - obd2_min;
    if (speed_range < MIN_SPEED_RANGE) {
        // Pas assez de variation → étendre la passe 1
        // (le conducteur n'a pas assez accéléré/freiné)
        return false;
    }

    // Sélectionner candidats avec bon score
    // Éliminer ceux dont le range de valeurs est trop petit
    for (uint8_t i = 0; i < candidate_count; i++) {
        uint8_t val_range = candidates[i].max_val - candidates[i].min_val;
        if (val_range < 10 || candidates[i].score < 5) {
            candidates[i].active = false;
        }
    }

    return true;
}

bool learner_has_candidates(void) {
    for (uint8_t i = 0; i < candidate_count; i++) {
        if (candidates[i].active) return true;
    }
    return false;
}

// ─── PASSE 2 ─────────────────────────────────────────────────────────────────

static void init_pass2(void) {
    pass2_count = 0;
    for (uint8_t i = 0; i < candidate_count; i++) {
        if (!candidates[i].active) continue;
        if (pass2_count >= MAX_CANDIDATES) break;

        Pass2Entry *p = &pass2[pass2_count++];
        p->can_id      = candidates[i].can_id;
        p->byte_offset = candidates[i].byte_offset;
        p->sum_x = p->sum_y = p->sum_xy = p->sum_x2 = p->sum_y2 = 0;
        p->n = 0;
        p->best_correlation = 0;
        p->factor = 255; /* 1:1 par défaut (255/256 ≈ 1.0) */
    }
}

void learner_pass2_task(uint32_t tick_ms) {
    if (!pass2_active) {
        init_pass2();
        pass2_active = true;
    }
    (void)tick_ms;
}

// Appelé pour chaque trame pendant passe 2
static void pass2_on_frame(uint32_t can_id, uint8_t *data, uint8_t dlc) {
    // Filtrer OBD2 11-bit ET 29-bit (ISO 15765-4)
    bool is_obd2 = (can_id == 0x7DF) ||
                   (can_id >= 0x7E8 && can_id <= 0x7EF) ||
                   (can_id == 0x18DB33F1UL) ||
                   (can_id >= 0x18DAF100UL && can_id <= 0x18DAF1FFUL);
    if (is_obd2) return;

    for (uint8_t i = 0; i < pass2_count; i++) {
        Pass2Entry *p = &pass2[i];
        if (p->can_id != can_id) continue;
        if (p->byte_offset >= dlc) continue;

        int32_t x = data[p->byte_offset];  // valeur byte CAN
        int32_t y = current_obd2_speed;    // vitesse OBD2

        p->sum_x  += x;
        p->sum_y  += y;
        p->sum_xy += x * y;
        p->sum_x2 += x * x;
        p->sum_y2 += y * y;
        p->n++;
    }
}

/**
 * Pearson en virgule fixe (pas de float, pas de sqrt)
 * r² = (n*sum_xy - sum_x*sum_y)² / ((n*sum_x2 - sum_x²) * (n*sum_y2 - sum_y²))
 * On compare r² * 256² avec CORRELATION_THRESH² * n²
 * Retourne 0-255 représentant |r| * 255
 */
static uint8_t compute_correlation(Pass2Entry *p) {
    if (p->n < MIN_SAMPLES) return 0;

    int32_t n = p->n;
    int32_t num = n * p->sum_xy - p->sum_x * p->sum_y;
    int32_t den_x = n * p->sum_x2 - p->sum_x * p->sum_x;
    int32_t den_y = n * p->sum_y2 - p->sum_y * p->sum_y;

    if (den_x <= 0 || den_y <= 0) return 0;

    // Éviter overflow : normaliser num et den
    // Utiliser comparaison num²/(den_x*den_y)
    // Pour rester en int32 : diviser par n² d'abord
    // Approximation : r_approx = |num| * 255 / sqrt(den_x * den_y)
    // Utiliser sqrt entier

    // sqrt entier rapide
    uint32_t product = (uint32_t)(den_x >> 4) * (uint32_t)(den_y >> 4);
    uint32_t sq = 0;
    uint32_t bit = 1UL << 15;
    while (bit > product) bit >>= 2;
    while (bit != 0) {
        if (product >= sq + bit) {
            product -= sq + bit;
            sq = (sq >> 1) + bit;
        } else {
            sq >>= 1;
        }
        bit >>= 2;
    }
    // sq ≈ sqrt(den_x * den_y) / 16

    if (sq == 0) return 0;

    int32_t abs_num = num < 0 ? -num : num;
    uint32_t r_scaled = (uint32_t)(abs_num >> 4) * 255 / sq;
    if (r_scaled > 255) r_scaled = 255;

    return (uint8_t)r_scaled;
}

bool learner_pass2_done(uint32_t tick_ms, uint32_t start_tick) {
    if (tick_ms - start_tick < PASS2_DURATION_MS) return false;

    // Calculer corrélations et trouver le meilleur
    uint8_t best_corr = 0;
    Pass2Entry *best = NULL;

    for (uint8_t i = 0; i < pass2_count; i++) {
        uint8_t r = compute_correlation(&pass2[i]);
        pass2[i].best_correlation = r;

        if (r > best_corr) {
            best_corr = r;
            best = &pass2[i];
        }
    }

    if (!best || best_corr < CORRELATION_THRESH) return true; // done mais pas trouvé

    // Calculer facteur de conversion
    // factor = mean(obd2) / mean(byte) en Q8
    if (best->sum_x > 0) {
        // mean_x = sum_x / n, mean_y = sum_y / n
        // factor = mean_y / mean_x * 256 (Q8)
        // Clipper à 255 max pour tenir dans uint8_t
        uint32_t f = (uint32_t)(best->sum_y * 256) / (uint32_t)best->sum_x;
        best->factor = (f > 255) ? 255 : (uint8_t)f;
    }

    result.can_id      = best->can_id;
    result.byte_offset = best->byte_offset;
    result.factor      = best->factor;
    result.correlation = best_corr;
    result_valid       = true;

    return true;
}

bool learner_get_result(VehicleConfig *cfg) {
    if (!result_valid) return false;
    *cfg = result;
    return true;
}
