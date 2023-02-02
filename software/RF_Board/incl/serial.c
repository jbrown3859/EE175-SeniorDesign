#include <msp430.h>
#include <serial.h>
#include <util.h>

#define MCLK_FREQ_MHZ 8                     // MCLK = 8MHz

char SPI_TIMEOUT = 0; //set in ISR

/* UART data structure */
char UART_RXBUF[256];
unsigned char UART_RX_PTR = 0;
unsigned char UART_RX_BASE = 0;

/* This function is from the TI MSP430 example code. Things work without it but I'm not taking any chances */
void Software_Trim() {
    unsigned int oldDcoTap = 0xffff;
    unsigned int newDcoTap = 0xffff;
    unsigned int newDcoDelta = 0xffff;
    unsigned int bestDcoDelta = 0xffff;
    unsigned int csCtl0Copy = 0;
    unsigned int csCtl1Copy = 0;
    unsigned int csCtl0Read = 0;
    unsigned int csCtl1Read = 0;
    unsigned int dcoFreqTrim = 3;
    unsigned char endLoop = 0;

    do
    {
        CSCTL0 = 0x100;                         // DCO Tap = 256
        do
        {
            CSCTL7 &= ~DCOFFG;                  // Clear DCO fault flag
        }while (CSCTL7 & DCOFFG);               // Test DCO fault flag

        __delay_cycles((unsigned int)3000 * MCLK_FREQ_MHZ);// Wait FLL lock status (FLLUNLOCK) to be stable
                                                           // Suggest to wait 24 cycles of divided FLL reference clock
        while((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) && ((CSCTL7 & DCOFFG) == 0));

        csCtl0Read = CSCTL0;                   // Read CSCTL0
        csCtl1Read = CSCTL1;                   // Read CSCTL1

        oldDcoTap = newDcoTap;                 // Record DCOTAP value of last time
        newDcoTap = csCtl0Read & 0x01ff;       // Get DCOTAP value of this time
        dcoFreqTrim = (csCtl1Read & 0x0070)>>4;// Get DCOFTRIM value

        if(newDcoTap < 256)                    // DCOTAP < 256
        {
            newDcoDelta = 256 - newDcoTap;     // Delta value between DCPTAP and 256
            if((oldDcoTap != 0xffff) && (oldDcoTap >= 256)) // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else
            {
                dcoFreqTrim--;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }
        else                                   // DCOTAP >= 256
        {
            newDcoDelta = newDcoTap - 256;     // Delta value between DCPTAP and 256
            if(oldDcoTap < 256)                // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else
            {
                dcoFreqTrim++;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }

        if(newDcoDelta < bestDcoDelta)         // Record DCOTAP closest to 256
        {
            csCtl0Copy = csCtl0Read;
            csCtl1Copy = csCtl1Read;
            bestDcoDelta = newDcoDelta;
        }

    }while(endLoop == 0);                      // Poll until endLoop == 1

    CSCTL0 = csCtl0Copy;                       // Reload locked DCOTAP
    CSCTL1 = csCtl1Copy;                       // Reload locked DCOFTRIM
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked
}


void init_clock() {
    __bis_SR_register(SCG0);                 // disable FLL
    CSCTL3 |= SELREF__REFOCLK;               // Set REFO as FLL reference source
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_3;// DCOFTRIM=3, DCO Range = 8MHz
    CSCTL2 = FLLD_0 + 243;                  // DCODIV = 8MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                // enable FLL
    Software_Trim();                        // Software Trim to get the best DCOFTRIM value

    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK; // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
                                             // default DCODIV as MCLK and SMCLK source
}


/* init UART to a standard baud rate */
void init_UART(unsigned long baud) {
    // Configure UART pins
    P1SEL0 |= BIT6 | BIT7;                    // set 2-UART pin as second function

    // Configure Clock
    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK;

    switch(baud) {
    case 115200:
        /* 115200 BAUD settings (from user guide):
        * UCOS16 = 1
        * UCBRF = 5
        * UCBRS = 0x55
        * UCA0BR0 = 4
        */
        UCA0BR0 = 4;
        UCA0BR1 = 0x00;
        UCA0MCTLW = 0x5500 | UCOS16 | UCBRF_5;
        break;
    default: //9600 baud
        // Baud Rate calculation
        // 8000000/(16*9600) = 52.083
        // Fractional portion = 0.083
        // User's Guide Table 17-4: UCBRSx = 0x49
        // UCBRFx = int ( (52.083-52)*16) = 1
        UCA0BR0 = 52;                             // 8000000/16/9600
        UCA0BR1 = 0x00;
        UCA0MCTLW = 0x4900 | UCOS16 | UCBRF_1;
        break;
    }

    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}

/* UART vector */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void) {
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG)) {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:
            UART_RXBUF[UART_RX_PTR] = UCA0RXBUF;
            UART_RX_PTR++;
            break;
        case USCI_UART_UCTXIFG: break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

/* Grab char from the RX buffer IF it is full, but do not wait UNTIL it's full */
int getchar() {
    int chr = -1;

    /* if there is a char in the buffer */
    if (UCA0IFG & UCRXIFG) {
        chr = UCA0RXBUF;
    }

    return chr;
}


/* Wait until the TX buffer is empty and load a char to it */
void putchar(char c) {
    /* wait until transmit buffer is ready (THIS LINE IS VERY IMPORTANT AND UART WILL BREAK WITHOUT IT) */
    while (( UCA0IFG & UCTXIFG ) == 0 );

    /* Transmit data */
    UCA0TXBUF = c;
    __no_operation();
}


/* Iterate through a string and transmit chars until the null terminator is encountered */
void putchars(char* msg) {
    unsigned int i = 0;
    char c;

    c = msg[i];
    while(c != '\0') {
        putchar(c);
        i++;
        c = msg[i];
    }
}

void print_binary(char b) {
    int i;
    for (i = 7; i >= 0; i--) {
        if (((b >> i) & 0x01) == 0x01) {
            putchar('1');
        }
        else {
            putchar('0');
        }
    }
}

void print_hex(char h) {
    int i;
    char nibble;
    for (i = 1; i >= 0; i--) {
        nibble = ((h >> (4 * i)) & 0x0F);
        if (nibble < 10) { //decimal number
            putchar(nibble + 48);
        }
        else { //letter
            putchar(nibble + 55);
        }
    }
}

void print_dec(const long long data, const unsigned char len) {
    int i;
    for (i = (len-1); i>=0; i--) {
        putchar(((data / pow(10,i)) % 10) + 48);
    }
}


/* Init SPI on UCB1 and init timeout timer */
void init_SPI_master(void) {
    /* SPI Pin Configuration:
     * P4.7: MISO
     * P4.6: MOSI
     * P4.5: SCK
     * P4.4: SS
     */
    P4SEL0 |= BIT7 | BIT6 | BIT5; //configure pins

    /* NSS */
    P4SEL0 &= ~(0b1 << 4);
    P4DIR |= (0b1 << 4);
    P4OUT |= (0b1 << 4);

    UCB1CTLW0 |= UCSWRST;
    UCB1CTLW0 |= UCMST|UCSYNC|UCCKPL|UCMSB|UCMODE_0; //3-pin, 8-bit SPI master

    //configure clock source
    UCB1CTLW0 |= UCSWRST;
    UCB1CTLW0 |= UCSSEL__SMCLK;

    //1 MHz SPI clock
    UCB1BR0 |= 8; //(8MHz / 8)
    UCB1BR1 = 0;

    UCB1CTLW0 &= ~UCSWRST;
    UCB1IE |= UCRXIE;

    /* Timeout clock init */
    TB1CCTL0 = CCIE; //interrupt mode
    TB1CCR0 = 500; //trigger value
    TB1CTL = TBSSEL__SMCLK | ID_0 | TBCLR; //fast peripheral clock, no division, clear at start
}

/* set SPI polarity/phase */
void set_SPI_mode(char phase, char polarity) {
    UCB1CTLW0 |= UCSWRST; //enable editing
    UCB1CTLW0 &= 0x3FFF; //clear bits 14 and 15 
    
    UCB1CTLW0 |= ((phase & 0x01) << 15);
    UCB1CTLW0 |= ((polarity & 0x01) << 14);
    
    UCB1CTLW0 &= ~UCSWRST; //release for operation
}

void set_SPI_timer(char mode) {
    if (mode == 1) { //enable timer
        TB1CTL |= MC__UP | TBCLR;
    }
    else { //disable timer
        TB1CTL |= TBCLR;
        TB1CTL &= ~(0b11 << 4); //stop timer
    }
}

#pragma vector=TIMER1_B0_VECTOR
__interrupt void TIMER1_B0_VECTOR_ISR (void) {
    SPI_TIMEOUT = 1;
    TB1CCTL0 &= ~CCIFG; //reset interrupt
}

/* SPI vector */
#pragma vector=USCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void) {
    switch(__even_in_range(UCB1IV,USCI_UART_UCTXCPTIFG)){
        case USCI_NONE: break;
        default: break;
    }
}

