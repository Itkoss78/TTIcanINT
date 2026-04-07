#include "obd2.h"
#include "can.h"
#include "config.h"

/**
 * OBD2 sur CAN :
 * Request  → ID 0x7DF, data: [0x02, 0x01, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00]
 * Response ← ID 0x7E8 (ECU principal), data: [0x03, 0x41, 0x0D, SPEED, ...]
 * SPEED = byte 3, valeur directe en km/h
 */

#define OBD2_REQ_ID     0x7DF
#define OBD2_RESP_ID    0x7E8
#define OBD2_RESP_MASK  0x7F8   // Accepter 0x7E8 à 0x7EF
#define PID_SPEED       0x0D

static uint32_t last_request_tick = 0;
static uint32_t last_response_tick = 0;
static Obd2Speed current_speed = {0, 0, false};
static bool      waiting_response = false;

static void send_speed_request(void) {
    uint8_t data[8] = {0x02, 0x01, PID_SPEED, 0x00, 0x00, 0x00, 0x00, 0x00};
    can_send(OBD2_REQ_ID, data, 8);
    waiting_response = true;
}

static void parse_obd2_response(CanFrame *frame) {
    // Vérifier que c'est bien une réponse vitesse
    // Format ISO 15765-2 single frame : [len, 0x41, PID, value, ...]
    if (frame->dlc >= 4 &&
        frame->data[0] == 0x03 &&
        frame->data[1] == 0x41 &&
        frame->data[2] == PID_SPEED) {
        
        current_speed.speed_kmh  = frame->data[3];
        current_speed.timestamp  = frame->timestamp;
        current_speed.valid      = true;
        last_response_tick       = frame->timestamp;
        waiting_response         = false;
    }
}

void obd2_task(uint32_t tick_ms) {
    // Scanner les trames reçues pour réponse OBD2
    // Note: le learner consomme aussi les trames → on doit partager
    // Solution : obd2_task regarde le ring buffer sans consommer
    // (can_peek pas implémenté → on utilise un buffer dédié OBD2 via ID filter)
    
    // Envoyer requête toutes les 100ms
    if (tick_ms - last_request_tick >= OBD2_REQUEST_INTERVAL_MS) {
        send_speed_request();
        last_request_tick = tick_ms;
    }

    // Timeout réponse
    if (waiting_response && tick_ms - last_request_tick > OBD2_TIMEOUT_MS) {
        waiting_response = false;
    }
}

// Appelé depuis can_rx_task quand on reçoit une trame OBD2
void obd2_on_frame(CanFrame *frame) {
    if ((frame->id & OBD2_RESP_MASK) == (OBD2_RESP_ID & OBD2_RESP_MASK)) {
        parse_obd2_response(frame);
    }
}

bool obd2_get_speed(Obd2Speed *out) {
    if (!current_speed.valid) return false;
    *out = current_speed;
    return true;
}

uint8_t obd2_get_speed_raw(void) {
    return current_speed.speed_kmh;
}

bool obd2_is_responding(void) {
    // Considérer comme actif si réponse < 1 seconde
    extern uint32_t get_tick(void);
    return current_speed.valid && 
           (get_tick() - last_response_tick) < 1000;
}
