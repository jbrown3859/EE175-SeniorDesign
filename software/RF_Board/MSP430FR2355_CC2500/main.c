#include <msp430.h>

#include <util.h>
#include <serial.h>
#include <cc2500.h>

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

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    init_clock();
    //init_GPIO();
    init_UART(115200);
    init_SPI_master();

    /* LED */
    P1SEL0 &= ~(0b1); //set P1 to I/O
    P1DIR |= 0b1; //set P1.0 to output
    P1OUT &= ~(0b1); //set P1.0 to zero

    unsigned char i;
    for (i=0;i<=0x3F;i++) {
        char c;
        putchars("Address: ");
        print_hex(i);
        c = cc2500_read(i);
        putchars(" Data: ");
        print_hex(c);
        putchars("\n\r");
    }

	/* infinite loop */
	for(;;){
	    P1OUT ^= 0b1;
	    hardware_delay(30000);
	}

	return 0;
}