/* Transmit address + char */
void SPI_TX(char addr, char c) {
    set_SPI_timer(1); //enable timeout

    P4OUT &= ~(1 << 4); //NSS low
    __no_operation();

    while ((UCB1IFG & UCTXIFG) == 0 && SPI_TIMEOUT == 0);
    UCB1TXBUF = addr;
    SPI_TIMEOUT = 0;

    while ((UCB1IFG & UCTXIFG) == 0 && SPI_TIMEOUT == 0);
    UCB1TXBUF = c;
    SPI_TIMEOUT = 0;

    __no_operation();
    while ((UCB1STATW & UCBUSY) == 1 ); //wait until not busy
    P4OUT |= (1 << 4); //NSS high

    set_SPI_timer(0); //disable timeout
}

/* send 8 bit address to slave in order to get data back */
char SPI_RX(char addr) {
    char r;

    set_SPI_timer(1); //enable timeout

    P4OUT &= ~(1 << 4); //NSS low
    __no_operation();

    while ((UCB1IFG & UCTXIFG) == 0);
    UCB1TXBUF = addr;

    while ((UCB1IFG & UCRXIFG) == 0 && SPI_TIMEOUT == 0);
    r = UCB1RXBUF;
    SPI_TIMEOUT = 0;

    while ((UCB1IFG & UCTXIFG) == 0 );
    UCB1TXBUF = 0x00; //transmit nothing

    while ((UCB1IFG & UCRXIFG) == 0 && SPI_TIMEOUT == 0);
    r = UCB1RXBUF;
    SPI_TIMEOUT = 0;

    __no_operation();
    while ((UCB1STATW & UCBUSY) == 1 ); //wait until not busy
    P4OUT |= (1 << 4); //NSS high

    set_SPI_timer(0); //disable timeout
    return r;
}
