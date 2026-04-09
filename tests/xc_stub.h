/**
 * xc_stub.h — Remplace <xc.h> pour les tests host (gcc/clang sur Mac/Linux)
 *
 * Fournit tous les registres PIC18F46K80 utilisés dans le code comme
 * simples variables globales. Aucun comportement hardware — juste assez
 * pour que le code compile et que la logique pure soit testable.
 */
#ifndef XC_STUB_H
#define XC_STUB_H

#include <stdint.h>
#include <stdbool.h>

/* ── Types bit-field génériques ────────────────────────────────────────────── */
typedef struct { uint8_t TMR0IF:1; uint8_t TMR0IE:1; uint8_t GIE:1; uint8_t _pad:5; } INTCON_t;
typedef struct { uint8_t TMR0ON:1; uint8_t _pad:7; } T0CON_t;
typedef struct { uint8_t IRCF:3;   uint8_t _pad:5; } OSCCON_t;
typedef struct { uint8_t PLLEN:1;  uint8_t _pad:7; } OSCTUNE_t;
typedef struct { uint8_t EEPGD:1; uint8_t CFGS:1; uint8_t _pad1:2;
                 uint8_t WREN:1;  uint8_t WR:1;   uint8_t RD:1; uint8_t _pad2:1; } EECON1_t;
typedef struct { uint8_t MDSEL:2;  uint8_t _pad:6; } ECANCON_t;
typedef struct { uint8_t RXFUL:1;  uint8_t _pad:7; } RXBxCON_t;
typedef struct { uint8_t EXID:1;   uint8_t _pad:7; } RXBxSIDL_t;
typedef struct { uint8_t TXREQ:1;  uint8_t _pad:7; } TXB0CON_t;
typedef struct { uint8_t TRISA0:1; uint8_t TRISA1:1; uint8_t _pad:6; } TRISA_t;
typedef struct { uint8_t LATA0:1;  uint8_t LATA1:1;  uint8_t _pad:6; } LATA_t;
typedef struct { uint8_t TRISC6:1; uint8_t _pad:7; } TRISC_t;
typedef struct { uint8_t _p0:3; uint8_t TRISB3:1; uint8_t TRISB4:1; uint8_t TRISB5:1; uint8_t _p6:2; } TRISB_t;
typedef struct { uint8_t _p0:4; uint8_t LATB4:1;  uint8_t LATB5:1;  uint8_t _p6:2; } LATB_t;
typedef struct { uint8_t _p0:4; uint8_t TRISD4:1; uint8_t _p5:3; } TRISD_t;
typedef struct { uint8_t _p0:4; uint8_t LATD4:1;  uint8_t _p5:3; } LATD_t;

/* ── Registres globaux ─────────────────────────────────────────────────────── */
extern volatile INTCON_t  INTCONbits;
extern volatile T0CON_t   T0CONbits;
extern volatile OSCCON_t  OSCCONbits;
extern volatile OSCTUNE_t OSCTUNEbits;
extern volatile EECON1_t  EECON1bits;
extern volatile ECANCON_t ECANCONbits;
extern volatile RXBxCON_t RXB0CONbits;
extern volatile RXBxCON_t RXB1CONbits;
extern volatile RXBxSIDL_t RXB0SIDLbits;
extern volatile RXBxSIDL_t RXB1SIDLbits;
extern volatile TXB0CON_t TXB0CONbits;
extern volatile TRISA_t   TRISAbits;
extern volatile LATA_t    LATAbits;
extern volatile TRISC_t   TRISCbits;
extern volatile TRISB_t   TRISBbits;
extern volatile LATB_t    LATBbits;
extern volatile TRISD_t   TRISDbits;
extern volatile LATD_t    LATDbits;

extern volatile uint8_t  T0CON, TMR0H, TMR0L;
extern volatile uint8_t  CANCON, CANSTAT;
extern volatile uint8_t  BRGCON1, BRGCON2, BRGCON3;
extern volatile uint8_t  RXM0SIDH, RXM0SIDL, RXM0EIDH, RXM0EIDL;
extern volatile uint8_t  RXM1SIDH, RXM1SIDL, RXM1EIDH, RXM1EIDL;
extern volatile uint8_t  RXF0SIDH, RXF0SIDL;
extern volatile uint8_t  RXB0CON,  RXB1CON;
extern volatile uint8_t  RXB0SIDH, RXB0SIDL, RXB0EIDH, RXB0EIDL, RXB0DLC;
extern volatile uint8_t  RXB0D0,RXB0D1,RXB0D2,RXB0D3,RXB0D4,RXB0D5,RXB0D6,RXB0D7;
extern volatile uint8_t  RXB1SIDH, RXB1SIDL, RXB1EIDH, RXB1EIDL, RXB1DLC;
extern volatile uint8_t  RXB1D0,RXB1D1,RXB1D2,RXB1D3,RXB1D4,RXB1D5,RXB1D6,RXB1D7;
extern volatile uint8_t  TXB0SIDH, TXB0SIDL, TXB0DLC;
extern volatile uint8_t  TXB0D0,TXB0D1,TXB0D2,TXB0D3,TXB0D4,TXB0D5,TXB0D6,TXB0D7;
extern volatile uint8_t  EEADR, EEDATA, EECON1, EECON2;

/* ── Macros ignorées sur host ──────────────────────────────────────────────── */
#define CLRWDT()          ((void)0)
#define __interrupt()
#define __delay_ms(x)     ((void)0)

/* Remplace <xc.h> quand ce header est inclus directement */
#define _XC_H_

#endif /* XC_STUB_H */
