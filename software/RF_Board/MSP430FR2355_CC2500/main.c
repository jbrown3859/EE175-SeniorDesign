#include <msp430.h>

#include <util.h>
#include <serial.h>
#include <cc2500.h>

#include <backend.h>

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

    /*
    putchars("\n\rRegister Dump\n\r");
    cc2500_set_base_frequency(2420000000);
    cc2500_set_IF_frequency(457000);
    cc2500_register_dump();
    //cc2500_write(0x00, 0x29);
    putchars("\n\rResetting Chip\n\r");
    cc2500_write(0x31152000,0x00); //reset chip
    cc2500_register_dump();
    */

	/* infinite loop */
    /*
	for(;;){
	    unsigned char i;
	    if (get_UART_FIFO_size() == 10) {
	        for (i=0;i<10;i++) {
	            putchar(read_UART_FIFO());
	        }
	        putchars("\n\r");
	    }
	}
	*/
    main_loop();

	return 0;
}
