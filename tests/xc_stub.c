/**
 * xc_stub.c — Définitions des variables stub PIC18 pour tests host
 */
#include "xc_stub.h"

volatile INTCON_t  INTCONbits  = {0};
volatile T0CON_t   T0CONbits   = {0};
volatile OSCCON_t  OSCCONbits  = {0};
volatile OSCTUNE_t OSCTUNEbits = {0};
volatile EECON1_t  EECON1bits  = {0};
volatile ECANCON_t ECANCONbits = {0};
volatile RXBxCON_t RXB0CONbits = {0};
volatile RXBxCON_t RXB1CONbits = {0};
volatile RXBxSIDL_t RXB0SIDLbits = {0};
volatile RXBxSIDL_t RXB1SIDLbits = {0};
volatile TXB0CON_t TXB0CONbits = {0};
volatile TRISA_t   TRISAbits   = {0};
volatile LATA_t    LATAbits    = {0};
volatile TRISC_t   TRISCbits   = {0};
volatile TRISB_t   TRISBbits   = {0};

volatile uint8_t T0CON=0, TMR0H=0, TMR0L=0;
volatile uint8_t CANCON=0, CANSTAT=0;
volatile uint8_t BRGCON1=0, BRGCON2=0, BRGCON3=0;
volatile uint8_t RXM0SIDH=0,RXM0SIDL=0,RXM0EIDH=0,RXM0EIDL=0;
volatile uint8_t RXM1SIDH=0,RXM1SIDL=0,RXM1EIDH=0,RXM1EIDL=0;
volatile uint8_t RXF0SIDH=0,RXF0SIDL=0;
volatile uint8_t RXB0CON=0,RXB1CON=0;
volatile uint8_t RXB0SIDH=0,RXB0SIDL=0,RXB0EIDH=0,RXB0EIDL=0,RXB0DLC=0;
volatile uint8_t RXB0D0=0,RXB0D1=0,RXB0D2=0,RXB0D3=0,RXB0D4=0,RXB0D5=0,RXB0D6=0,RXB0D7=0;
volatile uint8_t RXB1SIDH=0,RXB1SIDL=0,RXB1EIDH=0,RXB1EIDL=0,RXB1DLC=0;
volatile uint8_t RXB1D0=0,RXB1D1=0,RXB1D2=0,RXB1D3=0,RXB1D4=0,RXB1D5=0,RXB1D6=0,RXB1D7=0;
volatile uint8_t TXB0SIDH=0,TXB0SIDL=0,TXB0DLC=0;
volatile uint8_t TXB0D0=0,TXB0D1=0,TXB0D2=0,TXB0D3=0,TXB0D4=0,TXB0D5=0,TXB0D6=0,TXB0D7=0;
volatile uint8_t EEADR=0,EEDATA=0,EECON1=0,EECON2=0;
