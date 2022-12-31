#include <msp430.h>

#include <util.h>
#include <serial.h>
#include <cc2500.h>

#include <backend.h>

char RX_done;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    init_clock();
    init_UART(115200);
    init_SPI_master();

    /* LED */
    P1SEL0 &= ~(0b1); //set P1 to I/O
    P1DIR |= 0b1; //set P1.0 to output
    P1OUT &= ~(0b1); //set P1.0 to zero

    cc2500_init_gpio();
    putchars("\n\rResetting Chip\n\r");
    cc2500_command_strobe(STROBE_SRES); //reset chip
    cc2500_register_dump();
    putchars("\n\rRegister Dump\n\r");
    //cc2500_set_base_frequency(2420000000);
    //cc2500_set_IF_frequency(457000);
    cc2500_set_vco_autocal(AUTOCAL_FROM_IDLE);
    cc2500_configure_gdo(GDO2, RX_END_OF_PACKET); //RX detect
    cc2500_register_dump();

	/* infinite loop */
	for(;;){
	    cc2500_transmit("I have never met Napoleon", 25);
	    __no_operation();
	}

    //main_loop();

	return 0;
}
