#include <msp430.h>

#include <serial.h>

char rxbuf[32];
char rx_ptr = 0;

void init_GPIO(void) {
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode

    /* LED */
    P1SEL0 |= 0b0; //set P1 to I/O
    P1DIR |= 0b1; //set P1.0 to output
    P1OUT &= ~(0b1); //set P1.0 to zero

    /* temp, delete me later */
    P1DIR = 0xFF; P2DIR = 0xFF; P4DIR = 0xFF;
    P1REN = 0xFF; P2REN = 0xFF; P4REN = 0xFF;
    P1OUT = 0x00; P2OUT = 0x00; P4OUT = 0x00;
}

void init_Timer_B0() {
    TB0CCTL0 = CCIE; //interrupt mode
    TB0CCR0 = 8000; //trigger value
    TB0CTL = TBSSEL__ACLK | MC__UP | ID_0 | TBCLR; //slow clock, count up, no division, clear at start
}

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_VECTOR_ISR (void) {
    P1OUT ^= 0x01; //toggle led
    TB1CCTL0 &= ~CCIFG; //reset timer?
    //SPI_TX('a');
}

/* ISRs must be defined for serial controllers or a trap will be thrown */
/* UART ISR vector */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void) {
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:
                //while(!(UCA0IFG&UCTXIFG));
                //rxchar = UCA0RXBUF;
                rxbuf[rx_ptr] = UCA0RXBUF;
                if (rx_ptr < 31){
                    rx_ptr++;
                }
                __no_operation();
              break;
        case USCI_UART_UCTXIFG:
            break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

/* SPI vector */
#pragma vector=USCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void) {
    switch(__even_in_range(UCB1IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG: //this is always set on every transmit
             break;
        case USCI_UART_UCTXIFG:
            break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}


/**
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    init_clock();
    init_GPIO();
    init_UART();
    init_SPI_master();
    init_Timer_B0();
    __bis_SR_register(GIE); //enable interrupts

    char c;
    for(;;) {
        /*
        c = getchar();
        if (c == 'w') {
            putchars("What hath God wrought?\n\r");
        }
        */
        SPI_TX('a');
        unsigned int i;
        for (i = 0; i < 10; i++);
    }

    return 0;
}
