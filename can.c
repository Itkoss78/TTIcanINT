#include "can.h"
#include "config.h"
#include <string.h>

extern uint32_t get_tick(void);

// Ring buffer réception
static CanFrame rx_buffer[CAN_RX_BUFFER_SIZE];
static uint8_t rx_head = 0;
static uint8_t rx_tail = 0;
static uint16_t rx_count = 0;

// Dernière trame par ID cible (mode production)
static CanFrame last_speed_frame;
static bool     speed_frame_fresh = false;

void can_init(void) {
    /**
     * ECAN PIC18F46K80
     * Pins : RC6 = CANTX, RB3 = CANRX
     * MCP2562 connecté sur CANH/CANL
     * 
     * Bitrate : 500 kbps (standard OBD2/CAN auto)
     * BRP = 2, SJW = 1, PROPSEG = 6, PS1 = 7, PS2 = 2
     * TQ total = 1 + 6 + 7 + 2 = 16 TQ
     * Fclk = 24MHz / (2 * (2+1)) = 4MHz → 4MHz/16TQ = 250kbps
     * Pour 500kbps : BRP=1 → 24MHz/(2*2)=6MHz → 6MHz/16=375kbps
     * 
     * NOTE: Ajuster BRP selon crystal réel pour atteindre 500kbps exact
     * Avec 6MHz xtal + PLL x4 = 24MHz :
     * BRP=0 → Fclk=12MHz → 12/16=750kbps
     * BRP=1 → Fclk=6MHz  → 6/16 =375kbps  
     * BRP=2 → Fclk=4MHz  → 4/16 =250kbps
     * → Utiliser 250kbps ou ajuster crystal/PLL pour 500kbps exact
     */

    // Mode configuration ECAN
    CANCON = 0x80;
    while ((CANSTAT & 0xE0) != 0x80); // Attendre mode config

    // ECAN mode 2 (FIFO étendu)
    ECANCONbits.MDSEL = 0b10;

    // === BAUD RATE CAN ===
    // Crystal 6MHz + PLL x4 = Fclk 24MHz
    // Objectif : 500 kbps (standard véhicules modernes)
    // BRP=1 → Fq = 24MHz / (2*(1+1)) = 6MHz
    // TQ config : PROPSEG=4, PS1=4, PS2=3 → total 12 TQ
    // Bitrate = 6MHz / 12 = 500 kbps ✓
    // Sample point = (1+4+4)/12 = 75% ✓ (recommandé automotive)
    BRGCON1 = 0x01;  // SJW=1, BRP=1
    BRGCON2 = 0x9C;  // SAM=1, SEG1PH=4, PRSEG=4  (0b10011100)
    BRGCON3 = 0x02;  // SEG2PH=3                   (0b00000010)

    // Pour 250 kbps (anciens véhicules) : doubler BRP → BRP=3
    // BRGCON1 = 0x03;  // SJW=1, BRP=3
    // BRGCON2 = 0x9C;  // même
    // BRGCON3 = 0x02;  // même
    // Bitrate = 24MHz / (2*(3+1)) / 12 = 250 kbps

    // Filtres/masques : TOUT accepter (mode promiscuous)
    // Masque 0 = 0x00000000 → tous les bits ignorés → tout passe
    RXM0SIDH = 0x00;
    RXM0SIDL = 0x00;
    RXM0EIDH = 0x00;
    RXM0EIDL = 0x00;

    RXM1SIDH = 0x00;
    RXM1SIDL = 0x00;
    RXM1EIDH = 0x00;
    RXM1EIDL = 0x00;

    // Filtre 0 → masque 0, buffer RXB0
    RXF0SIDH = 0x00;
    RXF0SIDL = 0x00;

    // RXB0 et RXB1 : accepter tous les messages
    RXB0CON = 0x60; // Receive all messages (RXMODE=11)
    RXB1CON = 0x60;

    // Pins I/O
    TRISCbits.TRISC6 = 0; // CANTX = sortie
    TRISBbits.TRISB3 = 1; // CANRX = entrée

    // Mode normal
    CANCON = 0x00;
    while ((CANSTAT & 0xE0) != 0x00);
}

