#include <msp430.h>

#include <serial.h>
#include <rfm95w.h>

char rxbuf[32];
char rx_ptr = 0;

void init_GPIO(void) {
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode

    /* LED */
    P1SEL0 |= 0b0; //set P1 to I/O
    P1DIR |= 0b1; //set P1.0 to output
    P1OUT &= ~(0b1); //set P1.0 to zero

    /* temp, delete me later */
    P1DIR = 0xFF; P2DIR = 0xFF;
    P1REN = 0xFF; P2REN = 0xFF;
    P1OUT = 0x00; P2OUT = 0x00;
}

void init_Timer_B0() {
    TB0CCTL0 = CCIE; //interrupt mode
    TB0CCR0 = 8000; //trigger value
    TB0CTL = TBSSEL__ACLK | MC__UP | ID_0 | TBCLR; //slow clock, count up, no division, clear at start
}

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_VECTOR_ISR (void) {
    P1OUT ^= 0x01; //toggle led
    TB0CCTL0 &= ~CCIFG; //reset interrupt
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
                /*
                rxbuf[rx_ptr] = UCA0RXBUF;
                if (rx_ptr < 31){
                    rx_ptr++;
                }
                __no_operation();
                */
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
    init_UART(115200);
    init_SPI_master();
    init_Timer_B0();
    __bis_SR_register(GIE); //enable interrupts

    putchars("\n\rProgramming LoRa Registers\n\r");
    rfm95w_set_lora_mode(MODE_LORA);
    rfm95w_display_register(0x01);
    putchars("\n\r");

    rfm95w_set_carrier_frequency(433500000);
    rfm95w_display_register(0x06);
    rfm95w_display_register(0x07);
    rfm95w_display_register(0x08);
    putchars("\n\r");

    rfm95w_set_tx_power(PA_RFO, 0x0, 0x0);
    rfm95w_display_register(0x09);
    putchars("\n\r");

    __no_operation(); //debug
    for(;;) {
        char j;
        for(j=0;j<16;j++) {
            rfm95w_set_tx_power(PA_RFO, 0x0, j);
            char i;
            for (i=1;i<256;i++) {
                while(rfm95w_get_mode() != MODE_STDBY); //wait until out of TX
                rfm95w_write_fifo(i); //send a single byte to TX buffer
                rfm95w_set_mode(MODE_TX); //set to transmit mode
            }
        }
    }

    return 0;
}
