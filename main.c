/**
 * CANM8 Universal - Firmware PIC18F46K80
 * Mode apprentissage automatique avec détection véhicule
 * 
 * Auteur: Itsik Hassine
 * MCU: PIC18F46K80 @ 6MHz crystal (24MHz avec PLL x4)
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "led.h"
#include "can.h"
#include "obd2.h"
#include "learner.h"
#include "eeprom.h"
#include "signal_w.h"
#include "vehicle_id.h"

// === MACHINE À ÉTATS PRINCIPALE ===
typedef enum {
    STATE_BOOT,
    STATE_SCAN_VEHICLE,
    STATE_LEARN_PASS1,
    STATE_LEARN_PASS2,
    STATE_PRODUCTION,
    STATE_ERROR
} SystemState;

static SystemState state = STATE_BOOT;

// Tick 1ms via Timer0
volatile uint32_t tick_ms = 0;

void __interrupt() isr(void) {
    if (INTCONbits.TMR0IF) {
        tick_ms++;
        INTCONbits.TMR0IF = 0;
        TMR0H = 0xF8;  // Reload pour 1ms @ 24MHz
        TMR0L = 0x30;
    }
}

uint32_t get_tick(void) {
    return tick_ms;
}

void system_init(void) {
    // Oscillateur interne 24MHz (6MHz xtal * PLL 4x)
    OSCCONbits.IRCF = 0b111;
    OSCTUNEbits.PLLEN = 1;

    // Timer0 en mode 16-bit pour tick 1ms
    T0CON = 0b00000111; // prescaler 1:256, 16-bit
    TMR0H = 0xF8;
    TMR0L = 0x30;
    INTCONbits.TMR0IE = 1;
    INTCONbits.GIE = 1;
    T0CONbits.TMR0ON = 1;

    // Init modules
    led_init();
    can_init();
    eeprom_init();
    signal_w_init();
}

int main(void) {
    system_init();

    EepromData eep;
    VehicleConfig cfg;
    uint32_t state_enter_tick = 0;

    while (1) {
        CLRWDT(); // Nourrir le watchdog à chaque itération
        led_update(get_tick());
        can_rx_task(); // Pousser les trames HW → ring buffer

        switch (state) {

            // ─── BOOT ───────────────────────────────────────────────
            case STATE_BOOT:
                led_set_pattern(LED_PATTERN_BOOT);
                eeprom_read(&eep);

                if (!eep.valid) {
                    // EEPROM vide → apprentissage direct
                    state = STATE_SCAN_VEHICLE;
                } else {
                    // EEPROM pleine → vérifier véhicule
                    state = STATE_SCAN_VEHICLE;
                }
                state_enter_tick = get_tick();
                break;

            // ─── SCAN VÉHICULE (10 secondes) ────────────────────────
            case STATE_SCAN_VEHICLE:
                led_set_pattern(LED_PATTERN_SEARCHING);
                vehicle_id_scan(get_tick());

                if (get_tick() - state_enter_tick > SCAN_DURATION_MS) {
                    uint32_t sig = vehicle_id_get_signature();

                    if (eep.valid && sig == eep.vehicle_signature) {
                        // Même véhicule → mode production
                        cfg.can_id     = eep.speed_can_id;
                        cfg.byte_offset = eep.speed_byte_offset;
                        cfg.factor     = eep.speed_factor;
                        state = STATE_PRODUCTION;
                    } else {
                        // Nouveau véhicule → effacer + apprentissage
                        eeprom_clear();
                        learner_init();
                        state = STATE_LEARN_PASS1;
                    }
                    state_enter_tick = get_tick();
                }
                break;

            // ─── PASSE 1 : FILTRAGE GROSSIER (30 secondes) ──────────
            case STATE_LEARN_PASS1:
                led_set_pattern(LED_PATTERN_IDENTIFIED);
                obd2_task(get_tick());
                learner_pass1_task(get_tick());

                if (learner_pass1_done(get_tick(), state_enter_tick)) {
                    if (learner_has_candidates()) {
                        state = STATE_LEARN_PASS2;
                    } else {
                        state = STATE_ERROR;
                    }
                    state_enter_tick = get_tick();
                }
                break;

            // ─── PASSE 2 : CORRÉLATION PRÉCISE (30 secondes) ────────
            case STATE_LEARN_PASS2:
                led_set_pattern(LED_PATTERN_IDENTIFIED);
                obd2_task(get_tick());
                learner_pass2_task(get_tick());

                if (learner_pass2_done(get_tick(), state_enter_tick)) {
                    if (learner_get_result(&cfg)) {
                        // Sauvegarder en EEPROM
                        eep.valid              = true;
                        eep.speed_can_id       = cfg.can_id;
                        eep.speed_byte_offset  = cfg.byte_offset;
                        eep.speed_factor       = cfg.factor;
                        eep.vehicle_signature  = vehicle_id_get_signature();
                        eeprom_write(&eep);

                        /* Flash vert fixe 2s : "Vehicle type recognised" */
                        led_set_pattern(LED_PATTERN_RECOGNISED);
                        {
                            uint32_t t = get_tick();
                            while (get_tick() - t < 2000UL) {
                                CLRWDT();
                                led_update(get_tick());
                            }
                        }

                        state = STATE_PRODUCTION;
                    } else {
                        state = STATE_ERROR;
                    }
                    state_enter_tick = get_tick();
                }
                break;

            // ─── PRODUCTION ─────────────────────────────────────────
            case STATE_PRODUCTION:
                led_set_pattern(LED_PATTERN_SPEED_DETECTED);
                {
                    CanFrame frame;
                    if (can_get_speed_frame(&frame, cfg.can_id)) {
                        uint8_t raw = frame.data[cfg.byte_offset];
                        uint8_t speed_kmh = (uint8_t)(((uint16_t)raw * cfg.factor) >> 8);
                        signal_w_set_speed(speed_kmh);
                    }
                    signal_w_task(get_tick());
                }
                break;

            // ─── ERREUR ─────────────────────────────────────────────
            case STATE_ERROR:
                led_set_pattern(LED_PATTERN_ERROR);
                signal_w_stop();
                // Attendre 5s puis retenter
                if (get_tick() - state_enter_tick > 5000) {
                    learner_init();
                    state = STATE_SCAN_VEHICLE;
                    state_enter_tick = get_tick();
                }
                break;
        }

        // === DISPATCHER UNIQUE — lire une fois, distribuer à tous ===
        {
            CanFrame frame;
            while (can_get_frame(&frame)) {
                switch (state) {
                    case STATE_SCAN_VEHICLE:
                        vehicle_id_on_frame(&frame);
                        obd2_on_frame(&frame);
                        break;

                    case STATE_LEARN_PASS1:
                    case STATE_LEARN_PASS2:
                        obd2_on_frame(&frame);
                        learner_on_frame(frame.id, frame.data, frame.dlc, frame.timestamp);
                        if ((frame.id & 0x7F8) == 0x7E8) {
                            if (frame.data[0] == 0x03 && frame.data[1] == 0x41 && frame.data[2] == 0x0D) {
                                learner_on_obd2_speed(frame.data[3], frame.timestamp);
                            }
                        }
                        break;

                    case STATE_PRODUCTION:
                        // En production : ne garder que l'ID vitesse → déjà géré par can_get_speed_frame()
                        break;

                    default:
                        break;
                }
            }
        }
    }
    return 0;
}