void can_rx_task(void) {
    // Vérifier RXB0
    if (RXB0CONbits.RXFUL) {
        CanFrame f;
        f.timestamp = get_tick();
        f.extended  = RXB0SIDLbits.EXID;

        if (f.extended) {
            f.id = ((uint32_t)RXB0SIDH << 21) |
                   ((uint32_t)(RXB0SIDL & 0xE0) << 13) |
                   ((uint32_t)(RXB0SIDL & 0x03) << 16) |
                   ((uint32_t)RXB0EIDH << 8) |
                   RXB0EIDL;
        } else {
            f.id = ((uint32_t)RXB0SIDH << 3) | (RXB0SIDL >> 5);
        }

        f.dlc = RXB0DLC & 0x0F;
        f.data[0] = RXB0D0; f.data[1] = RXB0D1;
        f.data[2] = RXB0D2; f.data[3] = RXB0D3;
        f.data[4] = RXB0D4; f.data[5] = RXB0D5;
        f.data[6] = RXB0D6; f.data[7] = RXB0D7;

        RXB0CONbits.RXFUL = 0; // Libérer buffer

        // Push dans ring buffer
        uint8_t next = (rx_head + 1) % CAN_RX_BUFFER_SIZE;
        if (next != rx_tail) {
            rx_buffer[rx_head] = f;
            rx_head = next;
            rx_count++;
        }
    }

    // Vérifier RXB1
    if (RXB1CONbits.RXFUL) {
        CanFrame f;
        f.timestamp = get_tick();
        f.extended  = RXB1SIDLbits.EXID;

        if (f.extended) {
            f.id = ((uint32_t)RXB1SIDH << 21) |
                   ((uint32_t)(RXB1SIDL & 0xE0) << 13) |
                   ((uint32_t)(RXB1SIDL & 0x03) << 16) |
                   ((uint32_t)RXB1EIDH << 8) |
                   RXB1EIDL;
        } else {
            f.id = ((uint32_t)RXB1SIDH << 3) | (RXB1SIDL >> 5);
        }

        f.dlc = RXB1DLC & 0x0F;
        f.data[0] = RXB1D0; f.data[1] = RXB1D1;
        f.data[2] = RXB1D2; f.data[3] = RXB1D3;
        f.data[4] = RXB1D4; f.data[5] = RXB1D5;
        f.data[6] = RXB1D6; f.data[7] = RXB1D7;

        RXB1CONbits.RXFUL = 0;

        uint8_t next = (rx_head + 1) % CAN_RX_BUFFER_SIZE;
        if (next != rx_tail) {
            rx_buffer[rx_head] = f;
            rx_head = next;
            rx_count++;
        }
    }
}

bool can_get_frame(CanFrame *frame) {
    if (rx_tail == rx_head) return false;
    *frame = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % CAN_RX_BUFFER_SIZE;
    return true;
}

bool can_get_speed_frame(CanFrame *frame, uint32_t target_id) {
    // En mode production, chercher uniquement l'ID cible
    CanFrame f;
    while (can_get_frame(&f)) {
        if (f.id == target_id) {
            *frame = f;
            return true;
        }
    }
    return false;
}

bool can_send(uint32_t id, uint8_t *data, uint8_t len) {
    // Attendre buffer TX libre
    uint16_t timeout = 1000;
    while (TXB0CONbits.TXREQ && timeout--);
    if (!timeout) return false;

    // Charger trame (11-bit standard)
    TXB0SIDH = (uint8_t)(id >> 3);
    TXB0SIDL = (uint8_t)(id << 5);
    TXB0DLC  = len & 0x0F;

    if (len > 0) TXB0D0 = data[0];
    if (len > 1) TXB0D1 = data[1];
    if (len > 2) TXB0D2 = data[2];
    if (len > 3) TXB0D3 = data[3];
    if (len > 4) TXB0D4 = data[4];
    if (len > 5) TXB0D5 = data[5];
    if (len > 6) TXB0D6 = data[6];
    if (len > 7) TXB0D7 = data[7];

    TXB0CONbits.TXREQ = 1; // Envoyer
    return true;
}

uint16_t can_get_rx_count(void) {
    return rx_count;
}
